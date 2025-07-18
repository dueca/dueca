/* ------------------------------------------------------------------   */
/*      item            : main.cxx
        made by         : Rene van Paassen
        date            : 250712
        category        : body file
        description     : Test file gtk4 glarea
        changes         : 250712 first version
        language        : C++
        copyright       : (c)2025 Rene van Paassen
*/
#include <unistd.h>
#include <iostream>
#include <gtk/gtk.h>
#include "display.hxx"

static const char *APPLICATION_ID = "nl.tudelft.dueca.gtk4glarea";
static GtkApplication *app = NULL;
static MyDisplay *display;

void app_activate(GApplication *app, gpointer user_data)
{
  std::cout << "app activate" << std::endl;
  // create display
  display = new MyDisplay();
}

void exit_gtk()
{
  // return control
  g_application_quit(G_APPLICATION(app));
  g_application_release(G_APPLICATION(app));
}

static gboolean on_render(GtkGLArea *area, gpointer u)
{
  gtk_gl_area_make_current(area);
  display->display();
  return TRUE;
}

static void on_realize(GtkGLArea *area, gpointer u)
{
  gtk_gl_area_make_current(area);
  if (gtk_gl_area_get_error(area) != NULL) {
    std::cerr << "cauthg an error, see you later" << std::endl;
    return;
  }

  display->initGL();
}

GtkWidget *area = NULL;

static gint call_environment_loop(gpointer user_data)
{
  bool opened = false;

  if (!opened) {
    auto gdk_display_id = gdk_display_get_default();
    auto gtk_win_id = GTK_WINDOW(gtk_window_new());
    area = gtk_gl_area_new();
    // gtk_gl_area_set_allowed_apis(GTK_GL_AREA(area), GDK_GL_API_GLES);
    auto apis = gtk_gl_area_get_allowed_apis(GTK_GL_AREA(area));
    if (apis & GDK_GL_API_GL) {
      std::cerr << "GL Api enabled" << std::endl;
    }
    if (apis & GDK_GL_API_GLES) {
      std::cerr << "GLES Api enabled" << std::endl;
    }
    gtk_window_set_child(GTK_WINDOW(gtk_win_id), area);

    g_signal_connect(area, "render", G_CALLBACK(on_render), NULL);
    g_signal_connect(area, "realize", G_CALLBACK(on_realize), NULL);

    gtk_widget_set_visible(GTK_WIDGET(gtk_win_id), TRUE);
    opened = true;
    return TRUE;
  }
  usleep(500000);
  display->update();

  static unsigned count = 1000;
  if (!--count) {
    delete display;
    display = NULL;
    exit_gtk();
  }
  return TRUE;
}

int main(int argc, char **argv)
{
  // ref: dueca/gui/gtk4/GtkHandler, init
  app = gtk_application_new(APPLICATION_ID, G_APPLICATION_NON_UNIQUE);
  g_signal_connect(G_APPLICATION(app), "activate", G_CALLBACK(app_activate),
                   NULL);
  g_application_hold(G_APPLICATION(app));

  // ref: passcontrol
  g_idle_add(call_environment_loop, NULL);

  int res = g_application_run(G_APPLICATION(app), argc, argv);

  // stuff to do here

  return 0;
}