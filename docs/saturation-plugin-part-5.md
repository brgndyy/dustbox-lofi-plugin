---
title: '직접 Saturation 플러그인 만들어보기 5'
date: '2026-08-09'
description: '브라우저에서 실제 오디오 파일에 새츄레이션을 적용해보자'
thumbnail: ''
---

## 숫자 배열 대신 실제 오디오를 넣어보자

앞에서는 직접 만든 샘플 배열로 새츄레이션의 동작을 확인했다.

```js
const inputSamples = [
  0.02, 0.18, 0.46, 0.81, 0.43, 0.1, -0.25, -0.72,
];
```

원리를 확인하기에는 충분하지만 실제 음악은 이렇게 짧은 배열로 준비되어 있지 않다.

이번에는 브라우저에서 오디오 파일을 선택하고, 파일 안의 샘플 전체에 지금까지 만든 로직을 적용해본다.

```text
오디오 파일 선택
→ 브라우저가 파일을 샘플로 변환
→ 각 샘플에 Drive와 tanh 적용
→ RMS를 비교해 Output 계산
→ 원본과 처리본 재생
```

이번 장의 코드는 원리를 직접 듣기 위한 브라우저 실험이다. 실제 DustBox VST3는 C++로 구현한다.

## 브라우저는 오디오 파일을 어떻게 읽을까?

브라우저에는 소리를 읽고 재생하고 처리하기 위한 `Web Audio API`가 있다.

이 기능을 사용하려면 먼저 `AudioContext`를 만든다. AudioContext는 브라우저 안에서 오디오를 처리하고 재생하는 작업 공간이다.

```js
const audioContext = new AudioContext();
```

사용자가 선택한 MP3나 WAV 파일을 곧바로 샘플 배열처럼 사용할 수는 없다. 먼저 파일 데이터를 읽은 뒤 브라우저가 처리할 수 있는 `AudioBuffer`로 변환해야 한다.

```text
MP3 또는 WAV 파일
        ↓
File.arrayBuffer()
        ↓
decodeAudioData()
        ↓
AudioBuffer
```

`AudioBuffer`에는 다음 정보가 들어 있다.

```text
numberOfChannels   채널 수
length             채널마다 들어 있는 샘플 수
sampleRate         1초당 샘플 수
getChannelData()   특정 채널의 샘플 배열
```

AudioBuffer의 Sample Rate가 44.1kHz라면 한 채널에 1초당 44,100개의 샘플이 들어 있다. 스테레오는 왼쪽과 오른쪽 채널이 따로 있으므로 각 채널의 샘플을 모두 처리해야 한다.

여기서 `AudioBuffer.sampleRate`가 원본 파일의 Sample Rate와 항상 같다고 생각하면 안 된다. `decodeAudioData()`는 보통 파일을 현재 AudioContext의 Sample Rate에 맞춰 리샘플링한다. 예를 들어 48kHz 파일을 44.1kHz AudioContext에서 디코딩하면 AudioBuffer는 44.1kHz가 될 수 있다.

따라서 처리할 샘플 수와 Sample Rate는 추측하지 않고 디코딩된 `AudioBuffer.length`와 `AudioBuffer.sampleRate`에서 읽는다.

## 오디오 파일 선택하기

HTML에는 브라우저가 기본으로 제공하는 파일 입력창을 사용한다.

```html
<input id="audio-file" type="file" accept="audio/*">
```

`accept="audio/*"`는 오디오 파일을 선택한다는 힌트를 브라우저에 전달한다.

사용자가 파일을 선택하면 파일 데이터를 읽고 AudioBuffer로 변환한다.

```js
const fileInput = document.querySelector('#audio-file');
const audioContext = new AudioContext();

let originalBuffer = null;

fileInput.addEventListener('change', async () => {
  const [file] = fileInput.files;
  if (!file) return;

  const fileData = await file.arrayBuffer();
  originalBuffer = await audioContext.decodeAudioData(fileData);
});
```

이 처리는 브라우저 안에서만 진행된다. 현재 데모는 선택한 파일을 서버로 전송하지 않는다.

## AudioBuffer에서 샘플 꺼내기

AudioBuffer는 채널별로 샘플을 보관한다.

```js
const leftSamples = originalBuffer.getChannelData(0);
```

스테레오 파일이라면 다음처럼 오른쪽 채널도 있다.

```js
const rightSamples = originalBuffer.getChannelData(1);
```

하지만 모든 파일이 스테레오인 것은 아니다. 채널 수를 고정하지 않고 반복문을 사용한다.

```js
for (let channel = 0;
  channel < originalBuffer.numberOfChannels;
  channel += 1) {
  const samples = originalBuffer.getChannelData(channel);
  console.log(channel, samples.length);
}
```

각 채널에서 얻은 `samples`는 앞에서 직접 만들었던 숫자 배열과 역할이 같다.

```text
직접 만든 배열              실제 오디오 파일

inputSamples                getChannelData(channel)
[0.02, 0.18, ...]     →     Float32Array(44100) ...
```

차이는 샘플이 훨씬 많다는 점이다.

## 모든 채널에 새츄레이션 적용하기

원본 AudioBuffer를 직접 바꾸지 않고 같은 길이와 채널 수를 가진 새 AudioBuffer를 만든다. 그래야 원본과 처리본을 각각 재생하며 비교할 수 있다.

```js
function createSaturatedBuffer(inputBuffer, drive) {
  const outputBuffer = audioContext.createBuffer(
    inputBuffer.numberOfChannels, inputBuffer.length, inputBuffer.sampleRate,
  );

  for (let channel = 0;
    channel < inputBuffer.numberOfChannels;
    channel += 1) {
    const input = inputBuffer.getChannelData(channel);
    const output = outputBuffer.getChannelData(channel);

    for (let i = 0; i < input.length; i += 1) {
      output[i] = Math.tanh(input[i] * drive);
    }
  }

  return outputBuffer;
}
```

처리 순서는 지금까지 만든 코드와 같다.

```text
input[i]
   ↓ Drive를 곱함
input[i] * drive
   ↓ tanh 적용
output[i]
```

바뀐 것은 샘플 하나가 아니라 AudioBuffer의 모든 채널과 모든 샘플에 반복한다는 점뿐이다.

## AudioBuffer의 RMS 구하기

앞에서 만든 `getRms()`는 배열 하나를 받았다. 실제 오디오에서는 모든 채널의 샘플을 합쳐 RMS를 계산한다.

```js
function getRmsFromBuffer(buffer) {
  let squareSum = 0;
  let sampleCount = 0;

  for (let channel = 0; channel < buffer.numberOfChannels; channel += 1) {
    const samples = buffer.getChannelData(channel);

    for (let i = 0; i < samples.length; i += 1) {
      squareSum += samples[i] * samples[i];
    }

    sampleCount += samples.length;
  }

  return sampleCount === 0 ? 0 : Math.sqrt(squareSum / sampleCount);
}
```

계산 원리는 달라지지 않았다.

```text
모든 채널의 샘플을 제곱해 더함
→ 전체 샘플 수로 나눔
→ 제곱근 적용
→ AudioBuffer 전체의 RMS
```

여기서 구한 RMS는 원본과 처리본의 신호 크기를 같은 방식으로 비교하기 위한 값이다. 사람이 느끼는 음량을 완벽하게 나타내는 값은 아니다.

## Output을 계산하고 적용하기

이제 원본 RMS와 새츄레이션 처리 후 RMS를 구한다.

```js
const inputRms = getRmsFromBuffer(originalBuffer);
const processedBuffer = createSaturatedBuffer(originalBuffer, 2);
const saturatedRms = getRmsFromBuffer(processedBuffer);
```

원본과 같은 RMS로 맞추기 위한 Output은 다음처럼 계산한다.

```js
const outputGain = saturatedRms === 0
  ? 1
  : inputRms / saturatedRms;
```

여기서 `outputGain`은 dB가 아니라 각 샘플에 곱하는 선형 배율이다. 실제 검증값 `0.614626`은 약 `-4.23dB`에 해당한다.

RMS를 맞춰도 사람이 느끼는 음량이나 파형의 가장 높은 값인 Peak까지 같아지는 것은 아니다. Output을 적용한 뒤 Peak가 허용 범위를 넘지 않는지도 별도로 확인해야 한다. RMS 보정 자체가 클리핑을 자동으로 막아주지는 않는다.

계산한 Output을 처리된 모든 샘플에 곱한다.

```js
function applyOutputGain(buffer, outputGain) {
  for (let channel = 0; channel < buffer.numberOfChannels; channel += 1) {
    const samples = buffer.getChannelData(channel);

    for (let i = 0; i < samples.length; i += 1) {
      samples[i] *= outputGain;
    }
  }
}

applyOutputGain(processedBuffer, outputGain);
```

전체 처리 순서는 다음과 같다.

```text
원본 AudioBuffer의 RMS 측정
        ↓
새 AudioBuffer 생성
        ↓
모든 채널의 모든 샘플에 Drive와 tanh 적용
        ↓
처리된 AudioBuffer의 RMS 측정
        ↓
원본 RMS ÷ 처리 후 RMS
        ↓
처리된 모든 샘플에 Output 적용
```

## 원본과 처리본 재생하기

AudioBuffer를 재생하려면 `AudioBufferSourceNode`를 만든다.

```js
let playingSource = null;

function stopPlayback() {
  if (!playingSource) return;

  playingSource.onended = null;
  try {
    playingSource.stop();
  } catch {
    // 이미 끝난 source는 다시 멈출 필요가 없다.
  }
  playingSource.disconnect();
  playingSource = null;
}

async function playBuffer(buffer) {
  await audioContext.resume();
  stopPlayback();

  const source = audioContext.createBufferSource();
  source.buffer = buffer;
  source.connect(audioContext.destination);
  source.onended = () => {
    source.disconnect();
    if (playingSource === source) playingSource = null;
  };
  playingSource = source;
  source.start();
}
```

`audioContext.destination`은 브라우저가 소리를 내보낼 최종 목적지다. 일반적으로 현재 사용 중인 스피커나 오디오 출력 장치로 연결된다.

AudioBufferSourceNode는 한 번 `start()`한 뒤 다시 사용할 수 없는 일회용 노드다. 재생 버튼을 누를 때마다 새 노드를 만들고, 기존 재생은 `stop()`과 `disconnect()`로 정리한다. 새 파일을 선택하거나 Drive를 바꿀 때도 이전 재생을 멈춰야 화면에 표시된 상태와 실제 소리가 어긋나지 않는다.

이제 원본 AudioBuffer와 처리된 AudioBuffer를 각각 넣어 재생할 수 있다.

```js
playOriginalButton.addEventListener(
  'click',
  () => playBuffer(originalBuffer),
);

playProcessedButton.addEventListener(
  'click',
  () => playBuffer(processedBuffer),
);
```

브라우저는 자동 재생을 제한하므로 실제 데모에서는 사용자가 버튼을 눌렀을 때만 재생한다.

## 실제로 실행해보기

이번 장의 전체 데모는 다음 파일에 작성했다.

```text
examples/browser-saturation-demo.html
```

별도의 라이브러리나 설치 과정은 없다. 파일을 브라우저에서 열고 오디오 파일을 선택하면 된다.

실제 검증에서는 44.1kHz, 440Hz 사인파 WAV를 생성해 브라우저에서 불러왔다. Drive 2를 적용한 결과는 다음과 같았다.

```text
원본 RMS       0.353548
처리 후 RMS    0.575224
Output         0.614626
보정 후 RMS    0.353548
```

```text
원본 RMS와 보정 후 RMS
0.353548 = 0.353548
```

원본 재생, 새츄레이션 처리, 처리본 재생 버튼도 모두 활성화됐고 브라우저 콘솔 오류는 발생하지 않았다.

다만 RMS가 같아졌다고 음색까지 같아진 것은 아니다. `tanh`가 파형을 바꾸면서 생긴 배음은 남아 있다. Output은 바뀐 파형의 최종 크기만 맞춘다.

<div style="break-before: page;"></div>

## 브라우저 데모와 실제 VST3의 차이

이번 코드는 오디오 파일 전체를 메모리에 읽은 다음 한 번에 처리한다.

```text
브라우저 데모
오디오 파일 전체를 읽음
→ 전체 샘플 처리
→ 결과 재생
```

실제 VST3 플러그인은 곡 전체를 미리 받지 않는다. Cubase가 재생 중인 오디오를 짧은 샘플 묶음으로 계속 전달한다.

```text
실제 VST3
Cubase가 짧은 오디오 버퍼 전달
→ C++가 즉시 처리
→ Cubase에 반환
→ 다음 버퍼 처리
```

새츄레이션의 계산 원리는 같지만 오디오를 전달받고 처리하는 방식은 다르다.

이번 데모의 자동 Output 계산은 원본과 처리본의 전체 RMS를 먼저 알아야 한다. 파일 전체를 읽을 수 있는 오프라인 실험이기 때문에 가능한 방식이다.

실시간 VST3는 아직 재생되지 않은 미래의 샘플을 알 수 없다. 따라서 다음 장에서는 Output을 사용자가 정한 고정 선형 배율로 적용한다.

```text
실시간 VST3의 기본 흐름
짧은 버퍼를 받음
→ 각 샘플에 Drive와 tanh 적용
→ 사용자가 정한 Output을 곱함
→ 즉시 반환
```

자동 보정 기능을 나중에 추가한다면 지금까지 들어온 신호로 RMS를 추적하고, Output이 갑자기 바뀌지 않도록 변화를 부드럽게 이어야 한다. 버퍼마다 계산한 비율을 즉시 적용하면 음량이 출렁이거나 경계에서 불연속이 생길 수 있다.

실시간 오디오 처리 함수 안에서는 파일 전체를 다시 훑거나 처리할 때마다 새 AudioBuffer를 만들지 않는다. Cubase가 전달한 버퍼를 정해진 시간 안에 바로 처리한다.

다음에는 JavaScript로 만든 `Drive → tanh → 고정 Output` 로직을 실제 DustBox의 C++ 오디오 처리 코드에 연결해보자.
