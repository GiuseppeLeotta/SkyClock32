#ifndef _SOLAR_SYS_H
#define _SOLAR_SYS_H

#include <math.h>
#include <time.h>


// Costanti
const double J2000 = 2451545.0;  // Julian date at J2000.0
const double DEG2RAD = (M_PI / 180.0);
const double RAD2DEG = (180.0 / M_PI);
#define STAR_HORIZON -0.5667


const double MaxErr = 0.0000001;  // Max Error Calc rise/transit/set

#define AU_KM 149597870.7
#define MOON_DIAM_KM 3474.2
#define SOLAR_DIAM_KM 1391400 /* mean solar diameter in km */


#define MERCURY 0
#define VENUS 1
#define EARTH 2
#define MARS 3
#define JUPITER 4
#define SATURN 5

#define N_PLANETS 6

const double ReEarth = 6378.137;  // raggio equatoriale Terra [km]

struct Observer {
  double lat;    // Gradi decimali
  double lon;    // Gradi decimali
  double elev;   // Metri
  double temp;   // Celsius
  double press;  // hPa
};

struct CelestialPosition {
  double RA;    // ore
  double dec;   // Gradi
  double az;    // Gradi
  double el;    // Gradi
  double dist;  // UA (Sole), Km (Luna)
  double magnitude;
  double phase;
  double moonlimb;  // posizione angolare (°) del bright-limb della Luna, misurata dal punto Nord del disco verso est
  double age;
  double diam;  // diametro apparente arcsec
  double parallacticAngle; // Angolo Parallattico della luna
};

struct RiseSet {
  double rise;     // JD alba
  double transit;  // JD transito
  double set;      // JD tramonto
  double minEl;    // Minimo dell'elevazione
  double maxEl;    // Massimo dell'elevazione  
  int polar;       // 1 if the object is circumpolar, that is it remains the whole day above the horizon.
                   // -1 when it remains whole day bellow the horizon.
  double Err[3];   // error calcola alba/tramonto  Err[0]->rise, Err[1]->set, Err[2]->transit;
};

const double SUN_ANGLE[4] = {
  -0.833,  // Standard
  -6.0,    // Civile
  -12.0,   // Nautica
  -18.0    // Astronomica
};


void SunPosition(double jd, Observer &obs, CelestialPosition &pos);
void MoonPosition(double jd, Observer &obs, CelestialPosition &pos);
void MercuryPosition(double jd, Observer &obs, CelestialPosition &pos);
void VenusPosition(double jd, Observer &obs, CelestialPosition &pos);
void MarsPosition(double jd, Observer &obs, CelestialPosition &pos);
void JupiterPosition(double jd, Observer &obs, CelestialPosition &pos);
void SaturnPosition(double jd, Observer &obs, CelestialPosition &pos);

int SolarRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight);
int MoonRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight);
int MercuryRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight);
int VenusRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight);
int MarsRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight);
int JupiterRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight);
int SaturnRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight);

char * strMoonPhase(double lunarAge);

char *StrdegToDMS(double x);
char *StrhourToHMS(double x);
double time2jd(time_t t);
time_t jd2time(double jd);
double julianDay(int year, int month, int day, double hour);
void JD_to_DateTime(double jd, int &year, int &month, int &day, int &hour, int &minute, double &second);
char *jd2str(double JD, bool olHour = false);


#endif