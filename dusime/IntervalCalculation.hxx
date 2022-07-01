/* ------------------------------------------------------------------   */
/*      item            : IntervalCalculation.hxx
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

#ifndef IntervalCalculation_hxx
#define IntervalCalculation_hxx

#ifdef IntervalCalculation_cxx
#endif

#include <vector>
#include <Interval.hxx>
using namespace std;
#include <dueca_ns.h>
DUECA_NS_START

/** Class that implements a sort of pseudo interval technology,
    finding an optimum/zero for a function. */
class IntervalCalculation
{
  /** The contraction speed for the intervals. Make it too large, and
      you will always miss, make it too small, and convergence takes
      forever. Take your pick. */
  double sspeed;

  /** A counter that permits the ordered query of all interval
      evaluation requests. */
  int current_query;

  /** The cycle in the current calculation. */
  int cycle;

  int eval_counter;

  /** Number of observed outputs that are used in the calculation. */
  int n_out;

  /** The intervals themselves. */
  vector<Interval> intervals;

public:
  /** Constructor. */
  IntervalCalculation();

  /** Destructor. */
  ~IntervalCalculation();

  /** Initialise a new calculation.
      \param xmin   Minimum value of controls.
      \param xmax   Maximum value of controls.
      \param n_out  Number of observed variables. */
  void initialise(const Vector& xmin,
                  const Vector& xmax, int n_out);

  /** Take the next step in the iterative process. */
  void step();

  /** Returns true, and with a filled "x" vector, when an evaluation
      is needed. */
  int needEvaluation(Vector& x);

  /** Insert the result of a query in the calculation. Invocation of
      this routine should be paired with the needEvaluation
      invocation, i.e. if 20 evaluations are requested, these are
      done, and then resultQuery is called 20 times with the results. */
  void mergeResult(int eval, Vector& y);

  /** Get the current results. */
  void getResult(Vector& y);
};
DUECA_NS_END
#endif
