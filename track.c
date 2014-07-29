#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "track.h"

void fatal(const char *fmt, ...);

struct track *new_track(jack_client_t *client, const char *name, int length,
                        const char *inport, const char *outport)
{
        struct track *track = malloc(sizeof(struct track));
        track->name = malloc(strlen(name)+1);
        track->length = length;
        strcpy(track->name, name);
        track->tape = calloc(length, sizeof(jack_default_audio_sample_t));

        track->input_port =  jack_port_register(client, inport,
                                                JACK_DEFAULT_AUDIO_TYPE,
                                                JackPortIsInput, 0);
        track->output_port = jack_port_register(client, outport,
                                                JACK_DEFAULT_AUDIO_TYPE,
                                                JackPortIsOutput, 0);

        track->in_buf  = NULL;
        track->out_buf = NULL;

        if ((track->input_port==NULL) || (track->output_port==NULL))
                fatal("no more JACK ports available\n");

        track->vol = 1.0;
        track->pan = 0.5;
        track->flags = 0;

        return track;
}

void delete_track(jack_client_t *client, struct track *track)
{
        jack_port_unregister(client, track->input_port);
        jack_port_unregister(client, track->output_port);
        free(track->tape);
        free(track->name);
        free(track);
}

void export_track(struct track *track, const char *filename, int length)
{
        FILE *f = fopen(filename, "w");
        fwrite(track->tape, sizeof(jack_default_audio_sample_t), length, f);
        fclose(f);

}

void process_track(struct track *track,
                   int offset,
                   const jack_position_t *pos,
                   int pos_min,
                   int pos_max,
                   jack_transport_state_t transport)
{
        jack_default_audio_sample_t *p = track->tape + pos->frame;
        jack_nframes_t i;
        int j = pos->frame;
        if (transport!=JackTransportRolling || track->flags&TRACK_MUTE) {
                for (i = 0; i<track->nframes; ++i) *track->out_buf++ = 0.0;
        } else if (track->flags&TRACK_REC) {
                p -= offset;
                j -= offset;
                for (i = 0; i<track->nframes; ++i) {
                        if (j>=pos_min && j<pos_max) *p++ = *track->in_buf++;
                        *track->out_buf++ = 0.0;
                        j++;
                }
        } else {
                for (i = 0; i<track->nframes; ++i) {
                        if (j>=pos_min && j<pos_max) *track->out_buf++ = *p++;
                        else *track->out_buf++ = 0.0;
                        j++;
                }
        }
}

void mix_track_to_master(struct track *track,
                         jack_default_audio_sample_t *L,
                         jack_default_audio_sample_t *R)
{
        jack_nframes_t i;
        for (i = 0; i<track->nframes; ++i) {
                *L++ += 2.0 * (1.0-track->pan) * track->vol * *track->in_buf;
                *R++ += 2.0 *      track->pan  * track->vol * *track->in_buf;
                ++track->in_buf;
        }
}
