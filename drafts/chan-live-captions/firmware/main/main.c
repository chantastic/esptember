// Reference ESP client for devices.chan.dev live-transcription protocol v1.
// Waveshare ESP32-S3 AMOLED 1.8 V2. No provider keys. Mic starts only on explicit Start.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_websocket_client.h"
#include "esp_crt_bundle.h"
#include "esp_codec_dev.h"
#include "esp_timer.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "cJSON.h"
#include "portal.h"
#include "board.h"
#include "display.h"
#include "caption_text.h"

#define LIVE_URI "wss://devices.chan.dev/v1/transcriptions/deepgram/live"
#define CHUNK_SAMPLES 3200
static lv_obj_t *transcript_group,*state_label,*button_label,*qr,*login_label,*caption_scroll;
static lv_span_t *committed_span,*provisional_span;
static caption_text_t transcript;
static uint32_t rendered_revision=UINT32_MAX,ui_updates=0;
static bool follow_live=true,dragging_history=false,programmatic_scroll=false,reset_view=true;
static SemaphoreHandle_t lock, send_lock;
static char state_text[96]="Starting";
static volatile bool got_ip=false, want_start=false, want_stop=false, live_ready=false, disconnected=false, need_auth=false, complete=false, session_open=false;
static volatile uint32_t interim_count=0, final_count=0, audio_frames=0, sent_bytes=0, peak=0;
static volatile unsigned test_seconds=0;
static uint64_t last_sequence=0;
static volatile int handshake_status=0;
static esp_websocket_client_handle_t ws=NULL;
static void set_state(const char *text) { xSemaphoreTake(lock,portMAX_DELAY); strlcpy(state_text,text,sizeof(state_text)); xSemaphoreGive(lock); }
static void show_login(const char *code,const char *uri) {
    bsp_display_lock(0); lv_label_set_text(login_label,code); lv_qrcode_update(qr,uri,strlen(uri)); lv_obj_remove_flag(qr,LV_OBJ_FLAG_HIDDEN); bsp_display_unlock();
    set_state("Scan to sign in");
}
#include "auth.h"
static void status(void) {
    printf("LIVE_STATUS wifi=%d client_match=%d workspace=%d token_current=%d session=%d ready=%d frames=%lu bytes=%lu interim=%lu final=%lu complete=%d peak=%lu heap=%lu\n",
        got_ip,!strcmp(client_id,EXPECTED_CLIENT),organization[0]!=0,token_expires>time(NULL),session_open,live_ready,(unsigned long)audio_frames,(unsigned long)sent_bytes,(unsigned long)interim_count,
        (unsigned long)final_count,complete,(unsigned long)peak,(unsigned long)esp_get_free_heap_size());
    printf("LIVE_DISPLAY updates=%lu dma_free=%u dma_largest=%u\n",(unsigned long)ui_updates,
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_DMA),(unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
}
static void toggle(lv_event_t *event) { (void)event; if(session_open) want_stop=true; else want_start=true; }
static void update_label(lv_obj_t *label,const char *text) {
    if(strcmp(lv_label_get_text(label),text)) { lv_label_set_text(label,text); ui_updates++; }
}
static void history_scroll(lv_event_t *event) {
    if(programmatic_scroll) return;
    if(lv_event_get_code(event)==LV_EVENT_SCROLL_BEGIN) {
        dragging_history=true; follow_live=false;
    } else if(lv_event_get_code(event)==LV_EVENT_SCROLL_END) {
        dragging_history=false;
        follow_live=lv_obj_get_scroll_bottom(caption_scroll)<=2;
    }
}
static void ui_tick(lv_timer_t *timer) {
    (void)timer; xSemaphoreTake(lock,portMAX_DELAY);
    update_label(state_label,state_text);
    if(reset_view) { follow_live=true; dragging_history=false; }
    // Incoming words continue into the bounded model while history is being read.
    // Do not relayout the paragraph under an active drag, or pull the reader to the tail.
    if(!dragging_history && follow_live && rendered_revision!=transcript.revision) {
        programmatic_scroll=true;
        if(reset_view) lv_obj_scroll_to_y(caption_scroll,0,LV_ANIM_OFF);
        const int32_t old_y=lv_obj_get_scroll_y(caption_scroll);
        lv_span_set_text(committed_span,transcript.committed);
        lv_span_set_text(provisional_span,transcript.interim);
        lv_spangroup_refresh(transcript_group);
        lv_obj_update_layout(caption_scroll);
        const int32_t below=lv_obj_get_scroll_bottom(caption_scroll);
        if(below>0) lv_obj_scroll_to_y(caption_scroll,old_y+below,LV_ANIM_OFF);
        programmatic_scroll=false; reset_view=false;
        rendered_revision=transcript.revision; ui_updates++;
    }
    update_label(button_label,session_open?"STOP":"START"); xSemaphoreGive(lock);
}
static void build_ui(void) {
    lv_obj_t *screen=lv_screen_active(); lv_obj_set_style_bg_color(screen,lv_color_black(),0);
    lv_obj_t *title=lv_label_create(screen); lv_label_set_text(title,"LIVE CAPTIONS"); lv_obj_set_style_text_color(title,lv_color_hex(0xff5b04),0);
    lv_obj_set_style_text_font(title,&lv_font_montserrat_28,0); lv_obj_align(title,LV_ALIGN_TOP_MID,0,12);
    caption_scroll=lv_obj_create(screen); lv_obj_set_size(caption_scroll,350,310); lv_obj_align(caption_scroll,LV_ALIGN_TOP_MID,0,52);
    lv_obj_set_style_bg_color(caption_scroll,lv_color_black(),0); lv_obj_set_style_border_width(caption_scroll,0,0);
    lv_obj_set_scroll_dir(caption_scroll,LV_DIR_VER);
    lv_obj_add_event_cb(caption_scroll,history_scroll,LV_EVENT_SCROLL_BEGIN,NULL);
    lv_obj_add_event_cb(caption_scroll,history_scroll,LV_EVENT_SCROLL_END,NULL);
    transcript_group=lv_spangroup_create(caption_scroll); lv_obj_set_size(transcript_group,320,LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(transcript_group,&lv_font_montserrat_28,0);
    lv_obj_set_style_text_line_space(transcript_group,4,0);
    committed_span=lv_spangroup_add_span(transcript_group);
    provisional_span=lv_spangroup_add_span(transcript_group);
    static lv_style_t committed_style,provisional_style;
    lv_style_init(&committed_style); lv_style_set_text_color(&committed_style,lv_color_hex(0xeeeeee));
    lv_style_init(&provisional_style); lv_style_set_text_color(&provisional_style,lv_color_hex(0xbdbdbd));
    lv_spangroup_set_span_style(transcript_group,committed_span,&committed_style);
    lv_spangroup_set_span_style(transcript_group,provisional_span,&provisional_style);
    state_label=lv_label_create(screen); lv_obj_set_width(state_label,340); lv_obj_align(state_label,LV_ALIGN_TOP_MID,0,368); lv_label_set_text(state_label,"Starting");
    lv_obj_t *button=lv_button_create(screen); lv_obj_set_size(button,160,45); lv_obj_align(button,LV_ALIGN_BOTTOM_MID,0,-4);
    lv_obj_add_event_cb(button,toggle,LV_EVENT_CLICKED,NULL); button_label=lv_label_create(button); lv_label_set_text(button_label,"START"); lv_obj_center(button_label);
    qr=lv_qrcode_create(screen); lv_qrcode_set_size(qr,210); lv_obj_align(qr,LV_ALIGN_TOP_MID,0,80); lv_obj_add_flag(qr,LV_OBJ_FLAG_HIDDEN);
    login_label=lv_label_create(screen); lv_obj_align(login_label,LV_ALIGN_TOP_MID,0,300); lv_label_set_text(login_label,"");
    lv_timer_create(ui_tick,200,NULL);
}
static void protocol_error(const char *reason) { live_ready=false; want_stop=true; set_state(reason); printf("LIVE_PROTOCOL error\n"); }
static void result(const char *json,int length) {
    cJSON *r=cJSON_ParseWithLength(json,length); if(!r) { protocol_error("Invalid service event"); return; }
    cJSON *type=cJSON_GetObjectItem(r,"type");
    if(!cJSON_IsString(type)) { cJSON_Delete(r); protocol_error("Invalid service event"); return; }
    if(!strcmp(type->valuestring,"ready")) {
        cJSON *version=cJSON_GetObjectItem(r,"protocol"),*rate=cJSON_GetObjectItem(r,"sampleRate"),*channels=cJSON_GetObjectItem(r,"channels");
        if(!cJSON_IsNumber(version)||version->valueint!=1||!cJSON_IsNumber(rate)||rate->valueint!=16000||!cJSON_IsNumber(channels)||channels->valueint!=1) protocol_error("Unsupported audio format");
        else { live_ready=true; set_state("Listening - tap STOP to finish"); printf("LIVE_READY protocol=1 rate=16000 channels=1\n"); }
    } else if(!strcmp(type->valuestring,"transcript")) {
        cJSON *text=cJSON_GetObjectItem(r,"text"),*final=cJSON_GetObjectItem(r,"final"),*seq=cJSON_GetObjectItem(r,"sequence");
        if(!cJSON_IsString(text)||strlen(text->valuestring)>3000||!cJSON_IsBool(final)||!cJSON_IsNumber(seq)||(uint64_t)seq->valuedouble!=last_sequence+1) protocol_error("Transcript interrupted");
        else {
            last_sequence++;
            xSemaphoreTake(lock,portMAX_DELAY);
            if(cJSON_IsTrue(final)) {
                final_count++; caption_commit(&transcript,text->valuestring);
            } else { interim_count++; caption_interim(&transcript,text->valuestring); }
            xSemaphoreGive(lock);
            printf("LIVE_RESULT final=%d chars=%u sequence=%llu\n",cJSON_IsTrue(final),(unsigned)strlen(text->valuestring),(unsigned long long)last_sequence);
        }
    } else if(!strcmp(type->valuestring,"reauth_required")) need_auth=true;
    else if(!strcmp(type->valuestring,"authorized")) printf("LIVE_AUTH renewed=1\n");
    else if(!strcmp(type->valuestring,"stopped")) { live_ready=false; complete=true; set_state("Stopped - final words received"); printf("LIVE_STOPPED clean=1\n"); }
    else if(!strcmp(type->valuestring,"error")) {
        cJSON *error=cJSON_GetObjectItem(r,"error");
        // Only display short service error codes; never log arbitrary response bodies.
        if(cJSON_IsString(error)&&strlen(error->valuestring)<64) { set_state(error->valuestring); printf("LIVE_ERROR service=1\n"); }
        else set_state("Service unavailable");
        live_ready=false; want_stop=true;
    }
    cJSON_Delete(r);
}
static char message[8193]; static size_t message_length=0;
static void ws_event(void *arg,esp_event_base_t base,int32_t id,void *data) {
    (void)arg;(void)base; const esp_websocket_event_data_t *e=data;
    if(id==WEBSOCKET_EVENT_CONNECTED) { printf("LIVE_WS connected=1\n"); message_length=0; }
    else if(id==WEBSOCKET_EVENT_DISCONNECTED) { live_ready=false; disconnected=true; printf("LIVE_WS disconnected=1 complete=%d\n",complete); }
    else if(id==WEBSOCKET_EVENT_ERROR) {
        live_ready=false; disconnected=true; handshake_status=e->error_handle.esp_ws_handshake_status_code;
        printf("LIVE_WS error=1 http=%d kind=%d\n",handshake_status,e->error_handle.error_type);
        if(e->data_ptr && e->data_len>0 && e->data_len<400 && !strncmp(e->data_ptr,"esp_transport_",14)) {
            // These SDK formats contain only operation names and numeric/TLS error codes, never headers.
            printf("LIVE_TRANSPORT %.*s\n",e->data_len,e->data_ptr);
        }
    }
    else if(id==WEBSOCKET_EVENT_DATA && (e->op_code==1 || e->op_code==0)) {
        if(e->op_code==1 && e->payload_offset==0) message_length=0;
        if(e->data_len<0 || message_length+(size_t)e->data_len>8192) { protocol_error("Service message too large"); return; }
        memcpy(message+message_length,e->data_ptr,e->data_len); message_length+=e->data_len;
        if(e->fin && e->payload_offset+e->data_len==e->payload_len) { message[message_length]=0; result(message,message_length); message_length=0; }
    }
}
static bool open_stream(void) {
    static char header[16420]; snprintf(header,sizeof(header),"Authorization: Bearer %s\r\n",access_token);
    printf("LIVE_CONNECT bearer_bytes=%u handshake_buffer=%u\n",(unsigned)strlen(access_token),CONFIG_WS_BUFFER_SIZE);
    const esp_websocket_client_config_t cfg={.uri=LIVE_URI,.headers=header,.disable_auto_reconnect=true,
        .network_timeout_ms=60000,.buffer_size=18000,.task_stack=8192,.crt_bundle_attach=esp_crt_bundle_attach};
    disconnected=false; complete=false; live_ready=false; handshake_status=0; last_sequence=0; interim_count=final_count=audio_frames=sent_bytes=peak=0;
    xSemaphoreTake(lock,portMAX_DELAY); caption_reset(&transcript); reset_view=true; xSemaphoreGive(lock);
    ws=esp_websocket_client_init(&cfg); if(!ws) return false;
    esp_websocket_register_events(ws,WEBSOCKET_EVENT_ANY,ws_event,NULL);
    session_open=true;
    if(esp_websocket_client_start(ws)!=ESP_OK) { session_open=false; esp_websocket_client_destroy(ws); ws=NULL; return false; }
    return true;
}
static void run_stream(void) {
    set_state("Connecting to devices.chan.dev");
    if(!refresh_session(organization) || !select_workspace()) { set_state("Sign-in refresh failed"); return; }
    if(!open_stream()) { set_state("Connection failed"); return; }
    int64_t began=esp_timer_get_time(),end=0,last_ping=began;
    while(!live_ready && !disconnected && esp_timer_get_time()-began<65000000LL) vTaskDelay(pdMS_TO_TICKS(50));
    unsigned bounded=test_seconds; test_seconds=0;
    if(bounded) end=esp_timer_get_time()+(int64_t)bounded*1000000;
    while(live_ready && !disconnected && !want_stop) {
        if(end && esp_timer_get_time()>=end) break;
        if(need_auth || token_expires-time(NULL)<120) {
            need_auth=false;
            if(!refresh_session(organization)) { set_state("Session renewal failed"); break; }
            cJSON *auth=cJSON_CreateObject(); cJSON_AddStringToObject(auth,"type","auth"); cJSON_AddStringToObject(auth,"bearer",access_token);
            char *body=cJSON_PrintUnformatted(auth); cJSON_Delete(auth);
            int sent=esp_websocket_client_send_text(ws,body,strlen(body),pdMS_TO_TICKS(1000)); memset(body,0,strlen(body)); free(body);
            if(sent<0) break;
        }
        if(esp_timer_get_time()-last_ping>10000000LL) { last_ping=esp_timer_get_time(); esp_websocket_client_send_text(ws,"{\"type\":\"ping\"}",15,pdMS_TO_TICKS(1000)); }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    live_ready=false; // finish the in-flight microphone chunk before CloseStream
    xSemaphoreTake(send_lock,portMAX_DELAY);
    if(!disconnected) esp_websocket_client_send_text(ws,"{\"type\":\"stop\"}",15,pdMS_TO_TICKS(1000));
    xSemaphoreGive(send_lock);
    if(!disconnected) {
        int64_t until=esp_timer_get_time()+13000000LL;
        while(!complete && !disconnected && esp_timer_get_time()<until) vTaskDelay(pdMS_TO_TICKS(50));
    }
    xSemaphoreTake(send_lock,portMAX_DELAY);
    esp_websocket_client_stop(ws); esp_websocket_client_destroy(ws); ws=NULL;
    xSemaphoreGive(send_lock);
    session_open=false; want_stop=false;
    if(!complete) {
        char reason[96];
        if(handshake_status) snprintf(reason,sizeof(reason),"HTTP %d - not recording",handshake_status);
        else snprintf(reason,sizeof(reason),"Interrupted - tap START to retry");
        set_state(reason);
    }
    status();
}
static void mic_task(void *arg) {
    esp_codec_dev_handle_t mic=arg; esp_codec_dev_sample_info_t format={.sample_rate=16000,.channel=1,.bits_per_sample=16};
    ESP_ERROR_CHECK(esp_codec_dev_open(mic,&format)); ESP_ERROR_CHECK(esp_codec_dev_set_in_gain(mic,30.0));
    static int16_t samples[CHUNK_SAMPLES];
    while(1) {
        if(!live_ready) { vTaskDelay(pdMS_TO_TICKS(50)); continue; }
        xSemaphoreTake(send_lock,portMAX_DELAY);
        if(!live_ready || !ws) { xSemaphoreGive(send_lock); continue; }
        int err=esp_codec_dev_read(mic,samples,sizeof(samples));
        if(err!=ESP_OK) { xSemaphoreGive(send_lock); protocol_error("Microphone failed"); continue; }
        if(disconnected) { xSemaphoreGive(send_lock); continue; }
        uint32_t max=0; for(size_t i=0;i<CHUNK_SAMPLES;i++) { int v=abs((int)samples[i]); if((uint32_t)v>max)max=v; } peak=max;
        int sent=esp_websocket_client_send_bin(ws,(char *)samples,sizeof(samples),pdMS_TO_TICKS(1000));
        xSemaphoreGive(send_lock);
        if(sent!=sizeof(samples)) { protocol_error("Audio interrupted"); continue; }
        audio_frames++; sent_bytes+=sent;
    }
}
#include "capture.h"
static void console_task(void *arg) {
    (void)arg; usb_serial_jtag_driver_config_t cfg=USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT(); usb_serial_jtag_driver_install(&cfg); usb_serial_jtag_vfs_use_driver();
    char line[128];
    while(1) {
        if(!fgets(line,sizeof(line),stdin)) { clearerr(stdin); vTaskDelay(pdMS_TO_TICKS(100)); continue; }
        char *nl=strpbrk(line,"\r\n"); if(nl)*nl=0;
        unsigned duration=0;
        if(!strcmp(line,"s")) status();
        else if(!strcmp(line,"capture")) capture_screen();
        else if(!strcmp(line,"preview")) display_preview();
        else if(!strcmp(line,"preview-final")) display_preview_final();
        else if(!strcmp(line,"preview-history")) display_preview_history();
        else if(!strcmp(line,"preview-up")) preview_scroll(false);
        else if(!strcmp(line,"preview-bottom")) preview_scroll(true);
        else if(!strcmp(line,"start")) want_start=true;
        else if(!strcmp(line,"stop")) want_stop=true;
        else if(sscanf(line,"test %u",&duration)==1 && duration>=1 && duration<=120 && !session_open) { test_seconds=duration; want_start=true; }
    }
}
static void wifi_event(void *arg,esp_event_base_t base,int32_t id,void *data) {
    (void)arg;(void)data;
    if(base==WIFI_EVENT && id==WIFI_EVENT_STA_START) esp_wifi_connect();
    else if(base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED) { got_ip=false; if(session_open) want_stop=true; esp_wifi_connect(); }
    else if(base==IP_EVENT && id==IP_EVENT_STA_GOT_IP) got_ip=true;
}
static void portal_submit(const char *ssid,const char *pass) { portal_save_creds(ssid,pass); vTaskDelay(pdMS_TO_TICKS(1000)); esp_restart(); }
static void app_task(void *arg) {
    (void)arg; while(!got_ip) vTaskDelay(pdMS_TO_TICKS(200));
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL); esp_sntp_setservername(0,"pool.ntp.org"); esp_sntp_init();
    for(int i=0;i<100 && time(NULL)<1700000000;i++) vTaskDelay(pdMS_TO_TICKS(200));
    set_state("Checking saved device login");
    if(!authenticate()) { set_state("Devices sign-in required"); vTaskDelete(NULL); return; }
    bsp_display_lock(0); lv_obj_add_flag(qr,LV_OBJ_FLAG_HIDDEN); lv_label_set_text(login_label,""); bsp_display_unlock();
    set_state("Ready - tap START (uses Deepgram)"); printf("LIVE_IDLE ready=1\n");
    while(1) { if(want_start && got_ip) { want_start=false; want_stop=false; run_stream(); } vTaskDelay(pdMS_TO_TICKS(100)); }
}
void app_main(void) {
    lock=xSemaphoreCreateMutex(); send_lock=xSemaphoreCreateMutex(); ESP_ERROR_CHECK(nvs_flash_init()); ESP_ERROR_CHECK(esp_event_loop_create_default());
    board_init(); ESP_ERROR_CHECK(bsp_audio_init(NULL)); esp_codec_dev_handle_t mic=bsp_audio_codec_microphone_init();
    display_start(); bsp_display_backlight_on(); bsp_display_lock(0); build_ui(); bsp_display_unlock();
    esp_log_level_set("websocket_client",ESP_LOG_WARN); esp_log_level_set("HTTP_CLIENT",ESP_LOG_WARN);
    // ESP-IDF transport error logging can include the entire Authorization header.
    esp_log_level_set("transport_ws",ESP_LOG_NONE);
    xTaskCreate(console_task,"console",6144,NULL,5,NULL);
    char ssid[33]={0},pass[65]={0};
    if(!portal_load_creds(ssid,sizeof(ssid),pass,sizeof(pass))) { set_state("Wi-Fi: join esptember-setup"); portal_start(portal_submit); return; }
    ESP_ERROR_CHECK(esp_netif_init()); esp_netif_create_default_wifi_sta(); wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT(); ESP_ERROR_CHECK(esp_wifi_init(&init));
    esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_event,NULL); esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,wifi_event,NULL);
    wifi_config_t wifi={0}; strlcpy((char *)wifi.sta.ssid,ssid,sizeof(wifi.sta.ssid)); strlcpy((char *)wifi.sta.password,pass,sizeof(wifi.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,&wifi)); ESP_ERROR_CHECK(esp_wifi_start());
    xTaskCreate(mic_task,"mic",6144,mic,6,NULL); xTaskCreate(app_task,"captions",32768,NULL,5,NULL);
}
