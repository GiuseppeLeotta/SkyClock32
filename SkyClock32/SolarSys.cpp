#include <stdio.h>
#include <string.h>
#include "SolarSys.h"

//#include <Arduino.h>


const double LUNAR_CYCLE_DAYS = 29.530588853;

typedef struct {
  double RA_hours;  // Ascensione retta in ore
  double dec_deg;   // Declinazione in gradi
  double az_deg;    // Azimut in gradi
  double el_deg;    // Elevazione in gradi
} EphemData;


/* Perturbation terms for Saturn from Jupiter (Meeus Ch.47, Tbl.47.A excerpts) */
typedef struct {
  double A, B;
  int k;
} PertTerm;


/* Jupiter perturbations on Saturn's longitude (Meeus Tbl.45.A, amplitudes in degrees) */
static const PertTerm saturn_pert[] = {
  { 0.812, 0.0, 1 }, /* 1×M_J: dominant term */
  { 0.051, 0.0, 2 }, /* 2×M_J */
  { 0.006, 0.0, 3 }, /* 3×M_J */
  { 0.003, 0.0, 4 }, /* 4×M_J */
  { 0.001, 0.0, 5 }  /* 5×M_J (small) */
};

static const int N_SAT_PERT = sizeof(saturn_pert) / sizeof(saturn_pert[0]);

/* Saturn perturbations on Jupiter's longitude (amplitudes in degrees) */
static const PertTerm jupiter_pert[] = {
  { 0.240, 0.0, 1 }, /* 1×M_S: effect of Saturn on Jupiter */
  { 0.016, 0.0, 2 }, /* 2×M_S */
  { 0.004, 0.0, 3 }, /* 3×M_S */
  { 0.001, 0.0, 4 }, /* 4×M_S */
  { 0.0005, 0.0, 5 } /* 5×M_S */
};

static const int N_JUP_PERT = sizeof(jupiter_pert) / sizeof(jupiter_pert[0]);


typedef void (*EphemCallback)(double JD, Observer &obs, EphemData &et);


// elementi orbitali (Meeus Tbl.31.A): a0(adot), e0(edot), I0(Idot), L0(Ldot), wbar0(wbardot), O0(Odot) */
static const double a0[N_PLANETS] = { 0.38709843, 0.72332102, 1.00000261, 1.52371243, 5.20288700, 9.53667594 };
static const double adot[N_PLANETS] = { 0.00000000, 0.00000092, -0.00000005, 0.00000097, -0.00011607, 0.00000609 };
static const double e0[N_PLANETS] = { 0.20563661, 0.00676399, 0.01671123, 0.09336511, 0.04838624, 0.05386179 };
static const double edot[N_PLANETS] = { 0.00002123, -0.00005107, -0.00004392, 0.00009149, -0.00013253, -0.00050991 };
static const double I0[N_PLANETS] = { 7.00559432, 3.39777545, 0.00001531, 1.85181869, 1.30439695, 2.48599187 };
static const double Idot[N_PLANETS] = { -0.00590158, -0.00078890, -0.01294668, -0.00724757, -0.00183714, 0.00193609 };
static const double L0[N_PLANETS] = { 252.25166724, 181.97970850, 100.46457166, -4.56813164, 34.39644051, 49.95424423 };
static const double Ldot[N_PLANETS] = { 149472.67486623, 58517.81560260, 35999.37244981, 19140.29934243, 3034.74612775, 1222.49362201 };
static const double wbar0[N_PLANETS] = { 77.45771895, 131.76755713, 102.93768193, -23.91744784, 14.72847983, 92.59887831 };
static const double wbardot[N_PLANETS] = { 0.15940013, 0.05679648, 0.32327364, 0.45223625, 0.21252668, -0.41897216 };
static const double O0[N_PLANETS] = { 48.33961819, 76.67261496, 0.00000000, 49.71320984, 100.47390909, 113.66242448 };
static const double Odot[N_PLANETS] = { -0.12214182, -0.27274174, 0.00000000, -0.26852431, 0.20469106, -0.28867794 };


static const double D_phys_km[N_PLANETS] = {
  4879.4, 12104.0, 12756.3, 6792.4, 142984.0, 120536.0
};


// Normalizza un angolo in gradi nell'intervallo [0,360)
double normalizeDeg(double x) {
  double y = fmod(x, 360.0);
  if (y < 0) y += 360.0;
  return y;
}

char *StrdegToDMS(double x) {
  static char buff[24];
  int deg = (int)floor(x);
  double rem = (x - deg) * 60.0;
  int min = (int)floor(rem);
  double sec = (rem - min) * 60.0;
  sprintf(buff, "%4d\260 %2.2d\264 %04.1f\250", deg, min, sec);
  return buff;
}
// Converte ore decimali in (ore, minuti, secondi)
char *StrhourToHMS(double x) {
  static char buff[24];
  int h = (int)floor(x);
  double rem = (x - h) * 60.0;
  int m = (int)floor(rem);
  double s = (rem - m) * 60.0;
  sprintf(buff, "%2.2d:%2.2d:%04.1f", h, m, s);
  return buff;
}

double time2jd(time_t t) {
  return (double) t / 86400.0 + 2440587.5;
}

time_t jd2time(double jd) {
  return round((jd - 2440587.5) * 86400.0);
}

// Calcola il tempo giuliano (Julian Day) per una data UTC
// year, month, day: parte intera; hour: ora decimale (es. 18.75 per le 18:45)
double julianDay(int year, int month, int day, double hour) {
  if (month <= 2) {
    year -= 1;
    month += 12;
  }
  int A = year / 100;
  int B = 2 - A + (A / 4);
  double JD0 = std::floor(365.25 * (year + 4716))
               + std::floor(30.6001 * (month + 1))
               + day + B - 1524.5;
  return JD0 + hour / 24.0;
}

void JD_to_DateTime(double jd, int &year, int &month, int &day, int &hour, int &minute, double &second) {
  // Aggiungiamo 0.5 per la conversione
  double J = jd + 0.5;
  long Z = (long)(floor(J));
  double F = J - Z;

  long A;
  // Controllo per calendario gregoriano vs. giuliano:
  if (Z < 2299161) {
    A = Z;
  } else {
    long alpha = (long)(floor((Z - 1867216.25) / 36524.25));
    A = Z + 1 + alpha - (long)(floor(alpha / 4.0));
  }

  long B = A + 1524;
  long C = (long)(floor((B - 122.1) / 365.25));
  long D = (long)(floor(365.25 * C));
  long E = (long)(floor((B - D) / 30.6001));

  // Giorno con parte frazionaria
  double day_decimal = B - D - static_cast<int>(floor(30.6001 * E)) + F;
  day = (int)(floor(day_decimal));

  // Estrazione di ore, minuti e secondi dalla parte frazionaria del giorno:
  double frac_day = day_decimal - day;
  double totalSeconds = frac_day * 86400;  // 24*3600
  hour = (int)(totalSeconds / 3600);
  totalSeconds -= hour * 3600;
  minute = (int)(totalSeconds / 60);
  second = totalSeconds - minute * 60;

  // Calcolo del mese
  if (E < 14)
    month = E - 1;
  else
    month = E - 13;

  // Calcolo dell'anno
  if (month > 2)
    year = C - 4716;
  else
    year = C - 4715;
}

char *jd2str(double JD, bool olHour) {
  int year, month, day, hour, minute;
  double second;
  static char buff[28];
  JD_to_DateTime(JD, year, month, day, hour, minute, second);
  //sprintf(buff,"%4.4d/%2.2d/%2.2d - %2.2d:%2.2d:%02.0f",year, month, day, hour, minute,second);
  if (second > 30) JD_to_DateTime(JD + 30.0 / 86400.0, year, month, day, hour, minute, second);
  if (olHour) sprintf(buff, "%2.2d:%2.2d", hour, minute);
  else sprintf(buff, "%4.4d/%2.2d/%2.2d-%2.2d:%2.2d", year, month, day, hour, minute);
  return buff;
}

void computeSunDistDiam(double JD, CelestialPosition &pos) {
  /* Tempo in secoli juliani dal J2000.0 */
  double T = (JD - 2451545.0) / 36525.0;

  /* Orbital elements for Earth's orbit (Sun as seen from Earth) */
  double a = 1.000001018; /* semi-major axis in AU */
  double e = 0.01670863 - 0.000042037 * T - 0.0000001267 * T * T;

  /* Mean anomaly (deg) and convert to rad */
  double M = 357.52911 + 35999.05029 * T - 0.0001537 * T * T - 0.00000048 * T * T * T;
  M = fmod(M, 360.0);
  if (M < 0) M += 360.0;
  double M_rad = M * DEG2RAD;

  /* Solve Kepler's equation E - e*sinE = M for eccentric anomaly E */
  double E = M_rad;
  for (int i = 0; i < 10; i++) {
    double dE = (E - e * sin(E) - M_rad) / (1 - e * cos(E));
    E -= dE;
    if (fabs(dE) < 1e-8)
      break;
  }
  /* Radius vector (AU) */
  double r_AU = a * (1 - e * cos(E));
  double r_km = r_AU * AU_KM;
  pos.dist = r_AU;
  pos.diam = (206265.0 * (SOLAR_DIAM_KM / r_km)) / 60.;
}

void sunEquatorial(double JD, double &RA_hours, double &dec_deg, double &lambda) {
  // Tempo in secoli giuliani da J2000.0
  double T = (JD - J2000) / 36525.0;
  double T2 = T * T;
  double T3 = T2 * T;

  // 1) Parametri orbitali
  // Longitudine media del Sole (L0)
  double L0 = normalizeDeg(280.46646 + 36000.76983 * T + 0.0003032 * T2);
  // Anomalia media (M)
  double M = normalizeDeg(357.52911 + 35999.05029 * T - 0.0001537 * T2);

  // Correzione del raggio vettore (C)
  double Mrad = M * DEG2RAD;
  double C = (1.914602 - 0.004817 * T - 0.000014 * T2) * sin(Mrad)
             + (0.019993 - 0.000101 * T) * sin(2 * Mrad)
             + 0.000289 * sin(3 * Mrad);

  // Longitudine vera del Sole
  double trueLong = L0 + C;

  // Correzione dovuta alla nutazione
  double omega = normalizeDeg(125.04 - 1934.136 * T);
  lambda = trueLong - 0.00569 - 0.00478 * sin(omega * DEG2RAD);

  // Obliquità dell'eclittica
  double eps0 = 23.0 + (26.0 + ((21.448 - T * (46.815 + T * (0.00059 - T * 0.001813))) / 60.0)) / 60.0;
  double eps = eps0 + 0.00256 * cos(omega * DEG2RAD);

  // 2) Conversione su coordinate equatoriali
  double lamRad = lambda * DEG2RAD;
  double epsRad = eps * DEG2RAD;

  double sinDec = sin(epsRad) * sin(lamRad);
  double decRad = asin(sinDec);
  double y = cos(epsRad) * sin(lamRad);
  double x = cos(lamRad);
  double raRad = atan2(y, x);

  // Normalizza RA in [0,2π)
  if (raRad < 0) raRad += 2 * M_PI;

  // Output
  RA_hours = raRad * RAD2DEG / 15.0;  // da gradi a ore (360° = 24h)
  dec_deg = decRad * RAD2DEG;         // in gradi
}




/*-------------------------------------------------
  2) Calcolo del Greenwich Mean Sidereal Time (GMST)
     in gradi, per un dato JD
     (algoritmo IAU 1982 / NOAA)
  -------------------------------------------------*/
static double gmst_deg(double JD) {
  double T = (JD - 2451545.0) / 36525.0;
  double T2 = T * T;
  /* GMST in gradi */
  double gmst = 280.46061837
                + 360.98564736629 * (JD - 2451545.0)
                + 0.000387933 * T2
                - T2 * T / 38710000.0;
  return normalizeDeg(gmst);
}

void equatorialToHorizontal(double JD, Observer &obs, double RA_hours, double dec_deg, double &az_dec, double &el_dec) {
  /* 1) GMST */
  double gmst = gmst_deg(JD);

  /* 2) Local Sidereal Time */
  double lst_deg = normalizeDeg(gmst + obs.lon);

  /* 3) Hour Angle H = LST − RA */
  double HA_deg = normalizeDeg(lst_deg - (RA_hours * 15.0));

  /* 4) trasformazioni in radianti */
  double HA = HA_deg * DEG2RAD;
  double dec = dec_deg * DEG2RAD;
  double lat = obs.lat * DEG2RAD;

  //    double Pi    = 0;
  //  	double dRA   = 0;
  //  	double dDec  = 0;
  //  	if (dist>0) {
  //  		if (dist<20) dist*=AU_KM;
  //     	Pi    = asin(ReEarth /dist);          // asin(Re/Δ)
  //     	dRA   = -Pi * cos(lat) * sin(HA) / cos(dec);
  //     	dDec  = -Pi * ( sin(lat)*cos(dec)*cos(HA)- cos(lat)*sin(dec) )/ cos(dec);
  //  	}
  //  	HA+=dRA;
  // 	dec+=dDec;
  /* 5) elevazione */
  double sin_el = sin(lat) * sin(dec) + cos(lat) * cos(dec) * cos(HA);
  el_dec = asin(sin_el) * RAD2DEG;

  /* Azimut (da Nord verso Est) */
  double y = sin(HA);
  double x = cos(HA) * sin(lat) - tan(dec) * cos(lat);
  double az = atan2(y, x);
  /* shift +180° per avere 0=NORD, poi normalizzo */
  az_dec = normalizeDeg(az * RAD2DEG + 180.0);
}


void moonEquatorial(double JD, Observer &obs, CelestialPosition &pos, bool AllCalc = true)

{
  /* 1) Tempo in secoli giuliani da J2000.0 */
  double T = (JD - 2451545.0) / 36525.0;
  double T2 = T * T;

  /* 2) Elementi orbitali medi (Meeus 47.A) */
  double Lp = normalizeDeg(
    218.3164477 + 481267.88123421 * T
    - 0.0015786 * T2
    + T2 * T / 538841.0
    - T2 * T2 / 65194000.0); /* longitudine media Luna */

  double D = normalizeDeg(
    297.8501921 + 445267.1114034 * T
    - 0.0018819 * T2
    + T2 * T / 545868.0
    - T2 * T2 / 113065000.0); /* elongazione media Luna‑Sole */

  double M = normalizeDeg(
    357.5291092 + 35999.0502909 * T
    - 0.0001536 * T2
    + T2 * T / 24490000.0); /* anomalia media del Sole */

  double Mp = normalizeDeg(
    134.9633964 + 477198.8675055 * T
    + 0.0087414 * T2
    + T2 * T / 69699.0
    - T2 * T2 / 14712000.0); /* anomalia media della Luna */

  double F = normalizeDeg(
    93.2720950 + 483202.0175233 * T
    - 0.0036539 * T2
    - T2 * T / 3526000.0
    + T2 * T2 / 863310000.0); /* argomento di latitudine */

  /* 3) Somme periodiche (solo i termini principali, per brevità) */
  /*   Termini per longitudine (×10^(-6)°) */
  //static const int Nlon = 10;
  static const int Nlon = 20;
  static const struct {
    int D, M, Mp, F;
    double coeff;
  } LonTerms[Nlon] = {
    { 0, 0, 1, 0, 6288774 }, { 2, 0, -1, 0, 1274027 }, { 2, 0, 0, 0, 658314 }, { 0, 0, 2, 0, 213618 }, { 0, 1, 0, 0, -185116 }, { 0, 0, 0, 2, -114332 }, { 2, 0, -2, 0, 58793 }, { 2, -1, 1, 0, 57066 }, { 2, 0, 1, 0, 53322 }, { 2, -1, 0, 0, 45758 }, { 0, 1, -1, 0, -40923 }, { 1, 0, 0, 0, -34720 }, { 0, 1, 1, 0, -30383 }, { 2, 0, 0, -1, 15327 }, { 0, 0, 1, 2, -12528 }, { 0, 0, 1, -2, 10980 }, { 4, 0, -1, 0, 10675 }, { 0, 0, 3, 0, 10034 }, { 4, 0, -2, 0, 8548 }, { 2, 1, -1, 0, -7888 }
  };

  double sumLon = 0.0;
  for (int i = 0; i < Nlon; i++) {
    double arg = LonTerms[i].D * D + LonTerms[i].M * M
                 + LonTerms[i].Mp * Mp + LonTerms[i].F * F;
    sumLon += LonTerms[i].coeff * sin(arg * DEG2RAD);
  }

  /*   Termini per latitudine (×10^(-6)°) */
  //static const int Nlat = 7;
  static const int Nlat = 15;
  static const struct {
    int D, M, Mp, F;
    double coeff;
  } LatTerms[Nlat] = {
    { 0, 0, 0, 1, 5128122 }, { 0, 0, 1, 1, 280602 }, { 0, 0, 1, -1, 277693 }, { 2, 0, -1, 1, 173237 }, { 2, 0, 0, -1, 55413 }, { 2, 0, -1, -1, 46271 }, { 2, 1, 0, 1, 32573 }, { 0, 0, 3, 1, 17198 }, { 2, 0, 1, 1, 9266 }, { 2, 0, 1, -1, 8822 }, { 2, -1, 1, 1, 8216 }, { 2, 0, -2, -1, 4324 }, { 0, 0, 2, -1, 4200 }, { 2, 0, 0, 1, -3359 }, { 2, 0, -3, 1, 2463 }
  };

  double sumLat = 0.0;
  for (int i = 0; i < Nlat; i++) {
    double arg = LatTerms[i].D * D + LatTerms[i].M * M
                 + LatTerms[i].Mp * Mp + LatTerms[i].F * F;
    sumLat += LatTerms[i].coeff * sin(arg * DEG2RAD);
  }

  /*   Termini per distanza (km) */
  static const int Ndist = 10;
  static const struct {
    int D, M, Mp, F;
    double coeff;
  } DistTerms[Ndist] = {
    { 0, 0, 1, 0, -20905355 },
    { 2, 0, -1, 0, -3699111 },
    { 2, 0, 0, 0, -2955968 },
    { 0, 0, 2, 0, -569925 },
    { 0, 1, 0, 0, 48888 },
    { 0, 0, 0, 2, -3149 },
    { 2, 0, -2, 0, 246158 },
    { 2, -1, -1, 0, -152138 },
    { 2, 0, 1, 0, -170733 },
    { 2, -1, 0, 0, -204586 }
  };
  double sumDist = 0.0;
  for (int i = 0; i < Ndist; i++) {
    double arg = DistTerms[i].D * D + DistTerms[i].M * M
                 + DistTerms[i].Mp * Mp + DistTerms[i].F * F;
    sumDist += DistTerms[i].coeff * cos(arg * DEG2RAD);
  }
  sumDist /= 1000.;
  /* 4) Eclittiche geocentriche della Luna */
  double lambda_moon = Lp + sumLon / 1e6; /* deg */
  double beta_moon = sumLat / 1e6;        /* deg */



  /* 5) Converti in coordinate equatoriali */
  /* obliquità media dell’eclittica */
  double eps = (23.0 + 26.0 / 60.0 + 21.448 / 3600.0)
               - (46.8150 * T + 0.00059 * T2 - 0.001813 * T * T2) / 3600.0;
  double epsRad = eps * DEG2RAD;

  double lamRad = lambda_moon * DEG2RAD;
  double betRad = beta_moon * DEG2RAD;

  double x = cos(betRad) * cos(lamRad);
  double y = cos(betRad) * sin(lamRad) * cos(epsRad) - sin(betRad) * sin(epsRad);
  double z = cos(betRad) * sin(lamRad) * sin(epsRad) + sin(betRad) * cos(epsRad);

  double RAm = atan2(y, x);
  if (RAm < 0) RAm += 2 * M_PI;
  double DecM = atan2(z, sqrt(x * x + y * y));

  /* lat_obs, lon_obs in radianti, h_obs in km sopra il geoide (≈0) */
  //double R_e = 6378.137;    /* km, raggio equatoriale terrestre :contentReference[oaicite:1]{index=1} */
  double x_obs = (ReEarth + obs.elev / 1000.) * cos(obs.lat * DEG2RAD) * cos(obs.lon * DEG2RAD);
  double y_obs = (ReEarth + obs.elev / 1000.) * cos(obs.lat * DEG2RAD) * sin(obs.lon * DEG2RAD);
  double z_obs = (ReEarth + obs.elev / 1000.) * sin(obs.lat * DEG2RAD);

  double cosb = cos(betRad), sinb = sin(betRad);
  double cosl = cos(lamRad), sinl = sin(lamRad);
  double Xe = (385000.56 + sumDist) * cosb * cosl;
  double Ye = (385000.56 + sumDist) * cosb * sinl;
  double Ze = (385000.56 + sumDist) * sinb;

  Xe -= x_obs;
  Ye -= y_obs;
  Ze -= z_obs;

  pos.dist = sqrt(Xe * Xe + Ye * Ye + Ze * Ze); /* km */


  double LST_deg = normalizeDeg(gmst_deg(JD) + obs.lon);
  double phi = obs.lat * DEG2RAD;      // latitudine osservatore
  double delta = DecM;                 // declinazione geocentrica
  double H = LST_deg * DEG2RAD - RAm;  // angolo orario in rad

  // parallasse orizzontale (rad)
  //double Re    = 6378.137;                        // raggio equatoriale Terra [km]
  double Pi = asin(ReEarth / pos.dist);  // asin(Re/Δ)

  // correzione RA (rad) e Dec (rad)
  double dRA = -Pi * cos(phi) * sin(H) / cos(delta);
  double dDec = -Pi * (sin(phi) * cos(delta) * cos(H) - cos(phi) * sin(delta)) / cos(delta);
  //dRA=dDec=0;
  pos.RA = ((RAm + dRA) * RAD2DEG) / 15.0; /* ore */
  pos.dec = (DecM + dDec) * RAD2DEG;       /* deg */

  if (AllCalc == false) return;

  double dec_sun_deg;
  double lambda_sun_deg;
  double RA_sun_hours;
  CelestialPosition pos_sun;
  sunEquatorial(JD, RA_sun_hours, dec_sun_deg, lambda_sun_deg);
  computeSunDistDiam(JD, pos_sun);
  /* 6) Fase, età, illuminazione */


  double xs = pos_sun.dist * AU_KM * cos(dec_sun_deg * DEG2RAD) * cos(RA_sun_hours * 15 * DEG2RAD);
  double ys = pos_sun.dist * AU_KM * cos(dec_sun_deg * DEG2RAD) * sin(RA_sun_hours * 15 * DEG2RAD);
  double zs = pos_sun.dist * AU_KM * sin(dec_sun_deg * DEG2RAD);

  double xm = pos.dist * cos(pos.dec * DEG2RAD) * cos(pos.RA * 15 * DEG2RAD);
  double ym = pos.dist * cos(pos.dec * DEG2RAD) * sin(pos.RA * 15 * DEG2RAD);
  double zm = pos.dist * sin(pos.dec * DEG2RAD);

  /* 2) vettore dalla Luna al Sole: rs_m = (xs-xm, ys-ym, zs-zm) */
  double rx = xs - xm;
  double ry = ys - ym;
  double rz = zs - zm;

  /* 3) vettore dalla Luna alla Terra: re_m = - (xm,ym,zm) */
  double ex = -xm;
  double ey = -ym;
  double ez = -zm;

  /* moduli */
  double d_rs_m = sqrt(rx * rx + ry * ry + rz * rz);
  double d_re_m = sqrt(ex * ex + ey * ey + ez * ez);

  /* 4) cos φ = (rs_m · re_m) / (|rs_m|·|re_m|) */
  double cos_phi = (rx * ex + ry * ey + rz * ez) / (d_rs_m * d_re_m);
  if (cos_phi > 1.0) cos_phi = 1.0;
  if (cos_phi < -1.0) cos_phi = -1.0;

  //*phase_angle = acos(cos_phi);


  pos.phase = (1.0 + cos_phi) / 2;

  /* 6) magnitudine */
  /* converti φ in gradi */


  double phi_deg = acos(cos_phi) * RAD2DEG;

  /* distanza in raggi terrestri */
  //double R_earth = 6378.0;
  double D_mean = 384400.0;
  double delta_rearth = d_re_m / D_mean;
  pos.magnitude = -12.73
                  + 5.0 * log10(delta_rearth)
                  + 0.026 * phi_deg
                  + 4e-9 * pow(phi_deg, 4);


  //    double xs = AU_KM * cos(lambda_sun_deg*DEG2RAD);
  //    double ys = AU_KM * sin(lambda_sun_deg*DEG2RAD);
  //    double dot = Xe*xs + Ye*ys;
  //    double cos_phi = dot/(pos.dist * AU_KM);
  //    if(cos_phi>1) cos_phi=1; if(cos_phi<-1) cos_phi=-1;
  //
  ////    double cos_phi = sin(dec_sun_deg*DEG2RAD)*sin(pos.dec*DEG2RAD)
  ////                	+ cos(dec_sun_deg*DEG2RAD)*cos(pos.dec*DEG2RAD)
  ////                  	* cos((RA_sun_hours - pos.RA)*15*DEG2RAD);
  //    if(cos_phi>1) cos_phi=1; if(cos_phi<-1) cos_phi=-1;
  //
  //    cos_phi=-cos_phi;
  //    double phase_ang = acos(cos_phi);
  //    pos.phase = (1 + cos_phi)/2;
  //    //pos.magnitude = -12.73 + 0.026*(phase_ang *RAD2DEG) + 4e-9*pow(phase_ang *RAD2DEG,4)-2.5*log10(pos.phase<0.012 ? 0.0012 : pos.phase);
  //    //pos.magnitude = -12.73 + 0.026*(acos(pos.phase) *RAD2DEG) - 4e-9*pow(acos(pos.phase) *RAD2DEG ,4); //-2.5*log10(pos.phase<0.012 ? 0.0012 : pos.phase);
  //    pos.magnitude = -12.73 + 0.026*(phase_ang *RAD2DEG) - 0.0000001*pow(phase_ang *RAD2DEG,4);


  double elong = normalizeDeg(lambda_moon - lambda_sun_deg);
  pos.age = LUNAR_CYCLE_DAYS * (elong / 360.0);

  pos.diam = 206265.0 * (MOON_DIAM_KM / pos.dist) / 60.;

  double num = cos(dec_sun_deg * DEG2RAD) * sin((RA_sun_hours - pos.RA) * 15.0 * DEG2RAD);
  double den = sin(dec_sun_deg * DEG2RAD) * cos(pos.dec * DEG2RAD)
               - cos(dec_sun_deg * DEG2RAD) * sin(pos.dec * DEG2RAD) * cos((RA_sun_hours - pos.RA) * 15.0 * DEG2RAD);
  pos.moonlimb = atan2(num, den) * RAD2DEG;
  if (pos.moonlimb < 0) pos.moonlimb += 360;
}

char *strMoonPhase(double lunarAge) {
  static char moonPhase[24];
  if (lunarAge >= 0 && lunarAge <= LUNAR_CYCLE_DAYS
      && (lunarAge < 1 || lunarAge > LUNAR_CYCLE_DAYS - 1)) {
    strcpy(moonPhase, "New Moon");
  } else if (lunarAge >= 1 && lunarAge < 6.4) {
    strcpy(moonPhase, "Waxing Crescent");
  } else if (lunarAge >= 6.4 && lunarAge < 8.4) {
    strcpy(moonPhase, "First Quarter");
  } else if (lunarAge >= 8.4 && lunarAge < 13.8) {
    strcpy(moonPhase, "Waxing Gibbous");
  } else if (lunarAge >= 13.8 && lunarAge < 15.8) {
    strcpy(moonPhase, "Full Moon");
  } else if (lunarAge >= 15.8 && lunarAge < 21.1) {
    strcpy(moonPhase, "Waning Gibbous");
  } else if (lunarAge >= 21.1 && lunarAge < 23.1) {
    strcpy(moonPhase, "Last/Third Quarter");
  } else if (lunarAge >= 23.1 && lunarAge <= LUNAR_CYCLE_DAYS - 1) {
    strcpy(moonPhase, "Waning Crescent");
  }
  return moonPhase;
}





/* Elicentriche eclittiche: risolve Keplero e ruota nel piano */
static void heliocentric_ecl(int planet, double JD, double &x, double &y, double &z) {
  double T = (JD - 2451545.0) / 36525.0;
  /* aggiorna elementi */
  double a = a0[planet] + adot[planet] * T;
  double e = e0[planet] + edot[planet] * T;
  double I = normalizeDeg(I0[planet] + Idot[planet] * T) * DEG2RAD;
  double L = normalizeDeg(L0[planet] + Ldot[planet] * T);
  double wbar = normalizeDeg(wbar0[planet] + wbardot[planet] * T);
  double Omega = normalizeDeg(O0[planet] + Odot[planet] * T) * DEG2RAD;
  /* anomalia media */
  double M = normalizeDeg(L - wbar) * DEG2RAD;
  /* soluzione di Keplero via Newton */
  double E = M, delta;
  for (int i = 0; i < 100; i++) {
    delta = (E - e * sin(E) - M) / (1 - e * cos(E));
    E -= delta;
    if (fabs(delta) < 1e-6) break;
  }
  /* vero anomalia e r */
  double nu = 2 * atan2(sqrt(1 + e) * sin(E / 2), sqrt(1 - e) * cos(E / 2));
  double r = a * (1 - e * cos(E));
  /* coordinate nel piano orbitale */
  double arg = nu + (wbar * DEG2RAD - Omega);  // ω = wbar - Ω in radianti
  double cosarg = cos(arg), sinarg = sin(arg), cosI = cos(I), sinI = sin(I);
  /* rotazioni: Ω intorno a Z, inclinazione intorno a X */
  x = r * (cos(Omega) * cosarg - sin(Omega) * sinarg * cosI);
  y = r * (sin(Omega) * cosarg + cos(Omega) * sinarg * cosI);
  z = r * (sinarg * sinI);
}



/* Generic perturbation applier for Jupiter and Saturn */
double apply_perturbations(int p, double JD, double lon) {
  /* compute mean anomaly of perturbing planet */
  double T = (JD - 2451545.0) / 36525.0;
  double M_pert;
  const PertTerm *terms;
  int n;
  if (p == SATURN) {
    /* perturbing planet = Jupiter */
    double LJ = normalizeDeg(L0[JUPITER] + Ldot[JUPITER] * T);
    double wJ = normalizeDeg(wbar0[JUPITER] + wbardot[JUPITER] * T);
    M_pert = (LJ - wJ) * DEG2RAD;
    terms = saturn_pert;
    n = N_SAT_PERT;
  } else if (p == JUPITER) {
    /* perturbing planet = Saturn */
    double LS = normalizeDeg(L0[SATURN] + Ldot[SATURN] * T);
    double wS = normalizeDeg(wbar0[SATURN] + wbardot[SATURN] * T);
    M_pert = (LS - wS) * DEG2RAD;
    terms = jupiter_pert;
    n = N_JUP_PERT;
  } else {
    return lon;
  }
  double corr = 0;
  for (int i = 0; i < n; i++) {
    corr += terms[i].A * sin(terms[i].k * M_pert)
            + terms[i].B * cos(terms[i].k * M_pert);
  }
  return lon + corr;
}

void planetEquatorial(int planet, double JD, Observer &obs, CelestialPosition &pos) {
  double xp, yp, zp, xe, ye, ze;
  /* eliocentriche */
  heliocentric_ecl(planet, JD, xp, yp, zp);
  heliocentric_ecl(EARTH, JD, xe, ye, ze);
  //    if(planet==SATURN) {
  //	   fixPertPlatnet(SATURN,JD,saturn_pert,N_SAT_PERT,xp,yp,zp);
  //	}
  //    if(planet==JUPITER) fixPertPlatnet(JUPITER,JD,jupiter_pert,N_JUP_PERT,xp,yp,zp);
  //

  /* geocentriche */
  double x = xp - xe, y = yp - ye, z = zp - ze;
  /* coordinate equatoriali */
  double T = (JD - 2451545.0) / 36525.0;
  double eps = (23.439291 - 0.0130042 * T - 1.64e-7 * T * T + 5.04e-7 * T * T * T) * DEG2RAD;
  //double eps = (23.439291 - 0.0130042*(JD-2451545.0)/36525.0)*DEG2RAD;

  double xecl = x, yecl = y * cos(eps) - z * sin(eps), zecl = y * sin(eps) + z * cos(eps);
  double R = sqrt(xecl * xecl + yecl * yecl);
  double RA = atan2(yecl, xecl) * RAD2DEG;
  if (RA < 0) RA += 360.0;
  double Dec = atan2(zecl, R) * RAD2DEG;
  /* distanze */
  double Delta = sqrt(x * x + y * y + z * z);      /* UA */
  double r = sqrt(xp * xp + yp * yp + zp * zp);    /* UA */
  double R_au = sqrt(xe * xe + ye * ye + ze * ze); /* UA */
  /* fase: angolo Sole−Pianeta−Terra */
  double cos_alpha = (r * r + Delta * Delta - R_au * R_au) / (2 * r * Delta);
  if (cos_alpha > 1) cos_alpha = 1;
  if (cos_alpha < -1) cos_alpha = -1;
  double alpha = acos(cos_alpha);     /* rad */
  double phase = (1 + cos_alpha) / 2; /* % */
  /* magnitudo (Meeus) */
  double alpha_deg = alpha * RAD2DEG;
  double V;
  switch (planet) {
    case MERCURY:
      V = -0.42 + 5 * log10(r * Delta)
          + 0.0380 * alpha_deg
          - 0.000273 * alpha_deg * alpha_deg
          + 0.000002 * alpha_deg * alpha_deg * alpha_deg;
      break;
    case VENUS:
      V = -4.40 + 5 * log10(r * Delta)
          + 0.0009 * alpha_deg
          + 0.000239 * alpha_deg * alpha_deg
          - 0.00000065 * alpha_deg * alpha_deg * alpha_deg;
      break;
    case MARS:
      V = -1.52 + 5 * log10(r * Delta)
          + 0.016 * alpha_deg;
      break;
    case JUPITER:
      V = -9.40 + 5 * log10(r * Delta)
          + 0.005 * alpha_deg;
      break;
    case SATURN:
      V = -8.88 + 5 * log10(r * Delta)
          + 0.044 * alpha_deg
          - 0.0004 * alpha_deg * alpha_deg;
      break;
    default:
      V = 99.9;
  }

  Dec = apply_perturbations(planet, JD, Dec);

  /* diametro apparente (arcsec) */
  double diam = 206265.0 * (D_phys_km[planet] / (Delta * AU_KM));

  double phi = obs.lat * DEG2RAD;                                    // latitudine osservatore
  double delta = Dec * DEG2RAD;                                      // declinazione geocentrica
  double H = (normalizeDeg(gmst_deg(JD) + obs.lon) + RA) * DEG2RAD;  // angolo orario in rad

  double Pi = asin(ReEarth / (Delta * AU_KM));
  double dDec = -Pi * (sin(phi) * cos(delta) * cos(H) - cos(phi) * sin(delta)) / cos(delta);
  double dRA = -Pi * cos(phi) * sin(H) / cos(delta);
  dRA = dDec = 0;
  /* output */
  pos.RA = (RA + dRA * RAD2DEG) / 15.0;
  pos.dec = Dec + dDec * RAD2DEG;
  pos.dist = Delta;
  pos.phase = phase;
  pos.magnitude = V;
  pos.diam = diam;
  pos.age = pos.moonlimb = 0;
}

void SunPosition(double jd, Observer &obs, CelestialPosition &pos) {
  double lambda;
  sunEquatorial(jd, pos.RA, pos.dec, lambda);
  equatorialToHorizontal(jd, obs, pos.RA, pos.dec, pos.az, pos.el);
  computeSunDistDiam(jd, pos);
  //pos.illumination=100;
  pos.parallacticAngle = pos.phase = pos.age = pos.moonlimb = 0;
  pos.magnitude = -26.73;
}

#define EPS 1e-12

void MoonPosition(double jd, Observer &obs, CelestialPosition &pos) {
  moonEquatorial(jd, obs, pos);
  equatorialToHorizontal(jd, obs, pos.RA, pos.dec, pos.az, pos.el);
  //  calcola Angolo Parallattico
  if (cos(obs.lat * DEG2RAD) < EPS || cos(pos.el * DEG2RAD) < EPS) pos.parallacticAngle = 0;
  else {
    /* 3) Hour Angle H = LST − RA */
    double H = normalizeDeg(normalizeDeg(gmst_deg(jd) + obs.lon) - (pos.RA * 15.0));
    double num = cos(obs.lat * DEG2RAD) * sin(H * DEG2RAD);
    double den = sin(obs.lat * DEG2RAD) * cos(pos.dec * DEG2RAD) - cos(obs.lat * DEG2RAD) * sin(pos.dec * DEG2RAD) * cos(H * DEG2RAD);
    //double num = cos(obs.lat * DEG2RAD) * sin(pos.az * DEG2RAD);
    //double den = sin(obs.lat * DEG2RAD) * cos(pos.el * DEG2RAD) - cos(obs.lat * DEG2RAD) * sin(pos.el * DEG2RAD) * cos(pos.az * DEG2RAD);
    //double num=sin(pos.az*DEG2RAD);
    //double den=tan(obs.lat*DEG2RAD)*cos(pos.el*DEG2RAD)-sin(pos.el*DEG2RAD)*cos(pos.az*DEG2RAD);
    pos.parallacticAngle = atan2(num, den) * RAD2DEG;
  }
}

void MercuryPosition(double jd, Observer &obs, CelestialPosition &pos) {
  planetEquatorial(MERCURY, jd, obs, pos);
  equatorialToHorizontal(jd, obs, pos.RA, pos.dec, pos.az, pos.el);
  pos.parallacticAngle = 0;
}

void VenusPosition(double jd, Observer &obs, CelestialPosition &pos) {
  planetEquatorial(VENUS, jd, obs, pos);
  equatorialToHorizontal(jd, obs, pos.RA, pos.dec, pos.az, pos.el);
  pos.parallacticAngle = 0;
}

void MarsPosition(double jd, Observer &obs, CelestialPosition &pos) {
  planetEquatorial(MARS, jd, obs, pos);
  equatorialToHorizontal(jd, obs, pos.RA, pos.dec, pos.az, pos.el);
  pos.parallacticAngle = 0;
}

void JupiterPosition(double jd, Observer &obs, CelestialPosition &pos) {
  planetEquatorial(JUPITER, jd, obs, pos);
  equatorialToHorizontal(jd, obs, pos.RA, pos.dec, pos.az, pos.el);
  pos.parallacticAngle = 0;
}

void SaturnPosition(double jd, Observer &obs, CelestialPosition &pos) {
  planetEquatorial(SATURN, jd, obs, pos);
  equatorialToHorizontal(jd, obs, pos.RA, pos.dec, pos.az, pos.el);
  pos.parallacticAngle = 0;
}


void solarCoord(double jd, Observer &obs, EphemData &et) {
  double lambda;
  sunEquatorial(jd, et.RA_hours, et.dec_deg, lambda);
  equatorialToHorizontal(jd, obs, et.RA_hours, et.dec_deg, et.az_deg, et.el_deg);
}

void moonCoord(double jd, Observer &obs, EphemData &et) {
  CelestialPosition pos;
  moonEquatorial(jd, obs, pos, false);
  equatorialToHorizontal(jd, obs, pos.RA, pos.dec, et.az_deg, et.el_deg);
  et.RA_hours = pos.RA;
  et.dec_deg = pos.dec;
}

void mercuryCoord(double jd, Observer &obs, EphemData &et) {
  CelestialPosition pos;
  planetEquatorial(MERCURY, jd, obs, pos);
  equatorialToHorizontal(jd, obs, pos.RA, pos.dec, et.az_deg, et.el_deg);
  et.RA_hours = pos.RA;
  et.dec_deg = pos.dec;
}

void venusCoord(double jd, Observer &obs, EphemData &et) {
  CelestialPosition pos;
  planetEquatorial(VENUS, jd, obs, pos);
  equatorialToHorizontal(jd, obs, pos.RA, pos.dec, et.az_deg, et.el_deg);
  et.RA_hours = pos.RA;
  et.dec_deg = pos.dec;
}

void marsCoord(double jd, Observer &obs, EphemData &et) {
  CelestialPosition pos;
  planetEquatorial(MARS, jd, obs, pos);
  equatorialToHorizontal(jd, obs, pos.RA, pos.dec, et.az_deg, et.el_deg);
  et.RA_hours = pos.RA;
  et.dec_deg = pos.dec;
}

void jupiterCoord(double jd, Observer &obs, EphemData &et) {
  CelestialPosition pos;
  planetEquatorial(JUPITER, jd, obs, pos);
  equatorialToHorizontal(jd, obs, pos.RA, pos.dec, et.az_deg, et.el_deg);
  et.RA_hours = pos.RA;
  et.dec_deg = pos.dec;
}

void saturnCoord(double jd, Observer &obs, EphemData &et) {
  CelestialPosition pos;
  planetEquatorial(SATURN, jd, obs, pos);
  equatorialToHorizontal(jd, obs, pos.RA, pos.dec, et.az_deg, et.el_deg);
  et.RA_hours = pos.RA;
  et.dec_deg = pos.dec;
}


/*-------------------------------------------------
  Calcolo alba/meridian transit/tramonto
  -------------------------------------------------*/



int computeRiseSet(double jd, EphemCallback cb, Observer &obs, const double horizon, RiseSet &out) {
  

  //double Jnoon = (int)floor(jd + 0.5) - 0.5;
  //double Jnoon = floor(jd + 0.5); 
  
  //Serial.printf("Ora\njd %s\n",jd2str(jd));
  //Serial.printf("jnoon %s\n",jd2str(Jnoon));

 
  // //    GMST a mezzogiorno
  // double gmst0 = gmst_deg(Jnoon);
  // //    callback per RA/Dec a mezzogiorno
  // EphemData e0;
  // cb(Jnoon, obs, e0);
  // //    Local Sidereal Time a mezzogiorno
  // double LST0 = normalizeDeg(gmst0 + obs.lon);
  // //    Transit guess: H = LST - RA*15 → H=0 → JDt0
  // double RA_deg = e0.RA_hours * 15.0;
  // double dLST = normalizeDeg(RA_deg - LST0);
  // double Jt = Jnoon + dLST / 360.0;  // in giorni
  // //printf("Jt %s\n", jd2str(Jt));
  
  //    GMST 
  double gmst0 = gmst_deg(jd);
  //    callback per RA/Dec
  EphemData e0;
  cb(jd, obs, e0);
  //    Local Sidereal Time 
  double LST0 = normalizeDeg(gmst0 + obs.lon);
  //    Transit guess: H = LST - RA*15 → H=0 → JDt0
  double RA_deg = e0.RA_hours * 15.0;
  //double dLST = normalizeDeg(RA_deg - LST0);
  double dLST = fmod(RA_deg - LST0 + 540.0, 360.0) - 180.0;
  double Jt = jd + dLST / 360.0;  // in giorni
  //printf("Jt %s\n", jd2str(Jt));



  //Serial.printf("Jt %s\n",jd2str(Jt));

  double j1 = Jt - 0.5;
  double j2 = Jt + 0.5;
  double x1 = j1, x2 = j2;
  const double ph = 0.6;
  //printf("---- %s ", jd2str(j1)); printf("%s diff=%f \n",jd2str(j2),j2-j1);
  for (int i = 0; i < 500; i++) {
    EphemData e1, e2;
    cb(x1, obs, e1);
    cb(x2, obs, e2);
    if (e1.el_deg > e2.el_deg) j2 = x2;
    else j1 = x1;
    //printf("%d %s ", i, jd2str(j1)); printf("%s diff=%f \n",jd2str(j2),j2-j1);
    x1 = j2 - ph * (j2 - j1);
    x2 = j1 + ph * (j2 - j1);
    out.Err[2] = j2 - j1;
    if (j2 - j1 < MaxErr) break;
  }
  out.transit = (j2 + j1) / 2;
  cb(out.transit, obs, e0);
  out.maxEl=e0.el_deg;
  if (out.transit>jd) {
    j1=out.transit;
    j2=out.transit+0.5;
  } else {
    j1=out.transit-0.5;
    j2=out.transit;
  }
  x1 = j1;
  x2 = j2; 
  for (int i = 0; i < 500; i++) {
    EphemData e1, e2;
    cb(x1, obs, e1);
    cb(x2, obs, e2);
    if (e1.el_deg < e2.el_deg) j2 = x2;
    else j1 = x1;
    //printf("%d %s ", i, jd2str(j1)); printf("%s diff=%f \n",jd2str(j2),j2-j1);
    x1 = j2 - ph * (j2 - j1);
    x2 = j1 + ph * (j2 - j1);
    if (j2 - j1 < MaxErr) break;
  }
  cb((j2 + j1) / 2, obs, e0);
  out.minEl=e0.el_deg;

  // 4) Angular depression of horizon (dip) [deg]
  //    dip_rad = arccos(R/(R+h)) → elev_horizon = -dip
  const double Re = 6371000.0;
  double dip_rad = acos(Re / (Re + obs.elev));
  double dip_deg = dip_rad * RAD2DEG;
  //    add refraction ~0.5667°
  //double h0 = -(dip_deg + 0.5667);
  double h0 = -(dip_deg - horizon);

  // 5) Compute H0 for rise/set: cos H0 = (sin h0 - sinφ sinδ)/(cosφ cosδ)
  EphemData et;
  //double jd_tr = out.transit ;
  cb(out.transit, obs, et);
  double phi = obs.lat * DEG2RAD;
  double dec = et.dec_deg * DEG2RAD;
  double cosH0 = (sin(h0 * DEG2RAD) - sin(phi) * sin(dec))
                 / (cos(phi) * cos(dec));

  //printf("Cosh %f phi %f dec %f h0 %f\n",cosH0,phi,dec,h0);

  if (cosH0 > 1.0) {
    // Sempre sotto l'orizzonte
    out.rise = 0;
    out.set = 0;
    out.polar = -1;
    return -1;
  }
  if (cosH0 < -1.0) {
    // Sempre sopra l'orizzonte
    out.rise = 0;
    out.set = 0;
    out.polar = 1;
    return 1;
  }

  for (int l = 0; l < 2; l++) {
    double step, m;
    double j = out.transit;
    m = (l == 0 ? -1 : 1);
    step = m * 30. / 1440.;
    for (int i = 0; i < 1000; i++) {
      cb(j, obs, et);
      //double diff=et.el_deg-h0;
      double diff = (et.el_deg - h0) * m;
      if (diff > 0 && step < 0) step = -step / 2.;
      if (diff < 0 && step > 0) step = -step / 2.;
      //printf("%d %d %s %f %f %f \n",i,l, jd2str(j), et.el_deg, step, et.el_deg-h0);
      if (fabs(et.el_deg - h0) < MaxErr) break;
      j += step;
    }
    if (l) out.set = j;
    else out.rise = j;
    out.Err[l] = et.el_deg - h0;
    //printf("Err %f\n",et.el_deg-h0);
  }
  out.polar = 0;
  return 0;
}


int planetRiseSet(double jd, EphemCallback cb, Observer &obs, const double horizon, RiseSet &Twilight) {
  int ret = 0;
  double ors, ort;
  if ((ret = computeRiseSet(jd, cb, obs, horizon, Twilight)) != 0) return ret;
  //	if (Twilight.set<Twilight.rise) {
  //		ors=Twilight.rise; ort=Twilight.transit;
  //		if ((ret=computeRiseSet(jd,cb,obs, horizon,Twilight))!=0) return ret;
  //		Twilight.transit=(Twilight.transit>Twilight.set ? ort : Twilight.transit);
  //		Twilight.rise=ors;
  //    }
  return ret;
}

int SolarRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight) {
  return computeRiseSet(jd, solarCoord, obs, horizon, Twilight);
}

int MoonRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight) {
  return planetRiseSet(jd, moonCoord, obs, horizon, Twilight);
}

int MercuryRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight) {
  return planetRiseSet(jd, mercuryCoord, obs, horizon, Twilight);
}

int VenusRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight) {
  return planetRiseSet(jd, venusCoord, obs, horizon, Twilight);
}

int MarsRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight) {
  return planetRiseSet(jd, marsCoord, obs, horizon, Twilight);
}

int JupiterRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight) {
  return planetRiseSet(jd, jupiterCoord, obs, horizon, Twilight);
}

int SaturnRiseSet(double jd, Observer &obs, const double horizon, RiseSet &Twilight) {
  return planetRiseSet(jd, saturnCoord, obs, horizon, Twilight);
}
