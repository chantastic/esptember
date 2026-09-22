// Shared Arduino captive portal — day 21's pattern, ported for the
// StopWatch days. The device can't show a browser, but it never needed
// to: it hosts an open AP ("esptember-setup"), a liar's DNS answers
// every lookup with our address, the phone's connectivity probe trips
// over it, and the setup page opens by itself. SSID + password are
// standard; each day can add extra fields (API keys, URLs) that land in
// its own Preferences namespace. Serial provisioning stays available as
// the power-user path.
#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

struct PortalField {
  const char *key;   // Preferences key
  const char *label; // form label
  bool secret;       // render as a password box
};

class WifiPortal {
 public:
  // Setup mode: no saved Wi-Fi. Blocks forever (reboots on save).
  void run(Preferences &wifiPrefs, Preferences &extraPrefs,
           const PortalField *fields, int fieldCount) {
    bind(wifiPrefs, extraPrefs, fields, fieldCount);
    WiFi.mode(WIFI_AP_STA); // STA half powers the network scan
    WiFi.softAP("esptember-setup");
    dns_.start(53, "*", WiFi.softAPIP());
    server_.onNotFound([this]() { handle(); });
    server_.begin();
    Serial.printf("PORTAL_UP ssid=esptember-setup ip=%s\n",
                  WiFi.softAPIP().toString().c_str());
    while (true) {
      dns_.processNextRequest();
      server_.handleClient();
      delay(2);
    }
  }

  // Settings mode: connected. The SAME form stays reachable at the
  // device's station IP, forever — no chord, no serial, no ceremony.
  // Call beginSettings() after Wi-Fi connects and handleLoop() every
  // loop() pass.
  void beginSettings(Preferences &wifiPrefs, Preferences &extraPrefs,
                     const PortalField *fields, int fieldCount) {
    bind(wifiPrefs, extraPrefs, fields, fieldCount);
    settingsMode_ = true;
    server_.onNotFound([this]() { handle(); });
    server_.begin();
    Serial.printf("SETTINGS_UP url=http://%s/\n",
                  WiFi.localIP().toString().c_str());
  }

  void handleLoop() { server_.handleClient(); }

 private:
  void handle() {
    if (server_.method() == HTTP_POST && server_.uri() == "/save") {
      save();
      return;
    }
    if (server_.uri() != "/") { // captive probes: redirect home
      server_.sendHeader("Location", "http://" +
                                         WiFi.softAPIP().toString() + "/");
      server_.send(302, "text/plain", "");
      return;
    }
    form();
  }

  void form() {
    String page =
        "<!doctype html><meta name=viewport content='width=device-width,"
        "initial-scale=1'><title>ESPtember setup</title>"
        "<body style='font-family:sans-serif;background:#111;color:#eee;"
        "padding:24px'><h2 style='color:#ff5b04'>ESPtember setup</h2>"
        "<form method=post action=/save>"
        "<label>Network<br><select name=ssid style='width:100%;padding:8px;"
        "font-size:16px'>";
    const String saved = wifiPrefs_->getString("ssid", "");
    const int n = WiFi.scanNetworks();
    bool listed = false;
    for (int i = 0; i < n && i < 12; i++) {
      const bool sel = WiFi.SSID(i) == saved;
      listed |= sel;
      page += String("<option") + (sel ? " selected" : "") + ">" +
              WiFi.SSID(i) + "</option>";
    }
    if (saved.length() && !listed)
      page += "<option selected>" + saved + "</option>";
    page +=
        "</select></label><br><br><label>Password";
    if (settingsMode_) page += " <small>(blank = keep current)</small>";
    page +=
        "<br><input name=pass type=password style='width:100%;padding:8px;"
        "font-size:16px'></label>";
    for (int i = 0; i < fieldCount_; i++) {
      page += "<br><br><label>";
      page += fields_[i].label;
      page += "<br><input name=";
      page += fields_[i].key;
      page += fields_[i].secret ? " type=password" : " type=text";
      // Pre-fill non-secret fields so re-runs keep old values.
      if (!fields_[i].secret) {
        page += " value='" + extraPrefs_->getString(fields_[i].key, "") + "'";
      }
      page += " style='width:100%;padding:8px;font-size:16px'></label>";
    }
    page +=
        "<br><br><button style='padding:10px 24px;font-size:16px;"
        "background:#ff5b04;border:0;color:#000'>Save & connect</button>"
        "</form>";
    server_.send(200, "text/html", page);
  }

  void save() {
    // In settings mode an empty password means "keep the old one" —
    // people change API keys far more often than networks.
    const String ssid = server_.arg("ssid");
    const String pass = server_.arg("pass");
    if (ssid.length()) wifiPrefs_->putString("ssid", ssid);
    if (pass.length() || !settingsMode_)
      wifiPrefs_->putString("pass", pass);
    for (int i = 0; i < fieldCount_; i++) {
      const String value = server_.arg(fields_[i].key);
      if (value.length()) extraPrefs_->putString(fields_[i].key, value);
    }
    server_.send(200, "text/html",
                 "<body style='font-family:sans-serif;background:#111;"
                 "color:#eee;padding:24px'><h2 style='color:#ff5b04'>Saved."
                 "</h2>The board is rebooting onto your network.");
    Serial.printf("PORTAL_SAVED ssid=%s\n", server_.arg("ssid").c_str());
    delay(1500);
    ESP.restart();
  }

  void bind(Preferences &wifiPrefs, Preferences &extraPrefs,
            const PortalField *fields, int fieldCount) {
    wifiPrefs_ = &wifiPrefs;
    extraPrefs_ = &extraPrefs;
    fields_ = fields;
    fieldCount_ = fieldCount;
  }

  bool settingsMode_ = false;
  Preferences *wifiPrefs_ = nullptr;
  Preferences *extraPrefs_ = nullptr;
  const PortalField *fields_ = nullptr;
  int fieldCount_ = 0;
  WebServer server_{80};
  DNSServer dns_;
};
