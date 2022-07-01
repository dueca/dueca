/* ------------------------------------------------------------------   */
/*      item            : IncoVariableWork.hxx
        made by         : Rene van Paassen
        date            : 200527
        category        : header file
        description     :
        changes         : 200527 first version
        language        : C++
        copyright       : (c) 20 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef IncoVariableWork_hxx
#define IncoVariableWork_hxx

#include <dueca/dueca_ns.h>
#include <dusime/IncoVariable.hxx>

DUECA_NS_START;

class IncoVariableWork: public IncoVariable
{
  double value;

  double target;
public:
  /** Constructor */
  IncoVariableWork();

  /** Constructor */
  IncoVariableWork(const IncoVariable& o);

  /** Destructor */
  ~IncoVariableWork();

  /** Merge an inco result */
  bool merge(IncoMode mode, double value);

  /** Return value */
  inline double getValue() const { return value; }

  /** Return value */
  inline void setValue(double v) { value = v; }

  /** Return target */
  inline double getTarget() const { return target; }

  /** Return value */
  void setTarget(double v);

  bool meetsTarget(IncoMode mode) const;
  bool isUserControllable(IncoMode mode) const;

  /** Reset from self */
  IncoVariableWork& operator = (const IncoVariable& );
};

DUECA_NS_END;

#endif
