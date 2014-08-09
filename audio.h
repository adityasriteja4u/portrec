#ifndef AUDIO_H
#define AUDIO_H

#include <jack/jack.h>

// General

float signal_power(jack_default_audio_sample_t *buf, jack_nframes_t nframes);

extern jack_client_t *client;
extern jack_port_t *master_port[2];

/* Returns:
 *    0 on success,
 *   -1 if audio startup failed
 *   -2 if audio cannot be activated
 */
int init_audio(const char *name);

void shutdown_audio();

// Transport

enum transport {STOPPED, ROLLING};

/* These variables are read-only outside transport_*() functions below.
 * They hold the status of the transport at the beginning of the last
 * callback cycle.
 */
extern volatile int frame;
extern volatile int frame_rate;
extern volatile enum transport transport;

void transport_start();
void transport_stop();
void transport_locate(int where);

#endif
