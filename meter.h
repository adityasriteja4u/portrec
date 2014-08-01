#ifndef METER_H
#define METER_H

#include <jack/jack.h>

extern float meters_decay; // dB/sec

float signal_power(jack_default_audio_sample_t *buf, jack_nframes_t nframes);

#endif
