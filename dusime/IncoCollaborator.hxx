/* ------------------------------------------------------------------   */
/*      item            : IncoCollaborator.hxx
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

#ifndef IncoCollaborator_hxx
#define IncoCollaborator_hxx

#ifdef IncoCollaborator_cxx
#endif

#include <IncoSpec.hxx>
#include <IncoNotice.hxx>
#include "Callback.hxx"
#include <CallbackWithId.hxx>
#include "Activity.hxx"
#include "IncoCalculator.hxx"
#include "IncoVariableWork.hxx"
#include <dueca/ChannelReadToken.hxx>
#include <dueca/ChannelWriteToken.hxx>
#include <cmath>
#include <cstdlib>
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

/** Objects of this class acts as a specification for a "player" in
    the trim condition calculation.

    For each module that reports that it wants to have a part in the
    trim calculation (basically every SimulationModule with a non-null
    IncoTable), an IncoCollaborator is created. The IncoCollaborator
    keeps access tokens for the channels that are used in
    communication with the specified module, and it keeps the inco
    specification that the IncoCalculator works with. */
class IncoCollaborator
{
  /** Initial condition specification for one module. It contains the
      full name of the module, and the list of trim calculation
      variables. */
  IncoSpec          specification;

  /** Work variables */
  std::vector<IncoVariableWork>  table;

  /** An offset counter, gives the number of inco variables by
      collaborators in front of this one (list-wise). */
  unsigned int      offset;

  /** A mark counter, which helps to keep all collaborators in
      line. */
  TimeTickType      mark;

  /** The results from calculation (only the targets). Several results
      may come in in a block, before these are needed (several
      calculations can be requested in one go). Therefore a list of
      results is kept. */
  list< vector<double> > results;

  /** Access token for receiving trim calculation results. */
  ChannelReadToken                t_inco_feedback;

  /** Access token for writing trim calculation input. */
  ChannelWriteToken               t_inco_control;

  /** A callback to a member function of the IncoCalculator class. A
      callback to this class is not possible, an object needs to be a
      "NamedObject" in order to have callbacks */
  CallbackWithId<IncoCalculator, IncoCollaborator*> cb;

  /** An activity for processing trim calculation results. */
  ActivityCallback                    process;

public:
  /// Constructor, makes a copy of the inco spec
  IncoCollaborator(const IncoSpec& spec, IncoCalculator* calculator,
                   unsigned int offset);

  /// Destructor
  ~IncoCollaborator();

  /** Returns the name set for the module. */
  inline NameSet getModuleName() const {return specification.module;}

  /** Returns the number of inco variables handled by this
      collaborator. */
  inline unsigned int noVariables()
  { return specification.table.size();}

  /** Return the offset, i.e. the number of the first variable. */
  inline unsigned int getOffset() { return offset;}

  /** Return a reference to an inco variable. */
  IncoVariableWork& getIncoVariable(unsigned int variable);

  /** This function handles and incoming event with trim calculation
      results. */
  bool processEvent(const TimeSpec& ts, IncoMode mode);

  /** Obtain the mark, which functions as a cycle counter. */
  TimeTickType getMark() const { return mark;}

  /** This returns true if all the targets for the calculation are
      met. It also returns true if this collaborator does not handle
      any targets. */
  bool haveTargets(IncoMode mode) const;

  /** Put the results from a calculation into a vector. */
  bool insertTargetResults(Vector& y, IncoMode mode,
                           unsigned int& idx);

  /** Count the number of targets and controls. */
  void count(IncoMode mode, unsigned int& n_targets,
             unsigned int& n_controls) const;

  /** Fill the vector with all minimum and maximum values of the
      controls. */
  void fillMinMax(IncoMode mode, unsigned int& idx,
                  Vector& xmin, Vector& xmax) const;

  /** Set out a calculation. This sends out an IncoNotice that
      triggers the trim calculation in the associated module. The
      sending is done with the time indicated in the tick. Control
      data is pulled from the vector x.
      \param mode     Mode in which the calculation takes place.
      \param tick     Tick time for sending the event.
      \param x        Vector with control information.
      \param idx      Index keeping count of the elements of vector x
                      that are already used. */
  void initiateCalculation(IncoMode mode, TimeTickType tick,
                           const Vector& x, unsigned int& idx);

  /** Return the trigger puller (channel), for inclusion in the
      triggering conditions of the IncoCalculator. */
  TriggerPuller& getTrigger() { return t_inco_feedback; }
};
DUECA_NS_END
#endif
