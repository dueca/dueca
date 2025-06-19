/* ------------------------------------------------------------------   */
/*      item            : RigidBody.hxx
        made by         : Rene van Paassen
        date            : 040414
        category        : header file
        description     : Implements the dynamics of a rigid body.
                          After applying forces and moments on the
                          body, and specifying the gravity, one can
                          ask for an integration step.
        documentation   : DUECA_API
        changes         : 040414 first version
                          060427 integration in DUECA-extra,
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef RigidBody_hxx
#define RigidBody_hxx

#ifdef RigidBody_cxx
#endif

#ifndef USING_EIGEN3
#define USING_EIGEN3
#endif

#include <Eigen/Dense>

/** \file RigidBody.hxx Helper class for implementing rigid body
    vehicle simulation. */

/// A 3x3 matrix, allocates its own storage
typedef Eigen::Matrix3d Matrix3;
/// A 4x4 matrix, allocates its own storage
typedef Eigen::Matrix4d Matrix4;
/// a normal matrix, allocates its own storage
typedef Eigen::MatrixXd Matrix;
/// a matrix that takes external storage
typedef Eigen::Map<Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> > MatrixE;
/// a 3x1 vector, allocates its own storage
typedef Eigen::Vector3d Vector3;
/// a 4x1 vector, allocates its own storage
typedef Eigen::Vector4d Vector4;
/// a normal vector, allocates its own storage
typedef Eigen::VectorXd Vector;
/// a vector that takes external storage
typedef Eigen::Map<Eigen::VectorXd> VectorE;

// quaternion derivative
template<class V1, class V2, class V4>
static void quat_der(const V1& q, const V2& omega, V4& dq)
{
  // in principle:
  // dq = 0.5 [ dot(-omega, q(2..4)) ;
  //       q(1) * omega - omega x q(2..4)]
  dq[3] = -0.5 * (omega[0]*q[0] + omega[1]*q[1] + omega[2]*q[2]);
  dq[0] =  0.5 * (q[3]*omega[0] - (omega[1]*q[2] - omega[2]*q[1]));
  dq[1] =  0.5 * (q[3]*omega[1] - (omega[2]*q[0] - omega[0]*q[2]));
  dq[2] =  0.5 * (q[3]*omega[2] - (omega[0]*q[1] - omega[1]*q[0]));
}


/** Rigid body dynamics function, calculates derivative of a rigid
    body, given sum of moments and forces and the acting gravity
    field.

    The contents of the state vector are:

    u, v, w, x, y, z, p, q, r, l1, l2, l3, L

    <ul>
    <li> u, v, w are the velocity of the body, in body coordinates.
    <li> x, y, z are the position of the body, in inertial reference
    coordinates.
    <li> p, q, r are the components of the rotational speed vector, in
    body coordinates
    <li> l1, l2, l3, L are the elements of the quaternion describing
    the body attitude.
    </ul>

    Note that the RigidBody class only calculates the derivatives. If
    you want to integrate the equations of motion, you need an
    integration routine. Two templated integration functions are
    available, integrate_euler() and integrate_rungekutta().

    The normal procedure would be to create a class that derives from
    the RigidBody class. You can also indicate that a number of
    additional state variables is needed, the RigidBody class will
    make these available to you, from index 13 onward.

    The normal "work cycle" would be to call RigidBody::zeroForces(),
    which zeroes forces and gravity applied to the body, then
    repeatedly call RigidBody::applyBodyForce(),
    RigidBody::applyInertialForce(), RigidBody::applyBodyMoment()
    RigidBody::addInertialGravity() as needed, until all forces (drag,
    lift, wheel forces, thrusters, whatever) and all gravitational
    effects have been applied.

    Then call  RigidBody::derivative() to obtain the derivative.

    RigidBody::specific(), RigidBody::X() and RigidBody::phi(),
    RigidBody::theta(), RigidBody::psi() can be used to get your
    outputs.
 */
class RigidBody
{
protected:
  /** State vector. Contents: u, v, w, x, y, z,
      then p, q, r, l1, l2, l3, L.
      After this, thus from state 13 onwards, the state vector
      contains states for derived classes.*/
  Vector x;

private:
  /** Auxiliary output. \{ */
  /** Speed. */
  double iV;

  /** Angle of attack. */
  double ialpha;

  /** Sideslip angle. */
  double ibeta;

  /** Euler angles. \{ */
  double iphi, itheta, ipsi; /// \}

  /** \} */

  /** Properties of the rigid body. */
  /// \{
  /** Body mass. */
  double mass;

  /** Normalized inertia matrix. */
  Matrix3 Jn;

  /** Inverse normalized inertia matrix */
  Matrix3 Jninv;
  /// \}

  /** Working variables. */
  /// \{

  /** Attitude conversion matrix, earth-to-body (its transpose does
      the reverse). */
  Matrix3 _A;

  /** Rotation cross-product matrix. */
  Matrix3 Omega;

  /** Summed moments for this step, body axis. */
  Vector3 moment;

  /** Summed forces for this step, body axis. */
  Vector3 force;

  /** Two temporary vectors, 3 elt each. \{ */
  Vector3 tmp0, tmp1; /// \}

  /** Temporary vector for quat derivative. */
  Vector4 dq;

  /** Gravitational field, in inertial coordinates. */
  Vector3 einstein;

  /// \}
public:
  /** Simple constructor for a rigid body dynamics module. Note
      that motion is always described around center of mass.
      \param mass    mass of the object
      \param Jxx     Moment of inertia around x axis
      \param Jyy     Moment of inertia around y axis
      \param Jzz     Moment of inertia around z axis
      \param Jxy     Inertia cross product x and y
      \param Jxz     Inertia cross product x and z
      \param Jyz     Inertia cross product y and z
      \param extrastates Number of additional states needed by derived
                     classes.
  */
  RigidBody(double mass,
            double Jxx, double Jyy, double Jzz,
            double Jxy, double Jxz, double Jyz, int extrastates = 0);

  /** Destructor. */
  ~RigidBody();

public:
  /** Initialize a state vector x. Remember that x has 13 elements,
      due to the use of a quaternion representation.
      \param x       Carthesian position, x direction
      \param y       Carthesian position, y direction
      \param z       Carthesian position, z direction
      \param u       Velocity, along body x axis
      \param v       Velocity, along body y axis
      \param w       Velocity, along body z axis
      \param phi     Euler angle phi
      \param theta   Euler angle theta
      \param psi     Euler angle psi
      \param p       Rotational velocity, along body x axis
      \param q       Rotational velocity, along body y axis
      \param r       Rotational velocity, along body z axis
  */
  void initialize(double x, double y, double z,
                  double u, double v, double w,
                  double phi, double theta, double psi,
                  double p, double q, double r);

  /** Obtain and calculate the derivative, given sum of forces and moments
      and the gravitational field acting on the body.
      \param xd      Resultant derivative vector
      \param unused  Unused variable, for compatibility with
                     integration functions.
  */
  void derivative(VectorE& xd, double unused = 0.0);

  /** Obtain and calculate the derivative, given sum of forces and moments
      and the gravitational field acting on the body.
      \tparam V      Vector type
      \param xd      Resultant derivative vector
      \param unused  Unused variable, for compatibility with
                     integration functions.
  */
  template<class V>
  void derivative(V& xd, double unused=0.0)
  {
    // derivatives of u, v, w (x(0,2)
    // \dot{u} = force/mass - Omega u
    xd.head(3) = -1.0 * Omega * x.head(3) + force / mass;

    // add gravitational acceleration, Gravitation is in inertial, so
    // translate to the body, since we are considering body coordinate accel.
    xd.head(3) += _A * einstein;

    // derivative of the earth position, calculate from speed in body
    // coordinates, the speed in inertial coordinates, that is the
    // derivative
    // remember A is DCM, gives transformation from inertial to body,
    // have to go reverse here
    xd.segment(3,3) = _A * x.head(3);

    // derivative of the rotation speed vector \omega_b
    // \dot{\omega} = Jn^{-1} \left[ M - \Omega Jn \omega \right]
    xd.segment(6,3) = Jninv * (-1.0 * Omega * (Jn * x.segment(6,3)) + moment / mass);

    // derivative of the attitude quaternion, from rotational speed
    quat_der(x.segment(9,4), x.segment(6,3), dq);
    // quat_der does not handle ranges
    xd.segment(9,4) = dq;
  }


  /** Calculate specific forces and moments, forces stored in elements
      0, 1, and 2, moments in elements 3, 4 and 6. Forces and moments
      calculated in body coordinates.
      \param sp      Vector with 6 elements, for result. */
  void specific(Vector& sp);

  /** Initialize sum of forces on the body to zero. Call before
      applying a new set of forces and moments. */
  void zeroForces();

  /** Apply a force expressed in body coordinates. at a specific body
      point. */
  void applyBodyForce(const Vector3& Fb, const Vector3& point);

  /** Apply a force expressed in inertial coordinates, at a specific
      body point, give in body coordinates. */
  void applyInertialForce(const Vector& Fi, const Vector& point);

  /** Apply a moment expressed in body coordinates. */
  void applyBodyMoment(const Vector& M);

  /** Change the state vector by a quantity dx. */
  void changeState(const Vector& dx);

  /** Add a gravitational field; 3 element vector, gravitation
      expressed in the inertial system. */
  void addInertialGravity(const Vector& g);

  /** Put in a new state vector. Note that you need a 13-element
      vector */
  void setState(const Vector& newx);

  /** Add to the mass of the body. */
  void changeMass(const double dm);

  /** Set a new mass. */
  void setMass(const double newm);

  /** Calculate auxiliary outputs, V, \f$\alpha\f$, \f$\beta\f$, \f$\phi\f$,
      \f$\theta\f$ and \f$\Psi\f$. */
  void output();

  /** Obtain the state, 13 elements. */
  inline const Vector& X() const {return x;}

  /** Obtain speed. */
  inline const double V() const {return iV;}

  /** Obtain alpha. */
  inline const double alpha() const {return ialpha;}

  /** Obtain beta. */
  inline const double beta() const {return ibeta;}

  /** Get current mass. */
  inline const double getMass() const {return mass;}

  /** Get euler angle phi */
  inline const double phi() const {return iphi;}
  /** Get euler angle theta */
  inline const double theta() const {return itheta;}
  /** Get euler angle psi */
  inline const double psi() const {return ipsi;}\

  /** Get the transformation matrix, for transforming a vector in
      earth coordinates to one in the body's coordinate system.

      Note that for the reverse translation, you need the transpose of
      this matrix. Example:
      \code
      // suspension_point_b is a vector in the body coordinate system,
      // defining the location of the wheel suspension
      // point. This calculates that position in earth coordinates
      mult(trans(body.A()), suspension_point_b, body.X()(3,6),
           suspension_point_e);

      // Mult(A, x, y, z) z <- A x + y, see MTL documentation. State
      // elements 3, 4, 5 (body.X()(3,6)) contain the c.g. position in
      // earth coordinates.
      \endcode
  */
  inline const Matrix3& A() { return _A; }

private:
  /** Prepare for the next iteration step. Call this before applying
      any forces or moment on the body */
  void prepare();
};

/**  An example, including subclassing RigidBody and using Runge-Kutta
     integration to calculate the updates, is given here

     @example PulsedBody.cxx
*/

#endif
