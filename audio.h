#ifndef AUDIO_H
#define AUDIO_H

#include <jack/jack.h>

extern jack_client_t *client;
extern jack_port_t *master_port[2];

float signal_power(jack_default_audio_sample_t *buf, jack_nframes_t nframes);

/* Returns:
 *    0 on success,
 *   -1 if audio startup failed
 *   -2 if audio cannot be activated
 */
int init_audio(const char *name);

void shutdown_audio();

#endif
