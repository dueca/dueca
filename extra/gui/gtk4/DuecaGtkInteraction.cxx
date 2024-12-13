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

#include "gdk/gdk.h"
#include <GL/freeglut_std.h>
#include <gtk/gtk.h>
#define DuecaGtkInteraction_cxx
#include "DuecaGtkInteraction.hxx"
#include <dueca-conf.h>
#include <iostream>
#include <algorithm>
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
  unsigned char glut_char;
};

DuecaGtkInteraction::DuecaGtkInteraction(GtkWidget *widget, int w, int h) :
  wdgt(widget),
  width(w),
  height(h),
  x(-1),
  y(-1),
  dopass(false)
{
  if (widget)
    init();
}

DuecaGtkInteraction::~DuecaGtkInteraction()
{
  //
}

static gboolean on_resize(GtkWidget *w, gint width, gint height, gpointer self)
{
  reinterpret_cast<DuecaGtkInteraction *>(self)->newSize(width, height);
  return TRUE;
}

void DuecaGtkInteraction::newSize(int w, int h)
{
  this->width = w;
  this->height = h;
  if (wdgt) {
#if GTK_CHECK_VERSION(3, 16, 0)
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

static unsigned glut_key(unsigned keyval)
{
  static const int NGTK_TRANS_KEYS = 31;
  static const GtkGlutKeyLink key_trans_table[NGTK_TRANS_KEYS] = {
    { GDK_KEY_F1, GLUT_KEY_F1 },
    { GDK_KEY_F2, GLUT_KEY_F2 },
    { GDK_KEY_F3, GLUT_KEY_F3 },
    { GDK_KEY_F4, GLUT_KEY_F4 },
    { GDK_KEY_F5, GLUT_KEY_F5 },
    { GDK_KEY_F6, GLUT_KEY_F6 },
    { GDK_KEY_F7, GLUT_KEY_F7 },
    { GDK_KEY_F8, GLUT_KEY_F8 },
    { GDK_KEY_F9, GLUT_KEY_F9 },
    { GDK_KEY_F10, GLUT_KEY_F10 },
    { GDK_KEY_F11, GLUT_KEY_F11 },
    { GDK_KEY_F12, GLUT_KEY_F12 },
    { GDK_KEY_Left, GLUT_KEY_LEFT },
    { GDK_KEY_Up, GLUT_KEY_UP },
    { GDK_KEY_Right, GLUT_KEY_RIGHT },
    { GDK_KEY_Down, GLUT_KEY_DOWN },
    { GDK_KEY_Page_Up, GLUT_KEY_PAGE_UP },
    { GDK_KEY_Page_Down, GLUT_KEY_PAGE_DOWN },
    { GDK_KEY_Home, GLUT_KEY_HOME },
    { GDK_KEY_End, GLUT_KEY_END },
    { GDK_KEY_Insert, GLUT_KEY_INSERT },
    { GDK_KEY_KP_Insert, GLUT_KEY_INSERT },
    { GDK_KEY_KP_Home, GLUT_KEY_HOME },
    { GDK_KEY_KP_Left, GLUT_KEY_LEFT },
    { GDK_KEY_KP_Up, GLUT_KEY_UP },
    { GDK_KEY_KP_Right, GLUT_KEY_RIGHT },
    { GDK_KEY_KP_Down, GLUT_KEY_DOWN },
    { GDK_KEY_KP_Page_Up, GLUT_KEY_PAGE_UP },
    { GDK_KEY_KP_Page_Down, GLUT_KEY_PAGE_DOWN },
    { GDK_KEY_KP_End, GLUT_KEY_END }
  };

  static const int NGTK_TRANS_KEYS2 = 19;
  static const GtkGlutCharLink key_trans_table2[NGTK_TRANS_KEYS2] = {
    { GDK_KEY_KP_0, '0' },        { GDK_KEY_KP_1, '1' },
    { GDK_KEY_KP_2, '2' },        { GDK_KEY_KP_3, '3' },
    { GDK_KEY_KP_4, '4' },        { GDK_KEY_KP_5, '5' },
    { GDK_KEY_KP_6, '6' },        { GDK_KEY_KP_7, '7' },
    { GDK_KEY_KP_8, '8' },        { GDK_KEY_KP_9, '9' },
    { GDK_KEY_KP_Space, ' ' },    { GDK_KEY_KP_Tab, '\t' },
    { GDK_KEY_KP_Enter, '\n' },   { GDK_KEY_KP_Equal, '=' },
    { GDK_KEY_KP_Multiply, '*' }, { GDK_KEY_KP_Add, '+' },
    { GDK_KEY_KP_Subtract, '-' }, { GDK_KEY_KP_Decimal, '.' },
    { GDK_KEY_KP_Divide, '/' }
  };

  if (keyval <= 255)
    return keyval;

  int ii = 0;
  while ((ii < NGTK_TRANS_KEYS) && (key_trans_table[ii].gtk_key != keyval))
    ii++;
  if (ii < NGTK_TRANS_KEYS)
    return key_trans_table[ii].glut_key;

  ii = 0;
  while ((ii < NGTK_TRANS_KEYS2) && (key_trans_table2[ii].gtk_key != keyval))
    ii++;
  if (ii < NGTK_TRANS_KEYS2) {
    return key_trans_table2[ii].glut_char;
  }
  return 0xffffffff;
}

static int glut_button(unsigned button)
{
  unsigned buttons[] = { GLUT_LEFT_BUTTON, GLUT_MIDDLE_BUTTON,
                         GLUT_RIGHT_BUTTON };
  return buttons[std::min(std::max(button, 1U), 3U) - 1U];
}

static gint process_event(GtkEventControllerLegacy *ctrl, GdkEvent *event,
                          gpointer self)
{
  DuecaGtkInteraction *dgi = reinterpret_cast<DuecaGtkInteraction *>(self);
  gdouble x, y;
  gdk_event_get_position(event, &x, &y);
  switch (gdk_event_get_event_type(event)) {
  case GDK_MOTION_NOTIFY:
    dgi->motion(x, y);
    dgi->passive(x, y);
    return TRUE;
  case GDK_BUTTON_PRESS: {
    auto button = gdk_button_event_get_button(event);
    dgi->mouse(glut_button(button), GLUT_DOWN, x, y);
  } return TRUE;
  case GDK_BUTTON_RELEASE: {
    auto button = gdk_button_event_get_button(event);
    dgi->mouse(glut_button(button), GLUT_UP, x, y);
  } return TRUE;
  case GDK_KEY_RELEASE: {
    auto keyval = gdk_key_event_get_keyval(event);
    auto keyconv = glut_key(keyval);
    if (keyconv <= 255) {
      dgi->keyboard(char(keyconv), x, y);
      return TRUE;
    }
    else if (keyconv != 0xffffffff) {
      dgi->special(keyconv, x, y);
      return TRUE;
    }
  } break;

  default:
    break;
  }
  return FALSE;
}

void DuecaGtkInteraction::init(GtkWidget *widget)
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

  // gtk4 uses gestcbEmergure event controllers
  auto legacy = gtk_event_controller_legacy_new();
  gtk_event_controller_set_propagation_phase(legacy, GTK_PHASE_CAPTURE);
  g_signal_connect(legacy, "event", G_CALLBACK(process_event), this);
}

int DuecaGtkInteraction::getWidth() { return width; }

int DuecaGtkInteraction::getHeight() { return height; }

int DuecaGtkInteraction::getXOffset() { return x; }

int DuecaGtkInteraction::getYOffset() { return y; }

void DuecaGtkInteraction::reshape(int x, int y) {}
void DuecaGtkInteraction::keyboard(char c, int x, int y) {}
void DuecaGtkInteraction::special(int c, int x, int y) {}
void DuecaGtkInteraction::motion(int x, int y) {}
void DuecaGtkInteraction::passive(int x, int y) {}
void DuecaGtkInteraction::mouse(int button, int state, int x, int y) {}

DUECA_NS_END;
