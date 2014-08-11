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
