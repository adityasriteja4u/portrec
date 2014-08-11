#include "audio.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "track.h"

static PaStream *stream;

float signal_power(const frame_t *buf, int nframes)
{
        float peak = 0.0;
        int i;
        for (i = 0; i<nframes; ++i) {
                float amplitude = fabsf(buf[i]);
                if (amplitude>peak) peak = amplitude;
        }

        return 20.0 * log10(peak); // conversion to dB; reference value = 1.0
}

static void stream_finished(void *arg)
{
        exit(1);
}

static int process(const void *inputBuffer, void *outputBuffer,
                   unsigned long framesPerBuffer,
                   const PaStreamCallbackTimeInfo* timeInfo,
                   PaStreamCallbackFlags statusFlags,
                   void *userData)
{
        int t;
        int i;

        frame_t *in  = (frame_t*)inputBuffer;
        frame_t *out = (frame_t*)outputBuffer;
        for (i = 0; i<2*framesPerBuffer; ++i) { out[i] = 0.0f; }

        for (t = 0; t<track_count; ++t) {
                tracks[t]->nframes = framesPerBuffer;

                process_track(tracks[t], in, out);
        }

        // Update transport
        if (transport==ROLLING) frame += framesPerBuffer;

        return 0;
}

int init_audio(const char *name)
{
        PaError err;

        err = Pa_Initialize();
        if (err!=paNoError) return 1;
        err = Pa_OpenDefaultStream(&stream,
                                   1, 2, paFloat32, 48000,
                                   paFramesPerBufferUnspecified,
                                   process,
                                   NULL);
        if (err!=paNoError) goto error;

        err = Pa_SetStreamFinishedCallback(stream, stream_finished);
        if (err!=paNoError) goto error;

        err = Pa_StartStream(stream);
        if (err!=paNoError) goto error;

        const PaStreamInfo *stream_info = Pa_GetStreamInfo(stream);
        frame_rate = (int)stream_info->sampleRate;
        input_latency  = (int)(frame_rate*stream_info->inputLatency);
        output_latency = (int)(frame_rate*stream_info->outputLatency);

        return 0;

error:  Pa_Terminate();
        return 1;
}

void shutdown_audio()
{
        Pa_Terminate();
}

int frame = 0;
int frame_rate = 48000;
int input_latency  = 0;
int output_latency = 0;
enum transport transport = STOPPED;

void transport_start()
{
        transport = ROLLING;
}

void transport_stop()
{
        transport = STOPPED;
}

void transport_locate(int where)
{
        if (where<0) where = 0;
        frame = where;
}

struct bus *new_bus(enum direction direction, int channels, const char *name)
{
        if (channels<1 || channels>16) return NULL;

        struct bus *result;
        result = malloc(sizeof(*result));
        if (!result) return NULL;

        result->channel = 1;
        return result;
}

void delete_bus(struct bus *bus)
{
        free(bus);
}

int get_bus_min_latency(struct bus *bus)
{
        return 0; // TODO
}
