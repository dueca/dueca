/* ------------------------------------------------------------------   */
/*      item            : IntervalCalculation.cxx
        made by         : Rene' van Paassen
        date            : 010402
        category        : body file
        description     :
        changes         : 010402 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define IntervalCalculation_cxx
#include "IntervalCalculation.hxx"

#ifdef BUILD_DMODULES
#define E_TRM
#define W_TRM
#define I_TRM
#define D_TRM
#include <debug.h>

#include <iostream>
#include <cmath>
#include <iomanip>
#include <debprint.h>

PRINT_NS_START

ostream& operator << (ostream& os, const Matrix& x)
{
  for (unsigned int ii = 0; ii < x.rows(); ii++) {
    for (unsigned int jj = 0; jj < x.cols(); jj++) {
      os << setw(12) << x(ii,jj);
    }
    os << endl;
  }
  return os;
}
PRINT_NS_END

DUECA_NS_START

IntervalCalculation::IntervalCalculation() :
  sspeed(0.6),
  current_query(0),
  cycle(0),
  eval_counter(0),
  n_out(0)
{
  //
}

IntervalCalculation::~IntervalCalculation()
{
  //
}

void IntervalCalculation::initialise(const Vector& xmin,
                                     const Vector& xmax,
                                     int n_out)
{
  //  assert(xmin.Nrows() == xmax.Nrows());

  intervals.clear();

  for (unsigned int ii = 0; ii < xmin.size(); ii++) {
    intervals.push_back(Interval(xmin[ii], xmax[ii], n_out, ii*3));
  }

  this->n_out = n_out;
  eval_counter = 0;
}

int IntervalCalculation::needEvaluation(Vector& x)
{
  // return false if enough evaluations sent out
  if (eval_counter > int(intervals.size()) * 3)
    return -1;

  // fill the x vector with evaluation points
  for (int ii = intervals.size(); ii--; ) {
    x[ii] = intervals[ii].getX(eval_counter);
  }

  // update the evaluation counter, remember its value for returning
  // to the client
  int to_return = eval_counter++;

  // avoid evaluating the center points n times
  if (eval_counter > 3 && eval_counter % 3 == 1) eval_counter++;

  return to_return;
}

void IntervalCalculation::mergeResult(int eval, Vector& y_in)
{
  for (int ii = intervals.size(); ii--; ) {
    intervals[ii].mergeResult(eval, y_in);
  }
}


void IntervalCalculation::step()
{
  // get the ranges from all intervals
  Matrix yrange(intervals.size(), n_out);
  for (int ii = intervals.size(); ii--; ) {
    Vector yr(3);
    intervals[ii].getRange(yr);
    yrange.row(ii) = yr;
  }
  DEB1("range of Y\n" << yrange);

  // normalise the range. Row vectors of the range contain the effects
  // of all controls on one single target. These rows are normalised.
  for (int ii = n_out; ii--; ) {
    double nrm = yrange.col(ii).squaredNorm();
    yrange.col(ii) *= 1.0/nrm;
  }
  DEB1("Normalised range\n" << yrange);

  // let all the intervals shrink.
  int nshrink = 0;
  for (int ii = intervals.size(); ii--; ) {
    Vector yc(yrange.cols());
    yrange.row(ii) = yc;

    if (intervals[ii].shrink(sspeed, yc)) {
      nshrink++;
    }
  }
  DEB1("Shrinking " << nshrink << " out of " << intervals.size() <<
       " intervals");

  // need new evaluations
  eval_counter = 0;
}

void IntervalCalculation::getResult(Vector& y)
{
  intervals[0].getResult(y);
}
DUECA_NS_END

#endif
