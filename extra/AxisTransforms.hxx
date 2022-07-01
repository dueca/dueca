/* ------------------------------------------------------------------   */
/*      item            : AxisTransform.hxx
        made by         : rvanpaassen
        date            : Mon Jun 29 12:58:26 2009
        category        : header file
        description     : convert aircraft axes to Ogre objects
        changes         : Mon Jun 29 12:58:26 2009 first version
                          181115 Converted to part of dueca/extra
        api             : DUECA_API
        language        : C++
*/

#ifndef AxisTransforms_hxx
#define AxisTransforms_hxx

#include <cmath>
#include "RvPQuat.hxx"
#include <Eigen/Dense>

#ifdef DEBUG_AXIS
#define DUECA_NS_START namespace dueca {
#define DUECA_NS_END }
#else
#include <dueca_ns.h>
#endif

DUECA_NS_START;

#define USING_EIGEN3
/// a normal matrix, allocates its own storage
typedef Eigen::MatrixXd Matrix;
/// a matrix that takes external storage
typedef Eigen::Map<Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> > MatrixE;
/// a const matrix that takes external storage
typedef Eigen::Map<const Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> > cMatrixE;
/// a normal vector, allocates its own storage
typedef Eigen::VectorXd Vector;
/// a vector that takes external storage
typedef Eigen::Map<Eigen::VectorXd> VectorE;
/// float vector, external storage
typedef Eigen::Map<Eigen::VectorXf> VectorfE;
/// constant double vector, external storage
typedef Eigen::Map<const Eigen::VectorXd> cVectorE;

// pre-define
struct EulerAngles;

/** @file AxisTransforms.hxx
    Utilities for axis transformations.
*/


/** Represent an orientation or rotation transformation with a quaternion */
struct Orientation
{
  /** Quaternion Lambda parameter */
  double L;
  /** Quaternion lx */
  double lx;
  /** Quaternion ly */
  double ly;
  /** Quaternion lz */
  double lz;
  /** All coordinates as a vector */
  VectorE quat;

  /** Construction, phi, theta, psi */
  Orientation(const EulerAngles& angles);

  /** Construction from angle and axis */
  template<class V>
  Orientation(double angle, const V& axis);

  /** Default, null orientation */
  Orientation();

  /** Copy constructor */
  Orientation(const Orientation& o);

  /** Indexation operator */
  inline const double& operator[] (const int i) const { return quat[i]; }

  /** Indexation operator */
  inline double& operator[] (const int i) { return quat[i]; }

  /** Multiplication */
  inline Orientation operator* (const Orientation& o) const
  { Orientation res; QxQ(res, *this, o); return res; }
};


/** Euler angles describing an orientation */
struct EulerAngles
{
  /** roll angle */
  double phi;
  /** pitch angle */
  double tht;
  /** yaw angle */
  double psi;

  /** Construction, phi, theta, psi */
  EulerAngles(double phi, double tht, double psi);

  /** Construction from quaternion */
  EulerAngles(const Orientation& o);
};

class LocalAxis;
struct ECEF;
struct LatLonAlt;

/** Set of x, y, z Carthesian coordinates. */
struct Carthesian
{
  /** x coordinate */
  double x;
  /** y coordinate */
  double y;
  /** z coordinate */
  double z;
  /** All coordinates as a vector */
  VectorE xyz;

  /** Constructor, straightforward */
  Carthesian(double x = 0.0, double y = 0.0, double z = 0.0);

  /** Copy constructor */
  Carthesian(const Carthesian& o);

  /** Assignment !!! */
  Carthesian& operator = (const Carthesian&);
};


/** ECEF coordinate set class. */
struct ECEF: public Carthesian
{
  /** Constructor, straightforward from x, y and z */
  ECEF(double x = 0.0, double y = 0.0, double z = 0.0);

  /** Constructor from lat, lon, altitude */
  ECEF(const LatLonAlt& lla);

  /** Copy constructor */
  ECEF(const ECEF& o);
};


/** Position on the WGS geoid in geodetic coordinates */
struct LatLonAlt
{
  /** Latitude */
  double lat;
  /** Longitude */
  double lon;
  /** Altitude */
  double alt;

  /** Constructor, straightforward from lat, lon and alt */
  LatLonAlt(double lat, double lon, double alt);

  /** Constructor from an ECEF object. Uses approximate calculation
      based on Bowring, 1976 */
  LatLonAlt(const ECEF& ecef);

  /** Meridian radius of curvature here */
  double RM() const;

  /** Prime radius of curvature here */
  double RP() const;

  /** Produce the rotation that converts local N-E-down vectors to
      global vectors */
  Orientation toGlobal(const double psi_zero = 0.0) const;
};



/** This class produces an "efficient" implementation of a local axis
    frame mapped onto ECEF or lat-lon-alt coordinates.

    The local axis frame is not exactly squarely mapped; some fudging
    is done (parabolic correction) to ensure that height above the
    terrain is approximately correct. Note that there are still deviations;
    60 NM lead to a height error of approximately 7 m (versus 900 m
    uncorrected!). This approximation is good enough for flying around
    a single airport. Otherwise, consider ECEF coordinates for your
    simulation and use those */
class LocalAxis
{
  /** Conversion matrix to convert local position to ECEF relative
      coordinates */
  Matrix to_ECEF;

  /** Vector in ECEF to the lat-lon-zero origin */
  ECEF origin;

  /** Local Radius meridian */
  double RM;

  /** Local Radius parallel */
  double RP;

public:
  /** Constructor */
  LocalAxis(const LatLonAlt& lla, double psi_zero = 0.0);

  /** Create an ECEF representation from a local xy_altitude set */
  ECEF toECEF(const Carthesian& coords) const;

  /** Create a local representation from an ECEF location */
  Carthesian toLocal(const ECEF& ecef) const;

  /** Orientation conversion */
  Orientation toNorthUp(const Orientation& o) const;
};

DUECA_NS_END;
namespace std {

  /** Print an EulerAngles object. */
  ostream& operator << (ostream& os, const dueca::EulerAngles &c);
}
namespace std {

  /** Print a LatLonAlt object. */
  ostream& operator << (ostream& os, const dueca::LatLonAlt& lla);
}
namespace std {

  /** Print a Carthesian object. */
  ostream& operator << (ostream& os, const dueca::Carthesian& c);
}
namespace std {

  /** Print an Orientation object. */
  ostream& operator << (ostream& os, const dueca::Orientation& c);
}

#endif
