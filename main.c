#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include "audio.h"
#include "track.h"
#include "ui.h"

int tapeLength = 14400000; // 5 min
int track_count = 3;
struct track *tracks[10];
const char *name = "simple";

void fatal(const char *fmt, ...)
{
        shutdown_audio();
        shutdown_ui();
        va_list argptr;
        va_start(argptr, fmt);
        vfprintf(stderr, fmt, argptr);
        va_end(argptr);
        exit(1);
}

int main(int argc, char *argv[])
{
        init_ui();

        if (init_audio(name)) fatal("Could not initialize audio\n");

        tracks[0] = new_track(client, "track1", tapeLength, "track1:in", "track1:out");
        tracks[1] = new_track(client, "track2", tapeLength, "track2:in", "track2:out");
        tracks[2] = new_track(client, "track3", tapeLength, "track3:in", "track3:out");

        int t;
        main_loop(tracks, track_count);

        for (t = 0; t<track_count; ++t) {
                char filename[100] = "/tmp/";
                export_track(tracks[t], strcat(strncat(filename, tracks[t]->name, 80),".flac"), tapeLength);
        }

        for (t = 0; t<track_count; ++t) delete_track(client, tracks[t]);

        shutdown_audio();
        shutdown_ui();
        return 0;
}
