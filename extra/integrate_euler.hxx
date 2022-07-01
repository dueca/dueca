/* ------------------------------------------------------------------   */
/*      item            : integrate_euler.hxx
        made by         : Rene van Paassen
        date            : 060427
        category        : header file
        description     : Implements the dynamics of a rigid body.
                          After applying forces and moments on the
                          body, and specifying the gravity, one can
                          ask for an integration step.
        api             : DUECA_API
        changes         : 040414 first version
                  : 140207 Changed matrix library to eigen3
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef integrate_euler_hxx
#define integrate_euler_hxx

#ifndef USING_EIGEN3
#define USING_EIGEN3
#endif

#include <Eigen/Dense>

/** a vector that takes external storage */
typedef Eigen::Map<Eigen::VectorXd> VectorE;

/** \file integrate_euler.hxx
    Here a template function for Euler integration is defined. */

/** This defines a "data-pack", with room for workspace for the
    Euler integration below. Call with the correct state vector
    size. */
class EulerWorkspace
{
  /** Data space */
  double *data;

public:
  /** Eigen vectors.  */
  VectorE xd;

  /** Constructor.
      \param size    Size of the state vector for integration. */
  EulerWorkspace(unsigned int size);

  /** Constructor with external data.
      The data must hold 1xsize variables */
  EulerWorkspace(double *data, unsigned int size);

  /** Destructor. */
  ~EulerWorkspace();
};


/** This function applies one Euler integration step to the state
    given in the kinematics argument. The forces, moments and
    gravitation applied by the forcer are taken into account. Note
    that Euler integration steps are really simple, and in practice
    rather coarse. This function is meant to be a reference
    implementation, normally you would want a more sophisticated
    integration method like Runge Kutta.

    The template parameter needs to stick to the following signature:
    \code
    class MOD {
      // calculate derivative for current time + dt
      void derivative(VectorE& xd, double dt);

      // return state
      const Vector& X() const;

      // set state
      void setState(const VectorE& newx);
    };
    \endcode

    \param model        State-carrying object, that can calculate its
                        derivative.
    \param ws           Workspace.
    \param dt           Time step of the integration. */
template <class MOD>
void integrate_euler(MOD &model, EulerWorkspace &ws, double dt)
{
  // calculate the derivative, and update the state
  model.derivative(ws.xd, 0.0);
  ws.xd *= dt;
  model.changeState(ws.xd);
}

#endif
