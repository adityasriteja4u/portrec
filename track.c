#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include "audio.h"
#include "track.h"
#include "ui.h"

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
        SNDFILE *f;
        SF_INFO info;

        memset(&info, 0, sizeof(info));
        info.samplerate = 48000;
        info.channels = 1;
        info.format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
        /* Also possible formats:
         *   SF_FORMAT_FLAC | SF_FORMAT_PCM_S8
         *   SF_FORMAT_FLAC | SF_FORMAT_PCM_16
         *   SF_FORMAT_FLAC | SF_FORMAT_PCM_24
         *   SF_FORMAT_OGG  | SF_FORMAT_VORBIS
         */

        f = sf_open(filename, SFM_WRITE, &info);
        if (f==NULL)
                fatal("couldn't open file %s\n", filename);
        if (sf_writef_float(f, track->tape, length)!=length)
                fatal("couldn't write to file %s\n", filename);
        sf_close(f);

}

void process_track(struct track *track,
                   int offset,
                   const jack_position_t *pos,
                   int pos_min,
                   int pos_max,
                   jack_transport_state_t transport,
                   jack_default_audio_sample_t *L,
                   jack_default_audio_sample_t *R)
{
        jack_nframes_t i;
        int j = pos->frame;
        if (transport!=JackTransportRolling || track->flags&TRACK_MUTE) {
                for (i = 0; i<track->nframes; ++i) track->out_buf[i] = 0.0;
        } else if (track->flags&TRACK_REC) {
                j -= offset;
                for (i = 0; i<track->nframes; ++i) {
                        if (j>=pos_min && j<pos_max) track->tape[j] = track->in_buf[i];
                        track->out_buf[i] = 0.0;
                        j++;
                }
        } else {
                for (i = 0; i<track->nframes; ++i) {
                        if (j>=pos_min && j<pos_max) track->out_buf[i] = track->tape[j];
                        else track->out_buf[i] = 0.0;
                        j++;
                }
        }

        // update the meters
        float in  = signal_power(track->in_buf,  track->nframes);
        float out = signal_power(track->out_buf, track->nframes);
        float decay = meters_decay * track->nframes / pos->frame_rate;
        track->in_meter  -= decay;
        track->out_meter -= decay;
        if (in>track->in_meter)   track->in_meter  = in;
        if (out>track->out_meter) track->out_meter = out;

        // mix track to master bus
        for (i = 0; i<track->nframes; ++i) {
                *L++ += (1.0-track->pan) * track->vol * track->out_buf[i];
                *R++ +=      track->pan  * track->vol * track->out_buf[i];
        }
}
