
// =====================================================================
//  Hardware select  -- pick the board you are building for.
//  This is a COMPILE-TIME choice: the LilyGoWatch library configures pin
//  maps/peripherals from this define, so one binary can't run on both boards.
//  Build once per board.
//    * 2020_V2 -> has the onboard L76 GPS  -> location comes from GPS.
//    * 2020_V3 -> no GPS (microphone)      -> location comes from the WiFi
//                                              setup page (captive portal).
// =====================================================================
// #define LILYGO_WATCH_2019_WITH_TOUCH     // T-Watch 2019 with touchscreen
// #define LILYGO_WATCH_2019_NO_TOUCH       // T-Watch 2019 no touchscreen
// #define LILYGO_WATCH_BLOCK               // T-Watch Block
// #define LILYGO_WATCH_2020_V2             // T-Watch 2020 V2 (has GPS)
#define LILYGO_WATCH_2020_V3                // T-Watch 2020 V3 (no GPS -> WiFi setup)

// Derived location-source gates (used by Location.h and the sketch).
#if defined(LILYGO_WATCH_2020_V2)
     #define HAS_GPS 1
#elif defined(LILYGO_WATCH_2020_V3)
     #define USE_WIFI_LOCATION 1
#endif

// =====================================================================
//  Prayer-time calculation method  (Muslim World League defaults).
//  These are read by PrayerCalc.h, which is included AFTER this file.
// =====================================================================
#define FAJR_ANGLE   18.0      // Fajr twilight depression angle (deg)
#define ISHA_ANGLE   17.0      // Isha twilight depression angle (deg)
#define ASR_FACTOR   1         // shadow factor: 1 = Shafi/Standard, 2 = Hanafi
#define SUNSET_ANGLE 0.833     // sunrise/Maghrib refraction+radius (deg)
#define DUHR_OFFSET_MIN 0      // minutes added after true solar noon (zawal margin)
// For Umm al-Qura instead of an Isha angle, set:  #define ISHA_MINUTES_AFTER_MAGHRIB 90

// =====================================================================
//  Timezone  --  GPS/UTC is converted to local clock time with this.
//  Effective offset = UTC_OFFSET + (DST_ENABLED ? 1 : 0).
//  Northern hemisphere summer (Europe ~late Mar..late Oct): set DST_ENABLED 1.
// =====================================================================
#define UTC_OFFSET   1         // base UTC offset in hours
#define DST_ENABLED  0         // 1 = add one hour for daylight saving

// =====================================================================
//  Default location  --  used only until the first GPS/WiFi fix is cached.
//  (Matches the location the bundled times.json table was generated for.)
// =====================================================================
#define DEFAULT_LAT  54.65
#define DEFAULT_LON  12.95

// =====================================================================
//  WiFi setup access point (V3 only).  Empty AP_PASS => open network.
// =====================================================================
#define AP_SSID  "T-Watch-Azan"
#define AP_PASS  ""

#include <LilyGoWatch.h>
