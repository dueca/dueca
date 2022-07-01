/* ------------------------------------------------------------------   */
/*      item            : PulsedBody.cxx
        made by         : Rene' van Paassen
        date            : 060502
        category        : body file
        description     :
        changes         : 060502 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define PulsedBody_cxx
#include "PulsedBody.hxx"
#include <iostream>

using namespace std;

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
  cout << X() << endl;

  // after a call to output, things like Psi, theta and phi should be
  // calculated
  output();
  cout << "psi=" << psi() << " theta=" << theta() << " phi=" << phi()
       << " V=" << V() << " alpha=" << alpha() << " beta=" << beta() << endl;

  // specific forces. Note that it is really inefficient (especially
  // for a real-time program) to be creating and removing vector sp
  // for each step.
  Vector sp(6);
  specific(sp);
  cout << sp << endl;
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
