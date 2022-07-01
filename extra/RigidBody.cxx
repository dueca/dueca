/* ------------------------------------------------------------------   */
/*      item            : RigidBody.cxx
        made by         : Rene' van Paassen
        date            : 040414
        category        : body file
        description     :
        changes         : 040414 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define RigidBody_cxx
#include "RigidBody.hxx"

#include <iostream>
using namespace std;

#ifdef __QNXNTO__
#define cos ::cos
#define sin ::sin
#define sqrt ::sqrt
#define asin ::asin
#define atan2 ::atan2
#endif


RigidBody::RigidBody(double mass,
                     double Jxx, double Jyy, double Jzz,
                     double Jxy, double Jxz, double Jyz, int extrastates) :
  x(13 + extrastates),
  iV(0.0),
  ialpha(0.0),
  ibeta(0.0),
  mass(mass)
{
  // inertia matrix
  Jn <<  Jxx, -Jxy, -Jxz,
        -Jxy,  Jyy, -Jyz,
        -Jxz, -Jyz,  Jzz;

  // and normalized
  Jn /= mass;


  // calculate inverse, put into Jinv matrix
  Jninv = Jn.inverse();

  // initialize state at pos 0, att 0 etc.
  x = Vector::Zero(13 + extrastates);
  x(12) = 1.0;
  moment.setZero();
  force.setZero();
  prepare();

  // calculate outputs
  output();
}

RigidBody::~RigidBody()
{
  //
}


void RigidBody::initialize(double x, double y, double z,
                           double u, double v, double w,
                           double phi, double theta, double psi,
                           double p, double q, double r)
{
  this->x[ 0] = u;
  this->x[ 1] = v;
  this->x[ 2] = w;
  this->x[ 3] = x;
  this->x[ 4] = y;
  this->x[ 5] = z;
  this->x[ 6] = p;
  this->x[ 7] = q;
  this->x[ 8] = r;
  this->x[12] = cos(0.5*phi)*cos(0.5*theta)*cos(0.5*psi) +
    sin(0.5*phi)*sin(0.5*theta)*sin(0.5*psi);
  this->x[ 9] = sin(0.5*phi)*cos(0.5*theta)*cos(0.5*psi) -
    cos(0.5*phi)*sin(0.5*theta)*sin(0.5*psi);
  this->x[10] = cos(0.5*phi)*sin(0.5*theta)*cos(0.5*psi) +
    sin(0.5*phi)*cos(0.5*theta)*sin(0.5*psi);
  this->x[11] = cos(0.5*phi)*cos(0.5*theta)*sin(0.5*psi) -
    sin(0.5*phi)*sin(0.5*theta)*cos(0.5*psi);

  // calculate outputs
  output();
}

/** Calculate a 3by3 rotation matrix for a rotation defined by a
    quaternion.
    \param q     The quaternion, in the form of
                 (lambda_x, lambda_y, lambda_z, Lambda).
    \param Uq    Result, the rotation matrix. */
static void u_quat(const Vector& q, Matrix3& Uq)
{

  double u = sqrt(q.dot(q));
  double Au = q[0] / u;
  double Bu = q[1] / u;
  double Cu = q[2] / u;
  double Du = q[3] / u;
#if 0
  Uq(0,0) = Du*Du + Au*Au - Bu*Bu - Cu*Cu;
  Uq(0,1) = 2*(Au*Bu - Cu*Du);
  Uq(0,2) = 2*(Au*Cu + Bu*Du);
  Uq(1,0) = 2*(Au*Bu + Cu*Du);
  Uq(1,1) = Du*Du - Au*Au + Bu*Bu - Cu*Cu;
  Uq(1,2) = 2*(Bu*Cu - Au*Du);
  Uq(2,0) = 2*(Au*Cu - Bu*Du);
  Uq(2,1) = 2*(Bu*Cu + Au*Du);
  Uq(2,2) = Du*Du - Au*Au - Bu*Bu + Cu*Cu;
#else
  // as per Stevens + Lewis, DCM, is transverse, inertial->body
  Uq(0,0) = Du*Du + Au*Au - Bu*Bu - Cu*Cu;
  Uq(0,1) = 2*(Au*Bu + Cu*Du);
  Uq(0,2) = 2*(Au*Cu - Bu*Du);
  Uq(1,0) = 2*(Au*Bu - Cu*Du);
  Uq(1,1) = Du*Du - Au*Au + Bu*Bu - Cu*Cu;
  Uq(1,2) = 2*(Bu*Cu + Au*Du);
  Uq(2,0) = 2*(Au*Cu + Bu*Du);
  Uq(2,1) = 2*(Bu*Cu - Au*Du);
  Uq(2,2) = Du*Du - Au*Au - Bu*Bu + Cu*Cu;
#endif
#ifdef DEBUG
  cout << "    q="; print_vector(q);
  cout << "    C="; print_all_matrix(Uq);
#endif
}

void RigidBody::derivative(VectorE& xd, double unused)
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
  xd.segment(3,3) = _A.transpose() * x.head(3);

  // derivative of the rotation speed vector \omega_b
  // \dot{\omega} = Jn^{-1} \left[ M - \Omega Jn \omega \right]
  xd.segment(6,3) = Jninv * (-1.0 * Omega * (Jn * x.segment(6,3)) + moment / mass);

  // derivative of the attitude quaternion, from rotational speed
  quat_der(x.segment(9,4), x.segment(6,3), dq);
  // quat_der does not handle ranges
  xd.segment(9,4) = dq;
}

void RigidBody::specific(Vector& sp)
{
  // specific forces, in body ax, in elements 0 - 2
  sp.head(3) = force / mass;

  // specific moment, in body ax, in elements 3 - 5
  sp.segment(3,3) = Jninv * moment / mass;
}

void RigidBody::prepare()
{
  // calculate rotation matrix
  u_quat(x.segment(9,4), _A);

  // calculate cross-product matrix
  Omega <<  0.0, -x(8), x(7),
            x(8), 0.0, -x(6),
           -x(7), x(6), 0.0;
#ifdef DEBUG
  cout << "A: ";
  print_all_matrix(_A);
  cout << "Omega: ";
  print_all_matrix(Omega);
#endif
}

void RigidBody::zeroForces()
{
  // zero force and moment sums, and gravitation
  force.setZero();
  moment.setZero();
  einstein.setZero();
}

void RigidBody::changeState(const Vector& dx)
{
  x += dx;
  prepare();
}

void RigidBody::setState(const Vector& newx)
{
  x = newx;
  prepare();
}

void RigidBody::changeMass(const double dm)
{
  mass += dm;
}

void RigidBody::setMass(const double newm)
{
  mass = newm;
}

void RigidBody::output()
{
  iV = sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);
  ialpha = atan2(x[2], x[0]);
  ibeta = atan2(x[1], x[0]);

  // on the side, also normalize the quaternion
  Vector q = x.segment(9,4) / sqrt(x.segment(9,4).dot(x.segment(9,4)));

  double Au = q[0];
  double Bu = q[1];
  double Cu = q[2];
  double Du = q[3];

  iphi = atan2(2*(Bu*Cu + Au*Du), Du*Du - Au*Au - Bu*Bu + Cu*Cu);
  itheta = asin(-2.0*(Au*Cu - Bu*Du));
  ipsi = atan2(2*(Au*Bu + Cu*Du), Du*Du + Au*Au - Bu*Bu - Cu*Cu);
}

void RigidBody::applyBodyForce(const Vector3& Fb, const Vector3& point)
{
  // sum the force
  force += Fb;
  // add the moment, cross product of force and position of exertion
  tmp0 = point.cross(Fb);
#ifdef DEBUG
  print_vector(tmp0);
#endif
  applyBodyMoment(tmp0);
}

void RigidBody::applyInertialForce(const Vector& Fi, const Vector& point)
{
  // translate to body axes and apply
  tmp1 = _A * Fi;
  applyBodyForce(tmp1, point);
}

void RigidBody::applyBodyMoment(const Vector& M)
{
  moment += M;
}

void RigidBody::addInertialGravity(const Vector& g)
{
  einstein += g;
}


#ifdef TEST

#include "integrate_rungekutta.hxx"
#include "integrate_euler.hxx"

static const double mass   = 35000.0;
static const double mass_e = 25000.0;

class PulsedBody: public RigidBody
{
public:
  PulsedBody() :
    RigidBody(::mass,
              1.21 * ::mass, 2.9 * ::mass, 4.1 * ::mass,
              0.0,         0.02 * ::mass*0.0, 0.0)
  {
    initialize(-1.0, -500.0, 1000.0, 0.0, 0.0, 0.0,
               0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  }

  void derivative(VectorE& xd, double dt)
  {
    Vector cgpoint(3); cgpoint.setZero();
    Vector tmp(3); tmp.setZero();
    tmp[0] = 3500.0;
    Vector grav(3); grav.setZero();
    //grav[2] = 10;
    cgpoint[1]= 1;

    xd.setZero();
    zeroForces();
    applyBodyForce(tmp, cgpoint);
    addInertialGravity(grav);

    RigidBody::derivative(xd);
    //cout << "s:  "; print_vector(X());
    //cout << "xd: "; print_vector(xd);
  }
};

int main()
{
  Vector tmp(3);
  Vector cgpoint(3);
  Vector xd(13);
  double dt = 0.02;

  RigidBody body(mass,
                 1.21 * mass, 2.9 * mass, 4.1 * mass,
                 0.0,         0.02 * mass, 0.0);

  body.initialize(-10000.0, -500.0, 1000.0, 0.0, 0.0, 0.0,
                  0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

#ifdef EULER
  // Euler integration
  for (int ii = 0; ii < 10; ii++) {
    body.zeroForces();
    tmp[0] = 0.0*3500.0;
    body.applyBodyForce(tmp, cgpoint);

    body.derivative(xd);
    cout << "d: " << xd.transpose() <<endl;
    xd = xd * dt;
    body.changeState(xd);
    cout << "x:" << body.X().transpose()<<endl;
  }
#elif defined (IRK)
  Vector spec(6);
  PulsedBody b;
  RungeKuttaWorkspace ws(b.X().size());
  for (int ii = 0; ii < 10; ii++) {
    integrate_rungekutta(b, ws, dt);
    cout << b.X().transpose() << endl;
    b.specific(spec);
    cout << spec.transpose() << endl;
  }
  cout << b.A() << endl;
#elif defined (IEUL)
  PulsedBody b;
  EulerWorkspace ws(b.X().size());
  for (int ii = 0; ii < 10; ii++) {
    integrate_euler(b, ws, dt);
    cout << b.X().transpose() << endl;
  }
#endif

  return 0;

}
#endif
