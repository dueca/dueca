/* ------------------------------------------------------------------   */
/*      item            : BareDuecaGLWindow.cxx
        made by         : Rene' van Paassen
        date            : 121214
        category        : body file
        description     :
        changes         : 121214 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define BareDuecaGLWindow_cxx

#include <exception>
#include <debug.h>
#include "BareDuecaGLWindow.hxx"
#include <dueca/Environment.hxx>
#include "debprint.h"

// be extremely careful with X11 headers; these have all kinds of
// damaging defines, never include these in a header used by a "client"
#define GLX_GLXEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/glu.h>
#include <X11/XKBlib.h>
#include <X11/X.h>
#include <X11/extensions/XI.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <GL/glut.h>
#include <X11/extensions/XInput2.h>

DUECA_NS_START;

static ::Display* static_dpy = NULL;
static GLXContext shared_context = NULL;
static int xi_major_opcode;
static int xi_first_event;
static int xi_first_error;

struct BareDuecaGLWindow_XWindowData {

  /** Possibility to wait for video sync */
  PFNGLXWAITVIDEOSYNCSGIPROC glXWaitVideoSyncSGI;

  /** Joining a swap group, when multiple windows. */
  PFNGLXJOINSWAPGROUPSGIXPROC glXJoinSwapGroupSGIX;

  /** Cursor */
  Cursor     cursor;

  /** GL context */
  ::GLXContext glc;

  /** X window id */
  ::Window xwin;

  BareDuecaGLWindow_XWindowData() :
    glXWaitVideoSyncSGI(NULL),
    glXJoinSwapGroupSGIX(NULL),
    xwin(0)
  {}
};

BareDuecaGLWindow::BareDuecaGLWindow(const char* window_title,
                                     bool pass_pointer,
                                     bool pass_keys,
                                     bool pass_passive,
                                     bool pointer_visible,
                                     unsigned display_periods,
                                     bool pass_touch) :
  opened(false),
  my(new BareDuecaGLWindow_XWindowData()),
  glx_sync_divisor(display_periods),
  glx_sync_offset(0),
  keep_pointer(pointer_visible),
  name(window_title),
  width(400),
  height(300),
  offset_x(-1),
  offset_y(-1),
  fullscreen(false),
  eventmask(0),
  pass_touch(pass_touch)
{
  eventmask = ExposureMask | StructureNotifyMask |
    (pass_pointer ? ButtonPressMask|ButtonReleaseMask|ButtonMotionMask : 0) |
    (pass_keys ? KeyPressMask|KeyReleaseMask : 0) |
    (pass_passive ? PointerMotionMask : 0);
}

bool BareDuecaGLWindow::setFullScreen(const bool& fs)
{
  fullscreen = fs;
  return true;
}

bool BareDuecaGLWindow::setWindow(const std::vector<int>& wpos)
{
  /* should be an assertion or even better an exception */
  if (wpos.size() != 4 && wpos.size() != 2) {
    /* DUECA extra.

       GL window size is not correctly specified. */
    W_CNF("Need four elements (width, height, x, y) for window size");
    return false;
  }

  width = max(0, wpos[0]); height = max(0, wpos[1]);
  if (wpos.size() == 4){
    offset_x = wpos[2];
    offset_y = wpos[3];
  }

  return true;
}


void BareDuecaGLWindow::setWindow(int posx, int posy, int width, int height)
{
  std::vector<int> wpos;
  wpos.push_back(width);  wpos.push_back(height);
  wpos.push_back(posx);  wpos.push_back(posy);
  setWindow(wpos);
}


BareDuecaGLWindow::~BareDuecaGLWindow()
{
  delete my;
}

struct BareDuecaGLWindowError: public std::exception
{
  /** Say what is the problem */
  const char* what() const throw() { return "cannot open X window"; }
};

static Bool WaitForNotify (Display *d, XEvent *e, char *arg)
{
  return (e->type == MapNotify) & (e->xmap.window == (Window)arg);
}


void BareDuecaGLWindow::openWindow()
{
  if (opened) return;

  if (!static_dpy) {
    static_dpy = XOpenDisplay(NULL);

    if (!static_dpy) {
      throw BareDuecaGLWindowError();
    }

    int major = 2, minor = 2;
    XIQueryVersion(static_dpy, &major, &minor);
    if (major * 1000 + minor < 2002) {
      /* DUECA extra.

         Touch capability, as requested, is not possible with this X
         version. */
      W_XTR("Server does not support XI 2.2, no touch");
      return;
    }

    // determine opcode for xinput extension
    Bool res = XQueryExtension(static_dpy, "XInputExtension", &xi_major_opcode,
                               &xi_first_event, &xi_first_error);
    if (res == False) {
      /* DUECA extra.

         Unable to determine the XInputExtension version. */
      W_XTR("Cannot determine XInputExtension opcode");
      return;
    }

  }

  // get default screen
  int screen = XDefaultScreen(static_dpy);

  // access the root window for further steps
  Window xroot = RootWindow(static_dpy, screen);

  // continue with GL
  GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER,
                  GLX_USE_GL, GLX_RED_SIZE, 8,
                  GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8,
                  GLX_ACCUM_RED_SIZE, 8, GLX_ACCUM_GREEN_SIZE, 8,
                  GLX_ACCUM_BLUE_SIZE, 8, GLX_ACCUM_ALPHA_SIZE, 8,
                  GLX_AUX_BUFFERS, 4, None };
  XVisualInfo *vi = glXChooseVisual(static_dpy, screen, att);
  if (vi == NULL) {
    // try with less options
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER,
                    GLX_USE_GL,
                    GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
                    GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8,
                    None };
    vi = glXChooseVisual(static_dpy, screen, att);
  }
  if (vi == NULL) {
    // try with less options
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER,
                    GLX_USE_GL, None };
    vi = glXChooseVisual(static_dpy, screen, att);
  }

  if (vi == NULL) {
    cerr << "Cannot find appropriate visual" << endl;
  }

  my->glc = glXCreateContext(static_dpy, vi, shared_context, GL_TRUE);
  if (shared_context == NULL && CSE.getShareGLContexts()) {
    shared_context = my->glc;
  }

  XSetWindowAttributes    swa = { };

  swa.colormap = XCreateColormap(static_dpy, xroot, vi->visual, AllocNone);
  // events for user: XMotionEvent XKeyEvent XButtonEvent
  swa.event_mask = ExposureMask | StructureNotifyMask;
  if (fullscreen) {
    swa.override_redirect = True;
    XWindowAttributes rootWinAttr;
    XGetWindowAttributes(static_dpy, xroot, &rootWinAttr);
    offset_x = rootWinAttr.x; offset_y = rootWinAttr.y;
    width = rootWinAttr.width; height = rootWinAttr.height;
  }
  my->xwin = XCreateWindow
    (static_dpy, xroot, offset_x, offset_y,
     width, height, 0, vi->depth,
     InputOutput, vi->visual, CWColormap|CWEventMask, &swa);
  XMapWindow(static_dpy, my->xwin);
  XEvent event;
  XStoreName(static_dpy, my->xwin, name.c_str());

  if (fullscreen) {
    Atom wm_state   = XInternAtom(static_dpy, "_NET_WM_STATE", true );
    Atom wm_fullscreen = XInternAtom(static_dpy,
                                     "_NET_WM_STATE_FULLSCREEN", true );

    XChangeProperty(static_dpy, my->xwin, wm_state, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *) &wm_fullscreen, 1);
  }

  // remove mouse pointer
  if (!keep_pointer) {
    static const char emptycursor_bits[6] = {0x00};
    Pixmap cursor_pixmap = XCreateBitmapFromData(static_dpy, my->xwin,
                                                 emptycursor_bits, 1, 1);
    XColor black;
    black.pixel = BlackPixel(static_dpy, screen);
    my->cursor = XCreatePixmapCursor(static_dpy, cursor_pixmap, cursor_pixmap,
                                     &black, &black, 0, 0);
    XDefineCursor(static_dpy, my->xwin, my->cursor);
    XFreePixmap(static_dpy, cursor_pixmap);
  }

  // this waits for the creation notification
  XIfEvent(static_dpy, &event, WaitForNotify,
           reinterpret_cast<char*>(my->xwin));

  // now re-set the event stuff
  XSelectInput(static_dpy, my->xwin, eventmask);

  // if the GenericEvent is in, tell xinput to pass touch data
  if (pass_touch) {

    int ndevices = 0;
    XIDeviceInfo *devinfo = XIQueryDevice
      (static_dpy, XIAllDevices, &ndevices);

    // the slave(s) that have been made floating are tested; fingers are
    // enabled
    for (int idev = 0; idev < ndevices; idev++) {
      if (devinfo[idev].use == XIFloatingSlave) {

        XIAnyClassInfo **any = devinfo[idev].classes;
        for (int ic = 0; ic < devinfo[idev].num_classes; ic++) {
          if (any[ic]->type == XITouchClass) {
            uint64_t mask = (1 << XI_TouchBegin) |
              (1 << XI_TouchUpdate) | (1 << XI_TouchEnd);
            XIEventMask emask = { devinfo[idev].deviceid, sizeof(mask),
                                  reinterpret_cast<unsigned char*>(&mask) };
            int stat = XISelectEvents
              (static_dpy, my->xwin, &emask, 1);
            switch (stat) {
            case BadValue:
              /* DUECA extra.

                 Cannot select device for XInput events.
               */
              W_XTR("Touch device XISelectEvents bad value");
              break;
            case BadWindow:
              /* DUECA extra.

                 Window for XInput events is not valid.
               */
              W_XTR("Touch device XISelectEvents invalid window");
              break;
            case XI_BadDevice:
              /* DUECA extra.

                 Device for XInput events is not valid.
               */
              W_XTR("Touch device XISelectEvents invalid device");
              break;
            }
            /* DUECA extra.

               Information on enabling a touch device. */
            I_XTR("Enabling touch device " << devinfo[idev].deviceid);
          }
        }
      }
    }
  }
  // according to valgrind (SLED11, 100129)
  // there is still an uninitialised value/cond jump for this call??
  glXMakeCurrent(static_dpy, my->xwin, my->glc);

  my->glXWaitVideoSyncSGI = reinterpret_cast<PFNGLXWAITVIDEOSYNCSGIPROC>
    (glXGetProcAddress
     (reinterpret_cast<const GLubyte*>("glXWaitVideoSyncSGI")));
  if (!(my->glXWaitVideoSyncSGI)) {
    /* DUECA extra.

       Missing glXWaitVideoSyncSGI extension, not possible to wait on
       video sync. */
    W_XTR("Missing glXWaitVideoSyncSGI extension, cannot wait on sync");
  }

#if 0
  my->glXJoinSwapGroupSGIX = reinterpret_cast<PFNGLXJOINSWAPGROUPSGIXPROC>
    (glXGetProcAddress
     (reinterpret_cast<const GLubyte*>("glXJoinSwapGroupSGIX")));
  if (!(my->glXJoinSwapGroupSGIX)) {
    /* DUECA extra.

       Missing glXJoinSwapGroupSGIX extension, not possible to join
       swap group. */
    W _XTR("Missing glXJoinSwapGroupSGIX extension, ");
  }
#endif

  /** Select sync to vblank for gl pipeline commands */
  PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT =
    reinterpret_cast<PFNGLXSWAPINTERVALEXTPROC>
    (glXGetProcAddress
     (reinterpret_cast<const GLubyte*>("glXSwapIntervalExt")));
  if (glXSwapIntervalEXT) {
    (*(glXSwapIntervalEXT))(static_dpy, my->xwin, glx_sync_divisor);
  }
  else {
    PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI =
      reinterpret_cast<PFNGLXSWAPINTERVALSGIPROC>
      (glXGetProcAddress
       (reinterpret_cast<const GLubyte*>("glXSwapIntervalSGI")));
    if (glXSwapIntervalSGI) {
      (*(glXSwapIntervalSGI))(glx_sync_divisor);
    }
    else {
      PFNGLXSWAPINTERVALMESAPROC glXSwapIntervalMESA =
        reinterpret_cast<PFNGLXSWAPINTERVALMESAPROC>
        (glXGetProcAddress
         (reinterpret_cast<const GLubyte*>("glXSwapIntervalMESA")));
      if (glXSwapIntervalMESA) {
        (*(glXSwapIntervalMESA))(glx_sync_divisor);
      }
      else {
        /* DUECA extra.

           Missing swap interval extensions (glXSwapIntervalExt,
           glXSwapIntervalSGI, glXSwapIntervalMESA), cannot control
           the swap interval. */
        W_XTR("No swap interval control possible");
      }
    }
  }

  // now call client code to initialise GL
  this->reshape(width, height);
  this->initGL();
  opened = true;
}

/** link between X keycode and "glut" special char codes */
struct XGlutCharLink
{
  /** X keycode */
  unsigned long xk_key;
  /** GLUT corresponding */
  unsigned short glut_char;
};

static const XGlutCharLink key_trans_table[] = {
  { XK_F1, GLUT_KEY_F1},
  { XK_F2, GLUT_KEY_F2},
  { XK_F3, GLUT_KEY_F3},
  { XK_F4, GLUT_KEY_F4},
  { XK_F5, GLUT_KEY_F5},
  { XK_F6, GLUT_KEY_F6},
  { XK_F7, GLUT_KEY_F7},
  { XK_F8, GLUT_KEY_F8},
  { XK_F9, GLUT_KEY_F9},
  { XK_F10, GLUT_KEY_F10},
  { XK_F11, GLUT_KEY_F11},
  { XK_F12, GLUT_KEY_F12},
  { XK_Left, GLUT_KEY_LEFT},
  { XK_Up, GLUT_KEY_UP},
  { XK_Right, GLUT_KEY_RIGHT},
  { XK_Down, GLUT_KEY_DOWN},
  { XK_Page_Up, GLUT_KEY_PAGE_UP},
  { XK_Page_Down, GLUT_KEY_PAGE_DOWN},
  { XK_Home, GLUT_KEY_HOME},
  { XK_End, GLUT_KEY_END},
  { XK_Insert, GLUT_KEY_INSERT},
  { XK_KP_Insert, GLUT_KEY_INSERT},
  { XK_KP_Home, GLUT_KEY_HOME},
  { XK_KP_Left, GLUT_KEY_LEFT},
  { XK_KP_Up, GLUT_KEY_UP},
  { XK_KP_Right, GLUT_KEY_RIGHT},
  { XK_KP_Down, GLUT_KEY_DOWN},
  { XK_KP_Page_Up, GLUT_KEY_PAGE_UP},
  { XK_KP_Page_Down, GLUT_KEY_PAGE_DOWN},
  { XK_KP_End, GLUT_KEY_END},
  { 0, 0}};

void BareDuecaGLWindow::redraw()
{
  if (not my->xwin) openWindow();

  // process events
  union {
    XEvent x_event;
    XIDeviceEvent xi_event;
  } u;
    for(;;) {
    Bool res = XCheckWindowEvent(static_dpy, my->xwin,
                                 eventmask, &u.x_event);
    if (res == False) break;
    switch (u.x_event.type) {
    case KeyRelease: {
      unsigned long kcode = XkbKeycodeToKeysym(static_dpy,
                                               u.x_event.xkey.keycode, 0, 0);
      if (kcode < 0xff) {
        this->keyboard(char(kcode), u.x_event.xkey.x, u.x_event.xkey.y);
      }
      else {
        const XGlutCharLink *ln = key_trans_table;
        for (; ln->xk_key && ln->xk_key != kcode; ln++);
        if (ln->xk_key) {
          this->special(ln->glut_char, u.x_event.xkey.x, u.x_event.xkey.y);
        }
      }
    }
      break;

    case ButtonPress:
    case ButtonRelease:
      this->mouse(u.x_event.xbutton.button, u.x_event.xbutton.state,
            u.x_event.xbutton.x, u.x_event.xbutton.y);
      break;

    case MotionNotify:
      this->passive(u.x_event.xmotion.x, u.x_event.xmotion.y);
      break;

    case ConfigureNotify:
      width = u.x_event.xconfigure.width;
      height = u.x_event.xconfigure.height;
      offset_x = u.x_event.xconfigure.x;
      offset_y = u.x_event.xconfigure.y;
      this->reshape(width, height);
      break;

    case GenericEvent:
      // extension still unknown? should point to XInput2
      if (xi_major_opcode != u.x_event.xgeneric.extension) {
        DEB("Generic event from extension " << u.x_event.xgeneric.extension);
        break;
      }


      switch(u.x_event.xgeneric.evtype) {
      case XI_TouchBegin:
        DEB("Touch start");
        touch(u.xi_event.detail, true, u.xi_event.event_x, u.xi_event.event_y);
        break;

      case XI_TouchEnd:
        DEB("Touch end");
        touch(u.xi_event.detail, false, u.xi_event.event_x, u.xi_event.event_y);
        break;

      case XI_TouchUpdate:
        touch(u.xi_event.detail, true, u.xi_event.event_x, u.xi_event.event_y);
        DEB("Touch update");
        break;

      default:
        DEB("Unknown event type" << u.x_event.xgeneric.evtype);
        // no op
      }
    }
  }

  glXMakeCurrent(static_dpy, my->xwin, my->glc);
  display();
  glFlush();
  glXSwapBuffers(static_dpy, my->xwin);   // swap should occur at retrace
  glXMakeCurrent(static_dpy, 0, 0);
}

void BareDuecaGLWindow::waitSwap()
{
  if (my->glXWaitVideoSyncSGI) {

    // gl context active
    glXMakeCurrent(static_dpy, my->xwin, my->glc);

    // wait for the video sync
    unsigned int count;
    int res = (*(my->glXWaitVideoSyncSGI))
      (glx_sync_divisor, glx_sync_offset, &count);

    // check result
    switch (res) {
    case 0:
      return;
    case GLX_BAD_VALUE:
      cerr << "error glXWaitVideoSyncSGI, bad params" << endl;
      break;
    case GLX_BAD_CONTEXT:
      cerr << "error glXWaitVideoSyncSGI, bad context" << endl;
      break;
    default:
      cerr << "error glXWaitVideoSyncSGI, unknown return value "
           << res << endl;
    }
    my->glXWaitVideoSyncSGI = NULL;
  }
  else {
    static struct timespec waittime = {0, 10000000};
    // back up wait
    nanosleep(&waittime, NULL);
  }
}

// default dummy values for the virtual functions
void BareDuecaGLWindow::initGL() { }
void BareDuecaGLWindow::reshape(int x, int y) { }
void BareDuecaGLWindow::keyboard(unsigned char key, int x, int y) { }
void BareDuecaGLWindow::special(int key, int x, int y) { }
void BareDuecaGLWindow::mouse(int button, int state, int x, int y) { }
void BareDuecaGLWindow::motion(int x, int y) { }
void BareDuecaGLWindow::passive(int x, int y) { }
void BareDuecaGLWindow::touch(unsigned finger, bool s, float x, float y) { }

DUECA_NS_END;
