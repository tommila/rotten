#include "all.h"

typedef struct engine_sound_state {
  f64 phase;
  f32 prevHz;
  i32 sampleIndex;
} engine_sound_state;

inline f32 piston(f32 x) {  
  return cosf(4.f * PI * x);
}

inline f32 exhaust(f32 x) {  
  return (0.75 < x && x < 1.f) ? -sin(4 * PI * x) : 0;
}

inline f32 intake(f32 x) {  
  return (0 < x && x < 0.25f) ? sin(4 * PI * x) : 0.f;
}

inline f32 ignition(f32 x) {  
  return (0 < x && x < 0.15f) ? sin(2 * PI * (x * 0.15f + 0.5f)): 0.0f;
}


inline f32 sampleEngineAudio(car_audio_state* audioState, 
                             i32 samplesPerSecond, 
                             i32 rpm, b32 accelerating) {
  f32 normRpm = (f32)rpm / 7000.f;
  audioState->prevRpm = rpm;

  // Each ramp of the phasor corresponds to two full revolutions of the crankshaft,
  // according to the principles of operation of a four-stroke engine.
  // The engine speed is normally expressed in revolutions per minute, 
  // while frequency in audio signals is represented by cycles per second, 
  // therefore the phasor’s frequency is set according to the following conversion:
  // f = RPM / 120 (f = engine operation cycles per second)

  i32 cylinderAmount = 4;
  f32 invCylinderAmount = 1.f / cylinderAmount;
  f32 invSampleRate = 1.f / (f32)samplesPerSecond;
  audioState->engineCycle += rpm / 120.f;
  f32 r = ((f32)rand() / (f32)RAND_MAX) * 2.f - 1.f;
  f32 combined = 0.f;
  for (i32 i = 0; i < cylinderAmount; i++) {
    f32 x = audioState->engineCycle * invSampleRate + i * invCylinderAmount;
    x = x - (i32)x;

    f32 intakeNoiseVal = filter(
      r, invSampleRate, LERP(12000, 8000, MIN(normRpm * 1.2f, 1.0)), 0.2f, 
      Lowpass, audioState->intakeFilter);
    f32 pistonVal = piston(x) * 0.4f;
    f32 exhaustVal = exhaust(x) * 0.4f;
    f32 ignitionVal = ignition(x) * 0.4f;
    f32 intakeVal = intake(x) * intakeNoiseVal *
      (accelerating ?
      LERP(0.05, 0.4f, powf(normRpm,2.f)): 
      LERP(0.05, 0.3f, powf(normRpm,2.f)));

    combined += (pistonVal + exhaustVal + intakeVal + ignitionVal);
  }
  f32 filtered = filter(
    combined, invSampleRate, 12000,0.0f, Lowpass, audioState->engineFilter);
 
  return filtered;
} 
