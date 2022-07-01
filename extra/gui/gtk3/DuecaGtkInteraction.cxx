/* ------------------------------------------------------------------   */
/*      item            : DuecaGtkInteraction.cxx
        made by         : Rene' van Paassen
        date            : 151013
        category        : body file
        description     :
        changes         : 151013 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define DuecaGtkInteraction_cxx
#include "DuecaGtkInteraction.hxx"
#include <dueca-conf.h>
#include <iostream>

#if defined(HAVE_GL_FREEGLUT_H)
#include <GL/freeglut.h>
#elif defined(HAVE_GL_GLUT_H)
#include <GL/glut.h>
#elif defined(HAVE_GLUT_H)
#include <glut.h>
#endif

DUECA_NS_START;

/** A struct for translation of Gtk keycodes to the Glut key codes
    adopted in DUECA. */
struct GtkGlutKeyLink
{
  /** Gtk keypress. */
  unsigned short gtk_key;

  /** Corresponding Glut keypress. */
  unsigned short glut_key;
};

/** A struct for translation of Gtk keycodes to the Glut character codes
    adopted in DUECA. */
struct GtkGlutCharLink
{
  /** Gtk keypress. */
  unsigned short gtk_key;

  /** Corresponding character press */
  unsigned char  glut_char;
};

DuecaGtkInteraction::DuecaGtkInteraction(GtkWidget *widget, int w, int h) :
  wdgt(widget),
  width(w),
  height(h),
  x(-1),
  y(-1),
  dopass(false)
{
  if (widget) init();
}


DuecaGtkInteraction::~DuecaGtkInteraction()
{
  //
}

static gboolean on_resize(GtkWidget *w, gint width, gint height,
                          gpointer self)
{
  reinterpret_cast<DuecaGtkInteraction*>(self)->newSize(width, height);
  return TRUE;
}

void  DuecaGtkInteraction::newSize(int w, int h)
{
  this->width = w; this->height = h;
  if (wdgt) {
#if GTK_CHECK_VERSION(3,16,0)
    gtk_gl_area_make_current(GTK_GL_AREA(wdgt));
#endif
    this->reshape(w, h);
  }
  else {
    dopass = true;
  }
}

void DuecaGtkInteraction::passShape()
{
  if (dopass) {
    this->reshape(width, height);
    dopass = false;
  }
}

static gboolean on_keypress(GtkWidget *w, GdkEvent *event, gpointer self)
{
#if defined(HAVE_GL_GLUT_H) || defined(HAVE_GL_FREEGLUT_H) || defined(HAVE_GLUT_H)
  static const int NGTK_TRANS_KEYS = 31;
  static const GtkGlutKeyLink key_trans_table[NGTK_TRANS_KEYS] = {
    { GDK_KEY_F1, GLUT_KEY_F1},
    { GDK_KEY_F2, GLUT_KEY_F2},
    { GDK_KEY_F3, GLUT_KEY_F3},
    { GDK_KEY_F4, GLUT_KEY_F4},
    { GDK_KEY_F5, GLUT_KEY_F5},
    { GDK_KEY_F6, GLUT_KEY_F6},
    { GDK_KEY_F7, GLUT_KEY_F7},
    { GDK_KEY_F8, GLUT_KEY_F8},
    { GDK_KEY_F9, GLUT_KEY_F9},
    { GDK_KEY_F10, GLUT_KEY_F10},
    { GDK_KEY_F11, GLUT_KEY_F11},
    { GDK_KEY_F12, GLUT_KEY_F12},
    { GDK_KEY_Left, GLUT_KEY_LEFT},
    { GDK_KEY_Up, GLUT_KEY_UP},
    { GDK_KEY_Right, GLUT_KEY_RIGHT},
    { GDK_KEY_Down, GLUT_KEY_DOWN},
    { GDK_KEY_Page_Up, GLUT_KEY_PAGE_UP},
    { GDK_KEY_Page_Down, GLUT_KEY_PAGE_DOWN},
    { GDK_KEY_Home, GLUT_KEY_HOME},
    { GDK_KEY_End, GLUT_KEY_END},
    { GDK_KEY_Insert, GLUT_KEY_INSERT},
    { GDK_KEY_KP_Insert, GLUT_KEY_INSERT},
    { GDK_KEY_KP_Home, GLUT_KEY_HOME},
    { GDK_KEY_KP_Left, GLUT_KEY_LEFT},
    { GDK_KEY_KP_Up, GLUT_KEY_UP},
    { GDK_KEY_KP_Right, GLUT_KEY_RIGHT},
    { GDK_KEY_KP_Down, GLUT_KEY_DOWN},
    { GDK_KEY_KP_Page_Up, GLUT_KEY_PAGE_UP},
    { GDK_KEY_KP_Page_Down, GLUT_KEY_PAGE_DOWN},
    { GDK_KEY_KP_End, GLUT_KEY_END}};

  static const int NGTK_TRANS_KEYS2 = 19;
  static const GtkGlutCharLink key_trans_table2[NGTK_TRANS_KEYS2] = {
    { GDK_KEY_KP_0, '0'},
    { GDK_KEY_KP_1, '1'},
    { GDK_KEY_KP_2, '2'},
    { GDK_KEY_KP_3, '3'},
    { GDK_KEY_KP_4, '4'},
    { GDK_KEY_KP_5, '5'},
    { GDK_KEY_KP_6, '6'},
    { GDK_KEY_KP_7, '7'},
    { GDK_KEY_KP_8, '8'},
    { GDK_KEY_KP_9, '9'},
    { GDK_KEY_KP_Space, ' '},
    { GDK_KEY_KP_Tab, '\t'},
    { GDK_KEY_KP_Enter, '\n'},
    { GDK_KEY_KP_Equal, '='},
    { GDK_KEY_KP_Multiply, '*'},
    { GDK_KEY_KP_Add, '+'},
    { GDK_KEY_KP_Subtract, '-'},
    { GDK_KEY_KP_Decimal, '.'},
    { GDK_KEY_KP_Divide, '/'}};
#else
  static const int NGTK_TRANS_KEYS = 0;
  static const int NGTK_TRANS_KEYS2 = 0;
  static const GtkGlutKeyLink *key_trans_table = NULL;
  static const GtkGlutCharLink *key_trans_table2 = NULL;
#define  GLUT_LEFT_BUTTON                   0x0000
#define  GLUT_MIDDLE_BUTTON                 0x0001
#define  GLUT_RIGHT_BUTTON                  0x0002
#define  GLUT_DOWN                          0x0000
#define  GLUT_UP                            0x0001
#warning "No GLUT found, special() and keyboard() functions limited"
#endif

  DuecaGtkInteraction *dgi = reinterpret_cast<DuecaGtkInteraction*>(self);
  if (event->key.keyval <= 255) {
    char c = event->key.keyval;
    int x = 0, y = 0;
#if 1
    GdkModifierType state;
    gdk_window_get_device_position
      (event->key.window, gdk_event_get_source_device(event), &x, &y, &state);
#else
#warning "Key event position not correctly returned"
#endif
    dgi->keyboard(c, x, y);
    return TRUE;
  }

  int ii=0;
  while ( (ii<NGTK_TRANS_KEYS) &&
          (key_trans_table[ii].gtk_key!=event->key.keyval) ) ii++;
  if (ii < NGTK_TRANS_KEYS) {
    int x = 0, y = 0;
#if 1
    GdkModifierType state;
    gdk_window_get_device_position
      (event->key.window, gdk_event_get_source_device(event), &x, &y, &state);
#else
#warning "Key event position not correctly returned"
#endif
    dgi->special(key_trans_table[ii].glut_key, x, y);
    return TRUE;
  }

  ii = 0;
  while ( (ii<NGTK_TRANS_KEYS2) &&
          (key_trans_table2[ii].gtk_key!=event->key.keyval) ) ii++;
  if (ii < NGTK_TRANS_KEYS2) {
    int x = 0, y = 0;
#if 1
    GdkModifierType state;
    gdk_window_get_device_position
      (event->key.window, gdk_event_get_source_device(event), &x, &y, &state);
#else
#warning "Key event position not correctly returned"
#endif
    dgi->keyboard(key_trans_table2[ii].glut_char, x, y);
    return TRUE;
  }

  // cannot find anything here
  std::cerr << "No key for GTK keycode " << event->key.keyval << std::endl;
  return TRUE;
}


static gint on_motion(GtkWidget *w, GdkEventMotion *event, gpointer self)
{
  DuecaGtkInteraction *dgi = reinterpret_cast<DuecaGtkInteraction*>(self);
  int x, y;
  GdkModifierType state;
  gdk_window_get_device_position(event->window, event->device, &x, &y, &state);
  if ((state & GDK_BUTTON1_MASK) != 0 ||
      (state & GDK_BUTTON2_MASK) != 0 ||
      (state & GDK_BUTTON3_MASK) != 0 ||
      (state & GDK_BUTTON4_MASK) != 0 ||
      (state & GDK_BUTTON5_MASK) != 0) {
    dgi->motion(x, y);
  }
  else {
    dgi->passive(x, y);
  }
  return 0;
}

static gint on_button(GtkWidget *w, GdkEventButton *event, gpointer self)
{
  DuecaGtkInteraction *dgi = reinterpret_cast<DuecaGtkInteraction*>(self);
  int x, y;
  GdkModifierType state;
  gdk_window_get_device_position(event->window, event->device, &x, &y, &state);
  switch(event->button) {
  case 1:
    dgi->mouse(GLUT_LEFT_BUTTON, GLUT_DOWN, x, y);
    break;
  case 2:
    dgi->mouse(GLUT_MIDDLE_BUTTON, GLUT_DOWN, x, y);
    break;
  case 3:
    dgi->mouse(GLUT_RIGHT_BUTTON, GLUT_DOWN, x, y);
    break;
  }
  return 0;
}

static gint on_release(GtkWidget *w, GdkEventButton *event, gpointer self)
{
  DuecaGtkInteraction *dgi = reinterpret_cast<DuecaGtkInteraction*>(self);
  int x, y;
  GdkModifierType state;
  gdk_window_get_device_position(event->window, event->device, &x, &y, &state);
  switch(event->button) {
  case 1:
    dgi->mouse(GLUT_LEFT_BUTTON, GLUT_UP, x, y);
    break;
  case 2:
    dgi->mouse(GLUT_MIDDLE_BUTTON, GLUT_UP, x, y);
    break;
  case 3:
    dgi->mouse(GLUT_RIGHT_BUTTON, GLUT_UP, x, y);
    break;
  }
  return 0;
}



void DuecaGtkInteraction::init(GtkWidget* widget)
{
  if (wdgt == NULL && widget != NULL) {
    wdgt = widget;
  }
  else if (wdgt != NULL) {
    // ok
  }
  else {
    std::cerr << "DuecaGtkInteraction, improper initialisation" << std::endl;
    return;
  }
  g_signal_connect(wdgt, "resize", G_CALLBACK(on_resize), this);
  g_signal_connect(wdgt, "key-press-event", G_CALLBACK(on_keypress), this);
  g_signal_connect(wdgt, "motion-notify-event", G_CALLBACK(on_motion), this);
  g_signal_connect(wdgt, "button-press-event", G_CALLBACK(on_button), this);
  g_signal_connect(wdgt, "button-release-event", G_CALLBACK(on_release), this);
}

int DuecaGtkInteraction::getWidth()
{ return width; }

int DuecaGtkInteraction::getHeight()
{ return height; }

int DuecaGtkInteraction::getXOffset()
{ return x; }

int DuecaGtkInteraction::getYOffset()
{ return y; }

void DuecaGtkInteraction::reshape(int x, int y) { }
void DuecaGtkInteraction::keyboard(char c, int x, int y) { }
void DuecaGtkInteraction::special(int c, int x, int y) { }
void DuecaGtkInteraction::motion(int x, int y) { }
void DuecaGtkInteraction::passive(int x, int y) { }
void DuecaGtkInteraction::mouse(int button, int state, int x, int y) { }

DUECA_NS_END;
