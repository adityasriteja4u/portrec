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

void display_meter(int y, int x, float value)
{
        int i;
        mvaddstr(y, x, "          ");
        move(y, x);
        attron(COLOR_PAIR(1));
        for (i = 0; -80.0+i*10.0<=value; ++i) {
                if (i==4) {
                        attroff(COLOR_PAIR(1));
                        attron(COLOR_PAIR(2));
                }
                if (i==7) {
                        attroff(COLOR_PAIR(2));
                        attron(COLOR_PAIR(3));
                }
                addch(' ');
        }
        attroff(COLOR_PAIR(1));
        attroff(COLOR_PAIR(2));
        attroff(COLOR_PAIR(3));
}
