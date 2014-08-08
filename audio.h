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

extern jack_nframes_t frame;
extern jack_nframes_t frame_rate;
extern enum transport transport;

void update_transport_information();
void transport_start();
void transport_stop();

#endif
