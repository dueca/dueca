/* ------------------------------------------------------------------   */
/*      item            : integrate_euler.cxx
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

#include "integrate_euler.hxx"

EulerWorkspace::EulerWorkspace(unsigned size) :
  data(new double[size]),
  xd(data, size)
{
  //
}

EulerWorkspace::EulerWorkspace(double *data, unsigned int size) :
  data(NULL),
  xd(data, size)
{
  //
}

EulerWorkspace::~EulerWorkspace()
{
  delete[] data;
}
