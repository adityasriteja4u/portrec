#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <jack/jack.h>

#include "track.h"

jack_client_t *client;
jack_port_t *master_port[2];

int tapeLength = 4800000; // 100 secs
int track_count = 3;
int current_track = 0;
struct track *tracks[10];
int loop = 1;
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
                int j;
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

static void display()
{
        int t;
        for (t = 0; t<track_count; ++t) {
                //              -> R  M  S  vol   pan  name
                mvprintw(t+1, 0, "%s %s %s %s %4.2f %4.2f  %s",
                         current_track==t?"->":"  ",
                         tracks[t]->flags&TRACK_REC ?"R":" ",
                         tracks[t]->flags&TRACK_MUTE?"M":" ",
                         tracks[t]->flags&TRACK_SOLO?"S":" ",
                         tracks[t]->vol,
                         tracks[t]->pan,
                         tracks[t]->name);

                jack_position_t pos;
                switch (jack_transport_query(client, &pos)) {
                case JackTransportRolling: mvprintw(0, 0, "rolling"); break;
                case JackTransportStopped: mvprintw(0, 0, "stopped"); break;
                }
                mvprintw(0, 10, "%5.1f s", (double)pos.frame/pos.frame_rate);
        }
        refresh();
}

static int command(int key)
{
        // Returns 0 on success, 1 when user wants to exit

        static enum {NORMAL, SET_MARK, GOTO_MARK} mode = NORMAL;
        static int mark[26];
        static int where;

        if (mode==NORMAL) {
                switch (key) {
                case KEY_DOWN:
                        if (current_track<track_count-1) ++current_track;
                        break;
                case KEY_UP:
                        if (current_track>0) --current_track;
                        break;
                case ' ':
                        if (jack_transport_query(client, NULL)==JackTransportStopped)
                                jack_transport_start(client);
                        else
                                jack_transport_stop(client);
                        break;
                case 'z':
                        jack_transport_locate(client, 0);
                        break;
                case 'r':
                        tracks[current_track]->flags ^= TRACK_REC;
                        break;
                case 'm':
                        tracks[current_track]->flags ^= TRACK_MUTE;
                        break;
                case 's':
                        tracks[current_track]->flags ^= TRACK_SOLO;
                        break;
                case '-':
                        tracks[current_track]->vol -= 0.1;
                        if (tracks[current_track]->vol<=0.0) tracks[current_track]->vol = 0.0;
                        break;
                case '=':
                        tracks[current_track]->vol += 0.1;
                        if (tracks[current_track]->vol>=9.9) tracks[current_track]->vol = 9.9;
                        break;
                case ',':
                        tracks[current_track]->pan -= 0.1;
                        if (tracks[current_track]->pan<=0.0) tracks[current_track]->pan = 0.0;
                        break;
                case '.':
                        tracks[current_track]->pan += 0.1;
                        if (tracks[current_track]->pan>=1.0) tracks[current_track]->pan = 1.0;
                        break;
                case 'p':
                        mode = SET_MARK;
                        jack_position_t pos;
                        jack_transport_query(client, &pos);
                        where = pos.frame;
                        break;
                case '\'':
                        mode = GOTO_MARK;
                        break;
                case 'q':
                        return 1;
                }
        } else if (mode==SET_MARK) {
                if (key>='a' && key<='z') mark[key-'a'] = where;
                mode = NORMAL;
        } else if (mode==GOTO_MARK) {
                if (key>='a' && key<='z') jack_transport_locate(client, mark[key-'a']);
                mode = NORMAL;
        }
        return 0;
}

int main(int argc, char *argv[])
{
        initscr();
        cbreak();
        keypad(stdscr, TRUE);
        noecho();
        timeout(50);

        const char **ports;
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
                fatal("unique name `%s' assigned\n", name);
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
        while (loop) {
                display();
                int ch = getch();
                if (ch!=ERR)
                        if (command(ch)) break;
        }

        jack_deactivate(client);

        export_track(tracks[0], "/tmp/out1.flac", tapeLength);
        export_track(tracks[1], "/tmp/out2.flac", tapeLength);
        export_track(tracks[2], "/tmp/out3.flac", tapeLength);

        for (t = 0; t<track_count; ++t) delete_track(client, tracks[t]);
        jack_port_unregister(client, master_port[0]);
        jack_port_unregister(client, master_port[1]);

        jack_client_close(client);
        endwin();
        return 0;
}
