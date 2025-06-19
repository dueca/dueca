/* ------------------------------------------------------------------   */
/*      item            : PulsedBody.cxx
        made by         : Rene van Paassen
        date            : 060502
        category        : header file
        description     :
        changes         : 060502 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include <RigidBody.hxx>
#include <integrate_rungekutta.hxx>
#include <iostream>

/** This is an example, using the RigidBody class as basis for
    implementation of equations of motion.

    The PulsedBody class derives from RigidBody, and applies forces on
    the RigidBody object. Using a Runge Kutta integration, the states
    of the RigidBody object are integrated numerically. */
class PulsedBody: public RigidBody
{
public:

  /** Time constant simulating the "engine" dynamics. */
  double tau_e;

  /** Size of time steps. */
  double dt;

  /** For efficiency, auxiliary variable that represents a force
      vector. Note that RigidBody uses eigen3
      http://eigen.tuxfamily.org/ for matrix and vector
      operations. Vector is a typedef in the RigidBody header file,
      and defines a shortcut to the vector class used here. */
  Vector force;

  /** Auxiliary variable, vector giving the point where the force is
      applied. */
  Vector fpoint;

  /** Auxiliary variable, vector giving gravity. */
  Vector gravity;

  /** Workspace for the RungeKutta integration */
  RungeKuttaWorkspace workspace;

  /** Constructor.

      @param dt   Time step of the update. */
  PulsedBody(double dt);

  /** Destructor. */
  ~PulsedBody();

  /** Function needed for integration. This function will be called by
      the integrate_rungekutta() function. It calculates the
      derivative of the state vector.

      @param   xd         Derivative of the state vector
      @param   dt_offset  Time offset for calculating the
                          derivative. The integration routine might
                          need the derivative of the state for time
                          steps that are a fraction of the integration
                          time step, e.g. for 0dt, 0.5dt and dt after
                          the previous integration step. */
  void derivative(VectorE& xd, double dt_offset);

  /** Function that does an integration step */
  void step();
};

PulsedBody::PulsedBody(double dt) :
  // create a RigidBody, with mass and inertia. Request one additional
  // state for the engine.
  RigidBody(100.0, 5.0, 6.0, 9.0, 0.0, 0.0, 0.0, 1),
  // time constant of the engine, 1.2 s
  tau_e(1.2),
  // time step
  dt(dt),
  // force vector is initially zero, 3 elements.
  force(Vector::Zero(3)),
  // same for the point on the body
  fpoint(Vector::Zero(3)),
  // vector for the gravitational pull
  gravity(Vector::Zero(3)),
  // workspace integration, 13 states for body, 1 for me
  workspace(14)
{
  // modify point where forces are applied, index 2 is z-coordinate
  // there is a slight offset in x coordinate
  fpoint[2] = -1.0;
  fpoint[0] = 1e-8;

  // in z-direction
  gravity[2] = 9.810665;

  // with this, construction of the PulsedBody is complete.
}

PulsedBody::~PulsedBody()
{
  // nothing to delete
}


void PulsedBody::derivative(VectorE& xd, double dt_offset)
{
  // we added one state, engine thrust. The engine thrust is state
  // variable x[13]. Calculate the derivative, using the time
  // constant, assume we have a constant setpoint of 1330 N for the thrust:
  xd[13] = (1330.0 - x[13]) / tau_e;

  // now, all other motions are calculated by the RigidBody parent.
  // use this to clear all forces from the previous step
  zeroForces();

  // apply the thrust to the body
  force.setZero();          // for clarity, all elements of force vector
                            // are zeroed here.
  force[2] = -x[13];        // force in z direction = thruster state
  applyBodyForce(force, fpoint);  // apply force on the body
  addInertialGravity(gravity);    // add gravity field

  // if we now call derivative from the RigidBody parent, the
  // remaining 13 states (0 -- 12) are calculated. Expressly call the
  // parent method, otherwise we end up calling ourselves!
  RigidBody::derivative(xd);

  // at this point, the derivative of the state vector has been put in
  // xd. Return to the calling function, most likely the integration
  // routine.
  return;
}

void PulsedBody::step()
{
  // use Runge-kutta integration on myself
  integrate_rungekutta(*this, workspace, dt);

  // just to prove that we did something
  std::cout << X() << std::endl;

  // after a call to output, things like Psi, theta and phi should be
  // calculated
  output();
  std::cout << "psi=" << psi() << " theta=" << theta() << " phi=" << phi()
       << " V=" << V() << " alpha=" << alpha() << " beta=" << beta() << std::endl;

  // specific forces. Note that it is really inefficient (especially
  // for a real-time program) to be creating and removing vector sp
  // for each step.
  Vector sp(6);
  specific(sp);
  std::cout << sp << std::endl;
}


int main()
{
  // main function
  PulsedBody pb(0.1);

  for (int ii = 1000; ii--; ) {
    pb.step();
  }
  return 0;
}
