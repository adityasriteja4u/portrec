#include "track.h"

#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include "audio.h"
#include "ui.h"

void fatal(const char *fmt, ...);

struct track *new_track(const char *name, int length,
                        const char *port)
{
        struct track *track = malloc(sizeof(struct track));
        track->name = malloc(strlen(name)+1);
        track->length = length;
        strcpy(track->name, name);
        track->tape = calloc(length, sizeof(frame_t));

        track->vol = 1.0;
        track->pan = 0.5;
        track->flags = 0;

        return track;
}

void delete_track(struct track *track)
{
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
                   int nframes,
                   const frame_t *in,
                   frame_t *out)
{
        int i;
        int j = frame - input_latency;
        if (transport==ROLLING && track->flags&TRACK_REC) {
                for (i = 0; i<nframes; ++i) {
                        if (j>=0 && j<track->length) track->tape[j] = in[i];
                        j++;
                }
        }

        /* Update the meters */
        float in_pow  = signal_power(in, nframes);
        float out_pow = signal_power(track->tape+frame, nframes);
        float decay = meters_decay * nframes / frame_rate;
        track->in_meter  -= decay;
        track->out_meter -= decay;
        if (in_pow>track->in_meter)   track->in_meter  = in_pow;
        if (out_pow>track->out_meter) track->out_meter = out_pow;

        /* Mix track into the master bus
         *
         * We're mixing track into the master bus if all are true:
         *   - transport is rolling,
         *   - track is not muted with respect to both MUTE and SOLO,
         *   - track is not marked for recording.
         */
        if (transport==ROLLING && !(track->flags&(TRACK_MUTE|TRACK_REC))) {
                for (i = 0; i<nframes; ++i) {
                        *out++ += (1.0-track->pan) * track->vol * track->tape[frame+i];
                        *out++ +=      track->pan  * track->vol * track->tape[frame+i];
                }
        }
}
