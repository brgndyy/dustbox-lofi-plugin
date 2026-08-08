---
title: 'VST3 플러그인은 어떻게 Cubase에서 실행될까?'
date: '2026-08-08'
description: '오디오 플러그인, VST3, JUCE의 역할과 Cubase가 플러그인을 찾아 실행하는 과정을 처음부터 정리했다.'
thumbnail: ''
---

Cubase에서 오디오 트랙을 하나 만든다.

트랙에 효과를 끼워 넣는 자리인 `인서트(insert)` 목록에서 원하는 효과를 고르면 작은 창이 열린다. 음악을 재생하면 소리가 바로 달라진다.

사용자 입장에서는 버튼을 한 번 누른 것이 전부다.

하지만 그 순간 Cubase 안에서는 다음 작업이 이어진다.

```text
설치된 플러그인 찾기
        ↓
파일 안의 정보 읽기
        ↓
실행 코드를 메모리에 불러오기
        ↓
플러그인 객체 만들기
        ↓
입력·출력 채널 연결하기
        ↓
오디오를 짧은 묶음으로 계속 전달하기
        ↓
처리된 오디오를 다시 받아 재생하기
```

이번 글에서는 새츄레이션 계산 자체는 다루지 않는다.

대신 다음 질문을 순서대로 살펴보려고 한다.

- 오디오 플러그인은 일반 프로그램과 무엇이 다른가?
- VST3는 파일 이름인가, 개발 도구인가?
- JUCE는 왜 사용하는가?
- Cubase는 VST3를 어디서 찾는가?
- 재생 중에는 오디오를 어떤 방식으로 주고받는가?
- 화면의 노브와 프로젝트 저장값은 어떻게 연결되는가?

---

## 먼저 Cubase가 무엇인지부터 보자

Cubase는 음악을 녹음하고 편집하고 재생하는 프로그램이다.

이런 프로그램을 `DAW`라고 부른다. Digital Audio Workstation의 줄임말이며, 직역하면 디지털 오디오 작업실이다.

```text
DAW에서 하는 일

마이크와 악기 녹음
오디오 파일 배치와 편집
여러 트랙의 음량 조절
효과 적용
자동화 기록
최종 음악 파일 출력
```

DAW는 모든 효과를 자체 기능으로만 제공하지 않는다. 외부 개발자가 만든 효과와 악기를 불러올 수 있다.

외부 기능을 불러와 실행하는 쪽을 `호스트(host)`라고 한다.

이 글에서는 Cubase가 호스트다.

```text
Cubase
= DAW
= 플러그인을 불러오는 호스트
```

---

## 오디오 플러그인은 무엇일까?

플러그인은 다른 프로그램 안에 연결되어 기능을 추가하는 소프트웨어다.

오디오 플러그인은 들어온 소리를 처리하거나 새로운 소리를 만든다.

```text
효과 플러그인
입력 소리 → 처리 → 출력 소리

악기 플러그인
연주 정보 → 소리 생성 → 출력 소리
```

새츄레이션, EQ, 컴프레서와 리버브는 첫 번째에 해당한다. 이미 존재하는 오디오를 받아서 바꾼다.

피아노나 신시사이저 플러그인은 두 번째에 해당한다. 건반을 누른 정보 등을 받아 새로운 오디오를 만든다.

### 일반 앱과 무엇이 다를까?

일반 앱은 사용자가 직접 실행한다.

```text
사용자
  ↓ 더블 클릭
앱 실행
```

오디오 플러그인은 보통 혼자 실행되지 않는다.

```text
사용자
  ↓ Cubase 실행
Cubase
  ↓ 플러그인 불러오기
오디오 플러그인 실행
```

플러그인의 실행 시간과 입력·출력은 호스트가 관리한다.

플러그인이 스스로 음악 파일을 찾거나 스피커를 직접 제어하는 것이 아니다. Cubase가 오디오를 건네면 플러그인이 처리한 뒤 돌려준다.

---

## VST3는 무엇일까?

서로 처음 만난 두 프로그램이 오디오를 주고받으려면 약속이 필요하다.

Cubase는 다음 내용을 알아야 한다.

```text
이 플러그인의 이름은 무엇인가?
효과인가, 악기인가?
입력과 출력 채널은 몇 개인가?
어떤 조절 항목이 있는가?
오디오는 어느 함수로 전달해야 하는가?
설정값은 어떻게 저장하고 복원하는가?
화면은 어떻게 열어야 하는가?
```

플러그인도 Cubase가 어떤 방식으로 이 정보를 요청하는지 알아야 한다.

이 공통 약속이 플러그인 규격이다.

`VST3`는 Steinberg가 만든 오디오 플러그인 규격 가운데 하나다.

```text
VST3가 정하는 것

호스트가 플러그인을 찾는 방법
플러그인이 자신을 소개하는 방법
오디오와 이벤트를 전달하는 방법
파라미터와 자동화를 주고받는 방법
상태를 저장하고 복원하는 방법
화면을 연결하는 방법
```

VST3는 프로그래밍 언어가 아니다. 새츄레이션 알고리즘도 아니다. 플러그인과 호스트가 대화하는 규칙이다.

### 다른 규격도 있다

오디오 플러그인에는 VST3만 있는 것이 아니다.

```text
VST3  Steinberg가 만든 범용 규격
AU    Apple 플랫폼에서 사용하는 Audio Unit
AAX   Pro Tools에서 사용하는 규격
```

하나의 소리 처리 코드를 여러 규격으로 포장할 수 있다.

```text
같은 새츄레이션 계산
       ├─ VST3로 포장
       ├─ AU로 포장
       └─ AAX로 포장
```

이 글에서 만든 DustBox는 현재 VST3 형식만 빌드한다.

---

## `.vst3`는 한 개의 실행 파일일까?

macOS에서 `DustBox LoFi.vst3`는 Finder에 파일 하나처럼 보인다.

실제로는 여러 파일이 들어 있는 폴더다. macOS에서는 이런 구조를 `번들(bundle)`이라고 한다.

현재 빌드 결과를 열어보면 다음과 같다.

```text
DustBox LoFi.vst3/
└── Contents/
    ├── Info.plist
    ├── MacOS/
    │   └── DustBox LoFi
    ├── Resources/
    │   └── moduleinfo.json
    ├── PkgInfo
    └── _CodeSignature/
        └── CodeResources
```

각 파일의 역할은 다르다.

### `Contents/MacOS/DustBox LoFi`

컴퓨터의 계산을 담당하는 장치인 CPU가 실제로 실행하는 기계어 코드다.

C++로 작성한 소스 코드는 빌드 과정에서 이 실행 코드로 변환된다.

### `Info.plist`

macOS가 번들의 종류와 식별자 등을 확인할 때 사용하는 정보 파일이다.

DustBox의 번들 식별자는 다음과 같다.

```text
com.gndy.audio.dustboxlofi
```

### `moduleinfo.json`

VST3 모듈의 이름과 식별 정보 등을 담는다. 호스트가 어떤 플러그인이 들어 있는지 확인하는 데 사용한다.

### `_CodeSignature`

코드 서명에 관한 정보가 들어 있다.

서명은 이 번들의 내용이 서명된 뒤 바뀌지 않았는지 확인하는 데 쓰인다. 정식 배포용 서명은 개발자의 신원도 함께 확인하지만, 현재 DustBox의 임시 테스트 서명은 그 신원을 보증하지 않는다.

즉 `.vst3`는 확장자만 바꾼 음악 파일이 아니다. 실행 코드와 메타데이터, 리소스와 서명 정보를 묶은 배포 단위다.

---

## JUCE는 무엇일까?

VST3 규격을 지켜 플러그인을 만들려면 호스트와 주고받는 수많은 동작을 구현해야 한다.

```text
플러그인 클래스 등록
입력·출력 채널 협상
오디오 처리 함수 연결
파라미터와 자동화 연결
상태 저장과 복원
플러그인 화면 연결
운영체제별 창과 파일 처리
```

이 작업을 제품마다 처음부터 다시 작성할 필요는 없다.

`JUCE`는 C++로 오디오 앱과 플러그인을 만들 때 사용하는 프레임워크다.

프레임워크는 프로그램에 반복해서 필요한 구조와 기능을 미리 제공하는 코드 묶음이다.

```text
개발자가 작성하는 부분
새츄레이션, 필터, 노이즈처럼 제품 고유의 처리

JUCE가 제공하는 부분
오디오 버퍼, 파라미터, 화면 컴포넌트,
VST3 규격과 운영체제를 연결하는 공통 코드
```

JUCE가 소리를 대신 설계해주는 것은 아니다.

어떤 계산으로 소리를 바꿀지는 개발자가 작성한다. JUCE는 그 계산이 실제 호스트 안에서 실행될 수 있도록 주변 구조를 제공한다.

### JUCE는 언어가 아니다

DustBox의 소스 코드는 C++로 작성한다.

```text
C++   코드를 작성하는 프로그래밍 언어
JUCE  C++에서 사용하는 프레임워크
VST3  호스트와 플러그인이 통신하는 규격
```

세 용어는 서로 다른 역할을 가진다.

---

## CMake는 무엇을 할까?

소스 파일이 있다고 해서 바로 `.vst3`가 생기지는 않는다.

컴퓨터에 다음 내용을 알려줘야 한다.

```text
프로젝트 이름과 버전
사용할 C++ 버전
어떤 소스 파일을 컴파일할지
어떤 JUCE 기능을 연결할지
VST3를 만들지, 다른 형식을 만들지
결과물의 이름과 식별자는 무엇인지
```

DustBox는 이 정보를 `CMakeLists.txt`에 적는다.

`CMake`는 이 설정을 읽어 실제 빌드 도구가 이해할 프로젝트를 구성한다.

CMake 자체가 C++ 코드를 기계어로 바꾸는 컴파일러는 아니다. macOS에서는 Apple Clang 같은 컴파일러와 빌드 도구가 실제 변환을 담당한다.

```text
CMakeLists.txt
      ↓ CMake가 빌드 구조 생성
빌드 도구
      ↓ 컴파일러 실행
C++ 소스
      ↓
VST3 번들
```

### 실제 프로젝트의 시작 부분

```cmake
cmake_minimum_required(VERSION 3.22)

project(DustBoxLoFi VERSION 0.2.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)
```

한 줄씩 보면 다음 의미다.

```text
cmake_minimum_required
이 프로젝트를 구성할 수 있는 최소 CMake 버전

project
프로젝트 이름과 버전

CMAKE_CXX_STANDARD 17
C++17 문법과 기능 사용

CMAKE_CXX_STANDARD_REQUIRED ON
더 오래된 C++ 표준으로 조용히 낮추지 않음

add_subdirectory(JUCE)
프로젝트 안의 JUCE 빌드 설정을 불러옴
```

---

## `juce_add_plugin`은 무엇을 만드는가?

JUCE의 CMake 기능을 불러오면 `juce_add_plugin`을 사용할 수 있다.

현재 프로젝트에는 다음 설정이 있다.

```cmake
juce_add_plugin(DustBoxLoFi
    COMPANY_NAME "GNDY Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Gndy
    PLUGIN_CODE Dblf
    FORMATS VST3
    PRODUCT_NAME "DustBox LoFi"
    BUNDLE_ID "com.gndy.audio.dustboxlofi"
)
```

이 코드는 새츄레이션을 처리하지 않는다.

어떤 종류의 결과물을 어떻게 포장할지 설명한다.

### `COMPANY_NAME`

플러그인을 만든 회사 또는 개발자 이름이다.

### `IS_SYNTH FALSE`

악기 플러그인이 아니라는 뜻이다.

DustBox는 새 소리를 연주하는 악기가 아니라 들어온 오디오를 바꾸는 효과다.

### `NEEDS_MIDI_INPUT FALSE`

MIDI 입력이 필수는 아니라는 뜻이다.

MIDI는 건반의 음높이와 세기 같은 연주 정보를 전달하는 규격이다. DustBox는 오디오만 받아도 작동한다.

### `NEEDS_MIDI_OUTPUT FALSE`

플러그인이 MIDI 연주 정보를 밖으로 보내지 않는다.

### `IS_MIDI_EFFECT FALSE`

MIDI 정보 자체를 바꾸는 효과가 아니다.

### `COPY_PLUGIN_AFTER_BUILD TRUE`

빌드 후 결과물을 정해진 플러그인 위치로 복사하도록 JUCE 빌드 설정에 요청한다.

개발 중에는 편하지만 기존 버전을 덮어쓸 수 있으므로 어떤 경로에 복사됐는지 확인해야 한다.

### `PLUGIN_MANUFACTURER_CODE`와 `PLUGIN_CODE`

제조사와 플러그인을 구분하는 네 글자 코드다.

제품을 배포한 뒤에는 함부로 바꾸지 않는 편이 좋다. 호스트가 다른 플러그인으로 인식할 수 있기 때문이다.

### `FORMATS VST3`

이번 프로젝트에서 VST3 결과물을 만든다.

### `PRODUCT_NAME`

사용자와 호스트에 표시될 제품 이름이다.

### `BUNDLE_ID`

macOS 번들을 식별하는 역방향 도메인 형태의 문자열이다.

---

## 소스 파일은 어떻게 빌드 대상에 들어갈까?

다음 설정은 어떤 파일을 컴파일할지 지정한다.

```cmake
target_sources(DustBoxLoFi
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.h
)
```

현재 파일은 크게 두 역할로 나뉜다.

```text
PluginProcessor
오디오 처리, 파라미터, 상태 저장

PluginEditor
사용자가 보는 화면과 노브 배치
```

`Processor`는 소리를 처리하는 부분이고 `Editor`는 화면을 그리는 부분이다.

두 파일이 분리된 이유는 화면이 닫혀 있어도 오디오 처리는 계속되어야 하기 때문이다.

```text
플러그인 창 닫힘
화면 객체는 없을 수 있음
오디오 처리는 계속 실행
```

---

## 빌드하면 어떤 과정을 거칠까?

현재 프로젝트에서는 다음과 같은 명령으로 Release 결과물을 만들 수 있다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

첫 번째 명령은 빌드 구조를 만든다.

```text
-S .
현재 폴더의 CMakeLists.txt 사용

-B build
생성되는 빌드 파일을 build 폴더에 저장

-DCMAKE_BUILD_TYPE=Release
최적화된 배포용 설정 선택
```

두 번째 명령은 실제 컴파일과 링크를 실행한다.

`컴파일`은 각 C++ 소스 파일을 기계어 조각으로 바꾸는 과정이다.

`링크`는 기계어 조각과 JUCE의 필요한 코드를 하나의 실행 모듈로 묶는 과정이다.

그다음 JUCE의 빌드 설정이 실행 모듈과 정보를 VST3 번들 구조로 배치한다.

```text
.cpp 소스
   ↓ 컴파일
기계어 조각
   ↓ 링크
플러그인 실행 모듈
   ↓ 번들 생성
DustBox LoFi.vst3
```

---

## Cubase는 VST3를 어디서 찾을까?

VST3는 아무 폴더에 놓는다고 자동으로 발견되지 않는다.

macOS에서 표준으로 정해진 위치가 있다.

```text
사용자 전용
~/Library/Audio/Plug-Ins/VST3/

모든 사용자 공용
/Library/Audio/Plug-Ins/VST3/
```

`~`는 현재 사용자의 홈 폴더를 뜻한다.

```text
~/Library/...
= /Users/사용자이름/Library/...
```

사용자용 `VST3` 폴더가 없는 경우 직접 만들어도 된다.

Cubase는 이런 표준 경로를 스캔해 설치된 플러그인을 찾는다. 같은 제품을 사용자용과 공용 경로에 동시에 설치하면 중복이나 버전 혼동이 생길 수 있으므로 한 곳만 사용하는 편이 안전하다.

---

## 스캔은 무엇을 하는 과정일까?

스캔은 파일 이름만 목록에 적는 작업이 아니다.

호스트는 VST3 모듈을 열어 어떤 플러그인 클래스가 들어 있는지 확인한다. 제품 이름, 식별 정보와 종류를 읽고 사용할 수 있는지 검사한다.

```text
VST3 폴더 탐색
      ↓
.vst3 번들 발견
      ↓
모듈의 실행 코드 불러오기
      ↓
플러그인 클래스와 식별 정보 조회
      ↓
필요한 검증 수행
      ↓
목록과 캐시에 저장
```

`캐시(cache)`는 다음 실행에서 같은 정보를 매번 처음부터 찾지 않도록 저장해두는 데이터다.

스캔에 성공하면 Cubase의 플러그인 목록에 제품이 나타난다.

### 목록에 보인다고 실행까지 성공한 것은 아니다

스캔과 실제 삽입은 서로 다른 단계다.

```text
목록에 나타남
→ Cubase가 플러그인을 발견하고 기본 정보를 읽음

트랙에 삽입됨
→ 실제 플러그인 객체 생성과 초기화까지 성공
```

DustBox의 첫 충돌도 이 차이 때문에 원인을 좁힐 수 있었다.

목록에는 나타났지만 트랙에 삽입하는 순간 Cubase가 종료됐다. 설치 경로보다 플러그인 객체를 만드는 과정에 문제가 있다는 뜻이었다.

---

## 트랙에 삽입하면 어떤 일이 벌어질까?

사용자가 DustBox를 인서트에 추가하면 Cubase는 플러그인의 실행 인스턴스를 만든다.

`인스턴스(instance)`는 메모리 안에 실제로 생성된 플러그인 한 개를 뜻한다.

같은 플러그인을 세 트랙에 넣으면 인스턴스도 세 개다.

```text
보컬 트랙  → DustBox 인스턴스 1
기타 트랙  → DustBox 인스턴스 2
드럼 트랙  → DustBox 인스턴스 3
```

각 인스턴스는 서로 다른 노브 값과 내부 상태를 가진다.

JUCE 프로젝트에서는 다음 함수가 프로세서 객체를 만든다.

```cpp
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DustBoxLoFiAudioProcessor();
}
```

Cubase가 VST3 규격을 통해 플러그인 생성을 요청하면 JUCE의 VST3 연결 코드가 이 함수를 거쳐 `DustBoxLoFiAudioProcessor` 객체를 만든다.

### 생성자에서 일어나는 일

객체가 만들어지면 생성자가 실행된다.

```cpp
DustBoxLoFiAudioProcessor::DustBoxLoFiAudioProcessor()
    : AudioProcessor(
        BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    loadVinylCrackle();
}
```

구조를 단순화해서 보면 다음과 같다.

```text
스테레오 입력 선언
스테레오 출력 선언
파라미터 목록 생성
내장 바이닐 소리 불러오기
```

첫 알파에서 Cubase가 종료된 위치도 이 단계였다.

`loadVinylCrackle()`의 메모리 소유권이 잘못되어 생성 중 잘못된 메모리를 해제했다. Cubase는 플러그인 객체를 완성하지 못하고 함께 종료됐다.

따라서 `목록에는 나오는데 삽입하면 종료된다`면 설치 경로보다 생성자와 초기화 코드를 먼저 살펴봐야 한다.

---

## 입력과 출력 채널은 어떻게 정할까?

오디오에는 채널이 있다.

```text
모노
채널 1개

스테레오
왼쪽과 오른쪽, 채널 2개
```

Cubase와 플러그인은 사용할 채널 구성이 서로 맞는지 확인한다.

DustBox는 현재 모노 입력에는 모노 출력, 스테레오 입력에는 스테레오 출력을 허용한다.

```cpp
bool DustBoxLoFiAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet();
    const auto out = l.getMainOutputChannelSet();

    return (out == juce::AudioChannelSet::mono()
         || out == juce::AudioChannelSet::stereo())
        && in == out;
}
```

이 조건의 뜻은 다음과 같다.

```text
모노 입력  → 모노 출력   허용
스테레오 입력 → 스테레오 출력 허용
모노 입력  → 스테레오 출력 거부
```

줄바꿈만 읽기 쉽게 바꿨으며 조건은 현재 소스와 같다.

---

## 재생 전에 `prepareToPlay()`가 호출된다

오디오 처리를 시작하려면 현재 환경을 알아야 한다.

Cubase는 재생 전에 샘플레이트와 예상 최대 블록 크기를 플러그인에 알려준다.

```cpp
void prepareToPlay(double sampleRate, int blockSize);
```

### 샘플레이트

컴퓨터는 소리를 연속된 숫자로 저장한다. 숫자 하나를 샘플이라고 한다.

1초 동안 사용하는 샘플 수가 샘플레이트다.

```text
44.1kHz → 초당 44,100개
48kHz   → 초당 48,000개
96kHz   → 초당 96,000개
```

### 블록 크기

Cubase는 샘플을 한 개씩 함수에 전달하지 않는다. 여러 샘플을 짧은 묶음으로 전달한다.

이 묶음을 `블록(block)` 또는 `버퍼(buffer)`라고 한다.

```text
왼쪽 채널
L0 L1 L2 ... L255

오른쪽 채널
R0 R1 R2 ... R255
```

위 예시는 스테레오 256샘플 블록이다.

`prepareToPlay()`에서는 이 환경에 맞춰 필요한 메모리와 처리 도구를 준비한다.

DustBox에서는 다음 작업을 한다.

```text
현재 샘플레이트 저장
WARP용 지연 버퍼 생성
원본 보관 버퍼 생성
필터 상태 초기화
HEAT 내부 처리 도구 준비
노브 값이 갑자기 튀지 않도록 변화 시간 설정
```

재생 중에 큰 메모리를 새로 만들면 오디오가 끊길 수 있다. 그래서 가능한 준비 작업을 이 단계에서 끝낸다.

---

## 재생 중에는 `processBlock()`이 반복된다

준비가 끝나면 Cubase는 재생 중 `processBlock()`을 계속 호출한다.

```cpp
void processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages);
```

이 함수가 실제 오디오 처리의 중심이다.

```text
Cubase가 짧은 오디오 블록 전달
        ↓
processBlock() 실행
        ↓
플러그인이 버퍼 안의 숫자 변경
        ↓
함수가 끝남
        ↓
Cubase가 처리된 버퍼 사용
        ↓
다음 블록 전달
```

### `AudioBuffer<float>& buffer`

오디오 샘플이 들어 있는 버퍼다.

`float`는 소수점을 가진 숫자 형식이다. JUCE 오디오 처리에서는 보통 샘플을 -1.0부터 1.0 부근의 값으로 다룬다.

`&`는 버퍼를 복사해서 받는 것이 아니라 Cubase가 전달한 버퍼를 직접 다룬다는 뜻이다.

플러그인이 같은 버퍼 안의 값을 바꾸면 Cubase가 처리된 결과를 받는다.

### `MidiBuffer& midiMessages`

현재 블록에 해당하는 MIDI 정보가 들어 있다.

DustBox는 MIDI 효과가 아니므로 이 값을 사용하지 않는다. 하지만 JUCE의 공통 함수 형태에 포함되어 있어 인자로 받는다.

### 반환값이 없는 이유

함수 앞의 `void`는 별도의 값을 반환하지 않는다는 뜻이다.

처리 결과를 새 파일로 돌려주는 것이 아니라 전달받은 `buffer` 자체를 수정하기 때문이다.

---

## 한 블록 안에서 DustBox는 무엇을 할까?

현재 구현의 흐름을 단순화하면 다음과 같다.

```text
원본 오디오 복사
      ↓
AGE·WARP·DUST·HEAT 값 읽기
      ↓
WARP 피치 흔들림
      ↓
AGE 대역과 해상도 변화
      ↓
DUST 표면 질감 추가
      ↓
HEAT 새츄레이션
      ↓
원본과 처리음 MIX
      ↓
OUTPUT 적용
```

이 모든 작업은 한 블록의 제한 시간 안에 끝나야 한다.

48kHz에서 256샘플 블록 하나가 재생되는 시간은 약 5.33ms다.

```text
256 ÷ 48,000초
≈ 0.00533초
≈ 5.33ms
```

처리가 이 시간보다 계속 늦으면 다음 블록을 제때 전달하지 못한다. 사용자는 이를 클릭, 끊김 또는 드롭아웃으로 듣게 된다.

---

## 오디오 스레드에서는 왜 조심해야 할까?

Cubase는 오디오 처리를 담당하는 실행 흐름에서 `processBlock()`을 호출한다. 이를 오디오 스레드라고 부른다.

오디오 스레드의 우선순위는 결과를 정해진 시간 안에 끝내는 것이다.

다음 작업은 완료 시간이 일정하지 않을 수 있다.

```text
디스크에서 파일 읽기
인터넷 요청
큰 메모리 할당
다른 스레드가 풀 때까지 잠금 대기
과도한 로그 출력
```

이런 작업은 재생 중인 `processBlock()`에서 피해야 한다.

```text
파일과 큰 버퍼 준비
→ 생성자 또는 prepareToPlay()

매 블록의 계산
→ processBlock()
```

단, 생성자에 모든 작업을 몰아넣으면 스캔과 인스턴스 생성이 느려지거나 실패할 수 있다. 재생 전에 꼭 필요한 준비인지, 백그라운드에서 할 수 있는 작업인지 구분해야 한다.

---

## 화면은 오디오 처리와 별도로 만들어진다

플러그인 창은 항상 열려 있지 않다.

Cubase는 사용자가 편집 창을 열 때 화면 객체를 요청한다.

```cpp
juce::AudioProcessorEditor* createEditor()
{
    return new DustBoxLoFiAudioProcessorEditor(*this);
}
```

여기서 `PluginEditor` 객체가 만들어진다.

```text
Processor
소리 처리와 상태 보유

Editor
노브와 글자 등 화면 표시
```

화면을 닫으면 Editor 객체는 사라질 수 있다. Processor는 트랙에 플러그인이 삽입되어 있는 동안 계속 남아 오디오를 처리한다.

따라서 중요한 파라미터 값을 화면 객체에만 저장하면 안 된다.

---

## 노브는 어떻게 오디오 처리 값과 연결될까?

사용자가 HEAT 노브를 돌리면 세 곳이 같은 값을 알아야 한다.

```text
플러그인 화면
현재 노브 위치 표시

오디오 처리
현재 HEAT 값으로 새츄레이션 계산

Cubase
자동화 기록과 프로젝트 저장
```

DustBox는 JUCE의 `AudioProcessorValueTreeState`를 사용한다. 이름이 길어서 보통 `APVTS`라고 줄여 부른다.

APVTS는 파라미터 목록과 현재 값을 관리하고 상태 저장을 돕는 JUCE 클래스다.

HEAT 파라미터 선언을 보면 다음과 같다.

```cpp
AudioParameterFloat(
    "heat",
    "HEAT",
    NormalisableRange<float>(0, 1),
    0.34f
)
```

각 인자의 의미는 다음과 같다.

```text
"heat"
내부에서 사용하는 고유 ID

"HEAT"
화면과 호스트에 표시할 이름

0부터 1
허용 범위

0.34
처음 생성했을 때의 기본값
```

내부 ID는 프로젝트와 자동화 데이터를 연결하는 계약에 가깝다. 배포 후 ID를 바꾸면 기존 프로젝트에서 저장값을 찾지 못할 수 있다.

### 화면 노브 연결

Editor에서는 Attachment를 사용해 노브와 APVTS 값을 연결한다.

```cpp
attachments[i] = std::make_unique<Attachment>(
    audioProcessor.parameters,
    ids[i],
    knob
);
```

이 연결 덕분에 값이 양방향으로 움직인다.

```text
사용자가 노브를 움직임
→ APVTS 값 변경
→ 오디오 처리와 Cubase 자동화에 반영

Cubase가 자동화를 재생
→ APVTS 값 변경
→ 화면 노브도 이동
```

---

## 프로젝트를 다시 열어도 값이 남는 이유

Cubase 프로젝트를 저장했다가 다시 열면 플러그인 노브도 이전 값으로 돌아와야 한다.

호스트는 플러그인에 현재 상태를 데이터로 달라고 요청한다.

JUCE에서는 `getStateInformation()`이 이 역할을 한다.

```text
Cubase가 프로젝트 저장
        ↓
플러그인에 상태 요청
        ↓
파라미터 상태를 데이터로 변환
        ↓
Cubase 프로젝트 안에 저장
```

프로젝트를 열 때는 반대 과정이 일어난다.

```text
Cubase가 저장 데이터 전달
        ↓
setStateInformation() 호출
        ↓
파라미터 상태 복원
        ↓
노브와 오디오 처리 값 복원
```

이 때문에 파라미터 ID와 상태 데이터 형식은 출시 후 호환성을 고려해야 한다.

---

## Cubase가 플러그인을 실행하는 전체 흐름

지금까지의 과정을 한 번에 연결하면 다음과 같다.

```text
1. 설치
DustBox LoFi.vst3를 표준 VST3 경로에 둔다.

2. 스캔
Cubase가 번들을 찾고 플러그인 정보를 읽는다.

3. 목록 등록
사용 가능한 효과로 목록과 캐시에 기록한다.

4. 인스턴스 생성
사용자가 인서트에 추가하면 Processor 객체를 만든다.

5. 채널 연결
모노·스테레오 등 입력과 출력 구성을 맞춘다.

6. 재생 준비
prepareToPlay()에 샘플레이트와 블록 크기를 전달한다.

7. 실시간 처리
processBlock()에 오디오 블록을 계속 전달한다.

8. 화면 생성
사용자가 창을 열면 Editor 객체를 만든다.

9. 자동화와 저장
파라미터 변경을 주고받고 프로젝트 상태를 저장한다.

10. 제거
인서트에서 빼면 인스턴스와 사용하던 자원을 정리한다.
```

사용자가 보는 플러그인 창은 이 과정의 일부일 뿐이다.

화면이 없어도 Processor는 오디오를 처리할 수 있다. 목록에 나타나도 인스턴스 생성은 실패할 수 있다. 컴파일에 성공해도 Cubase가 정상적으로 불러온다는 보장은 없다.

각 단계는 서로 다른 검증이 필요하다.

---

## 문제가 생긴 위치로 원인을 좁힐 수 있다

### 목록에 나타나지 않는다

```text
설치 경로
번들 구조
플러그인 식별 정보
CPU 아키텍처
호스트 스캔과 차단 목록
서명과 보안 정책
```

발견과 로딩 전 단계부터 확인한다.

### 목록에는 있지만 삽입하면 종료된다

```text
Processor 생성자
리소스 초기화
메모리 소유권
채널 구성
prepareToPlay()
```

플러그인 인스턴스를 만드는 단계가 중심이다.

### 재생을 시작하면 끊기거나 종료된다

```text
processBlock()
버퍼 범위
샘플레이트와 블록 크기 가정
실시간에 부적절한 파일·메모리·잠금 작업
숫자로 표현할 수 없는 계산 결과나 무한대 값
```

실시간 처리 경로를 확인한다.

### 창을 열 때만 종료된다

```text
Editor 생성자
노브와 Attachment 연결
화면 크기와 그리기 코드
LookAndFeel 객체 수명
```

오디오 처리보다 GUI 경로를 먼저 확인한다.

문제가 언제 발생하는지 알면 관련 없는 설정을 전부 지우는 대신 실제 실행 단계를 따라갈 수 있다.

---

## 빌드 성공과 실제 실행 성공은 다르다

빌드가 성공했다는 말은 소스 코드가 실행 가능한 VST3 번들로 만들어졌다는 뜻이다.

다음 항목까지 자동으로 증명하지는 않는다.

```text
Cubase가 플러그인을 발견하는가?
인스턴스를 안전하게 만들 수 있는가?
모든 샘플레이트와 블록 크기에서 동작하는가?
화면이 실제 DAW에서 정상적으로 보이는가?
자동화와 상태 복원이 되는가?
소리가 의도한 대로 변하는가?
```

검증을 단계별로 나눠야 한다.

```text
컴파일과 링크
        ↓
번들 구조와 CPU 아키텍처
        ↓
코드 서명
        ↓
플러그인 검증 도구
        ↓
실제 Cubase 삽입과 재생
        ↓
화면과 청감 확인
```

전용 검증 도구가 성공해도 화면의 노브가 타원으로 그려지는 문제까지 판단하지는 못한다. 실제 호스트에서의 확인이 따로 필요한 이유다.

---

## 현재 DustBox의 범위

현재 DustBox LoFi는 다음 조건의 알파 버전이다.

```text
형식       VST3
운영체제   macOS
CPU        Apple Silicon arm64
버전       0.2.1
서명       ad-hoc 테스트 서명
```

Intel Mac용 실행 코드와 정식 판매용 서명·공증은 아직 확보하지 않았다.

따라서 현재 결과물을 정식 판매 버전처럼 설명하면 안 된다. 구조와 실행 흐름을 확인하기 위한 프로토타입에 가깝다.

---

## 정리

VST3 플러그인은 Cubase와 별개로 혼자 실행되는 일반 앱이 아니다.

```text
Cubase가 플러그인을 찾고
        ↓
규격에 따라 객체를 만들고
        ↓
오디오 블록을 계속 전달하면
        ↓
플러그인이 버퍼 안의 값을 바꾸고
        ↓
Cubase가 결과를 재생한다.
```

각 기술의 역할도 구분할 수 있다.

```text
C++
제품의 동작을 작성하는 언어

JUCE
오디오 처리와 플러그인 제작에 필요한 공통 구조를 제공하는 프레임워크

VST3
플러그인과 Cubase가 통신하는 규격

CMake
소스와 설정을 어떤 결과물로 빌드할지 구성하는 도구

Cubase
VST3를 불러오고 실행하는 호스트
```

사용자가 보는 노브 뒤에서는 스캔, 인스턴스 생성, 채널 협상, 재생 준비, 블록 처리, 상태 저장이 이어진다.

이 흐름을 알고 나면 오류가 생겼을 때도 `플러그인이 안 된다`고 한꺼번에 보지 않고 어느 단계에서 실패했는지 나눠볼 수 있다.

---

## 참고 자료

- [Steinberg VST 3 Developer Portal — Plug-in Locations](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html)
- [Steinberg VST 3 Developer Portal — Plug-in Format Structure](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Format.html)
- [JUCE AudioProcessor Class Reference](https://docs.juce.com/master/classjuce_1_1AudioProcessor.html)
- [JUCE CMake API](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md)
