#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef void (*portal_submit_cb_t)(const char *ssid, const char *pass);

bool portal_load_creds(char *ssid, size_t ssid_len, char *pass,
                       size_t pass_len);
void portal_save_creds(const char *ssid, const char *pass);
void portal_clear_creds(void);
void portal_start(portal_submit_cb_t cb);
