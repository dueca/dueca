/* ------------------------------------------------------------------   */
/*      item            : Interval.hxx
        made by         : Rene van Paassen
        date            : 010402
        category        : header file
        description     :
        changes         : 010402 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Interval_hxx
#define Interval_hxx

#ifdef Interval_cxx
#endif

#include <dueca-conf.h>

#include <Eigen/Dense>

// a normal matrix, allocates its own storage
typedef Eigen::MatrixXd Matrix;
// a matrix that takes external storage
typedef Eigen::Map<Eigen::MatrixXd> MatrixE;
// a normal vector, allocates its own storage
typedef Eigen::VectorXd Vector;
// a vector that takes external storage
typedef Eigen::Map<Eigen::VectorXd> VectorE;

#include <dueca_ns.h>

DUECA_NS_START

/** My home-brewn versions of parallel secant solution
    Assumes some unkown but continuous function y[] of a variable x. */
class Interval
{
  /** Low and high values of the x variable. This is the initial
      interval. */
  double lowx, highx;

  /** The function is measured at these points */
  Vector x;

  /** And these were the function values. */
  Matrix y;

  /** For collaborating in a query with other intervals, remember the
      offset. */
  int query_offset;

public:
  /** Constructor. */
  Interval(double lowx, double highx, int ny, int query_offset);

  /** Destructor. */
  ~Interval();

  /** Shrinks the interval. Uses a margin in y value to shrink the
      interval, returns true if successfull, false if not. */
  bool shrink(double sspeed, Vector& ymix);

  /** Get the x value needed for evaluation. */
  double getX(int ii);

  /** Get the range of the interval. */
  void getRange(Vector& y);

  /** Get the evaluation results */
  bool mergeResult(int ii, Vector& y_in);

  /** Get the current best value */
  void getResult(Vector& y);

private:
  /** Calculate a new range. */
  void newRange(double range);
};
DUECA_NS_END

#endif
