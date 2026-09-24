#pragma once
#include <stdint.h>

#define SOUND_COUNT 6
#define SOUND_SAMPLE_RATE 22050

typedef struct {
    const char *name;
    int (*generate)(int16_t *out);
} sound_def_t;

extern const sound_def_t sounds[SOUND_COUNT];

// Render one effect into the shared buffer; returns the buffer.
const int16_t *sound_render(int id, int *sample_count);
