#ifndef UI_H
#define UI_H

#include "track.h"

extern float meters_decay/*dB/sec*/;

void display_meter(int y, int x, float value/*dB*/, float range, int width); // range must be positive

int init_ui(); // returns 0 on success
void main_loop(struct track **tracks, int track_count);
void shutdown_ui();

#endif
