#pragma once
//
// LocationWiFi.h - V3 location provider (no GPS).
//
// Brings up a WiFi access point + captive-portal DNS, serves a setup page over
// HTTPS (self-signed, so the browser's navigator.geolocation works) and over
// plain HTTP (manual lat/lon fallback). The page POSTs lat/lon + the browser's
// UTC epoch back to /set; we cache the location, sync the RTC, then tear WiFi
// down again. Included by Location.h when USE_WIFI_LOCATION is defined.
//
// =====================================================================
//  !!! DEPENDENCY: install the "esp32_https_server" library (fhessel) !!!
//  (WiFi, DNSServer are part of the ESP32 Arduino core.) The HTTPS API below
//  follows that library's documented usage; not compile-checked here because
//  the toolchain/libraries are absent from this checkout.
// =====================================================================
//
#include "config.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <HTTPSServer.hpp>
#include <HTTPServer.hpp>
#include <SSLCert.hpp>
#include <ResourceNode.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "webcert.h"

using namespace httpsserver;

#ifndef WIFI_SETUP_TIMEOUT_MS
#define WIFI_SETUP_TIMEOUT_MS 300000UL    // close the AP after 5 min idle
#endif

static const IPAddress AP_IP(192, 168, 4, 1);

// Filled by the /set handler.
static volatile bool wifiGotCoords = false;
static double        wifiLat = 0, wifiLon = 0;

static const char SETUP_PAGE[] =
"<!doctype html><html><head><meta name=viewport content='width=device-width,initial-scale=1'>"
"<title>T-Watch Azan setup</title><style>body{font-family:sans-serif;margin:22px;max-width:460px}"
"input{font-size:1.2em;width:8em}button{font-size:1.2em;padding:.4em 1em;margin:.3em 0}"
"#s{margin-top:1em;font-weight:bold}</style></head><body>"
"<h2>Set watch location</h2>"
"<button onclick='loc()'>Use my location</button>"
"<p>or enter manually (from a map app):</p>"
"<div>Lat <input id=la type=number step=any></div>"
"<div>Lon <input id=lo type=number step=any></div>"
"<button onclick='man()'>Save</button><div id=s></div>"
"<script>"
"function S(t){document.getElementById('s').textContent=t;}"
"function send(a,o){var e=Math.floor(Date.now()/1000);S('Saving...');"
"fetch('/set?lat='+a+'&lon='+o+'&epoch='+e).then(r=>r.text()).then(t=>S(t)).catch(x=>S('Error: '+x));}"
"function loc(){if(!navigator.geolocation){S('No geolocation - use manual');return;}S('Locating...');"
"navigator.geolocation.getCurrentPosition(p=>send(p.coords.latitude.toFixed(5),p.coords.longitude.toFixed(5)),"
"e=>S('Blocked/denied - use manual ('+e.message+')'),{enableHighAccuracy:true,timeout:10000});}"
"function man(){var a=document.getElementById('la').value,o=document.getElementById('lo').value;"
"if(a===''||o===''){S('Enter both values');return;}send(a,o);}"
"</script></body></html>";

static void wifiServePage(HTTPResponse *res) {
     res->setHeader("Content-Type", "text/html");
     res->print(SETUP_PAGE);
}
static void handleRoot(HTTPRequest *req, HTTPResponse *res) { wifiServePage(res); }

// Captive-portal: serve the page for any unknown path so the "sign in" UI pops.
static void handleCaptive(HTTPRequest *req, HTTPResponse *res) { wifiServePage(res); }

static void handleSet(HTTPRequest *req, HTTPResponse *res) {
     std::string sLat, sLon, sEpoch;
     ResourceParameters *p = req->getParams();
     bool okLat = p->getQueryParameter("lat", sLat);
     bool okLon = p->getQueryParameter("lon", sLon);
     p->getQueryParameter("epoch", sEpoch);

     res->setHeader("Content-Type", "text/plain");
     if (!okLat || !okLon) { res->setStatusCode(400); res->print("Missing lat/lon"); return; }

     wifiLat = atof(sLat.c_str());
     wifiLon = atof(sLon.c_str());

     if (!sEpoch.empty()) {                     // sync RTC from the browser's UTC clock
          long long epoch = atoll(sEpoch.c_str());
          long days = (long)(epoch / 86400);
          long rem  = (long)(epoch - (long long)days * 86400);
          if (rem < 0) { rem += 86400; days -= 1; }
          int Y, Mo, Da; pc_civilFromDays(days, Y, Mo, Da);
          setRTCFromUTC(Y, Mo, Da, rem / 3600, (rem % 3600) / 60, rem % 60);
     }
     wifiGotCoords = true;                      // signal the setup loop
     res->print("Saved. You can close this page.");
}

static void wifiDrawStatus(const char *l1, const char *l2, const char *l3) {
     watch->tft->fillScreen(TFT_BLACK);
     watch->tft->setTextColor(TFT_WHITE, TFT_BLACK);
     watch->tft->drawString(l1, 0, 20);
     if (l2) watch->tft->drawString(l2, 0, 70);
     if (l3) watch->tft->drawString(l3, 0, 120);
}

bool acquireLocation(double &lat, double &lon) {
     wifiGotCoords = false;

     WiFi.mode(WIFI_AP);
     WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
     if (strlen(AP_PASS) >= 8) WiFi.softAP(AP_SSID, AP_PASS);
     else                      WiFi.softAP(AP_SSID);          // open network

     DNSServer dns;
     dns.start(53, "*", AP_IP);                                // captive portal

     SSLCert    cert((unsigned char *)crt_DER, sizeof(crt_DER),
                     (unsigned char *)key_DER, sizeof(key_DER));
     HTTPSServer secure(&cert);
     HTTPServer  insecure;

     // Distinct node instances per server (don't share one pointer across both).
     ResourceNode sRoot("/", "GET", &handleRoot), sSet("/set", "GET", &handleSet), sCap("", "GET", &handleCaptive);
     ResourceNode iRoot("/", "GET", &handleRoot), iSet("/set", "GET", &handleSet), iCap("", "GET", &handleCaptive);
     secure.registerNode(&sRoot);   secure.registerNode(&sSet);   secure.setDefaultNode(&sCap);
     insecure.registerNode(&iRoot); insecure.registerNode(&iSet); insecure.setDefaultNode(&iCap);
     secure.start();
     insecure.start();

     wifiDrawStatus("WiFi setup", "Join: " AP_SSID, "Open https://192.168.4.1");

     uint32_t start = millis();
     while (!wifiGotCoords && millis() - start < WIFI_SETUP_TIMEOUT_MS) {
          dns.processNextRequest();
          secure.loop();
          insecure.loop();
          int16_t tx, ty;
          if (watch->getTouch(tx, ty)) break;                 // touch to abort
          delay(5);
     }

     // Let the "Saved" response flush to the browser before we drop WiFi.
     if (wifiGotCoords)
          for (int k = 0; k < 40; k++) { secure.loop(); insecure.loop(); delay(10); }

     secure.stop();
     insecure.stop();
     dns.stop();
     WiFi.softAPdisconnect(true);
     WiFi.mode(WIFI_OFF);

     if (wifiGotCoords) {
          lat = wifiLat; lon = wifiLon;
          char l2[40]; snprintf(l2, sizeof(l2), "%.4f, %.4f", lat, lon);
          wifiDrawStatus("Location set", l2, nullptr);
          delay(1500);
          return true;
     }
     wifiDrawStatus("Setup closed", "Kept saved location", nullptr);
     delay(1500);
     return false;
}
