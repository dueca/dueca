/* ------------------------------------------------------------------   */
/*      item            : SimulationModelTemplate.hxx
        made by         : Rene van Paassen
        date            : 030107
        category        : header file
        description     :
        changes         : 030107 first version
        language        : C++
        copyright       : (c)
*/

#ifndef SimulationModelTemplate_hxx
#define SimulationModelTemplate_hxx

template<class T> Integrator;

class @Module@
{
  friend class Integrator<@Module@>;

  // state size
  const int nu, nx, ny, ncx;

  // room for the state
  double x1[nx], x2[nx], x3[nx];

  // room for the derivatives
  double dx[nx];

  // room for the input
  double u[nu];

  // output vector
  double y[ny];

public:

  // do an update step for the model, updates continuous states
  // (0 .. ncx -1) via an integration routine, and the discrete ones
  // with a discrete update
  void discreteStep();

  // calculate the output vector
  void calculateY();

  // calculate the derivative of the state vector.
  void calculateDerivatives();
};
#endif
