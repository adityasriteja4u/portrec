#include <math.h>
#include "meter.h"

float meters_decay = 3.0;

float signal_power(jack_default_audio_sample_t *buf, jack_nframes_t nframes)
{
        float peak = 0.0;
        jack_nframes_t i;
        for (i = 0; i<nframes; ++i) {
                float amplitude = abs(buf[i]);
                if (amplitude>peak) peak = amplitude;
        }

        return 20.0 * log10(peak); // conversion to dB; reference value = 1.0
}
