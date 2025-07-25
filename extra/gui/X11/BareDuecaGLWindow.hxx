/* ------------------------------------------------------------------   */
/*      item            : BareDuecaGLWindow.hxx
        made by         : Rene van Paassen
        date            : 121214
        category        : header file
        description     :
        changes         : 121214 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef BareDuecaGLWindow_hxx
#define BareDuecaGLWindow_hxx

#include <dueca_ns.h>
#include <vector>
#include <string>

DUECA_NS_START;

/** Advance definition */
struct BareDuecaGLWindow_XWindowData;

/** This class provides a barebones, simple GL canvas for you to draw
    on, as an alternative to the toolkit-integrated DuecaGLWindow. It
    will directly open a window in the native windowing environment of
    your system.

    You can use it to have a display that is synced to your graphics
    card retrace. In that case, either create a drawing class that
    derives from this BareDuecaGLWindow, or use double inheritance to
    derive a DUECA module also from this class.

    In your doCalculation routine, use the following basic
    pattern. You need two variables defined in your class,

    <ol>

    <li>claim_thread is a flag that indicates whether the
    drawing is in its own thread (a necessity for following the
    graphics card retrace) or not. Typically, you make this
    user-configurable. </li>

    <li>run_until is a TimeTickType, that is set in startModule and
    stopModule. Initialize to 0.</li>

    <li>have_to_open is a flag to remember to open the
    display. Initialize to true.</li>

    </ol>

    \code

    // at the start of your calculation loop:
    if (have_to_open) {
      canvas.openWindow();
      have_to_open = false;
    }

    // this loop is done continuously for running in a claimed thread,
    // once otherwise
    do {

      // if in an endless loop, record waiting for the next frame retrace and
      // the buffer swap
      if ( claim_thread ) {
        do_calc.logBlockingWait();
        canvas.waitSwap(); // or self->waitSwap();
        do_calc.logBlockingWaitOver();
      }

      // check the current simulation time, and determine how much time passed
      // since the clock ticked
      TimeTickType current_tick = SimTime::getTimeTick();
      int64_t frac = Ticker::single()->getUsecsSinceTick(current_tick);

      // get the latest data from the simulation
      try {
        // use StreamReaderLatest to get to your data.
        // if you can and need to, use the frac to extrapolate your data

        // calculation to get difference between writing (logical time) and
        // current time
        DataTimeSpec span(r.timeSpec().getValidityStart(), current_tick);
        double late = span.getDtInSeconds() + frac * 1e-6 + t_predict;

        // with that, update your canvas state, or your own state if
        // you inherited. For advanced simulations, use "late" to
        // extrapolate
      }

      // redraw the image
      canvas.redraw();

    }
    while (run_until >= SimTime::getTimeTick());
    \endcode

    startModule and stopModule need to be adjusted accordingly:

    \code
    void MyCode::startModule(const TimeSpec &time)
    {
      do_calc.switchOn(time);
      if (claim_thread) {
        run_until = MAX_TIMETICK;
        myalarm.requestAlarm(time.getValidityStart());
      }
    }

    // stop the module
    void MyCode::stopModule(const TimeSpec &time)
    {
      do_calc.switchOff(time);
      if (claim_thread) {
        run_until = time.getValidityEnd();
      }
    }
    /endcode

    By default run_until should be zero. myalarm and myclock give aperiodic and
    periodic activation, so in your header, add:

    \code
    PeriodicAlarm         myclock;
    AperiodicAlarm        myalarm;
    \endcode

    And depending on the mode, connect one or the other to your
    activity in your complete() method.

    \code
    if (claim_thread) {
      do_calc.setTrigger(myalarm);
    }
    else {
      do_calc.setTrigger(myclock);
    }
    \endcode

    Instead of triggering on the time of your own clock, you can of
    course also trigger on incoming data.

    Don't forget to adjust your clock's rate if you get passed a time spec

    \code
    bool MyCode::setTimeSpec(const TimeSpec& ts)
    {
      // a time span of 0 is not acceptable
      if (ts.getValiditySpan() == 0) return false;

      // specify the timespec to the activity
      myclock.changePeriodAndOffset(ts);
    }
    \endcode
*/
class BareDuecaGLWindow
{
  /** Flag to remember first use and opening */
  bool opened;

  BareDuecaGLWindow_XWindowData* my;

  /** Wait divisor */
  int         glx_sync_divisor;

  /** Wait offset */
  int         glx_sync_offset;

  /** keep the normal cursor */
  bool        keep_pointer;

  /** Window name */
  std::string name;

  /** Size and position of the window. */
  unsigned int width;

  /** Size and position of the window. */
  unsigned int height;

  /** Size and position of the window. */
  int offset_x;

  /** Size and position of the window. */
  int offset_y;

  /** Full screen flag */
  bool fullscreen;

  /** Event mask */
  unsigned long eventmask;

  /** Touch? */
  bool pass_touch;

public:

  /** Construct a GL window based on X11 interaction

      @param window_title    Title for the window
      @param pass_pointer    Pass pointer actions (clicks, release,
                             drag) through the  "mouse" callback
      @param pass_keys       Pass keypresses through the "keyboard" and
                             "special" callbacks
      @param pass_passive    Also pass passive mouse/pointer motion
      @param pointer_visible Show the pointer in the screen
      @param display_periods If nonzero, attempt to sync to the
                             display refresh, number indicates division
      @param pass_touch      Pass touch events
*/
  BareDuecaGLWindow(const char* window_title = "A DUECA GL window",
                    bool pass_pointer = false,
                    bool pass_keys = false,
                    bool pass_passive = false,
                    bool pointer_visible = false,
                    unsigned display_periods = 0,
                    bool pass_touch = false);

  /** Request full screen drawing -- or not. Can be linked to Scheme
      in a parameter table. Use this call before opening the window
      with openWindow. */
  bool setFullScreen(const bool& fs = true);

  /** Request a cursor type */
  bool selectCursor(const int& cursor) {
    keep_pointer = bool(cursor);
    return true;
  }

  /** Set the window position, at least if the window manager will
      honour this. Can be linked to Scheme in a parameter table. Use
      this call before opening the window with openWindow. */
  bool setWindow(const std::vector<int>& wpos);

  /** open the window. Take care to do this in the same thread as in
      which you will be running the drawing and waiting. */
  void openWindow();

  /** Set up the window initial position and size. Honouring of
      initial position depends on the window manager. Use this call
      before opening the window with openWindow. */
  void setWindow(int posx, int poxy, int width, int height);

  /** Set the graphics content as current. 

      Note that this is normally not needed, in the initGL, display and 
      reshape callbacks, the GC will be current. You can use this in your
      destructor, when GL objects are deleted, and you need a correct GC
      for that.

      @param do_select  When false, resets the GC */
  bool selectGraphicsContext(bool do_select=true);

  /** Destructor */
  virtual ~BareDuecaGLWindow();

public:
  /** This is called if the size of the window is changed. You might
      need to update the image set-up for a different screen
      format. */
  virtual void reshape(int x, int y);

  /** This is called whenever the display needs to be redrawn. When
      called, the appropriate display context has been made current. */
  virtual void display() = 0;

  /** This is called when the window is ready, for first-time
      set-up. DO NOT CALL THIS FUNCTION YOURSELF! Override this
      function, and when it is called, you can assume the gl code is
      possible. So creating viewports, GL lists, allocating textures
      etc. can be done in initGL. */
  virtual void initGL();

  /** Wait on the buffer swap */
  void waitSwap();

  /** Indicate that a redraw is required. This will set the context, and
      call your display routine. */
  void redraw();

  /** Called when a key is pressed, override to get key information. */
  virtual void keyboard(unsigned char key, int x, int y);

  /** Called when a function key is pressed. */
  virtual void special(int key, int x, int y);

  /** This is called whenever a mouse button event comes in */
  virtual void mouse(int button, int state, int x, int y);

  /** This is called whenever a mouse motion event comes in. */
  virtual void motion(int x, int y);

  /** This is called whenever a mouse motion event comes in, but none
      of the buttons are pressed. */
  virtual void passive(int x, int y);

  /** This is called for touch events */
  virtual void touch(unsigned finger, bool s, float x, float y);

  /** Obtain the current widget width. */
  inline int getWidth() { return width;}

  /** Obtain the current widget height. */
  inline int getHeight() {return height;}

  /** Obtain current widget x-position */
  inline int getXOffset() {return offset_x;}

  /** Obtain current widget y-position */
  inline int getYOffset() {return offset_y;}

  /** Swapbuffers is only for compatibility with DuecaGLWindow. Does
      nothing. */
  inline void swapBuffers() {}
};

DUECA_NS_END;

#endif
