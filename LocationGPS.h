#pragma once
//
// LocationGPS.h - V2 location provider.
// Reads lat/lon (+ UTC date/time) from the onboard L76 GPS, shows progress,
// syncs the RTC, then powers the GPS back off. Included by Location.h when
// HAS_GPS is defined (T-Watch 2020 V2).
//
// =====================================================================
//  !!! VERIFY THE GPS API AGAINST YOUR INSTALLED TTGO_TWatch_Library !!!
//  The library is not present in this checkout. The calls below follow the
//  common LilyGoWatch GPS examples; adjust if your library version differs:
//    watch->trunOnGPS() / trunOffGPS()  - AXP LDO power for the GPS module
//    watch->gps_begin()                 - start GPS UART + TinyGPSPlus parser
//    watch->gps                         - TinyGPSPlus*  (location/date/time)
//    watch->gpsHandler()                - pump available UART bytes into parser
//  If gpsHandler() is unavailable in your version, feed manually instead, e.g.
//    while (Serial1.available()) watch->gps->encode(Serial1.read());
// =====================================================================
//
#include "config.h"

#ifndef GPS_TIMEOUT_MS
#define GPS_TIMEOUT_MS 120000UL     // abort acquiring a fix after this long
#endif

static void gpsDrawStatus(const char *l1, const char *l2) {
     watch->tft->fillScreen(TFT_BLACK);
     watch->tft->setTextColor(TFT_WHITE, TFT_BLACK);
     watch->tft->drawString(l1, 0, 40);
     if (l2) watch->tft->drawString(l2, 0, 90);
}

bool acquireLocation(double &lat, double &lon) {
     watch->trunOnGPS();
     watch->gps_begin();
     gpsDrawStatus("Acquiring GPS", "Go outdoors...");

     uint32_t start = millis();
     char line2[40];
     bool got = false;

     while (millis() - start < GPS_TIMEOUT_MS) {
          watch->gpsHandler();                 // feed UART bytes into watch->gps

          uint32_t sats = watch->gps->satellites.isValid()
                          ? watch->gps->satellites.value() : 0;
          snprintf(line2, sizeof(line2), "Sats:%u  %lus", (unsigned)sats,
                   (unsigned long)((millis() - start) / 1000));
          gpsDrawStatus("Acquiring GPS", line2);

          if (watch->gps->location.isValid() && watch->gps->location.isUpdated()
              && watch->gps->date.isValid() && watch->gps->date.year() > 2019) {
               lat = watch->gps->location.lat();
               lon = watch->gps->location.lng();
               setRTCFromUTC(watch->gps->date.year(),  watch->gps->date.month(),
                             watch->gps->date.day(),    watch->gps->time.hour(),
                             watch->gps->time.minute(), watch->gps->time.second());
               got = true;
               break;
          }

          int16_t tx, ty;
          if (watch->getTouch(tx, ty)) break;  // touch to abort
          delay(200);
     }

     watch->trunOffGPS();                       // save power

     if (got) {
          snprintf(line2, sizeof(line2), "%.4f, %.4f", lat, lon);
          gpsDrawStatus("Location set", line2);
     } else {
          gpsDrawStatus("GPS timeout", "Kept saved location");
     }
     delay(1500);
     return got;
}
