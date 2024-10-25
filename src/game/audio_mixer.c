#include "all.h"

enum filter_type {
  Lowpass,
  Highpass,
  Bandpass
};

inline f32 mixAudio(f32 bufferDst, f32 bufferSrc) {
    return CLAMP(bufferSrc + bufferDst,-1.f,1.f);
}

inline f32 lowPassFilterValue(f32 value, f32 prevValue, 
                              f32 dt, i16 freq) {
  f32 RC = 1.f / ((f32)freq * 2.f * PI);
  f32 a = dt / (RC + dt);
  //printf("dt %f RC %f a %f value %f\n",dt, RC, a, value);
  return prevValue + a * (value - prevValue);
}

inline f32 highPassFilterValue(f32 value, f32 prevFilterValue,
                               f32 prevValue, f32 dt, i16 freq) {
  f32 RC = 1.f / ((f32)freq * 2.f * PI);
  f32 a = RC / (RC + dt);
  //printf("dt %f RC %f a %f value %f\n",dt, RC, a, value);
  return a * (prevFilterValue + value - prevValue);
}

inline f32 resonantLowpassFilter(f32 value, f32 prevValue,
                                 f32 dt, i16 freq, f32 feedbackAmount) {
  f32 RC = 1.f / ((f32)freq * 2.f * PI);
  f32 a = dt / (RC + dt);
  f32 fb = feedbackAmount + feedbackAmount/(1.0 - a);
  // lowpass
  f32 lowpassResult = prevValue + a * (value - prevValue);
  // Resonant feedback
  f32 result = lowpassResult + a * (value - lowpassResult + fb * (lowpassResult - prevValue));
  return result; 
}

inline f32 filter(f32 value, f32 dt, i16 freq, f32 feedbackAmount,
                  filter_type type, audio_filter_state filterState)
{
  f32 RC = 1.f / ((f32)freq * 2.f * PI);
  f32 a = dt / (RC + dt);

  f32 fb = feedbackAmount + feedbackAmount/(1.0f - a);

  filterState[0] += a * (value - filterState[0] + fb * (filterState[0] - filterState[1]));
  filterState[1] += a * (filterState[0] - filterState[1]);
  filterState[2] += a * (filterState[1] - filterState[2]);
  filterState[3] += a * (filterState[2] - filterState[3]);
  switch (type) {
    case Lowpass:
      return filterState[3];
    case Highpass:
      return value - filterState[3];
    case Bandpass:
      return filterState[0] - filterState[3];
    InvalidDefaultCase;
  }
  return 0.f;
}
