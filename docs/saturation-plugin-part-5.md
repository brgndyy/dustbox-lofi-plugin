---
title: '직접 Saturation 플러그인 만들어보기 5'
date: '2026-08-09'
description: 'JavaScript에서 벗어나 C++와 JUCE로 실제 VST3 플러그인의 구조를 만들어보자'
thumbnail: ''
---

## 이제 실제 플러그인으로 넘어가자

앞에서는 JavaScript로 새츄레이션의 핵심 계산을 확인했다.

```text
샘플
→ Drive를 곱함
→ tanh 적용
→ Output을 곱함
```

하지만 Cubase는 JavaScript 파일을 오디오 플러그인으로 불러오지 않는다. Cubase가 불러올 수 있는 VST3 파일을 만들어야 한다.

여기서부터는 실제 DustBox 플러그인을 만드는 과정으로 넘어간다.

이번 장의 목표는 노브나 디자인을 만드는 것이 아니다. Cubase가 플러그인을 불러오고 오디오를 전달했을 때, C++ 코드가 그 샘플을 처리해 다시 돌려주는 최소 흐름을 이해하는 것이다.

```text
Cubase
  ↓ 오디오 버퍼 전달
VST3 플러그인
  ↓
C++ processBlock()
  ↓ Drive → tanh → Output
Cubase로 처리된 버퍼 반환
```

## C++, JUCE, VST3는 각각 무엇일까?

실제 코드를 작성하기 전에 지금부터 등장할 도구의 역할을 구분할 필요가 있다.

```text
C++     실제 오디오 처리 코드를 작성하는 언어
JUCE    플러그인 개발에 필요한 공통 기능을 제공하는 프레임워크
VST3    Cubase와 플러그인이 통신하기 위한 플러그인 형식
CMake   소스 파일을 어떤 설정으로 빌드할지 정하는 도구
Cubase  완성된 VST3를 불러오고 오디오를 전달하는 호스트
```

### C++

JavaScript에서 작성했던 계산은 실제 플러그인에서 C++ 코드가 된다.

```js
// JavaScript
const saturated = Math.tanh(input * drive);
```

```cpp
// C++
const float saturated = std::tanh(input * drive);
```

표현은 조금 다르지만 계산 순서는 같다.

C++를 사용하는 이유는 단순히 더 어려운 언어이기 때문이 아니다. 오디오 플러그인은 재생 중에 많은 샘플을 정해진 시간 안에 계속 처리해야 한다. C++는 메모리 사용과 실행 시점을 직접 통제하기 좋고, VST3 SDK와 JUCE도 C++를 중심으로 만들어져 있다.

### JUCE

VST3를 만들려면 Cubase가 플러그인을 생성하고, 오디오를 전달하고, 파라미터를 저장하는 규칙에 맞춰야 한다.

이 규칙을 처음부터 직접 구현하면 새츄레이션 계산보다 플러그인 형식을 다루는 코드가 훨씬 많아진다.

JUCE는 그 사이를 연결한다.

```text
Cubase의 VST3 호출
        ↓
JUCE의 플러그인 래퍼
        ↓
우리가 작성한 AudioProcessor
```

JUCE를 사용해도 새츄레이션 알고리즘이 자동으로 생기는 것은 아니다. `Drive → tanh → Output` 계산은 우리가 작성한다. JUCE는 Cubase가 전달한 샘플을 C++ 코드에서 다룰 수 있도록 연결해준다.

### VST3

VST3는 프로그래밍 언어가 아니라 플러그인 형식이다.

완성된 macOS VST3는 다음과 같은 번들로 만들어진다.

```text
DustBox LoFi.vst3
```

Cubase는 이 번들을 스캔하고, 안에 들어 있는 플러그인을 생성한 뒤 오디오 처리를 요청한다.

### CMake

CMake는 C++ 컴파일러가 아니다. 어떤 소스 파일과 JUCE 모듈을 사용해 어떤 플러그인을 만들 것인지 빌드 설정을 만든다.

DustBox 저장소에서는 루트의 `CMakeLists.txt`가 그 역할을 한다.

```text
CMakeLists.txt
→ C++ 소스와 JUCE를 연결
→ VST3 빌드 설정 생성
→ 컴파일러가 실제 바이너리 생성
```

## 실제 프로젝트 구조 보기

현재 DustBox 프로젝트에서 처음 확인할 파일은 다음과 같다.

```text
dustbox-lofi-plugin/
├── CMakeLists.txt
├── JUCE/
└── Source/
    ├── PluginProcessor.h
    ├── PluginProcessor.cpp
    ├── PluginEditor.h
    └── PluginEditor.cpp
```

각 파일의 역할은 다음과 같다.

```text
CMakeLists.txt
→ VST3 프로젝트의 이름, 형식, 소스 파일, JUCE 모듈 설정

PluginProcessor.h / .cpp
→ 실제 오디오 처리와 파라미터 상태

PluginEditor.h / .cpp
→ 플러그인 창과 노브 같은 화면
```

새츄레이션의 소리를 만드는 핵심은 `PluginProcessor.cpp`에 들어간다. 화면은 소리를 처리하지 않는다. 노브가 바뀐 값을 Processor에 전달할 뿐이다.

## CMake에서 VST3 만들기

DustBox의 `CMakeLists.txt`에는 JUCE에게 오디오 플러그인을 만들라고 요청하는 코드가 있다.

```cmake
juce_add_plugin(DustBoxLoFi
    COMPANY_NAME "GNDY Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    FORMATS VST3
    PRODUCT_NAME "DustBox LoFi"
)
```

중요한 항목부터 보면 다음과 같다.

```text
DustBoxLoFi
→ CMake에서 사용할 빌드 대상 이름

IS_SYNTH FALSE
→ 소리를 새로 만드는 신시사이저가 아니라 입력 소리를 처리함

FORMATS VST3
→ VST3 형식으로 빌드함

PRODUCT_NAME
→ Cubase에서 표시될 플러그인 이름
```

그다음 실제로 컴파일할 C++ 파일을 연결한다.

```cmake
target_sources(DustBoxLoFi
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.h
)
```

마지막으로 오디오 플러그인과 DSP 기능에 필요한 JUCE 모듈을 연결한다.

```cmake
target_link_libraries(DustBoxLoFi
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
)
```

이 설정이 있다고 새츄레이션이 완성되는 것은 아니다. 아직은 C++ 코드를 VST3로 빌드할 수 있는 틀을 만든 것이다.

## Processor와 Editor를 구분하자

플러그인은 크게 두 부분으로 나눌 수 있다.

```text
Processor
→ 실제 오디오 샘플 처리
→ 재생 중에도 계속 작동

Editor
→ 노브와 글자 표시
→ 사용자의 조작을 Processor에 전달
```

플러그인 창을 닫아도 소리는 계속 처리되어야 한다. 따라서 새츄레이션 계산을 Editor에 넣으면 안 된다.

```text
PluginEditor.cpp       화면
PluginProcessor.cpp    소리
```

이번 장에서는 튜토리얼의 최소 예제만 떼어 화면 연결을 잠시 제외한다. 실제 DustBox 저장소에는 이미 `AudioProcessorValueTreeState` 파라미터와 여섯 개의 노브가 구현되어 있다. 완성된 코드를 처음부터 한꺼번에 설명하지 않고, Processor의 최소 오디오 흐름부터 다시 쌓아가는 것이다.

## Cubase는 언제 C++ 코드를 실행할까?

JUCE의 `AudioProcessor`에는 플러그인의 주요 실행 시점이 함수로 나뉘어 있다.

```text
플러그인 생성
    ↓
prepareToPlay()
    ↓
processBlock() 반복
    ↓
releaseResources()
```

이 흐름이 플러그인 실행 중에 딱 한 번만 일어나는 것은 아니다. Cubase가 재생을 멈추거나 오디오 장치의 Sample Rate와 버퍼 설정을 바꾸면 자원을 해제한 뒤 `prepareToPlay()`를 다시 호출할 수 있다.

### `prepareToPlay()`

재생을 시작하기 전에 Cubase가 Sample Rate와 한 번에 전달할 수 있는 최대 샘플 수를 알려준다.

```cpp
void prepareToPlay(double sampleRate, int maximumBlockSize);
```

지연 버퍼나 필터처럼 미리 준비해야 하는 메모리는 여기서 만든다. 재생 중인 `processBlock()` 안에서 계속 새 메모리를 만드는 것은 피해야 한다.

이번 최소 새츄레이션은 샘플마다 계산만 하므로 아직 준비할 버퍼가 없다.

### `processBlock()`

실제 오디오 처리는 `processBlock()`에서 일어난다.

```cpp
void processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages
);
```

Cubase는 곡 전체를 한 번에 주지 않는다. 짧은 오디오 버퍼를 반복해서 전달한다.

```text
첫 번째 버퍼  [샘플, 샘플, 샘플, ...]
두 번째 버퍼  [샘플, 샘플, 샘플, ...]
세 번째 버퍼  [샘플, 샘플, 샘플, ...]
```

`AudioBuffer<float>`에는 채널별 샘플이 들어 있다.

```text
buffer
├── channel 0: 왼쪽 또는 모노 채널
└── channel 1: 오른쪽 채널
```

플러그인은 이 버퍼의 값을 직접 바꾼다. 처리가 끝나 함수가 반환되면 Cubase는 바뀐 값을 다음 오디오 단계로 보낸다.

### `releaseResources()`

Cubase가 현재 오디오 처리를 멈추고 준비했던 자원이 더 이상 필요하지 않을 때 호출한다.

```cpp
void releaseResources();
```

`prepareToPlay()`에서 만든 큰 임시 버퍼나 장치 관련 자원이 있다면 여기서 정리할 수 있다. 이후 재생이나 오디오 설정이 바뀌면 `prepareToPlay()`가 다시 호출될 수 있으므로, 재준비되어도 안전하게 작성해야 한다.

이번 최소 새츄레이션 예제는 별도 자원을 만들지 않으므로 `releaseResources()`에서 정리할 것도 없다.

## 아무 처리도 하지 않으면 어떻게 될까?

가장 작은 `processBlock()`은 다음과 같다.

```cpp
void DustBoxLoFiAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&
)
{
    juce::ScopedNoDenormals noDenormals;
}
```

코드 안에서 `buffer`를 바꾸지 않았으므로 입력된 샘플이 그대로 남아 있다.

```text
Cubase가 전달한 입력 버퍼
→ 아무 값도 바꾸지 않음
→ 같은 버퍼를 Cubase가 다시 사용
```

즉 플러그인은 로드되지만 소리는 바뀌지 않는 통과 상태가 된다.

`ScopedNoDenormals`는 CPU가 매우 작은 부동소수점 값을 비효율적으로 처리하는 상황을 막기 위한 JUCE 도구다. 지금 단계에서는 오디오 처리 함수의 시작 부분에 두는 안전장치라고 이해하면 충분하다.

## JavaScript 계산을 processBlock으로 옮기기

앞에서 만든 최소 계산은 다음과 같았다.

```js
return Math.tanh(input * drive) * output;
```

C++에서는 다음처럼 옮길 수 있다.

```cpp
const float saturated = std::tanh(input * drive);
const float result = saturated * output;
```

이 계산을 모든 채널과 모든 샘플에 적용한다.

```cpp
void DustBoxLoFiAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&
)
{
    juce::ScopedNoDenormals noDenormals;

    const float drive = 2.0f;
    const float output = 0.75f;

    for (int channel = 0;
         channel < buffer.getNumChannels();
         ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);

        for (int sample = 0;
             sample < buffer.getNumSamples();
             ++sample)
        {
            const float driven = samples[sample] * drive;
            const float saturated = std::tanh(driven);
            samples[sample] = saturated * output;
        }
    }
}
```

이중 반복문이 하는 일은 단순하다.

```text
첫 번째 반복문
→ 왼쪽, 오른쪽 같은 채널을 하나씩 선택

두 번째 반복문
→ 선택한 채널의 샘플을 하나씩 처리
```

`getWritePointer(channel)`은 선택한 채널의 샘플을 직접 수정할 수 있는 위치를 돌려준다.

```cpp
auto* samples = buffer.getWritePointer(channel);
```

`samples[sample]`은 현재 처리할 샘플 하나다.

```cpp
const float driven = samples[sample] * drive;
const float saturated = std::tanh(driven);
samples[sample] = saturated * output;
```

입력이 `0.5`, Drive가 `2`, Output이 `0.75`라면 다음 순서로 처리된다.

```text
입력             0.5
Drive 적용       0.5 × 2 = 1
새츄레이션       tanh(1) = 약 0.761594
Output 적용      0.761594 × 0.75 = 약 0.571196
최종 샘플        약 0.571196
```

JavaScript 배열에서 확인했던 계산이 이제 Cubase가 전달한 실제 오디오 버퍼 안에서 실행된다.

위 코드는 원리를 보기 위해 DustBox의 HEAT 처리만 최소 형태로 분리한 예제다. 현재 DustBox 전체 `processBlock()`을 그대로 이 코드로 교체하라는 뜻은 아니다.

## 왜 RMS 자동 보정 코드는 넣지 않을까?

앞 장에서는 원본 배열 전체와 처리된 배열 전체의 RMS를 먼저 구한 뒤 Output을 계산했다.

```text
원본 전체 RMS ÷ 처리 후 전체 RMS
```

실시간 플러그인은 아직 재생되지 않은 미래의 샘플을 알 수 없다. 따라서 곡 전체 RMS를 미리 구하는 방식을 `processBlock()`에 그대로 넣을 수 없다.

이번 장에서 분리한 최소 예제에서는 Output을 고정된 선형 배율 `0.75`로 둔다. 실제 DustBox에는 이미 Output 파라미터와 노브가 연결되어 있다. 다음 장에서는 그 완성된 연결을 처음부터 따라가며 구현 원리를 설명한다.

자동 보정을 제품 기능으로 만들려면 지금까지 들어온 신호의 크기를 추적하고, 값이 갑자기 바뀌지 않도록 부드럽게 움직이는 별도의 설계가 필요하다. 지금은 만들지 않는다.

## processBlock에서 하지 말아야 할 일

`processBlock()`은 재생 중에 계속 호출된다. 처리 시간이 늦어지면 소리가 끊기거나 잡음이 날 수 있다.

따라서 다음 작업은 오디오 처리 함수 안에서 피한다.

```text
새 메모리를 계속 할당하기
파일 읽기
네트워크 요청
잠금이 풀릴 때까지 기다리기
화면 그리기
곡 전체를 다시 훑기
```

필요한 메모리는 `prepareToPlay()`에서 준비하고, `processBlock()`에서는 이미 준비된 버퍼와 값으로 정해진 계산만 수행한다.

## 실제 VST3 빌드하기

DustBox 저장소 루트에서 다음 명령을 실행한다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target DustBoxLoFi_VST3
```

첫 번째 명령은 `CMakeLists.txt`를 읽어 빌드 설정을 만든다.

두 번째 명령은 C++와 JUCE 코드를 실제 VST3로 컴파일한다.

완성된 플러그인은 다음 경로에서 확인할 수 있다.

```text
build/DustBoxLoFi_artefacts/Release/VST3/DustBox LoFi.vst3
```

macOS에서 Cubase가 VST3를 찾는 대표적인 경로는 다음과 같다.

```text
~/Library/Audio/Plug-Ins/VST3/
/Library/Audio/Plug-Ins/VST3/
```

현재 프로젝트는 `COPY_PLUGIN_AFTER_BUILD TRUE`로 빌드 후 사용자 VST3 폴더 복사를 요청한다. 먼저 build 경로의 결과물이 생성됐는지 확인한다.

## 이번 장에서 만든 것

```text
CMake가 VST3 빌드 설정을 만듦
        ↓
JUCE가 Cubase와 C++ Processor를 연결
        ↓
Cubase가 processBlock()에 오디오 버퍼 전달
        ↓
C++가 모든 채널과 샘플에
Drive → tanh → Output 적용
        ↓
처리된 버퍼가 Cubase로 돌아감
```

이번 장의 최소 예제에서는 Drive와 Output을 코드에 고정하고 화면 연결을 제외했다. 실제 DustBox에는 이미 파라미터와 노브가 구현되어 있다. 다음 장에서는 실제 코드의 `heat`와 `output` 등록, 노브 연결, Cubase 자동화와 프로젝트 저장을 순서대로 분해한다. 지금까지 `drive`라고 부른 입력 증폭은 HEAT 내부 계산으로 이어진다.
