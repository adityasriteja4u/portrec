#ifndef TRACK_H
#define TRACK_H

#include <jack/jack.h>

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
        jack_port_t *output_port;
        jack_default_audio_sample_t *tape;
        double vol;
        double pan; // 0.0=L 1.0=R
        int flags;

        // meters [dB, clipping=0 dB]; updated in process_track()
        float in_meter;
        float out_meter;

        // buffers (only valid during inside process callback)
        jack_nframes_t nframes;
        jack_default_audio_sample_t *in_buf;
        jack_default_audio_sample_t *out_buf;
};

struct track *new_track(jack_client_t *client, const char *name, int length,
                        const char *inport, const char *outport);

void delete_track(jack_client_t *client, struct track *track);

void export_track(struct track *track, const char *filename, int length);

void process_track(struct track *track,
                   int offset,
                   const jack_position_t *pos,
                   int pos_min,
                   int pos_max,
                   jack_transport_state_t transport);

void mix_track_to_master(struct track *track,
                         jack_default_audio_sample_t *L,
                         jack_default_audio_sample_t *R);

#endif
