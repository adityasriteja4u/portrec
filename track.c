#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include "audio.h"
#include "track.h"
#include "ui.h"

void fatal(const char *fmt, ...);

struct track *new_track(jack_client_t *client, const char *name, int length,
                        const char *port)
{
        struct track *track = malloc(sizeof(struct track));
        track->name = malloc(strlen(name)+1);
        track->length = length;
        strcpy(track->name, name);
        track->tape = calloc(length, sizeof(frame_t));

        track->input_port =  jack_port_register(client, port,
                                                JACK_DEFAULT_AUDIO_TYPE,
                                                JackPortIsInput, 0);

        track->in_buf  = NULL;

        if (track->input_port==NULL) fatal("no more JACK ports available\n");

        track->vol = 1.0;
        track->pan = 0.5;
        track->flags = 0;

        return track;
}

void delete_track(jack_client_t *client, struct track *track)
{
        jack_port_unregister(client, track->input_port);
        free(track->tape);
        free(track->name);
        free(track);
}

int import_track(struct track *track, const char *filename)
{
        SNDFILE *f;
        SF_INFO info;

        memset(&info, 0, sizeof(info));

        f = sf_open(filename, SFM_READ, &info);
        if (f==NULL)
                return 1;
        if (info.samplerate!=frame_rate || info.channels!=1) {
                sf_close(f);
                return 2;
        }
        if (sf_readf_float(f, track->tape, track->length)!=track->length)
                return 3;
        sf_close(f);
        return 0;
}

void export_track(struct track *track, const char *filename)
{
        SNDFILE *f;
        SF_INFO info;

        memset(&info, 0, sizeof(info));
        info.samplerate = frame_rate;
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
        if (sf_writef_float(f, track->tape, track->length)!=track->length)
                fatal("couldn't write to file %s\n", filename);
        sf_close(f);

}

void process_track(struct track *track,
                   int offset,
                   frame_t *L,
                   frame_t *R)
{
        jack_nframes_t i;
        int j = frame;
        if (transport==ROLLING && track->flags&TRACK_REC) {
                j -= offset;
                for (i = 0; i<track->nframes; ++i) {
                        if (j>=0 && j<track->length) track->tape[j] = track->in_buf[i];
                        j++;
                }
        }

        /* Update the meters */
        float in  = signal_power(track->in_buf, track->nframes);
        float out = signal_power(track->tape+frame, track->nframes);
        float decay = meters_decay * track->nframes / frame_rate;
        track->in_meter  -= decay;
        track->out_meter -= decay;
        if (in>track->in_meter)   track->in_meter  = in;
        if (out>track->out_meter) track->out_meter = out;

        /* Mix track into the master bus
         *
         * We're mixing track into the master bus if all are true:
         *   - transport is rolling,
         *   - track is not muted with respect to both MUTE and SOLO,
         *   - track is not marked for recording.
         */
        if (transport==ROLLING && !(track->flags&(TRACK_MUTE|TRACK_REC))) {
                for (i = 0; i<track->nframes; ++i) {
                        *L++ += (1.0-track->pan) * track->vol * track->tape[frame+i];
                        *R++ +=      track->pan  * track->vol * track->tape[frame+i];
                }
        }
}
