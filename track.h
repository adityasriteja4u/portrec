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
                        const char *port);

void delete_track(jack_client_t *client, struct track *track);

/* Returns:
 *   0 on success,
 *   1 if could not open a file
 *   2 if a file was in incompatible format (channels, sample rate)
 *   3 if could not read from a file
 */
int import_track(struct track *track, const char *filename);

void export_track(struct track *track, const char *filename);

void process_track(struct track *track,
                   int offset,
                   frame_t *L,
                   frame_t *R);

#endif
