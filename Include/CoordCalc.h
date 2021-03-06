/// @file       CoordCalc.h
/// @brief      Arithmetics with geographic coordinations and altitudes
/// @details    Basic calculations like distance, angle between vectors, point plus vector.\n
///             Definitions for classes positionTy, vectorTy, and boundingBoxTy.
/// @author     Birger Hoppe
/// @copyright  (c) 2018-2020 Birger Hoppe
/// @copyright  Permission is hereby granted, free of charge, to any person obtaining a
///             copy of this software and associated documentation files (the "Software"),
///             to deal in the Software without restriction, including without limitation
///             the rights to use, copy, modify, merge, publish, distribute, sublicense,
///             and/or sell copies of the Software, and to permit persons to whom the
///             Software is furnished to do so, subject to the following conditions:\n
///             The above copyright notice and this permission notice shall be included in
///             all copies or substantial portions of the Software.\n
///             THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///             IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///             FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///             AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///             LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///             OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
///             THE SOFTWARE.

#ifndef CoordCalc_h
#define CoordCalc_h

#include "XPLMScenery.h"
#include <valarray>
#include <deque>

// positions and angles are in degrees
// distances and altitude are in meters

//
// MARK: Mathematical helper functions
//
/// Square, ie. a^2
template <class T>
inline T sqr (T a) { return a*a; }

/// Pythagoras square, ie. a^2 + b^2
template <class T>
inline T pyth2 (T a, T b) { return sqr(a) + sqr(b); }

//
//MARK: Degree/Radian conversion
//      (as per stackoverflow post, adapted)
//
inline double deg2rad (const double deg)
{ return (deg * PI / 180); }

inline double rad2deg (const double rad)
{ return (rad * 180 / PI); }

// angle flown, given speed and vsi (both in m/s)
inline double vsi2deg (const double speed, const double vsi)
{ return rad2deg(std::atan2(vsi,speed)); }

//
//MARK: Functions on coordinates
//

struct positionTy;
struct vectorTy;

/// angle between two locations given in plain lat/lon
double CoordAngle (double lat1, double lon1, double lat2, double lon2);
/// distance between two locations given in plain lat/lon [meter]
double CoordDistance (double lat1, double lon1, double lat2, double lon2);
// angle between two coordinates
double CoordAngle (const positionTy& pos1, const positionTy& pos2 );
//distance between two coordinates
double CoordDistance (const positionTy& pos1, const positionTy& pos2);
// vector from one position to the other (combines both functions above)
vectorTy CoordVectorBetween (const positionTy& from, const positionTy& to );
// destination point given a starting point and a vetor
positionTy CoordPlusVector (const positionTy& pos, const vectorTy& vec);

// returns terrain altitude at given position
// returns NaN in case of failure
double YProbe_at_m (const positionTy& posAt, XPLMProbeRef& probeRef);

//
// MARK: Estimated Functions on coordinates
//

/// @details Length of a degree latitude
/// @see https://en.wikipedia.org/wiki/Geographic_coordinate_system#Length_of_a_degree
constexpr double LAT_DEG_IN_MTR = 111132.95;
/// @details Length of a degree longitude
/// @see https://en.wikipedia.org/wiki/Geographic_coordinate_system#Length_of_a_degree
inline double LonDegInMtr (double lat) { return LAT_DEG_IN_MTR * std::cos(deg2rad(lat)); }

/// @brief An _estimated_ **square** of the distance between 2 points given by lat/lon
/// @details Makes use simple formulas to convert lat/lon differences into meters
///          So this is not exact but quick and good enough for many purposes.
///          On short distances of less than 10m, difference to CoordDistance() is a few millimeters.
/// @return Square of distance (estimated) in meter
double DistLatLonSqr (double lat1, double lon1,
                      double lat2, double lon2);

/// @brief An _estimated_ distance between 2 points given by lat/lon
/// @details Makes use simple formulas to convert lat/lon differences into meters
///          So this is not exact but quick and good enough for many purposes.
///          On short distances of less than 10m, difference to CoordDistance() is a few millimeters.
/// @return Distance (estimated) in meter
inline double DistLatLon (double lat1, double lon1,
                          double lat2, double lon2)
{ return std::sqrt(DistLatLonSqr(lat1,lon1,lat2,lon2)); }


//
// MARK: Functions on 2D points, typically in meters
//

/// @brief Simple square of distance just by Pythagoras
inline double DistPythSqr (double x1, double y1,
                           double x2, double y2)
{ return pyth2(x2-x1, y2-y1); }

/// Return structure for DistPointToLineSqr()
struct distToLineTy {
    double      dist2 = NAN;        ///< main result: square distance of point to the line
    double      len2 = NAN;         ///< square of length of line between ln_x/y1 and ln_x/y2
    double      leg1_len2 = NAN;    ///< square length of leg from point 1 to base (base is point on the line with shortest distance to point)
    double      leg2_len2 = NAN;    ///< square length of leg from point 2 to base (base is point on the line with shortest distance to point)
    /// Is the base outside the endpoints of the line?
    bool IsBaseOutsideLine () const
    { return leg1_len2 > len2 || leg2_len2 > len2; }
    /// How much is the base outside the (nearer) endpoint? (squared)
    double DistSqrOfBaseBeyondLine () const
    { return std::max(leg1_len2,leg2_len2) - len2; }
};

/// @brief Square of distance between a location and a line defined by two points.
/// @note Function makes no assuptions about the coordinate system,
///       only that x and y are orthogonal. It uses good ole plain Pythagoras.
///       Ie., if x/y are in local coordinates, result is in meter.
///       if they are in geometric coordinates, result cannot be converted to an actual length,
///       but can still be used in relative comparions.
/// @note All results are square values. Functions avoids taking square roots for performance reasons.
/// @param pt_x Point's x coordinate
/// @param pt_y Point's y coordinate
/// @param ln_x1 Line: First endpoint's x coordinate
/// @param ln_y1 Line: First endpoint's y coordinate
/// @param ln_x2 Line: Second endpoint's x coordinate
/// @param ln_y2 Line: Second endpoint's y coordinate
/// @param[out] outResults Structure holding the results, see ::distToLineTy
void DistPointToLineSqr (double pt_x, double pt_y,
                         double ln_x1, double ln_y1,
                         double ln_x2, double ln_y2,
                         distToLineTy& outResults);

/// @brief Based on results from DistPointToLineSqr() computes locaton of base point on line
/// @param ln_x1 Line: First endpoint's x coordinate (same as passed in to DistPointToLineSqr())
/// @param ln_y1 Line: First endpoint's y coordinate (same as passed in to DistPointToLineSqr())
/// @param ln_x2 Line: Second endpoint's x coordinate (same as passed in to DistPointToLineSqr())
/// @param ln_y2 Line: Second endpoint's y coordinate (same as passed in to DistPointToLineSqr())
/// @param res Result returned by DistPointToLineSqr()
/// @param[out] x X coordinate of base point on the line
/// @param[out] y Y coordinate of base point on the line
void DistResultToBaseLoc (double ln_x1, double ln_y1,
                          double ln_x2, double ln_y2,
                          const distToLineTy& res,
                          double &x, double &y);

//
//MARK: Data Structures
//

// a vector
struct vectorTy {
    double  angle;                      // degrees
    double  dist;                       // meters
    double  vsi;                        // m/s
    double  speed;                      // m/s
    
    vectorTy () : angle(NAN), dist(NAN), vsi(NAN), speed(NAN) {}
    vectorTy ( double dAngle, double dDist, double dVsi=NAN, double dSpeed=NAN ) :
    angle(dAngle), dist(dDist), vsi(dVsi), speed(dSpeed) {}

    // standard string for any output purposes
    operator std::string() const;
    
    // convert to nautical units
    inline double speed_kn () const { return speed * KT_per_M_per_S; }
    inline double vsi_ft () const { return vsi / Ms_per_FTm; }
};

// a position: latitude (Z), longitude (X), altitude (Y), timestamp
struct positionTy {
    enum positionTyE { LAT=0, LON, ALT, TS, HEADING, PITCH, ROLL };
    std::valarray<double> v;
    
    int mergeCount;      // for posList use only: when merging positions this counts how many flight data objects made up this position
    
    enum onGrndE    { GND_UNKNOWN=0, GND_OFF, GND_ON } onGrnd;
    enum coordUnitE { UNIT_WORLD, UNIT_LOCAL } unitCoord;
    enum angleUnitE { UNIT_DEG, UNIT_RAD } unitAngle;
    
    // start of some special flight phase like rotate, take off, touch down?
    // (can't use LTAircraft::FlightPhase due to cyclic header inclusion)
    int flightPhase = 0;
public:
    positionTy () : v{NAN,NAN,NAN,NAN,NAN,NAN,NAN}, mergeCount(1),
                    onGrnd(GND_UNKNOWN), unitCoord(UNIT_WORLD), unitAngle(UNIT_DEG) {}
    positionTy (double dLat, double dLon, double dAlt_m=NAN,
                double dTS=NAN, double dHead=NAN, double dPitch=NAN, double dRoll=NAN,
                onGrndE grnd=GND_UNKNOWN, coordUnitE uCoord=UNIT_WORLD, angleUnitE uAngle=UNIT_DEG,
                int fPhase = 0) :
        v{dLat, dLon, dAlt_m, dTS, dHead, dPitch, dRoll}, mergeCount(1),
        onGrnd(grnd), unitCoord(uCoord), unitAngle(uAngle), flightPhase(fPhase) {}
    positionTy(const XPMPPlanePosition_t& x) :
        positionTy (x.lat, x.lon, x.elevation * M_per_FT,
                    NAN, x.heading, x.pitch, x.roll) {}
    positionTy ( const XPLMProbeInfo_t& probe ) :
        positionTy ( probe.locationZ, probe.locationX, probe.locationY ) { unitCoord=UNIT_LOCAL; }
    
    // merge with the given position
    positionTy& operator |= (const positionTy& pos);
    
    // typecast to what XPMP API needs
    operator XPMPPlanePosition_t() const;
    // standard string for any output purposes
    static const char* GrndE2String (onGrndE grnd);
    std::string dbgTxt() const;
    operator std::string() const;
    
    // timestamp-based comparison
    inline bool hasSimilarTS (const positionTy& p) const { return abs(ts()-p.ts()) <= SIMILAR_TS_INTVL; }
    inline bool canBeMergedWith (const positionTy& p) const { return hasSimilarTS(p); }
    inline int cmp (const positionTy& p)        const { return ts() < p.ts() ? -1 : (ts() > p.ts() ? 1 : 0); }
    inline bool operator<< (const positionTy& p) const { return ts() < p.ts() - SIMILAR_TS_INTVL; }
    inline bool operator<  (const positionTy& p) const { return ts() < p.ts(); }
    inline bool operator<= (const positionTy& p) const { return ts() <= p.ts() + SIMILAR_TS_INTVL; }
    inline bool operator>= (const positionTy& p) const { return ts() >= p.ts() - SIMILAR_TS_INTVL; }
    inline bool operator>  (const positionTy& p) const { return ts() > p.ts(); }
    inline bool operator>> (const positionTy& p) const { return ts() > p.ts() + SIMILAR_TS_INTVL; }

    // normalizes to -90/+90 lat, -180/+180 lon, 360° heading, return *this
    positionTy& normalize();
    // is a good valid position?
    bool isNormal (bool bAllowNanAltIfGnd = false) const;
    // is fully valid? (isNormal + heading, pitch, roll)?
    bool isFullyValid() const;
    
    // rad/deg conversion (only affects lat and lon)
    positionTy  deg2rad() const;
    positionTy& deg2rad();
    positionTy  rad2deg() const;
    positionTy& rad2deg();
    
    // named element access
    inline double lat()     const { return v[LAT]; }
    inline double lon()     const { return v[LON]; }
    inline double alt_m()   const { return v[ALT]; }                    // in meter
    inline double alt_ft()  const { return alt_m()/M_per_FT; }   // in feet
    inline double ts()      const { return v[TS]; }
    inline double heading() const { return v[HEADING]; }
    inline double pitch()   const { return v[PITCH]; }
    inline double roll()    const { return v[ROLL]; }

    inline bool   IsOnGnd() const { return onGrnd == GND_ON; }

    inline double& lat()        { return v[LAT]; }
    inline double& lon()        { return v[LON]; }
    inline double& alt_m()      { return v[ALT]; }
    inline double& ts()         { return v[TS]; }
    inline double& heading()    { return v[HEADING]; }
    inline double& pitch()      { return v[PITCH]; }
    inline double& roll()       { return v[ROLL]; }
    
    inline void SetAltFt (double ft) { alt_m() = ft * M_per_FT; }

    // named element access using local coordinate names
    // latitude and Z go north/south
    // longitude and X go east/west
    // altitude and Y go up/down
    inline double Z() const { return v[LAT]; }
    inline double X() const { return v[LON]; }
    inline double Y() const { return v[ALT]; }
    inline double& Z() { return v[LAT]; }
    inline double& X() { return v[LON]; }
    inline double& Y() { return v[ALT]; }

    // short-cuts to coord functions
    inline double angle (const positionTy& pos2 ) const       { return CoordAngle ( *this, pos2); }
    inline double dist (const positionTy& pos2 ) const        { return CoordDistance ( *this, pos2); }
    inline vectorTy between (const positionTy& pos2 ) const   { return CoordVectorBetween ( *this, pos2); }
    inline positionTy destPos (const vectorTy& vec ) const    { return CoordPlusVector ( *this, vec); }
    inline positionTy operator+ (const vectorTy& vec ) const  { return destPos (vec); }
    inline double vsi_m (const positionTy& posTo) const       { return (posTo.alt_m()-alt_m()) / (posTo.ts()-ts()); }   // [m/s]
    inline double vsi_ft (const positionTy& posTo) const      { return vsi_m(posTo) * SEC_per_M/M_per_FT; }             // [ft/min]
    inline double speed_m (const positionTy& posTo) const     { return dist(posTo) / (posTo.ts()-ts()); }               // [m/s]
    inline double speed_kt (const positionTy& posTo) const    { return speed_m(posTo) * KT_per_M_per_S; }               // [kn]

    // move myself by a certain distance in a certain direction (normalized)
    // also changes altitude applying vec.vsi
    positionTy& operator += (const vectorTy& vec );
    
    // convert between World and Local OpenGL coordinates
    positionTy& LocalToWorld ();
    positionTy& WorldToLocal ();
};

typedef std::deque<positionTy> dequePositionTy;

// stringify all elements of a list for debugging purposes
std::string positionDeque2String (const dequePositionTy& l,
                                  const positionTy* posAfterLast = nullptr);

// find youngest position with timestamp less than parameter ts
dequePositionTy::const_iterator positionDequeFindBefore (const dequePositionTy& l, double ts);

// find two positions around given timestamp ts (before <= ts < after)
// pBefore and pAfter can come back NULL!
void positionDequeFindAdjacentTS (double ts, dequePositionTy& l,
                                  positionTy*& pBefore, positionTy*& pAfter);

// return the average of two headings, shorter side, normalized to [0;360)
double HeadingAvg (double h1, double h2, double f1=1, double f2=1);

/// @brief Difference between two headings
/// @returns number of degrees to turn from h1 to reach h2
/// -180 <= HeadingDiff <= 180
double HeadingDiff (double h1, double h2);

/// Normaize a heading to the value range [0..360)
double HeadingNormalize (double h);

// a bounding box has a north/west and a south/east corner
// we use positionTy for convenience...alt is usually not used here
struct boundingBoxTy {
    positionTy nw, se;
    
    boundingBoxTy () : nw(), se() {}
    boundingBoxTy (const positionTy& inNw, const positionTy& inSe ) :
    nw(inNw), se(inSe) {}
    
    /// @brief computes a bounding box based on a central position and a box width/height
    /// @param center Center position
    /// @param width Width of box in meters
    /// @param height Height of box in meters (defaults to `width`)
    boundingBoxTy (const positionTy& center, double width, double height = NAN );
    
    /// Enlarge the box by the given x/y values in meters on each side (`y` defaults to `x`)
    void enlarge_m (double x, double y = NAN);
    /// Increases the bounding box to include the given position
    void enlarge_pos (double lat, double lon);
    /// Increases the bounding box to include the given position
    void enlarge (const positionTy& lPos);
    /// Increases the bounding box to include the given position(s)
    void enlarge (std::initializer_list<positionTy> lPos) {
       for (const positionTy& pos: lPos)
            enlarge(pos);
    }
    
    /// Center point of bounding box
    positionTy center () const;

    // standard string for any output purposes
    operator std::string() const;
    
    // is position within bounding box?
    bool contains (const positionTy& pos ) const;
    bool operator & (const positionTy& pos ) const { return contains(pos); }
    
    /// Do both boxes overlap?
    bool overlap (const boundingBoxTy& o) const;
    /// Do both boxes overlap?
    bool operator & (const boundingBoxTy& o) const { return overlap(o); }
};

#endif /* CoordCalc_h */
