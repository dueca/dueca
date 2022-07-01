/* ------------------------------------------------------------------   */
/*      item            : RvPQuat.hxx
        made by         : Rene van Paassen
        date            : 100127
        category        : header file
        description     :
        api             : DUECA_API
        changes         : 100127 first version
        language        : C++
*/

#ifndef RvPQuat_hxx
#define RvPQuat_hxx
#include <cmath>

#ifdef DEBUG_AXIS
#ifndef DUECA_NS_START
#define DUECA_NS_START  namespace dueca {
#define DUECA_NS_END    }
#endif
#else
#include <dueca_ns.h>
#endif

/** @file RvPQuat.hxx

    A random collection of quaternion code */


DUECA_NS_START;

/// Index of x component / i variable
static const int qX = 1;
/// Index of y component / j variable
static const int qY = 2;
/// index of z component / k variable
static const int qZ = 3;
/// real part of quaternion index
static const int qW = 0;

/** Extract the roll angle from a rotation quaternion.

    @tparam T     vector type
    @param  quat  given quaternion
    @returns      Roll angle, double
*/
template<class T>
inline double Q2phi(const T& quat)
{
  return atan2(2.0*double(quat[2]*quat[3] +
                          quat[1]*quat[0]),
               double(quat[0]*quat[0] -
                      quat[1]*quat[1] -
                      quat[2]*quat[2] +
                      quat[3]*quat[3]));
}

/** Extract the pitch angle from a rotation quaternion.

    @tparam T     vector type
    @param quat   given quaternion
    @returns      Pitch angle, double
*/
template<class T>
inline double Q2tht(const T& quat)
{
  return asin(-2.0*double(quat[1]*quat[3] -
                          quat[2]*quat[0]));
}

/** Extract the yaw angle from a rotation quaternion.

    @tparam T     vector type
    @param quat   given quaternion
    @returns      Yaw angle, double
*/
template<class T>
inline double Q2psi(const T& quat)
{
  return atan2(2.0*double(quat[qX]*quat[qY] +
                          quat[qW]*quat[qZ]),
               double(quat[qW]*quat[qW] +
                      quat[qX]*quat[qX] -
                      quat[qY]*quat[qY] -
                      quat[qZ]*quat[qZ]));
}

/** Construct a quaternion from Euler-Rodriguez angles

    @tparam T1     Vector type for quaternion
    @tparam T2     Data type for angles

    @param quat    Returned, quaternion vector
    @param phi     Roll angle
    @param tht     Pitch angle
    @param psi     Yaw angle
*/
template<class T1, class T2>
inline void phithtpsi2Q(T1& quat, const T2 phi, const T2 tht, const T2 psi)
{
  quat[qW] = cos(0.5*phi)*cos(0.5*tht)*cos(0.5*psi) +
    sin(0.5*phi)*sin(0.5*tht)*sin(0.5*psi);
  quat[qX] = sin(0.5*phi)*cos(0.5*tht)*cos(0.5*psi) -
    cos(0.5*phi)*sin(0.5*tht)*sin(0.5*psi);
  quat[qY] = cos(0.5*phi)*sin(0.5*tht)*cos(0.5*psi) +
    sin(0.5*phi)*cos(0.5*tht)*sin(0.5*psi);
  quat[qZ] = cos(0.5*phi)*cos(0.5*tht)*sin(0.5*psi) -
    sin(0.5*phi)*sin(0.5*tht)*cos(0.5*psi);
}

/** Multiply two quaternions

    @tparam T      Vector type for result quaternion
    @tparam U      Vector type for first
    @tparam V      Vector type for second

    @param result  Result quaternion
    @param q1      First input
    @param q2      Second input
*/
template<class T, class U, class V>
inline void QxQ(T& result, const U& q1, const V& q2)
{
  result[qW] = q1[qW]*q2[qW] - q1[qX]*q2[qX] - q1[qY]*q2[qY] - q1[qZ]*q2[qZ];
  result[qX] = q1[qW]*q2[qX] + q1[qX]*q2[qW] + q1[qY]*q2[qZ] - q1[qZ]*q2[qY];
  result[qY] = q1[qW]*q2[qY] + q1[qY]*q2[qW] + q1[qZ]*q2[qX] - q1[qX]*q2[qZ];
  result[qZ] = q1[qW]*q2[qZ] + q1[qZ]*q2[qW] + q1[qX]*q2[qY] - q1[qY]*q2[qX];
}

/** Convert a quaternion to its conjugate

    @tparam T   Four-element vector/array type
    @param q    Quaternion to change
*/
template<class T>
inline void Qconjugate(T& q)
{
  q[qX] = -q[qX];
  q[qY] = -q[qY];
  q[qZ] = -q[qZ];
}

/** Print a quaternion to stream

    @tparam S    Stream type
    @tparam T    Four-element vector/array type
    @param s     Stream object
    @param quat  Quaternion

    @returns     A reference to the stream object
*/
template<typename S, typename T>
S& printQ(S& s, const T& quat)
{
  s << '(' << quat[0] << ' ' << quat[1] << ' ' << quat[2] << ' ' << quat[3] << ')';
  return s;
}

/** Print a quaternion to stream in its Euler angles form

    @tparam S    Stream type
    @tparam T    Four-element vector/array type
    @param s     Stream object
    @param quat  Quaternion

    @returns     A reference to the stream object
*/
template<typename S, typename T>
S& printPTP(S& s, const T& quat)
{
  s << '(' << Q2phi(quat) << ' ' << Q2tht(quat) << ' ' << Q2psi(quat) << ')';
  return s;
}

/** Calculate the rotation matrix for a quaternion

    @tparam M    Matrix format
    @tparam T    Four-element vector/array type
    @param mat   Matrix to be filled
    @param q     Rotation quaternion
*/
template<typename M, typename T>
inline void Q2R(M& mat, const T& q)
{
  double xx2 = 2 * q[qX] * q[qX];
  double yy2 = 2 * q[qY] * q[qY];
  double zz2 = 2 * q[qZ] * q[qZ];
  double xy2 = 2 * q[qX] * q[qY];
  double wz2 = 2 * q[qW] * q[qZ];
  double zx2 = 2 * q[qZ] * q[qX];
  double wy2 = 2 * q[qW] * q[qY];
  double yz2 = 2 * q[qY] * q[qZ];
  double wx2 = 2 * q[qW] * q[qX];
  mat(0,0) = 1. - yy2 - zz2; mat(0,1) = xy2 - wz2; mat(0,2) = zx2 + wy2;
  mat(1,0) = xy2 + wz2; mat(1,1) = 1. - xx2 - zz2; mat(1,2) = yz2 - wx2;
  mat(2,0) = zx2 - wy2; mat(2,1) = yz2 + wx2; mat(2,2) = 1. - xx2 - yy2;
}

/** Cross-product matrix set up from a vector

    @tparam Mat   Matrix type
    @tparam Vec   Quaternion vector type
    @param m      Resulting matrix
    @param v      Three-element vector
 */
template<class Mat, class Vec>
inline void cross_prod_matrix(Mat& m, const Vec& v)
{
  m(0,0) =   0.0; m(0,1) = -v[2]; m(0,2) =  v[1];
  m(1,0) =  v[2]; m(1,1) =   0.0; m(1,2) = -v[0];
  m(2,0) = -v[1]; m(2,1) =  v[0]; m(2,2) =   0.0;
}

DUECA_NS_END;

#endif
