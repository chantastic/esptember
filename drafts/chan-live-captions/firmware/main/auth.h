// Public-client AuthKit device session. Provider keys never enter this firmware.
// Reuses day-32 NVS namespaces without exporting stored values or erasing partitions.
#include "esp_http_client.h"
#include "mbedtls/base64.h"
#define EXPECTED_CLIENT "client_01M24S76WQHXQ6QJJ7S83CMR3F"
#define TOKEN_URL "https://api.workos.com/user_management/authenticate"
static char client_id[64], access_token[16384], organization[128];
static int64_t token_expires=0;
static bool load_str(const char *ns,const char *key,char *out,size_t size) {
    nvs_handle_t handle; if(nvs_open(ns,NVS_READONLY,&handle)!=ESP_OK) return false;
    bool ok=nvs_get_str(handle,key,out,&size)==ESP_OK && out[0]; nvs_close(handle); return ok;
}
static bool save_str(const char *ns,const char *key,const char *value) {
    nvs_handle_t handle; if(nvs_open(ns,NVS_READWRITE,&handle)!=ESP_OK) return false;
    bool ok=nvs_set_str(handle,key,value)==ESP_OK && nvs_commit(handle)==ESP_OK; nvs_close(handle); return ok;
}
static cJSON *http_json(const char *url,const char *payload,const char *bearer,int *status) {
    *status=0;
    esp_http_client_config_t cfg={.url=url,.method=payload?HTTP_METHOD_POST:HTTP_METHOD_GET,.crt_bundle_attach=esp_crt_bundle_attach,
        .timeout_ms=30000,.buffer_size=2048,.buffer_size_tx=18000,.disable_auto_redirect=true};
    esp_http_client_handle_t h=esp_http_client_init(&cfg); if(!h) return NULL;
    char *auth=NULL; cJSON *result=NULL; char *body=calloc(1,32769);
    if(!body) goto done;
    if(bearer) { auth=malloc(strlen(bearer)+8); if(!auth) goto done; sprintf(auth,"Bearer %s",bearer); esp_http_client_set_header(h,"Authorization",auth); }
    if(payload) esp_http_client_set_header(h,"Content-Type","application/json");
    size_t n=payload?strlen(payload):0;
    if(esp_http_client_open(h,n)!=ESP_OK) goto done;
    if(n && esp_http_client_write(h,payload,n)!=(int)n) goto done;
    if(esp_http_client_fetch_headers(h)<0) goto done;
    *status=esp_http_client_get_status_code(h);
    int length=esp_http_client_read_response(h,body,32768);
    if(length>0 && length<32768 && esp_http_client_is_complete_data_received(h)) result=cJSON_ParseWithLength(body,length);
 done:
    if(body) { memset(body,0,32769); free(body); }
    if(auth) { memset(auth,0,strlen(auth)); free(auth); }
    esp_http_client_cleanup(h); return result;
}
// Decode only for local renewal timing. The server independently verifies every JWT claim/signature.
static bool inspect_token(void) {
    const char *a=strchr(access_token,'.'); if(!a) return false; a++;
    const char *b=strchr(a,'.'); if(!b || b-a>8192) return false;
    char encoded[8197]; size_t len=b-a;
    memcpy(encoded,a,len); for(size_t i=0;i<len;i++) { if(encoded[i]=='-') encoded[i]='+'; else if(encoded[i]=='_') encoded[i]='/'; }
    while(len%4) { encoded[len++]='='; }
    encoded[len]=0;
    unsigned char decoded[8193]; size_t out=0;
    if(mbedtls_base64_decode(decoded,sizeof(decoded)-1,&out,(const unsigned char *)encoded,len)!=0) return false;
    cJSON *claims=cJSON_ParseWithLength((char *)decoded,out); memset(decoded,0,sizeof(decoded)); if(!claims) return false;
    cJSON *exp=cJSON_GetObjectItem(claims,"exp"),*org=cJSON_GetObjectItem(claims,"org_id"),*client=cJSON_GetObjectItem(claims,"client_id");
    bool ok=cJSON_IsNumber(exp) && cJSON_IsString(client) && !strcmp(client->valuestring,EXPECTED_CLIENT);
    token_expires=ok?(int64_t)exp->valuedouble:0;
    organization[0]=0;
    if(cJSON_IsString(org) && strlen(org->valuestring)<sizeof(organization)) strcpy(organization,org->valuestring);
    cJSON_Delete(claims); return ok;
}
static bool absorb(cJSON *result) {
    cJSON *access=cJSON_GetObjectItem(result,"access_token"),*refresh=cJSON_GetObjectItem(result,"refresh_token");
    if(!cJSON_IsString(access)||strlen(access->valuestring)>=sizeof(access_token)) return false;
    if(cJSON_IsString(refresh) && !save_str("authkit","refresh",refresh->valuestring)) return false;
    strcpy(access_token,access->valuestring); return inspect_token();
}
static bool refresh_session(const char *org) {
    char refresh[2048]; if(!load_str("authkit","refresh",refresh,sizeof(refresh))) return false;
    cJSON *request=cJSON_CreateObject(); cJSON_AddStringToObject(request,"grant_type","refresh_token");
    cJSON_AddStringToObject(request,"client_id",client_id); cJSON_AddStringToObject(request,"refresh_token",refresh);
    if(org && org[0]) cJSON_AddStringToObject(request,"organization_id",org);
    char *body=cJSON_PrintUnformatted(request); cJSON_Delete(request); memset(refresh,0,sizeof(refresh));
    int status=0; cJSON *result=http_json(TOKEN_URL,body,NULL,&status); memset(body,0,strlen(body)); free(body);
    bool ok=status==200 && result && absorb(result); if(result) cJSON_Delete(result);
    printf("LIVE_AUTH refresh=%d http=%d\n",ok,status); return ok;
}
static bool select_workspace(void) {
    if(organization[0]) return true;
    int status=0; cJSON *r=http_json("https://auth.chan.dev/devices/workspace",NULL,access_token,&status);
    cJSON *v=r?cJSON_GetObjectItem(r,"organizationId"):NULL; char org[128]="";
    if(status==200 && cJSON_IsString(v) && strlen(v->valuestring)<sizeof(org)) strcpy(org,v->valuestring);
    if(r) cJSON_Delete(r);
    return org[0] && refresh_session(org);
}
static bool device_login(void) {
    cJSON *req=cJSON_CreateObject(); cJSON_AddStringToObject(req,"client_id",client_id);
    char *body=cJSON_PrintUnformatted(req); cJSON_Delete(req); int status=0;
    cJSON *r=http_json("https://api.workos.com/user_management/authorize/device",body,NULL,&status); free(body);
    if(!r||status!=200) { if(r) cJSON_Delete(r); return false; }
    cJSON *dc=cJSON_GetObjectItem(r,"device_code"),*uc=cJSON_GetObjectItem(r,"user_code"),*uri=cJSON_GetObjectItem(r,"verification_uri_complete");
    if(!cJSON_IsString(dc)||!cJSON_IsString(uc)||!cJSON_IsString(uri)) { cJSON_Delete(r); return false; }
    char code[256]; strlcpy(code,dc->valuestring,sizeof(code));
    show_login(uc->valuestring,uri->valuestring); // on-screen only; no codes/tokens in serial logs
    cJSON_Delete(r);
    for(int i=0;i<60;i++) {
        vTaskDelay(pdMS_TO_TICKS(5000)); req=cJSON_CreateObject();
        cJSON_AddStringToObject(req,"client_id",client_id); cJSON_AddStringToObject(req,"grant_type","urn:ietf:params:oauth:grant-type:device_code");
        cJSON_AddStringToObject(req,"device_code",code); body=cJSON_PrintUnformatted(req); cJSON_Delete(req);
        r=http_json(TOKEN_URL,body,NULL,&status); memset(body,0,strlen(body)); free(body);
        bool ok=status==200 && r && absorb(r); if(r) cJSON_Delete(r); if(ok) return true;
    }
    return false;
}
static bool authenticate(void) {
    if(!load_str("authkit","client_id",client_id,sizeof(client_id)) || strcmp(client_id,EXPECTED_CLIENT)) {
        set_state("Devices client setup required"); printf("LIVE_AUTH client_match=0\n"); return false;
    }
    if(!refresh_session(NULL) && !device_login()) return false;
    bool ready=select_workspace(); printf("LIVE_AUTH ready=%d workspace=%d\n",ready,organization[0]!=0); return ready;
}
