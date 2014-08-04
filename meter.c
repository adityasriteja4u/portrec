#include <math.h>
#include <ncurses.h>
#include "meter.h"

float meters_decay = 100.0;

float signal_power(jack_default_audio_sample_t *buf, jack_nframes_t nframes)
{
        float peak = 0.0;
        jack_nframes_t i;
        for (i = 0; i<nframes; ++i) {
                float amplitude = fabsf(buf[i]);
                if (amplitude>peak) peak = amplitude;
        }

        return 20.0 * log10(peak); // conversion to dB; reference value = 1.0
}

void display_meter(int y, int x, float value, float range, int width)
{
        int i;
        int strips = width*(value+range)/range;
        move(y, x);
        for (i = 0; i<width; ++i) addch(' ');
        move(y, x);
        for (i = 0; i<strips; ++i) addch('|');
        mvchgat(y, x, width-4, A_NORMAL, 1, NULL);
        mvchgat(y, x+width-4, 3, A_NORMAL, 2, NULL);
        mvchgat(y, x+width-1, 1, A_NORMAL, 3, NULL);
}
