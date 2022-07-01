/* ------------------------------------------------------------------   */
/*      item            : integrate_rungekutta.cxx
        made by         : Rene van Paassen
        date            : 060427
        category        : header file
        description     : Implements the dynamics of a rigid body.
                          After applying forces and moments on the
                          body, and specifying the gravity, one can
                          ask for an integration step.
        api             : DUECA_API
        changes         : 040414 first version
                          090629 Added option for external (on stack?)
                          data hold
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "integrate_rungekutta.hxx"

RungeKuttaWorkspace::RungeKuttaWorkspace(unsigned size) :
  data(new double[6*size]),
  k1(data, size),
  k2(&data[size], size),
  k3(&data[2*size], size),
  k4(&data[3*size], size),
  xhold(&data[4*size], size),
  xwork(&data[5*size], size)
{
  //
}

RungeKuttaWorkspace::RungeKuttaWorkspace(double *data, unsigned int size) :
  data(NULL),
  k1(data, size),
  k2(&data[size], size),
  k3(&data[2*size], size),
  k4(&data[3*size], size),
  xhold(&data[4*size], size),
  xwork(&data[5*size], size)
{
  //
}

RungeKuttaWorkspace::~RungeKuttaWorkspace()
{
  delete[] data;
}
