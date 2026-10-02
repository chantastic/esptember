// Explicit, local display diagnostic. Captures can contain private text; never commit them.
static void capture_screen(void) {
    if(session_open) { printf("LIVE_CAPTURE_BUSY\n"); return; }
    bsp_display_lock(0);
    lv_draw_buf_t *image=lv_snapshot_take(lv_screen_active(),LV_COLOR_FORMAT_RGB565);
    bsp_display_unlock();
    if(!image) { printf("LIVE_CAPTURE_FAILED\n"); return; }
    printf("LIVE_CAPTURE %u %u RGB565LE\n",image->header.w,image->header.h);
    unsigned char line[1024];
    for(unsigned y=0;y<image->header.h;y++) {
        size_t n=0;
        if(mbedtls_base64_encode(line,sizeof(line),&n,image->data+y*image->header.stride,image->header.w*2)!=0) break;
        fwrite(line,1,n,stdout); fputc('\n',stdout);
        if((y&31)==31) { fflush(stdout); vTaskDelay(1); }
    }
    printf("LIVE_CAPTURE_END\n"); fflush(stdout); lv_draw_buf_destroy(image);
}
static void display_preview(void) {
    if(session_open) return;
    xSemaphoreTake(lock,portMAX_DELAY);
    caption_reset(&transcript); reset_view=true;
    caption_commit(&transcript,"One flowing transcript.");
    caption_interim(&transcript,"These words stay in place as they become final.");
    xSemaphoreGive(lock);
    set_state("PREVIEW - microphone OFF");
}
static void display_preview_history(void) {
    if(session_open) return;
    xSemaphoreTake(lock,portMAX_DELAY);
    caption_reset(&transcript); reset_view=true;
    for(int i=0;i<24;i++) caption_commit(&transcript,"Earlier words stay in the transcript while you read back.");
    caption_interim(&transcript,"New words wait here at the live edge.");
    xSemaphoreGive(lock);
    set_state("PREVIEW - microphone OFF");
}
static void preview_scroll(bool bottom) {
    if(session_open) return;
    bsp_display_lock(0);
    // Exercise the reader path without an upstream call or synthetic microphone data.
    follow_live=bottom;
    lv_obj_scroll_to_y(caption_scroll,bottom?LV_COORD_MAX:0,LV_ANIM_OFF);
    bsp_display_unlock();
}
static void display_preview_final(void) {
    if(session_open) return;
    xSemaphoreTake(lock,portMAX_DELAY);
    caption_commit(&transcript,transcript.interim);
    xSemaphoreGive(lock);
}
