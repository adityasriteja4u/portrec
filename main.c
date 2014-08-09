#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include "audio.h"
#include "track.h"
#include "ui.h"

int track_count = 0;
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

        int c;
        while ((c = getopt(argc, argv, "f:l:t:"))!=-1) {
                int tape_len = 14400000; // 5 min
                char *import_from;
                switch (c) {
                case 'f':
                        import_from = optarg;
                        break;
                case 'l':
                        tape_len = frame_rate * 60 * atoi(optarg);
                        break;
                case 't':
                        tracks[track_count] = new_track(optarg, tape_len, optarg);
                        int res;
                        if (import_from) res = import_track(tracks[track_count], import_from);
                        fprintf(stderr, "%d\n", res);
                        ++track_count;
                        import_from = NULL;
                        break;
                default:
                        fatal("unknown command line option\n");
                        break;
                }
        }

        if (track_count==0) {
                /* If no tracks were created on the command line, create
                 * the default set of three tracks.
                 */
                track_count = 3;
                tracks[0] = new_track("track1", frame_rate*60*5, "track1");
                tracks[1] = new_track("track2", frame_rate*60*5, "track2");
                tracks[2] = new_track("track3", frame_rate*60*5, "track3");
        }

        int t;
        main_loop(tracks, track_count);

        for (t = 0; t<track_count; ++t) {
                char filename[100] = "/tmp/";
                export_track(tracks[t], strcat(strncat(filename, tracks[t]->name, 80),".flac"));
        }

        for (t = 0; t<track_count; ++t) delete_track(tracks[t]);

        shutdown_audio();
        shutdown_ui();
        return 0;
}
