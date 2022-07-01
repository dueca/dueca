/* ------------------------------------------------------------------   */
/*      item            : Integrator.hxx
        made by         : Joost Ellerbroek
        date            : 090515
        category        : header file
        description     :
        changes         : 090515 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef Integrator_hxx
#define Integrator_hxx
#include "LimitedLinearSystem.hxx"

/** This class creates a single integrator, if requested with limits on its state.
  *
  */
class Integrator : public LimitedLinearSystem
{
public:
  /// Default constructor
  Integrator(double dt = 0.01);

  /// Constructor for Integrator with limits
  Integrator(double lower_limit, double upper_limit, double dt = 0.01);

  /// Destructor
  ~Integrator();

  /** (Re-)discretize the integrator to match timestep size 'dt'
      \param dt The timestep size that should be used to discretize the integrator. */
  void setDT(double dt);

  /** Set the limits on the states */
  void setSaturationLimits(const Vector& lower, const Vector& upper);

  /// Set the limits for this integrator
  void setSaturationLimits(double lower_limit, double upper_limit);

  /** Accept a new state
      \param x_new  Vector with the new state. */
  void acceptState(const Vector& x_new);

  /** Accept a new state
      \param x_new  Vector with the new state. */
  void acceptState(double x_new);

  /** Calculate a single time step, return the output vector.
      \param u      Input vector. */
  const Vector& step(const Vector& u);

  /** Calculate a single time step, return the output vector.
      \param u      Input variable. Only valid for SI systems. */
  const Vector& step(double u);
};

#endif
