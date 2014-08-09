#include "audio.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "track.h"

static jack_client_t *client;
static jack_port_t *master_port[2];

float signal_power(frame_t *buf, int nframes)
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

static frame_t *bus_get_buffer(struct bus *bus, int nframes)
{
       // TODO
       return jack_port_get_buffer(bus->ports[0], nframes);
}

static void transport_update()
{
        jack_position_t pos;

        if (jack_transport_query(client, &pos)==JackTransportRolling)
                transport = ROLLING;
        else
                transport = STOPPED;

        frame = pos.frame;
        frame_rate = pos.frame_rate;
}

static int process(jack_nframes_t nframes, void *arg)
{
        int t;
        jack_nframes_t i;
        transport_update();

        frame_t *L, *R;
        L = jack_port_get_buffer(master_port[0], nframes);
        R = jack_port_get_buffer(master_port[1], nframes);
        for (i = 0; i<nframes; ++i) { L[i] = 0.0; R[i] = 0.0; }

        for (t = 0; t<track_count; ++t) {
                tracks[t]->nframes = nframes;
                tracks[t]->in_buf  = bus_get_buffer(tracks[t]->input_bus,  nframes);

                process_track(tracks[t], 3*get_bus_min_latency(tracks[t]->input_bus), L, R);

                tracks[t]->in_buf  = NULL;
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

volatile int frame = 0;
volatile int frame_rate = 48000;
volatile enum transport transport = STOPPED;

void transport_start()
{
        jack_transport_start(client);
        /* We don't know yet whether the transport is rolling or starting */
}

void transport_stop()
{
        jack_transport_stop(client);
        transport = STOPPED;
}

void transport_locate(int where)
{
        if (where<0) where = 0;
        jack_transport_locate(client, where);
}

struct bus *new_bus(enum direction direction, int channels, const char *name)
{
        if (channels<1 || channels>16) return NULL;

        int name_len = strlen(name);
        char *portname;
        portname = malloc(name_len+10);
        if (!portname) return NULL;
        strcpy(portname, name);

        struct bus *result;
        result = malloc(sizeof(*result));
        if (!result) return NULL;

        result->channels = channels;
        result->ports = malloc(sizeof(*result->ports));
        if (!result->ports) {
                free(result);
                return NULL;
        }

        int i;
        for (i = 0; i<channels; ++i) {
                if (channels==2) {
                        portname[name_len] = '\0';
                        strcat(portname, i==0?":L":":R");
                } else if (channels!=1) {
                        portname[name_len] = ':';
                        portname[name_len+1] = 'A'+i;
                        portname[name_len+2] = '\0';
                }
                result->ports[i] = jack_port_register(client,
                                                      portname,
                                                      JACK_DEFAULT_AUDIO_TYPE,
                                                      direction==INPUT?JackPortIsInput:JackPortIsOutput,
                                                      0);
        }
        return result;
}

void delete_bus(struct bus *bus)
{
        int i;
        for (i = 0; i<bus->channels; ++i) jack_port_unregister(client, bus->ports[i]);

        free(bus->ports);
        free(bus);
}

int get_bus_min_latency(struct bus *bus)
{
        int min = 0;
        int i;

        for (i = 0; i<bus->channels; ++i) {
                jack_latency_range_t latency;
                jack_port_get_latency_range(bus->ports[0], JackCaptureLatency, &latency);
                if (latency.min<min) min = latency.min;
        }
        return min;
}
