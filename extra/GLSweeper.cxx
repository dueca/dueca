/* ------------------------------------------------------------------   */
/*      item            : GLSweeper.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Tue Nov 29 17:12:56 2005
        category        : body file
        description     :
        changes         : Tue Nov 29 17:12:56 2005 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define GLSweeper_cxx
// include the definition of the module class
#include "GLSweeper.hxx"

# include <dueca-conf.h>

// include the debug writing header, by default, write warning and
// error messages
#define W_MOD
#define E_MOD
#define E_CNF
#include <debug.h>

// include additional files needed for your calculation here
#include <Environment.hxx>
#include <GuiHandler.hxx>
#include <OpenGLHelper.hxx>
#include <WindowingProtocol.hxx>


// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#define NO_TYPE_CREATION
#include <dueca.h>
#include <MemberCall2Way.hxx>


DUECA_NS_START;

// pointer to the only sweeper that may exist. If NULL, DuecaGLWindow
// will not function
static GLSweeper* static_GLSweeper_single = NULL;
const GLSweeper* GLSweeper_single()
{
  return static_GLSweeper_single;
}

// class/module name
const char* const GLSweeper::classname = "gl-sweeper";

// Parameters to be inserted
const ParameterTable* GLSweeper::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<GLSweeper,TimeSpec>
        (&GLSweeper::setTimeSpec), set_timing_description },

    { "check-timing",
      new MemberCall<GLSweeper,vector<int> >
      (&GLSweeper::checkTiming), check_timing_description },

    { "double-buffer",
      new VarProbe<GLSweeper,bool>
      (REF_MEMBER(&GLSweeper::double_buffer)),
      "Request double buffering for the windows" },

    { "alpha-buffer",
      new VarProbe<GLSweeper,bool>
      (REF_MEMBER(&GLSweeper::alpha_buffer)),
      "Request an alpha buffer (transparency) for the windows" },

    { "depth-buffer-size",
      new VarProbe<GLSweeper,int>
      (REF_MEMBER(&GLSweeper::depth_buffer_size)),
      "Minimum size for the depth buffer" },

    { "stencil-buffer-size",
      new VarProbe<GLSweeper,int>
      (REF_MEMBER(&GLSweeper::stencil_buffer_size)),
      "Minimum size for the stencil buffer" },

    { "set-protocol",
      new MemberCall2Way<GLSweeper,ScriptCreatable>
      (&GLSweeper::setProtocol),
      "Specify a helper class that handles the windowing protocol. Protocols\n"
      "for Glut and Glui-gui exist"},

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL,
      "This module can be used to run GL windows (glut or glui driven, \n"
      "currently) can be run a priority other 0. Create the gl-sweeper, then\n"
      "your GL window modules." }
  };

  return parameter_table;
}

// constructor
GLSweeper::GLSweeper(Entity* e, const char* part, const
                     PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments.
     If you give a NULL pointer instead of the inco table, you will not be
     called for trim condition calculations, which is normal if you for
     example implement logging or a display.
     If you give 0 for the snapshot state, you will not be called to
     fill a snapshot, or to restore your state from a snapshot. Only
     applicable if you have no state. */
  Module(e, classname, part),

  // initialize the data you need in your simulation
  double_buffer(true),
  alpha_buffer(false),
  depth_buffer_size(16),
  stencil_buffer_size(0),
  priority(ps.getPriority()),
  protocol(NULL),

  // activity initialization
  clock(TimeSpec(0, 1)),
  cb1(this, &GLSweeper::doCalculation),
  do_calc(getId(), "GL callbacks + redraw", &cb1, ps)
{
  if (static_GLSweeper_single == NULL) {
    static_GLSweeper_single = this;

    // connect the triggers for simulation
    do_calc.setTrigger(clock);
  }
}

bool GLSweeper::complete()
{
  if (protocol == NULL) {
    /* DUECA extra.

       No gui specified. */
    E_CNF("you should specify a valid protocol");
    return false;
  }

  static const string pname(protocol->getName());
  if (OpenGLHelper::all().find(pname) == OpenGLHelper::all().end()) {
    /* DUECA extra.

       No gui specified is not compatible with GLSweeper. Check
       arguments to environment in dueca.cnf/dueca_cnf.py. */
    E_CNF("protocol " << pname << " not configured");
    return false;
  }
  else {
    if (!OpenGLHelper::all()[pname]->setSweeper(priority)) {
      /* DUECA extra.

         Failed to install GL sweeper. Is a second sweeper
         attempted? */
      E_XTR("Cannot claim sweeper protocol " << pname);
      return false;
    }
  }

  /* Initialize the protocol */
  protocol->init();

  // check that there is only one
  if (GLSweeper_single() != this) {
    /* DUECA extra.

       Sweeper must be a singleton. */
    E_CNF(getId() << classname << " only one allowed");
    return false;
  }

  // always on module
  do_calc.switchOn();

  return true;
}

bool GLSweeper::setProtocol(ScriptCreatable& prot, bool dir_in)
{
  // check direction
  if (!dir_in) return false;

  // try a dynamic cast
  protocol = dynamic_cast<WindowingProtocol*> (&prot);
  if (protocol == NULL) {
    /* DUECA extra.

       No protocol configured for sweeper. */
    E_CNF("must supply a windowing protocol");
    return false;
  }
  return true;
}

// destructor
GLSweeper::~GLSweeper()
{
  if (protocol) protocol->close();
}

// as an example, the setTimeSpec function
bool GLSweeper::setTimeSpec(const TimeSpec& ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0) return false;

  // specify the timespec to the activity
  clock.changePeriodAndOffset(ts);

  // return true if everything is acceptable
  return true;
}

// and the checkTiming function
bool GLSweeper::checkTiming(const vector<int>& i)
{
  if (i.size() == 3) {
    new TimingCheck(do_calc, i[0], i[1], i[2]);
  }
  else if (i.size() == 2) {
    new TimingCheck(do_calc, i[0], i[1]);
  }
  else {
    return false;
  }
  return true;
}

// tell DUECA you are prepared
bool GLSweeper::isPrepared()
{
  // return result of check
  return true;
}

// start the module
void GLSweeper::startModule(const TimeSpec &time)
{
  // do_calc.switchOn(time - period);
}

// stop the module
void GLSweeper::stopModule(const TimeSpec &time)
{
  // do_calc.switchOff(time);
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void GLSweeper::doCalculation(const TimeSpec& ts)
{
  protocol->sweep();
}


DUECA_NS_END;
