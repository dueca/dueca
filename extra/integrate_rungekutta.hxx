/* ------------------------------------------------------------------   */
/*      item            : integrate_rungekutta.hxx
        made by         : Rene van Paassen
        date            : 060427
        category        : header file
        description     : Implements the dynamics of a rigid body.
                          After applying forces and moments on the
                          body, and specifying the gravity, one can
                          ask for an integration step.
        api             : DUECA_API
        changes         : 040414 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef integrate_rungekutta_hxx
#define integrate_rungekutta_hxx

#ifndef USING_EIGEN3
#define USING_EIGEN3
#endif
#include <Eigen/Dense>

/** a vector that takes external storage */
typedef Eigen::Map<Eigen::VectorXd> VectorE;

/** \file integrate_rungekutta.hxx
    Here a template function for Runge-Kutta integration is defined. */

/** This defines a "data-pack", with room for workspace for the
    Runge-Kutta integration. Call with the correct state vector
    size. */
class RungeKuttaWorkspace
{
  /** Data space */
  double *data;

public:
  /** Work vector */
  VectorE k1;
  /** Work vector */
  VectorE k2;
  /** Work vector */
  VectorE k3;
  /** Work vector */
  VectorE k4;
  /** Work vector */
  VectorE xhold;
  /** Work vector */
  VectorE xwork;

  /** Constructor.
      \param size    Size of the state vector for integration. */
  RungeKuttaWorkspace(unsigned size);

  /** Constructor with external data.
      The data must hold 6xsize variables */
  RungeKuttaWorkspace(double *data, unsigned int size);

  /** Destructor. */
  ~RungeKuttaWorkspace();
};

/** This function applies one Runge Kutta integration step to the state
    given in the kinematics argument. The forces, moments and
    gravitation applied by the forcer are taken into account.

    The template parameter needs to stick to the following signature:
    \code
    class MOD {
      // calculate derivative for current time + dt
      void derivative(VectorE& xd, double dt);

      // return state
      const VectorE& X() const;

      // set state
      void setState(const VectorE& newx);
    };
    \endcode
    \param model        State-carrying object, one that can calculate its
                        derivative.
    \param ws           workspace for the integration.
    \param dt           Time step of the integration. */
template <class MOD>
void integrate_rungekutta(MOD &model, RungeKuttaWorkspace &ws, double dt)
{
  // first step
  // k1 = T f(x, t)
  model.derivative(ws.k1, 0.0);    // derivative at time k
  ws.xhold = model.X();            // remember current state

  // second step
  // k2 = T f(x+beta1*k1, t+alpha1*T);  beta1=0.5 alpha1=0.5
  ws.xwork = ws.xhold + 0.5 * dt * ws.k1; // prediction k + 0.5dt
  model.setState(ws.xwork);        // make step
  model.derivative(ws.k2, 0.5*dt); // derivative at time k + 0.5dt

  // third step, improvement
  // k3 = T f(x+beta2*k1+beta3*k2, t+ alpha2*T) beta2=0 beta3=0.5 alpha2=0.5
  ws.xwork = ws.xhold + 0.5 * dt * ws.k2; // new pred k+0.5dt
  model.setState(ws.xwork);        // make step
  model.derivative(ws.k3, 0.5*dt); // derivative 2 at time k + 0.5dt

  // step 4
  // k4 = T f(x+beta4*k1+beta5*k2+beta6*k3, t+alpha3*T)beta6=1 alpha3=1, rest 0
  ws.xwork = ws.xhold + dt * ws.k3; // prediction k + dt
  model.setState(ws.xwork);        // make step
  model.derivative(ws.k4, dt);     // derivative at time k + dt

  // now xrk = x(t) + gamma1*k1 + gamma2*k2 + gamma3*k3 + gamma4*k4
  const double gamma1_4 = 1.0/6.0;
  const double gamma2_3 = 1.0/3.0;
  ws.xwork = ws.xhold + dt * (gamma1_4 * (ws.k1 + ws.k4) + gamma2_3 * (ws.k2 + ws.k3));

  // and update the state
  model.setState(ws.xwork);
}

#endif
