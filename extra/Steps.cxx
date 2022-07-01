/* ------------------------------------------------------------------   */
/*      item            : Steps.cxx
        made by         : Rene' van Paassen
        date            : 030228
        category        : body file
        description     :
        changes         : 030228 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define Steps_cxx
#define DO_INSTANTIATE
#include "Steps.hxx"
#include <exception>

class steps_exception: public std::exception
{
public:
  /** Re-implementation of std:exception what. */
  const char* what() const throw() {return "steps need 2 or more values"; }
};


void Steps_sort_and_copy(double *ui, double *yi,
                         double *tp, double *y, int n)
{
  // creating this with n < 2 is nonsense, and dangerous
  if (n < 2) {
    throw(steps_exception());
  }

  // for efficient look-up, values must be sorted in increasing
  // order. Start with a simple bubble sort
  int order[n];
  for (int ii = n; ii--; ) order[ii] = ii;

  bool sorted = false;
  while (!sorted) {
    sorted = true;
    for (int ii = n; --ii; ) {
      if (ui[order[ii]] < ui[order[ii-1]]) {
        sorted = false;        int tmp = order[ii];
        order[ii] = order[ii-1]; order[ii-1] = tmp;
      }
    }
  }

  // copy the result, calculate the transition points
  for (int ii = n; --ii; ) {
    y[ii] = yi[order[ii]];
    tp[ii-1] = 0.5*(ui[order[ii]] + ui[order[ii-1]]);
  }
  y[0] = yi[order[0]];
}
