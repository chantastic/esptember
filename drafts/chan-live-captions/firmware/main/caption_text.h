// Bounded display model. Final/provisional are protocol states, not separate UI panels.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define CAPTION_COMMITTED_BYTES 6001
#define CAPTION_INTERIM_BYTES 3001
typedef struct {
    char committed[CAPTION_COMMITTED_BYTES];
    char interim[CAPTION_INTERIM_BYTES];
    uint32_t revision;
    uint32_t trims;
} caption_text_t;
static inline void caption_reset(caption_text_t *text) {
    text->committed[0]=text->interim[0]=0;
    text->trims=0; text->revision++;
}
static inline bool caption_interim(caption_text_t *text,const char *words) {
    size_t n=strlen(words);
    if(n>=sizeof(text->interim)) return false;
    if(strcmp(text->interim,words)) { memcpy(text->interim,words,n+1); text->revision++; }
    return true;
}
static inline bool caption_commit(caption_text_t *text,const char *words) {
    size_t n=strlen(words),current=strlen(text->committed);
    if(n>=CAPTION_INTERIM_BYTES) return false;
    bool changed=n || text->interim[0];
    if(n) {
        // Drop only the oldest whole words when the bounded history fills up.
        // Word-boundary cuts cannot bisect a UTF-8 code point. Preserve recent context.
        if(current+n+2>sizeof(text->committed)) {
            size_t cut=current+n+2-sizeof(text->committed);
            while(cut<current && text->committed[cut-1]!=' ') cut++;
            memmove(text->committed,text->committed+cut,current-cut+1);
            current-=cut; text->trims++;
        }
        memcpy(text->committed+current,words,n);
        text->committed[current+n]=' '; // same inline separator before the next provisional tail
        text->committed[current+n+1]=0;
    }
    text->interim[0]=0;
    if(changed) text->revision++;
    return true;
}
