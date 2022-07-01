/* ------------------------------------------------------------------   */
/*      item            : PulsedBody.hxx
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

#ifndef PulsedBody_hxx
#define PulsedBody_hxx

#ifdef PulsedBody_cxx
#endif

#include <RigidBody.hxx>
#include <integrate_rungekutta.hxx>

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
      \param dt   Time step of the update. */
  PulsedBody(double dt);

  /** Destructor. */
  ~PulsedBody();

  /** Function needed for integration. This function will be called by
      the integrate_rungekutta() function. It calculates the
      derivative of the state vector.
      \param   xd         Derivative of the state vector
      \param   dt_offset  Time offset for calculating the
                          derivative. The integration routine might
                          need the derivative of the state for time
                          steps that are a fraction of the integration
                          time step, e.g. for 0dt, 0.5dt and dt after
                          the previous integration step. */
  void derivative(VectorE& xd, double dt_offset);

  /** Function that does an integration step */
  void step();
};

#endif
