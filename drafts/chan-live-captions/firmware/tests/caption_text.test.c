#include <assert.h>
#include <stdio.h>
#include "../main/caption_text.h"
int main(void) {
    caption_text_t text={0};
    assert(caption_commit(&text,"One flowing transcript."));
    assert(caption_interim(&text,"These words stay in place."));
    char before[9003]; snprintf(before,sizeof(before),"%s%s",text.committed,text.interim);
    assert(caption_commit(&text,text.interim));
    assert(!text.interim[0]);
    assert(!strncmp(before,text.committed,strlen(before))); // promotion doesn't relocate words
    assert(text.committed[strlen(before)]==' ');
    assert(!strchr(text.committed,'\n'));
    uint32_t revision=text.revision;
    assert(caption_interim(&text,"")); assert(text.revision==revision);
    assert(caption_commit(&text,"")); assert(text.revision==revision); // no blank final lines
    assert(caption_interim(&text,"first guess"));
    assert(caption_interim(&text,"revised guess"));
    revision=text.revision;
    assert(caption_interim(&text,"revised guess")); assert(text.revision==revision);
    assert(strstr(text.committed,"guess")==NULL);
    assert(caption_commit(&text,"Correct words.")); assert(!text.interim[0]);
    char oversized[3002]; memset(oversized,'x',sizeof(oversized)-1); oversized[3001]=0;
    revision=text.revision;
    assert(!caption_interim(&text,oversized)); assert(!caption_commit(&text,oversized));
    assert(text.revision==revision);
    for(int i=0;i<2000;i++) {
        assert(caption_commit(&text,"Longer history 世界 👋."));
        assert(strlen(text.committed)<CAPTION_COMMITTED_BYTES);
    }
    assert(text.trims>0); assert(strstr(text.committed,"世界 👋.")!=NULL);
    caption_reset(&text); assert(!text.committed[0] && !text.interim[0] && !text.trims);
    puts("PASS inline promotion, interim replacement, empty finals, no-op revisions, bounds and UTF-8 history");
}
