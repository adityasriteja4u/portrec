#ifndef AUDIO_H
#define AUDIO_H

#include <jack/jack.h>

// General

typedef jack_default_audio_sample_t frame_t;

float signal_power(frame_t *buf, int nframes);

/* Returns:
 *    0 on success,
 *   -1 if audio startup failed
 *   -2 if audio cannot be activated
 */
int init_audio(const char *name);

void shutdown_audio();

// Transport

enum transport {STOPPED, ROLLING};

/* These variables are read-only outside audio.c file. They hold the status
 * of the transport at the beginning of the last callback cycle.
 */
extern volatile int frame;
extern volatile int frame_rate;
extern volatile enum transport transport;

void transport_start();
void transport_stop();
void transport_locate(int where);

// Buses

struct bus
{
        int channels;
        jack_port_t **ports; /* pointer to array of pointers to ports */
};

enum direction {INPUT, OUTPUT};

struct bus *new_bus(enum direction direction, int channels, const char *name);
void delete_bus(struct bus *bus);
int get_bus_min_latency(struct bus *bus);

#endif
