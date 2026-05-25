#pragma once
//
// Location.h - cached watch location + the board-agnostic setup entry point.
//
// Holds the active latitude/longitude (persisted in NVS via Preferences), the
// effective timezone helper, an RTC clock-sync helper, and enterLocationSetup()
// which dispatches to the GPS provider (V2) or the WiFi provider (V3).
//
#include "config.h"        // brings in <LilyGoWatch.h> + all user #defines
#include "PrayerCalc.h"
#include <Preferences.h>
#include <math.h>

extern TTGOClass *watch;   // defined in T-Watch_Azan.ino

// ---- Active location (degrees, N/E positive). Defaults until a fix is cached ----
double gLat = DEFAULT_LAT;
double gLon = DEFAULT_LON;
bool   locationKnown = false;

static Preferences gPrefs;

// Effective UTC offset in hours, including DST.
static inline double effectiveTZ() {
     return (double)UTC_OFFSET + (DST_ENABLED ? 1.0 : 0.0);
}

void loadLocation() {
     gPrefs.begin("azan", true);              // read-only
     if (gPrefs.getBool("set", false)) {
          gLat = gPrefs.getDouble("lat", DEFAULT_LAT);
          gLon = gPrefs.getDouble("lon", DEFAULT_LON);
          locationKnown = true;
     } else {
          gLat = DEFAULT_LAT; gLon = DEFAULT_LON; locationKnown = false;
     }
     gPrefs.end();
}

void saveLocation(double lat, double lon) {
     gLat = lat; gLon = lon; locationKnown = true;
     gPrefs.begin("azan", false);             // read-write
     gPrefs.putDouble("lat", lat);
     gPrefs.putDouble("lon", lon);
     gPrefs.putBool("set", true);
     gPrefs.end();
}

// ---- Civil-date <-> days-since-1970 (Howard Hinnant), for UTC->local RTC sync ----
static long pc_daysFromCivil(int y, int m, int d) {
     y -= m <= 2;
     long era = (y >= 0 ? y : y - 399) / 400;
     unsigned yoe = (unsigned)(y - era * 400);
     unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
     unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
     return era * 146097L + (long)doe - 719468L;
}
static void pc_civilFromDays(long z, int &y, int &m, int &d) {
     z += 719468L;
     long era = (z >= 0 ? z : z - 146096) / 146097;
     unsigned doe = (unsigned)(z - era * 146097L);
     unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
     y = (int)(yoe) + (int)(era * 400);
     unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
     unsigned mp = (5 * doy + 2) / 153;
     d = (int)(doy - (153 * mp + 2) / 5 + 1);
     m = (int)(mp + (mp < 10 ? 3 : -9));
     y += (m <= 2);
}

// Set the RTC to local time given a UTC instant, applying effectiveTZ().
// Used by both the GPS (V2) and WiFi (V3) providers.
void setRTCFromUTC(int Y, int Mo, int Da, int H, int Mi, int S) {
     long long secs = (long long)pc_daysFromCivil(Y, Mo, Da) * 86400LL
                      + (long long)H * 3600 + Mi * 60 + S;
     secs += (long long)llround(effectiveTZ() * 3600.0);
     long days = (long)(secs / 86400);
     long rem  = (long)(secs - (long long)days * 86400);
     if (rem < 0) { rem += 86400; days -= 1; }
     int y, mo, da; pc_civilFromDays(days, y, mo, da);
     watch->rtc->setDateTime(y, mo, da, rem / 3600, (rem % 3600) / 60, rem % 60);
}

// Provided by the board-specific header below: blocks (with on-screen status)
// until a location is obtained or the user/timeout aborts. Returns true on
// success (and is responsible for any RTC sync).
bool acquireLocation(double &lat, double &lon);

// Run the location setup flow and persist the result.
void enterLocationSetup() {
     double lat, lon;
     if (acquireLocation(lat, lon))
          saveLocation(lat, lon);
}

#if defined(HAS_GPS)
     #include "LocationGPS.h"
#elif defined(USE_WIFI_LOCATION)
     #include "LocationWiFi.h"
#endif
