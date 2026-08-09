---
title: '직접 Saturation 플러그인 만들어보기 4'
date: '2026-08-09'
description: 'Output을 추가해 처리 전후의 음량을 맞춰보자'
thumbnail: ''
---

## 새츄레이션이 좋아진 걸까, 소리만 커진 걸까?

3장에서는 샘플 배열 전체에 새츄레이션을 적용했다.

```js
function saturate(input, drive) {
  const driven = input * drive;
  return Math.tanh(driven);
}

function processSamples(samples, drive) {
  return samples.map((sample) => saturate(sample, drive));
}
```

Drive를 높이면 `tanh` 곡선에 더 큰 값이 들어간다. 파형의 큰 부분은 완만하게 눌리고 배음도 생기지만, 작은 입력은 원래보다 크게 나올 수 있다. 처리 후의 전체 음량이 커지는 이유다.

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

`output`이 0.5면 절반으로 줄인다.

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

먼저 처리 전후 샘플의 전체 크기를 비교해야 한다. 여기서는 각 샘플을 제곱해 평균을 낸 뒤 제곱근을 구하는 `RMS`를 사용한다. RMS는 시간 구간 안에서 신호가 어느 정도 크기를 유지하는지 숫자 하나로 나타내는 방법이다.

```js
function getRms(samples) {
  const squareSum = samples.reduce(
    (sum, sample) => sum + sample * sample,
    0,
  );

  return Math.sqrt(squareSum / samples.length);
}
```

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

console.log(getRms(inputSamples));
console.log(getRms(saturatedSamples));
```

실행 결과는 다음과 같다.

```text
처리 전 RMS: 0.457753
처리 후 RMS: 0.616300
```

Drive 2를 적용한 뒤 RMS가 커졌다. 처리 후 RMS를 원본과 비슷하게 맞추려면 다음 비율을 구한다.

```js
const output = getRms(inputSamples) / getRms(saturatedSamples);

console.log(output);
```

```text
Output: 0.742743
```

이 값을 새츄레이션 뒤에 곱하면 된다.

```js
const matchedSamples = processSamples(inputSamples, 2, output);

console.log(getRms(matchedSamples));
```

```text
보정 후 RMS: 0.457753
```

이번 예제에서는 Output을 약 0.743으로 두자 처리 전후 RMS가 같아졌다.

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

자동 보정은 편리하지만 측정 구간에 따라 값이 계속 달라질 수 있다. Output이 빠르게 움직이면 의도하지 않은 음량 변화도 생긴다. 지금 만드는 최소 예제에서는 자동 보정 기능을 넣지 않고, Output을 하나의 고정된 값으로 둔다.

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
  const squareSum = samples.reduce(
    (sum, sample) => sum + sample * sample,
    0,
  );
  return Math.sqrt(squareSum / samples.length);
}

const inputSamples = [
  0.02, 0.18, 0.46, 0.81, 0.43, 0.1, -0.25, -0.72,
];

const saturatedSamples = processSamples(inputSamples, 2);
const output = getRms(inputSamples) / getRms(saturatedSamples);
const matchedSamples = processSamples(inputSamples, 2, output);

console.log('처리 전 RMS:', getRms(inputSamples));
console.log('새츄레이션 후 RMS:', getRms(saturatedSamples));
console.log('Output:', output);
console.log('보정 후 RMS:', getRms(matchedSamples));
```

실행 흐름은 다음과 같다.

```text
샘플 배열을 받는다
→ 각 샘플에 Drive를 곱한다
→ tanh로 파형을 바꾼다
→ Output을 곱한다
→ 처리된 샘플 배열을 반환한다
```

이제 새츄레이션의 최소 처리 구조가 완성됐다.

다만 지금 코드는 이미 준비된 숫자 배열만 처리한다. 다음에는 브라우저에서 실제 오디오 파일을 불러오고, 처리 전후의 소리를 직접 들어보자.
