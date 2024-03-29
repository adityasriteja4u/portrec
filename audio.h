#ifndef AUDIO_H
#define AUDIO_H

#include <portaudio.h>

// General

typedef float frame_t;

/* Channel is:
 *  0 for mono signal
 *  1 for left channel
 *  2 for right channel
 */
float signal_power(const frame_t *buf, int nframes, int channel);

extern float master_power_l;
extern float master_power_r;

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
extern int frame;
extern int frame_rate;
extern int input_latency;
extern int output_latency;
extern enum transport transport;

void transport_start();
void transport_stop();
void transport_locate(int where);

// Buses

struct bus
{
        int channel;
};

enum direction {INPUT, OUTPUT};

struct bus *new_bus(enum direction direction, int channels, const char *name);
void delete_bus(struct bus *bus);
int get_bus_min_latency(struct bus *bus);

#endif
