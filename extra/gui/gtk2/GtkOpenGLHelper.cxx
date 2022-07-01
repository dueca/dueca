/* ------------------------------------------------------------------   */
/*      item            : GtkOpenGLHelper.cxx
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


#define GtkOpenGLHelper_cxx
#include "GtkOpenGLHelper.hxx"
#include "DuecaGLWindow.hxx"
#include <GL/glut.h>
#include <gdk/gdkkeysyms.h>
#include <Environment.hxx>
#include <dassert.h>
#include <GuiHandler.hxx>

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

struct GtkGlutCharLink
{
  /** Gtk keypress. */
  unsigned short gtk_key;

  /** Corresponding character press */
  unsigned char  glut_char;
};

static gboolean on_gtk_gl_key_press_event(GtkWidget *w,
                                          GdkEventKey *event,
                                          gpointer user_data)
{
  static const int NGTK_TRANS_KEYS = 31;
  static const GtkGlutKeyLink key_trans_table[NGTK_TRANS_KEYS] = {
    { GDK_F1, GLUT_KEY_F1},
    { GDK_F2, GLUT_KEY_F2},
    { GDK_F3, GLUT_KEY_F3},
    { GDK_F4, GLUT_KEY_F4},
    { GDK_F5, GLUT_KEY_F5},
    { GDK_F6, GLUT_KEY_F6},
    { GDK_F7, GLUT_KEY_F7},
    { GDK_F8, GLUT_KEY_F8},
    { GDK_F9, GLUT_KEY_F9},
    { GDK_F10, GLUT_KEY_F10},
    { GDK_F11, GLUT_KEY_F11},
    { GDK_F12, GLUT_KEY_F12},
    { GDK_Left, GLUT_KEY_LEFT},
    { GDK_Up, GLUT_KEY_UP},
    { GDK_Right, GLUT_KEY_RIGHT},
    { GDK_Down, GLUT_KEY_DOWN},
    { GDK_Page_Up, GLUT_KEY_PAGE_UP},
    { GDK_Page_Down, GLUT_KEY_PAGE_DOWN},
    { GDK_Home, GLUT_KEY_HOME},
    { GDK_End, GLUT_KEY_END},
    { GDK_Insert, GLUT_KEY_INSERT},
    { GDK_KP_Insert, GLUT_KEY_INSERT},
    { GDK_KP_Home, GLUT_KEY_HOME},
    { GDK_KP_Left, GLUT_KEY_LEFT},
    { GDK_KP_Up, GLUT_KEY_UP},
    { GDK_KP_Right, GLUT_KEY_RIGHT},
    { GDK_KP_Down, GLUT_KEY_DOWN},
    { GDK_KP_Page_Up, GLUT_KEY_PAGE_UP},
    { GDK_KP_Page_Down, GLUT_KEY_PAGE_DOWN},
    { GDK_KP_End, GLUT_KEY_END}};


  static const int NGTK_TRANS_KEYS2 = 19;
  static const GtkGlutCharLink key_trans_table2[NGTK_TRANS_KEYS2] = {
    { GDK_KP_0, '0'},
    { GDK_KP_1, '1'},
    { GDK_KP_2, '2'},
    { GDK_KP_3, '3'},
    { GDK_KP_4, '4'},
    { GDK_KP_5, '5'},
    { GDK_KP_6, '6'},
    { GDK_KP_7, '7'},
    { GDK_KP_8, '8'},
    { GDK_KP_9, '9'},
    { GDK_KP_Space, ' '},
    { GDK_KP_Tab, '\t'},
    { GDK_KP_Enter, '\n'},
    { GDK_KP_Equal, '='},
    { GDK_KP_Multiply, '*'},
    { GDK_KP_Add, '+'},
    { GDK_KP_Subtract, '-'},
    { GDK_KP_Decimal, '.'},
    { GDK_KP_Divide, '/'}};

  DuecaGLWindow* gw = reinterpret_cast<DuecaGLWindow*>(user_data);
  if (event->keyval <= 255) {
    char c = event->keyval;
    int x, y;
    GdkModifierType state;
    gdk_window_get_pointer(event->window, &x, &y, &state);
    gw->keyboard(c, x, y);
    return TRUE;
  }

  int ii=0;
  while ( (ii<NGTK_TRANS_KEYS) &&
          (key_trans_table[ii].gtk_key!=event->keyval) ) ii++;
  if (ii < NGTK_TRANS_KEYS) {
    int x, y;
    GdkModifierType state;
    gdk_window_get_pointer(event->window, &x, &y, &state);
    gw->special(key_trans_table[ii].glut_key, x, y);
    return TRUE;
  }

  ii = 0;
  while ( (ii<NGTK_TRANS_KEYS2) &&
          (key_trans_table2[ii].gtk_key!=event->keyval) ) ii++;
  if (ii < NGTK_TRANS_KEYS2) {
    int x, y;
    GdkModifierType state;
    gdk_window_get_pointer(event->window, &x, &y, &state);
    gw->keyboard(key_trans_table2[ii].glut_char, x, y);
    return TRUE;
  }

  // cannot find anything here
  cerr << "No key for GTK keycode " << event->keyval << std::endl;
  return TRUE;
}

/*const GdkModifierType buttonmodifiers = reinterpret_cast<GdkModifierType>
                                                        (GDK_BUTTON1_MASK |
                                                         GDK_BUTTON2_MASK |
                                                         GDK_BUTTON3_MASK |
                                                         GDK_BUTTON4_MASK |
                                                         GDK_BUTTON5_MASK);*/


gint on_gtk_gl_mouse_motion(GtkWidget *w,
                            GdkEventMotion *event,
                            gpointer user_data)
{
  DuecaGLWindow* gw = reinterpret_cast<DuecaGLWindow*>(user_data);
  int x, y;
  GdkModifierType state;
  gdk_window_get_pointer(event->window, &x, &y, &state);
  if ((state & GDK_BUTTON1_MASK) != 0 ||
      (state & GDK_BUTTON2_MASK) != 0 ||
      (state & GDK_BUTTON3_MASK) != 0 ||
      (state & GDK_BUTTON4_MASK) != 0 ||
      (state & GDK_BUTTON5_MASK) != 0) {
    gw->motion(x, y);
  }
  else if (gw->passPassive()) {
    gw->passive(x, y);
  }
  return 0;
}

gint on_gtk_gl_mouse_button_press(GtkWidget *w,
                                  GdkEventButton *event,
                                  gpointer user_data)
{
  DuecaGLWindow* gw = reinterpret_cast<DuecaGLWindow*>(user_data);
  int x, y;
  GdkModifierType state;
  gdk_window_get_pointer(event->window, &x, &y, &state);
  switch(event->button) {
  case 1:
    gw->mouse(GLUT_LEFT_BUTTON, GLUT_DOWN, x, y);
    break;
  case 2:
    gw->mouse(GLUT_MIDDLE_BUTTON, GLUT_DOWN, x, y);
    break;
  case 3:
    gw->mouse(GLUT_RIGHT_BUTTON, GLUT_DOWN, x, y);
    break;
  }
  return 0;
}

gint on_gtk_gl_mouse_button_release(GtkWidget *w,
                                    GdkEventButton *event,
                                    gpointer user_data)
{
  DuecaGLWindow* gw = reinterpret_cast<DuecaGLWindow*>(user_data);
  int x, y;
  GdkModifierType state;
  gdk_window_get_pointer(event->window, &x, &y, &state);
  switch(event->button) {
  case 1:
    gw->mouse(GLUT_LEFT_BUTTON, GLUT_UP, x, y);
    break;
  case 2:
    gw->mouse(GLUT_MIDDLE_BUTTON, GLUT_UP, x, y);
    break;
  case 3:
    gw->mouse(GLUT_RIGHT_BUTTON, GLUT_UP, x, y);
    break;
  }
  return 0;
}

static void on_gtk_window_realize(GtkWidget *w, gpointer user_data)
{
  DuecaGLWindow* gw = reinterpret_cast<DuecaGLWindow*>(user_data);

  if (gw->getXOffset() != -1 && gw->getYOffset() != -1) {
    gtk_window_move(GTK_WINDOW(w), gw->getXOffset(), gw->getYOffset());
  }
}


GtkGLWindowHelper::GtkGLWindowHelper() :
  gtk_win_id(NULL)
{
  //
}

void GtkGLWindowHelper::open(const std::string &title,
                             DuecaGLWindow* master,
                             int offset_x, int offset_y,
                             int size_x, int size_y,
                             bool fullscreen)
{
  // open a top-level window
  gtk_win_id = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(gtk_win_id), title.c_str());
  gtk_container_set_reallocate_redraws (GTK_CONTAINER (gtk_win_id), TRUE);
#if 0
  if (offset_x != -1 && offset_y != -1) {
    gtk_widget_set_uposition(GTK_WIDGET(gtk_win_id), offset_x, offset_y);
  }
#endif

  // create a drawingarea in this window.
  gtk_glwidget_id = gtk_drawing_area_new();
  if (size_x > 0 && size_y > 0) {
    gtk_widget_set_size_request(gtk_glwidget_id, size_x, size_y);
  }

  gtk_container_add(GTK_CONTAINER(gtk_win_id), gtk_glwidget_id);

  // Call GtkGLWidgetHelper init for gl area
  init_gl_area(master);

  g_signal_connect (G_OBJECT (gtk_glwidget_id), "key-press-event",
                    G_CALLBACK (on_gtk_gl_key_press_event),
                                master);
  g_signal_connect (G_OBJECT (gtk_glwidget_id), "motion-notify-event",
                    G_CALLBACK (on_gtk_gl_mouse_motion),
                                master);
  g_signal_connect (G_OBJECT (gtk_glwidget_id), "button-press-event",
                    G_CALLBACK (on_gtk_gl_mouse_button_press),
                                master);
  g_signal_connect (G_OBJECT (gtk_glwidget_id),
                    "button-release-event",
                    G_CALLBACK (on_gtk_gl_mouse_button_release),
                                master);
  g_signal_connect_after (G_OBJECT (gtk_glwidget_id), "realize",
                          G_CALLBACK (on_gtk_window_realize),
                          master);

  gtk_widget_show_all(gtk_win_id);

  if (offset_x != -1 && offset_y != -1) {
    gtk_window_move(GTK_WINDOW(gtk_win_id), offset_x, offset_y);
  }
}

GtkGLWindowHelper::~GtkGLWindowHelper()
{
  //
}

void GtkGLWindowHelper::hide()
{
  gtk_widget_hide_all(gtk_win_id);
}


void GtkGLWindowHelper::show()
{
  gtk_widget_show_all(gtk_win_id);
}

DUECA_NS_END;
