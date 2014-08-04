#ifndef METER_H
#define METER_H

#include <jack/jack.h>

extern float meters_decay/*dB/sec*/;

float signal_power(jack_default_audio_sample_t *buf, jack_nframes_t nframes);
void display_meter(int y, int x, float value/*dB*/, float range, int width); // range must be positive

#endif
