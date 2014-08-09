#ifndef TRACK_H
#define TRACK_H

#include "audio.h"

enum
{
        TRACK_MUTE = 0x1,
        TRACK_SOLO = 0x2,
        TRACK_REC  = 0x4
};

struct track
{
        // general
        char *name;
        int length;
        jack_port_t *input_port;
        frame_t *tape;
        double vol;
        double pan; // 0.0=L 1.0=R
        int flags;

        // meters [dB, clipping=0 dB]; updated in process_track()
        float in_meter;
        float out_meter;

        // buffers (only valid during inside process callback)
        jack_nframes_t nframes;
        frame_t *in_buf;
};

struct track *new_track(jack_client_t *client, const char *name, int length,
                        const char *inport, const char *outport);

void delete_track(jack_client_t *client, struct track *track);

void export_track(struct track *track, const char *filename, int length);

void process_track(struct track *track,
                   int offset,
                   int pos_min,
                   int pos_max,
                   frame_t *L,
                   frame_t *R);

#endif
