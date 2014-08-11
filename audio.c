#include "audio.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "track.h"
#include "ui.h"

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
        for (i = 0; i<framesPerBuffer; ++i) { out[2*i] = 0.0f; out[2*i+1] = 0.0f; }

        for (t = 0; t<track_count; ++t) {
                int j = frame - input_latency;
                if (transport==ROLLING && tracks[t]->flags&TRACK_REC) {
                        for (i = 0; i<framesPerBuffer; ++i) {
                                if (j>=0 && j<tracks[t]->length) tracks[t]->tape[j] = in[i];
                                j++;
                        }
                }

                /* Update the meters */
                float in_pow  = signal_power(in, framesPerBuffer);
                float out_pow = signal_power(tracks[t]->tape+frame, framesPerBuffer);
                float decay = meters_decay * framesPerBuffer / frame_rate;
                tracks[t]->in_meter  -= decay;
                tracks[t]->out_meter -= decay;
                if (in_pow>tracks[t]->in_meter)   tracks[t]->in_meter  = in_pow;
                if (out_pow>tracks[t]->out_meter) tracks[t]->out_meter = out_pow;

                /* Mix track into the master bus
                 *
                 * We're mixing track into the master bus if all are true:
                 *   - transport is rolling,
                 *   - track is not muted with respect to both MUTE and SOLO,
                 *   - track is not marked for recording.
                 */
                if (transport==ROLLING && !(tracks[t]->flags&(TRACK_MUTE|TRACK_REC))) {
                        for (i = 0; i<framesPerBuffer; ++i) {
                                *out++ += (1.0-tracks[t]->pan) * tracks[t]->vol * tracks[t]->tape[frame+i];
                                *out++ +=      tracks[t]->pan  * tracks[t]->vol * tracks[t]->tape[frame+i];
                        }
                }
        }

        // Update transport
        if (transport==ROLLING) frame += framesPerBuffer;

        return paContinue;
}

static int set_input_parameters(PaStreamParameters *param)
{
        param->device = Pa_GetDefaultInputDevice();
        if (param->device==paNoDevice) return 1;
        param->channelCount = 1;
        param->sampleFormat = paFloat32;
        param->suggestedLatency = Pa_GetDeviceInfo(param->device)->defaultLowInputLatency;
        param->hostApiSpecificStreamInfo = NULL;
        return 0;
}

static int set_output_parameters(PaStreamParameters *param)
{
        param->device = Pa_GetDefaultOutputDevice();
        if (param->device==paNoDevice) return 1;
        param->channelCount = 2;
        param->sampleFormat = paFloat32;
        param->suggestedLatency = Pa_GetDeviceInfo(param->device)->defaultLowOutputLatency;
        param->hostApiSpecificStreamInfo = NULL;
        return 0;
}

int init_audio(const char *name)
{
        PaStreamParameters inputParameters;
        PaStreamParameters outputParameters;
        PaError err;

        err = Pa_Initialize();
        if (err!=paNoError) return 1;

        set_input_parameters(&inputParameters);
        set_output_parameters(&outputParameters);
        err = Pa_OpenStream(&stream,
                            &inputParameters,
                            &outputParameters,
                            48000,
                            paFramesPerBufferUnspecified,
                            paNoFlag,
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
