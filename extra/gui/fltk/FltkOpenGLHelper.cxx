/* ------------------------------------------------------------------   */
/*      item            : FltkOpenGLHelper.cxx
        made by         : Rene' van Paassen
        date            : 060531
        category        : body file
        description     :
        changes         : 060531 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define FltkOpenGLHelper_cxx

#include <Environment.hxx>
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/fl_draw.H>
#include <dueca_ns.h>

#include "FltkOpenGLHelper.hxx"
#include "DuecaGLWindow.hxx"

#include <GL/glut.h>

DUECA_NS_START;

inline void redraw_done(DuecaGLCanvas* gw)
{
  gw->redrawDone();
}

class FlBasedGLWindow : public Fl_Gl_Window
{
  class DuecaGLWindow* client;

public:
  /** Constructor. */
  FlBasedGLWindow(DuecaGLWindow* client, int x, int y, int w, int h,
                  const char* name);

  /** Destructor. Hope my base class destructors are virtual. */
  ~FlBasedGLWindow();

  /** Implements the draw functionality */
  void draw(void);

  /** Handle calbacks for mouse motion, keys, etc. */
  int handle(int event);
};

FlBasedGLWindow::FlBasedGLWindow(DuecaGLWindow* client,
                                 int x, int y, int w, int h,
                                 const char* name) :
  Fl_Gl_Window(x, y, w, h, name),
  client(client)
{
  // make the window resizable
  resizable(this);
}

FlBasedGLWindow::~FlBasedGLWindow()
{
  client->windowDestroyed();
}

void FlBasedGLWindow::draw(void)
{
  if (!valid()) {
    client->reshape(w(), h());
    client->initGL();
  }
  client->display();
  redraw_done(client);
}
struct FltkGlutKeyLink
{
  /** Gtk keypress. */
  int fltk_key;

  /** Corresponding Glut keypress. */
  unsigned short glut_key;
};

static int button_table(int button)
{
  if (button == 1) return GLUT_LEFT_BUTTON;
  if (button == 2) return GLUT_MIDDLE_BUTTON;
  if (button == 3) return GLUT_RIGHT_BUTTON;
  return 0;
}

int FlBasedGLWindow::handle(int event)
{
  static FltkGlutKeyLink key_trans_table[] = {

    { FL_Insert, GLUT_KEY_INSERT },
    { FL_Home, GLUT_KEY_HOME },
    { FL_Page_Up, GLUT_KEY_PAGE_UP },
    { FL_End, GLUT_KEY_END },
    { FL_Page_Down, GLUT_KEY_PAGE_DOWN },
    { FL_Left, GLUT_KEY_LEFT },
    { FL_Up, GLUT_KEY_UP },
    { FL_Right, GLUT_KEY_RIGHT },
    { FL_Down, GLUT_KEY_DOWN },
    { 0, 0}
  };

  switch(event) {
  case FL_PUSH:
    client->mouse(button_table(Fl::event_button()), GLUT_DOWN,
                  Fl::event_x(), Fl::event_y());
    return 1;
  case FL_DRAG:
    client->motion(Fl::event_x(), Fl::event_y());
    return 1;
  case FL_RELEASE:
    client->mouse(button_table(Fl::event_button()), GLUT_UP,
                  Fl::event_x(), Fl::event_y());
    return 1;
  case FL_MOVE:
    client->passive(Fl::event_x(), Fl::event_y());
    return 1;
  case FL_FOCUS :
  case FL_UNFOCUS :
    // Return 1 if you want keyboard events, 0 otherwise
    return 1;
  case FL_KEYBOARD: {
    int key = Fl::event_key();

    // keypad keys, or main keyboard keys
    if ((key >= FL_KP && key <= FL_KP_Last) ||
        (key >= 32 && key < 127)) {
      // get ascii
      char c = Fl::event_text()[0];
      client->keyboard(c, Fl::event_x(), Fl::event_y());
      return 1;
    }

    // function keys
    if (key >= FL_F && key <= FL_F_Last) {
      client->special(key - FL_F - 1 + GLUT_KEY_F1,
                      Fl::event_x(), Fl::event_y());
      return 1;
    }

    // odd stuff
    for (int ik = 0; key_trans_table[ik].fltk_key != 0; ik++) {
      if (key == key_trans_table[ik].fltk_key) {
        client->special(key_trans_table[ik].glut_key,
                        Fl::event_x(), Fl::event_y());
        return 1;
      }
    }

    break;
  }
  case FL_SHORTCUT:
    return 0;

 default:
    // pass other events to the base class...
    return Fl_Gl_Window::handle(event);
  }
  return 0;
}


FltkGLWindowHelper::FltkGLWindowHelper() :
  fl_window(NULL)
{
  //
}

void FltkGLWindowHelper::open(const std::string &title,
                              DuecaGLWindow* master,
                              int offset_x, int offset_y,
                              int size_x, int size_y,
                              bool fullscreen)
{
  fl_window = new FlBasedGLWindow
    (master, offset_x, offset_y, size_x, size_y, title.c_str());
  if (fullscreen) fl_window->fullscreen();

  // select proper mode
  int mode = FL_RGB | FL_ALPHA | FL_DOUBLE;
  if (CSE.getGraphicDepthBufferSize()) mode |= FL_DEPTH;
  if (CSE.getGraphicStencilBufferSize()) mode |= FL_STENCIL;
  fl_window->mode(mode);

  // show the widget
  fl_window->show();
}

FltkGLWindowHelper::~FltkGLWindowHelper()
{
  delete fl_window;
}

void FltkGLWindowHelper::hide()
{
  fl_window->hide();
}


void FltkGLWindowHelper::show()
{
  fl_window->show();
}


void FltkGLWindowHelper::current()
{
  fl_window->make_current();
}


void FltkGLWindowHelper::cursor(int i)
{
  if (fl_window == NULL) return;
  Fl_Window* current = Fl_Window::current();
  if (current != fl_window) fl_window->make_current();
  switch(i) {
  case 0:
    fl_cursor(FL_CURSOR_NONE);
    break;
  case 1:
    fl_cursor(FL_CURSOR_DEFAULT);
    break;
  case 2:
    fl_cursor(FL_CURSOR_CROSS);
    break;
  case 3:
    fl_cursor(FL_CURSOR_ARROW);
    break;
  case 4:
    fl_cursor(FL_CURSOR_ARROW);
    break;
  }
  if (current && current != fl_window) current->make_current();
}


void FltkGLWindowHelper::redraw()
{
  fl_window->redraw();
}


void FltkGLWindowHelper::swap()
{
  // not necessary, implicit
}


int FltkGLWindowHelper::width()
{
  return fl_window ? fl_window->w() : 0;
}


int FltkGLWindowHelper::height()
{
  return fl_window ? fl_window->h() : 0;
}

int FltkGLWindowHelper::xoffset()
{
  return fl_window ? fl_window->x() : 0;
}

int FltkGLWindowHelper::yoffset()
{
  return fl_window ? fl_window->y() : 0;
}

FltkOpenGLHelper::FltkOpenGLHelper(const std::string &name) :
  OpenGLHelper(name)
{
  //
}

FltkOpenGLHelper::~FltkOpenGLHelper()
{
  //
}

GLWindowHelper* FltkOpenGLHelper::newWindow()
{
  return new FltkGLWindowHelper();
}

DUECA_NS_END;

