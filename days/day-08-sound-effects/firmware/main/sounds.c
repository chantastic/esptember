// Day 08 — sound synthesis.
// No audio files anywhere in this project. Every effect is a few lines
// of math generating 16-bit mono PCM at 22050 Hz, rendered on demand
// into one shared buffer. Sound is just numbers fed to a DAC fast
// enough.
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "sounds.h"

#define SAMPLE_RATE 22050

// One reusable render buffer: longest effect is 600 ms.
#define MAX_SAMPLES (SAMPLE_RATE * 6 / 10)
static int16_t render_buf[MAX_SAMPLES];

static float noise(void)
{
    return (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
}

// Each generator fills the buffer and returns its sample count.
// t runs 0..1 across the effect; env shapes loudness over time.

static int gen_laser(int16_t *out)
{
    int n = SAMPLE_RATE * 300 / 1000;
    float phase = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float freq = 1800.0f - 1600.0f * t; // falling sweep
        phase += 2.0f * (float)M_PI * freq / SAMPLE_RATE;
        float env = 1.0f - t;
        out[i] = (int16_t)(sinf(phase) * env * 28000);
    }
    return n;
}

static int gen_coin(int16_t *out)
{
    // Two square-wave notes: B5 then E6 — the classic pickup.
    int n1 = SAMPLE_RATE * 80 / 1000, n2 = SAMPLE_RATE * 220 / 1000;
    float phase = 0;
    for (int i = 0; i < n1 + n2; i++) {
        float freq = i < n1 ? 988.0f : 1319.0f;
        phase += freq / SAMPLE_RATE;
        float t2 = i < n1 ? 0 : (float)(i - n1) / n2;
        float env = i < n1 ? 0.8f : 1.0f - t2;
        float square = (phase - floorf(phase)) < 0.5f ? 1.0f : -1.0f;
        out[i] = (int16_t)(square * env * 9000);
    }
    return n1 + n2;
}

static int gen_boom(int16_t *out)
{
    int n = SAMPLE_RATE * 550 / 1000;
    float low = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        // One-pole lowpass over noise: rumble instead of hiss.
        low += 0.08f * (noise() - low);
        float env = expf(-4.0f * t);
        out[i] = (int16_t)(low * env * 32000);
    }
    return n;
}

static int gen_drum(int16_t *out)
{
    int n = SAMPLE_RATE * 200 / 1000;
    float phase = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float freq = 150.0f - 90.0f * t; // pitch drop = kick
        phase += 2.0f * (float)M_PI * freq / SAMPLE_RATE;
        float env = expf(-6.0f * t);
        float attack = t < 0.02f ? noise() * 0.5f : 0;
        out[i] = (int16_t)((sinf(phase) + attack) * env * 26000);
    }
    return n;
}

static int gen_ring(int16_t *out)
{
    int n = SAMPLE_RATE * 450 / 1000;
    float phase = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        phase += 2.0f * (float)M_PI * 1000.0f / SAMPLE_RATE;
        // Tremolo: amplitude wobble at 20 Hz.
        float trem = 0.6f + 0.4f * sinf(2.0f * (float)M_PI * 20.0f * t * 0.45f);
        float env = 1.0f - t;
        out[i] = (int16_t)(sinf(phase) * trem * env * 22000);
    }
    return n;
}

static int gen_whoosh(int16_t *out)
{
    int n = SAMPLE_RATE * 450 / 1000;
    float low = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        // Swell up then down; filter opens with the swell.
        float swell = sinf((float)M_PI * t);
        low += (0.02f + 0.25f * swell) * (noise() - low);
        out[i] = (int16_t)(low * swell * 30000);
    }
    return n;
}

const sound_def_t sounds[SOUND_COUNT] = {
    {"Laser", gen_laser},   {"Coin", gen_coin},   {"Boom", gen_boom},
    {"Drum", gen_drum},     {"Ring", gen_ring},   {"Whoosh", gen_whoosh},
};

const int16_t *sound_render(int id, int *sample_count)
{
    if (id < 0 || id >= SOUND_COUNT) return NULL;
    *sample_count = sounds[id].generate(render_buf);
    return render_buf;
}
