/* ------------------------------------------------------------------   */
/*      item            : GLSweeper.hxx
        made by         : repa
        from template   : DuecaModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Tue Nov 29 17:12:56 2005
        category        : header file
        api             : DUECA_API
        description     :
        changes         : Tue Nov 29 17:12:56 2005 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GLSweeper_hxx
#define GLSweeper_hxx

// include the dusime header
#include <dueca.h>
#include <PeriodicAlarm.hxx>

DUECA_NS_START;

/*# Forward declaration */
class WindowingProtocol;
/*# Forward declaration */
class GLSweeper;

/*# Return the single sweeper implementation. */
const GLSweeper* GLSweeper_single();


/** Object that takes care of initializing GL interfacing code in a
    specific thread, and calling swapbuffers. If you use this method
    of GL programming, all gl windowing code must be in the same
    thread. The GLSweeper module must run last in this thread, you can
    achieve this by giving it a negative (the lowest) value for the
    "order" of its PrioritySpec.

    This only works reliably when you don't use any GL code
    from other threads (so no gtk or other windows). If you do use
    windowing code, combine *all* windowing code in one thread,
    normally priority 0.

    I you have no other windowing code, you can use a priority other
    than 0 for the sweeper. To create this priority, specify something
    like the following in your dueca.cnf file, e.g. with

    \verbatim
    (dueca-list
     (make-environment
       'multi-thread #t
       'priority-nice 0         ; becomes priority level 1, non-realtime
       'priority-fifo 10 11 12  ; become priority levels 2, 3, 4, realtime
       ...
     )
    )
    \endverbatim
    Note that the priority specifications replace the highest-priority
    variable. In this example the highest priority level becomes 4,
    and you need to specify this when making the ticker.

    Here an piece of example configuration for dueca.mod:

    \verbatim
    ;; NOTE that this is only allowed when not using a GL
    ;; interface for the priority 0 thread!!!!!
    (define opengl-priority      (make-priority-spec 1     0))
    (define opengl-priority-last (make-priority-spec 1   -99))
    \endverbatim

    If you want to use the sweeper for testing and development
    purposes (solo, single machine, etc.), use priority level 0

    \verbatim
    (define opengl-priority      (make-priority-spec 0     0))
    (define opengl-priority-last (make-priority-spec 0   -99))
    \endverbatim

    The rest of the configuration is as follows. Remember to first
    create the sweeper, otherwise the gl using module will use the gl
    facilities of your default graphics toolkit (or find none at all).

    \verbatim
    ;; in the entity creation list .....
    (list
     ;; make the GL sweeper. With the priority, it will come in the
     ;; graphics thread, but as last one, after all
     (make-module 'gl-sweeper "" opengl-priority-last

                  ;; some parameters that control the timing.
                  'set-timing opengl-last
                  'check-timing 15000 20000

                  ;; a set of parameters that specify the type of GL display
                  'double-buffer #t
                  'alpha-buffer #t
                  'depth-buffer-size 16
                  'stencil-buffer-size 0

                  ;; specify the toolkit that runs this GL code
                  ;; this example uses glut, check DUECA's startup blurp
                  ;; to see which protocols are available
                  'set-protocol
                    (make-glut-protocol)
                  )

     ;; your GL modules.
     (make-module 'my-opengl-module "" opengl-priority
                  'set-timing opengl-timing
                  ;; additional options to your liking)
                  )
    \endverbatim

    A final note is on the timing. Ideally, the sweeper runs after you
    have received new data in your module. This might be tricky, and
    you might need some tweaking. The sweeper by default runs on the
    clock, and, although you specify a priority that is *after* the
    priority of the module, the sweeper may have been scheduled and
    started by the time your module is scheduled, and then it will run
    first. Try tweaking the timing to get things right.

 */
class GLSweeper: public Module
{
private: // simulation data
  // declare the data you need in your simulation
  /** request double buffering */
  bool double_buffer;

  /** Request an alpha buffer. */
  bool alpha_buffer;

  /** Minimum size for the depth buffer. */
  int depth_buffer_size;

  /** Minimum size for the stencil buffer. */
  int stencil_buffer_size;

  /** Priority */
  int priority;

  /** Protocol. */
  WindowingProtocol *protocol;

private: // channel access
  // none

private: // activity allocation
  /** Timer. */
  PeriodicAlarm          clock;

  /** Callback object for simulation calculation. */
  Callback<GLSweeper>  cb1;

  /** Activity for simulation calculation. */
  ActivityCallback       do_calc;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const           classname;

  /** Return the parameter table. */
  static const ParameterTable*       getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  GLSweeper(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~GLSweeper();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

private: // member functions for cooperation with DUECA
  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec& ts);

  /** Request check on the timing. */
  bool checkTiming(const vector<int>& i);

  /** Specify a certain windowing protocol */
  bool setProtocol(ScriptCreatable& prot, bool dir_in);

  /** indicate that everything is ready. */
  bool isPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

private: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doCalculation(const TimeSpec& ts);

public:
  /** query double buffering.
      \returns true if double buffer used */
  inline bool getDoubleBuffer() const { return double_buffer; }

  /** query alpha buffer
      \returns true if alpha buffer used */
  inline bool getAlphaBuffer() const { return alpha_buffer; }

  /** get minimum depth buffer size.
      \returns The number of bits in the depth buffer. */
  inline int getDepthBufferSize() const { return depth_buffer_size; }

  /** get minimum stencil buffer size.
      \returns The number of bits in the stencil buffer */
  inline int getStencilBufferSize() const { return depth_buffer_size; }




};


#endif

DUECA_NS_END
