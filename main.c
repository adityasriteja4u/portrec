#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <jack/jack.h>

#include "track.h"
#include "ui.h"

jack_client_t *client;
jack_port_t *master_port[2];

int tapeLength = 14400000; // 5 min
int track_count = 3;
struct track *tracks[10];
const char *name = "simple";

void fatal(const char *fmt, ...)
{
        jack_client_close(client);
        endwin();
        va_list argptr;
        va_start(argptr, fmt);
        vfprintf(stderr, fmt, argptr);
        va_end(argptr);
        exit(1);
}

int process(jack_nframes_t nframes, void *arg)
{
        int t;
        jack_nframes_t i;
        jack_position_t pos;
        jack_transport_state_t transport = jack_transport_query(client, &pos);

        jack_default_audio_sample_t *L, *R;
        L = jack_port_get_buffer(master_port[0], nframes);
        R = jack_port_get_buffer(master_port[1], nframes);
        for (i = 0; i<nframes; ++i) { L[i] = 0.0; R[i] = 0.0; }

        for (t = 0; t<track_count; ++t) {
                jack_latency_range_t latency;

                jack_port_get_latency_range(tracks[t]->input_port, JackCaptureLatency, &latency);
                if (latency.min!=latency.max) fatal("latency.min (%d) != latency.max (%d)\n", latency.min, latency.max);

                tracks[t]->nframes = nframes;
                tracks[t]->in_buf  = jack_port_get_buffer(tracks[t]->input_port,  nframes);
                tracks[t]->out_buf = jack_port_get_buffer(tracks[t]->output_port, nframes);

                process_track(tracks[t], 3*latency.min, &pos, 0, tapeLength, transport);

                mix_track_to_master(tracks[t], L, R);

                tracks[t]->in_buf  = NULL;
                tracks[t]->out_buf = NULL;
        }
        return 0;
}

void jack_shutdown(void *arg)
{
        exit(1);
}

int main(int argc, char *argv[])
{
        init_ui();

        const char *server_name = NULL;
        jack_options_t options = JackNullOption;
        jack_status_t status;

        client = jack_client_open(name, options, &status, server_name);
        if (client==NULL) {
                fprintf(stderr, "jack_client_open() failed, status = 0x%2.0x\n", status);
                if (status&JackServerFailed) {
                        fprintf(stderr, "Unable to connect to JACK server\n");
                }
                endwin();
                exit(1);
        }
        if (status & JackServerStarted) {
                fatal("JACK server started\n");
        }
        if (status & JackNameNotUnique) {
                name = jack_get_client_name(client);
                fprintf(stderr, "unique name `%s' assigned\n", name);
        }

        master_port[0] = jack_port_register(client, "master:l",
                                            JACK_DEFAULT_AUDIO_TYPE,
                                            JackPortIsOutput, 0);
        master_port[1] = jack_port_register(client, "master:r",
                                            JACK_DEFAULT_AUDIO_TYPE,
                                            JackPortIsOutput, 0);

        tracks[0] = new_track(client, "track1", tapeLength, "track1:in", "track1:out");
        tracks[1] = new_track(client, "track2", tapeLength, "track2:in", "track2:out");
        tracks[2] = new_track(client, "track3", tapeLength, "track3:in", "track3:out");

        jack_set_process_callback(client, process, 0);
        jack_on_shutdown(client, jack_shutdown, 0);

        if (jack_activate(client)) fatal("cannot activate client");

        int t;
        main_loop(tracks, track_count);

        jack_deactivate(client);

        for (t = 0; t<track_count; ++t) {
                char filename[100] = "/tmp/";
                export_track(tracks[t], strcat(strncat(filename, tracks[t]->name, 80),".flac"), tapeLength);
        }

        for (t = 0; t<track_count; ++t) delete_track(client, tracks[t]);
        jack_port_unregister(client, master_port[0]);
        jack_port_unregister(client, master_port[1]);

        jack_client_close(client);
        endwin();
        return 0;
}
