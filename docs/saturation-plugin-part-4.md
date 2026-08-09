---
title: '직접 Saturation 플러그인 만들어보기 4'
date: '2026-08-09'
description: 'Output을 추가해 처리 전후의 음량을 맞춰보자'
thumbnail: ''
---

## 새츄레이션이 좋아진 걸까, 소리만 커진 걸까?

3장에서는 샘플 배열 전체에 새츄레이션을 적용했다.

이번 장의 JavaScript 코드는 신호 처리 원리를 확인하기 위한 예제다. 실제 DustBox VST3 플러그인은 C++로 구현한다.

```js
function saturate(input, drive) {
  const driven = input * drive;
  return Math.tanh(driven);
}

function processSamples(samples, drive) {
  return samples.map((sample) => saturate(sample, drive));
}
```

Drive를 높이면 `tanh`에 더 큰 값이 들어간다. `tanh`의 비선형 곡선이 파형을 바꾸어 배음을 만들고, 큰 값일수록 출력의 증가 폭을 줄인다. 이 예제에서는 작은 값과 중간 크기의 값이 원본보다 커지면서 처리 후 RMS도 커진다. 다만 처리 후 RMS가 항상 커지는 것은 아니며, 결과는 입력 신호와 Drive 값에 따라 달라진다.

소리는 조금만 커져도 더 선명하고 힘 있게 느껴지기 쉽다. 이 상태로 원본과 비교하면 새츄레이션의 음색이 좋아진 것인지, 단순히 음량이 커서 좋아진 것인지 구분하기 어렵다.

그래서 새츄레이션 뒤에 `Output`을 둔다.

```text
입력 샘플
    ↓
Drive로 입력을 키움
    ↓
tanh로 파형을 바꿈
    ↓
Output으로 최종 크기를 조절
    ↓
출력 샘플
```

## Output을 코드에 추가하기

Output은 `tanh`를 지난 값에 마지막으로 곱할 숫자다.

```js
function saturate(input, drive, output) {
  const driven = input * drive;
  const saturated = Math.tanh(driven);
  return saturated * output;
}
```

`output`이 1이면 크기를 바꾸지 않는다.

```text
0.8 × 1 = 0.8
```

`output`이 0.5면 `tanh`를 지난 각 샘플의 진폭을 절반으로 줄인다. 귀에 들리는 음량이 정확히 절반이 된다는 뜻은 아니다.

```text
0.8 × 0.5 = 0.4
```

중요한 순서는 `Drive → tanh → Output`이다. Output은 새츄레이션에 들어가기 전의 입력을 줄이는 값이 아니다. 이미 바뀐 파형의 최종 크기만 조절한다.

```js
function processSamples(samples, drive, output = 1) {
  return samples.map((sample) => saturate(sample, drive, output));
}
```

기본값을 1로 두었기 때문에 세 번째 값을 생략하면 이전 코드와 똑같이 작동한다.

## 얼마나 줄여야 할까?

Output을 무조건 0.5로 정할 수는 없다. 입력 소리와 Drive 값에 따라 처리 후 음량이 달라지기 때문이다.

먼저 처리 전후 샘플의 전체 크기를 비교해야 한다. 여기서는 `RMS`를 사용한다.

## RMS는 왜 필요할까?

RMS는 `Root Mean Square`의 줄임말이다. 한국어로 풀면 제곱한 값의 평균에 다시 제곱근을 씌운 값이다. 일정 구간의 신호가 어느 정도 크기를 유지하는지 숫자 하나로 비교할 때 쓴다.

샘플의 크기를 단순히 더해서 평균을 내면 안 될까?

소리의 파형은 0을 기준으로 양수와 음수를 오간다. 두 샘플이 `0.5`, `-0.5`라면 일반 평균은 0이 된다.

```text
(0.5 + -0.5) ÷ 2 = 0
```

하지만 실제로 신호가 없는 것은 아니다. 서로 반대편에 있는 두 값이 계산 과정에서 상쇄됐을 뿐이다.

RMS는 이 문제를 세 단계로 처리한다.

```text
샘플              0.5,  -0.5
                    ↓ 각각 제곱
제곱한 값         0.25,  0.25
                    ↓ 평균
제곱의 평균       0.25
                    ↓ 제곱근
RMS               0.5
```

먼저 각 샘플을 제곱하면 음수도 양수가 되어 서로 상쇄되지 않는다. 그 값들의 평균을 구한 뒤 제곱근을 씌우면, 제곱되어 바뀐 스케일을 원래 샘플 크기의 스케일로 되돌릴 수 있다.

이를 코드로 옮기면 다음과 같다.

```js
function getRms(samples) {
  if (samples.length === 0) {
    throw new Error('RMS를 계산할 샘플이 없습니다.');
  }

  const squareSum = samples.reduce(
    (sum, sample) => sum + sample * sample,
    0,
  );

  const meanSquare = squareSum / samples.length;
  return Math.sqrt(meanSquare);
}
```

```text
sample * sample                 각 샘플을 제곱
squareSum / samples.length      제곱한 값의 평균
Math.sqrt(meanSquare)           평균의 제곱근
```

RMS가 사람에게 들리는 음량을 완벽하게 나타내는 것은 아니다. 귀는 주파수와 소리의 지속 시간에도 영향을 받는다. 여기서는 새츄레이션 전후의 신호 크기를 같은 기준으로 비교하기 위해 RMS를 사용한다.

3장에서 사용한 샘플 배열을 다시 넣어보자.

```js
const inputSamples = [
  0.02,
  0.18,
  0.46,
  0.81,
  0.43,
  0.1,
  -0.25,
  -0.72,
];

const saturatedSamples = processSamples(inputSamples, 2);

console.log('처리 전 RMS:', getRms(inputSamples).toFixed(6));
console.log('처리 후 RMS:', getRms(saturatedSamples).toFixed(6));
```

실행 결과는 다음과 같다.

```text
처리 전 RMS: 0.457753
처리 후 RMS: 0.616300
```

Drive 2를 적용한 뒤 RMS가 커졌다. 처리 후 RMS를 원본과 비슷하게 맞추려면 다음 비율을 구한다.

```js
const saturatedRms = getRms(saturatedSamples);
const output =
  saturatedRms === 0
    ? 1
    : getRms(inputSamples) / saturatedRms;

console.log('Output:', output.toFixed(6));
```

처리 후 RMS가 0이면 비율을 계산할 수 없으므로 Output을 1로 둔다. 예를 들어 입력이 무음이면 RMS를 맞출 필요도 없다.

```text
Output: 0.742743
```

이 값을 새츄레이션 뒤에 곱하면 된다.

```js
const matchedSamples = processSamples(inputSamples, 2, output);

console.log('보정 후 RMS:', getRms(matchedSamples).toFixed(6));
```

```text
보정 후 RMS: 0.457753
```

이번 예제에서는 Output을 약 0.743으로 두었더니 처리 전후 RMS가 같아졌다.

```text
처리 전 RMS       0.457753
새츄레이션 후     0.616300
Output 보정 후    0.457753
```

이제 원본과 처리된 소리를 비슷한 크기로 비교할 수 있다. 차이가 들린다면 단순한 음량 상승보다는 파형과 배음의 변화를 듣고 있을 가능성이 커진다.

## 자동 계산과 Output 노브는 다르다

위 계산은 Output의 역할을 확인하기 위한 예제다. 실제 플러그인의 Output 노브가 매 순간 RMS를 계산해 자동으로 값을 바꾼다는 뜻은 아니다.

실제 플러그인에서는 보통 사용자가 Output 노브를 직접 조절한다.

```text
Drive를 올림
→ 처리 후 소리가 커짐
→ Output을 낮춤
→ 원본과 비슷한 음량에서 비교
```

자동 보정은 측정 구간에 따라 값이 계속 달라질 수 있다. Output이 빠르게 변하면 의도하지 않은 음량 변화도 생길 수 있다. 여기서는 준비된 샘플 배열 전체의 RMS를 한 번 측정해 Output 값을 구한 뒤, 그 값을 고정해서 결과를 확인한다. 이는 Output의 역할을 설명하기 위한 실험이며, 실제 DustBox VST3에 자동 RMS 보정 기능이 있다는 뜻은 아니다.

RMS가 같다고 사람에게 완전히 같은 음량으로 들리는 것도 아니다. 귀는 주파수와 소리의 지속 시간에도 영향을 받는다. RMS는 이번 실험에서 처리 전후의 크기를 비교하기 위한 간단한 기준이다.

## 지금까지 만든 코드

```js
function saturate(input, drive, output) {
  const driven = input * drive;
  const saturated = Math.tanh(driven);
  return saturated * output;
}

function processSamples(samples, drive, output = 1) {
  return samples.map((sample) => saturate(sample, drive, output));
}

function getRms(samples) {
  if (samples.length === 0) {
    throw new Error('RMS를 계산할 샘플이 없습니다.');
  }

  const squareSum = samples.reduce(
    (sum, sample) => sum + sample * sample,
    0,
  );
  const meanSquare = squareSum / samples.length;
  return Math.sqrt(meanSquare);
}

const inputSamples = [0.02, 0.18, 0.46, 0.81, 0.43, 0.1, -0.25, -0.72];

const saturatedSamples = processSamples(inputSamples, 2);
const saturatedRms = getRms(saturatedSamples);
const output = saturatedRms === 0
  ? 1
  : getRms(inputSamples) / saturatedRms;
const matchedSamples = processSamples(inputSamples, 2, output);

console.log('처리 전 RMS:', getRms(inputSamples).toFixed(6));
console.log('새츄레이션 후 RMS:', getRms(saturatedSamples).toFixed(6));
console.log('Output:', output.toFixed(6));
console.log('보정 후 RMS:', getRms(matchedSamples).toFixed(6));
```

이제 JavaScript 예제에서 `Drive → tanh → Output`으로 이어지는 최소 샘플 처리 순서가 완성됐다. `getRms()`와 Output 비율 계산은 이 순서를 비교하기 위한 실험용 코드이며, 자동 보정 제품 기능은 아니다.

다음에는 JavaScript 실험을 끝내고 C++, JUCE, VST3가 각각 어떤 역할을 하는지 살펴본다. 그다음 Cubase가 전달한 실제 오디오 버퍼를 `processBlock()`에서 처리해보자.
