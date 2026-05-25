#pragma once
//
// PrayerCalc.h - Astronomical prayer-time calculation.
//
// Replaces the old hardcoded PrayerTimes.h lookup table: given a civil date,
// a latitude/longitude and a UTC offset, it computes the six daily times using
// the standard PrayTimes.org / NOAA solar algorithm.
//
// Output (computePrayerTimes) fills out[6] with minutes-since-midnight in the
// SAME order the rest of the firmware expects:
//     [0]=Fajr  [1]=Shurooq(sunrise)  [2]=Duhr  [3]=Asr  [4]=Maghrib(sunset)  [5]=Isha
//
// Pure math, no Arduino/hardware dependencies, so it can be unit-tested on a
// host with g++ (see verification step in the plan).
//

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Calculation method (overridable from config.h). Defaults = Muslim World League ---
#ifndef FAJR_ANGLE
#define FAJR_ANGLE 18.0          // Fajr twilight depression angle (degrees)
#endif
#ifndef ISHA_ANGLE
#define ISHA_ANGLE 17.0          // Isha twilight depression angle (degrees)
#endif
#ifndef ISHA_MINUTES_AFTER_MAGHRIB
#define ISHA_MINUTES_AFTER_MAGHRIB 0   // >0 (e.g. 90 for Umm al-Qura) overrides ISHA_ANGLE
#endif
#ifndef ASR_FACTOR
#define ASR_FACTOR 1             // shadow factor: 1 = Shafi/Standard, 2 = Hanafi
#endif
#ifndef SUNSET_ANGLE
#define SUNSET_ANGLE 0.833       // sun radius + atmospheric refraction (sunrise/Maghrib)
#endif
#ifndef DUHR_OFFSET_MIN
#define DUHR_OFFSET_MIN 0        // minutes added to true solar noon (safety margin after zawal)
#endif

// --- Degree-based trig helpers ---
static inline double pc_dsin(double d)  { return sin(d * M_PI / 180.0); }
static inline double pc_dcos(double d)  { return cos(d * M_PI / 180.0); }
static inline double pc_dtan(double d)  { return tan(d * M_PI / 180.0); }
static inline double pc_dasin(double x) { return asin(x) * 180.0 / M_PI; }
static inline double pc_dacos(double x) { return acos(x) * 180.0 / M_PI; }
static inline double pc_datan2(double y, double x) { return atan2(y, x) * 180.0 / M_PI; }
static inline double pc_dacot(double x) { return atan2(1.0, x) * 180.0 / M_PI; }

static inline double pc_fixangle(double a) { a = fmod(a, 360.0); return a < 0 ? a + 360.0 : a; }
static inline double pc_fixhour(double a)  { a = fmod(a, 24.0);  return a < 0 ? a + 24.0  : a; }

// Julian Day at 0h UT for a Gregorian civil date.
static inline double pc_julian(int year, int month, int day) {
     if (month <= 2) { year -= 1; month += 12; }
     double A = floor(year / 100.0);
     double B = 2 - A + floor(A / 4.0);
     return floor(365.25 * (year + 4716)) + floor(30.6001 * (month + 1)) + day + B - 1524.5;
}

// Hour offset (from solar noon) at which the sun reaches the given depression
// angle. `angle` is degrees below the horizon (sunrise/Fajr/Isha positive; for
// Asr a negative angle is passed since the sun is above the horizon).
// Sets *ok=false and clamps when the sun never reaches the angle (high latitude).
static inline double pc_sunAngleOffset(double angle, double lat, double decl, bool *ok) {
     double x = (-pc_dsin(angle) - pc_dsin(lat) * pc_dsin(decl)) /
                (pc_dcos(lat) * pc_dcos(decl));
     if (x < -1.0) { x = -1.0; if (ok) *ok = false; }
     if (x >  1.0) { x =  1.0; if (ok) *ok = false; }
     return pc_dacos(x) / 15.0;   // degrees -> hours
}

// Convert an (possibly out-of-range) hour value to minutes since local midnight.
static inline int pc_toMinutes(double hours) {
     hours = fmod(hours, 24.0);
     if (hours < 0) hours += 24.0;
     int m = (int)floor(hours * 60.0 + 0.5);   // round to nearest minute
     m %= 1440;
     if (m < 0) m += 1440;
     return m;
}

// Compute the six prayer times into out[6] (minutes since midnight).
//   lat,lon : degrees, North/East positive.
//   tz      : UTC offset in hours, INCLUDING any DST already applied.
// Returns true if every time is well-defined; false if any was clamped
// (e.g. extreme latitude where Fajr/Isha don't occur).
static inline bool computePrayerTimes(int year, int month, int day,
                                      double lat, double lon, double tz,
                                      int out[6]) {
     double jd = pc_julian(year, month, day);

     // Sun position evaluated near local solar noon (good to well under a minute
     // for all six times across the day).
     double D = jd - 2451545.0 + (12.0 - tz) / 24.0;
     double g = pc_fixangle(357.529 + 0.98560028 * D);          // mean anomaly
     double q = pc_fixangle(280.459 + 0.98564736 * D);          // mean longitude
     double L = pc_fixangle(q + 1.915 * pc_dsin(g) + 0.020 * pc_dsin(2 * g)); // ecliptic lon
     double e = 23.439 - 0.00000036 * D;                        // obliquity

     double RA = pc_fixhour(pc_datan2(pc_dcos(e) * pc_dsin(L), pc_dcos(L)) / 15.0);
     double EqT  = q / 15.0 - RA;                               // equation of time (hours)
     double decl = pc_dasin(pc_dsin(e) * pc_dsin(L));           // declination

     double dhuhr = 12.0 + tz - lon / 15.0 - EqT;               // local solar noon (hours)

     bool ok = true;
     double fajr    = dhuhr - pc_sunAngleOffset(FAJR_ANGLE,   lat, decl, &ok);
     double shurooq = dhuhr - pc_sunAngleOffset(SUNSET_ANGLE, lat, decl, &ok);
     double maghrib = dhuhr + pc_sunAngleOffset(SUNSET_ANGLE, lat, decl, &ok);

     double asrAngle = -pc_dacot(ASR_FACTOR + pc_dtan(fabs(lat - decl)));
     double asr = dhuhr + pc_sunAngleOffset(asrAngle, lat, decl, &ok);

     double isha;
     if (ISHA_MINUTES_AFTER_MAGHRIB > 0)
          isha = maghrib + (double)ISHA_MINUTES_AFTER_MAGHRIB / 60.0;
     else
          isha = dhuhr + pc_sunAngleOffset(ISHA_ANGLE, lat, decl, &ok);

     out[0] = pc_toMinutes(fajr);
     out[1] = pc_toMinutes(shurooq);
     out[2] = pc_toMinutes(dhuhr + (double)DUHR_OFFSET_MIN / 60.0);
     out[3] = pc_toMinutes(asr);
     out[4] = pc_toMinutes(maghrib);
     out[5] = pc_toMinutes(isha);
     return ok;
}
