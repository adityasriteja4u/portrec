#include "audio.h"
#include "main.h"
#include "track.h"

#include <math.h>
#include <stdlib.h>

jack_client_t *client;
jack_port_t *master_port[2];

float signal_power(jack_default_audio_sample_t *buf, jack_nframes_t nframes)
{
        float peak = 0.0;
        jack_nframes_t i;
        for (i = 0; i<nframes; ++i) {
                float amplitude = fabsf(buf[i]);
                if (amplitude>peak) peak = amplitude;
        }

        return 20.0 * log10(peak); // conversion to dB; reference value = 1.0
}

static void jack_shutdown(void *arg)
{
        exit(1);
}

static int process(jack_nframes_t nframes, void *arg)
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

int init_audio(const char *name)
{
        const char *server_name = NULL;
        jack_options_t options = JackNullOption;
        jack_status_t status;

        client = jack_client_open(name, options, &status, server_name);
        if (client==NULL) return -1;

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

        if (jack_activate(client)) return -2;
        return 0;
}

void shutdown_audio()
{
        jack_deactivate(client);

        jack_port_unregister(client, master_port[0]);
        jack_port_unregister(client, master_port[1]);

        jack_client_close(client);
}
