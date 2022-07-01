/* ------------------------------------------------------------------   */
/*      item            : GtkGladeWindow.cxx
        made by         : Rene' van Paassen
        date            : 051017
        category        : body file
        description     :
        changes         : 051017 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define GtkGladeWindow_cxx

#include <list>

#include <dueca-conf.h>
#include "GtkGladeWindow.hxx"
#include "GtkDuecaButtons.hxx"
#include "DuecaPath.hxx"
#define E_CNF
#define W_CNF
#include "debug.h"

DUECA_NS_START

bool GtkGladeWindow::initialised_glade = false;
bool GtkGladeWindow::initialised_gtkmm = false;

GtkGladeWindow::GtkGladeWindow() :
  window(NULL),
  builder(NULL),
  offset_x(-1),
  offset_y(-1),
  size_x(0),
  size_y(0)
{
  //
}

GtkGladeWindow::~GtkGladeWindow()
{
  hide();
  g_object_unref(builder);
}

bool GtkGladeWindow::readGladeFile(const char* file,
                                   const char* mainwidget,
                                   gpointer client,
                                   const GladeCallbackTable *table,
                                   bool connect_signals)
{
  if (builder == NULL) builder = gtk_builder_new();

  gtk_builder_add_from_file(builder, file, NULL);

  // get the main widget
  window = GTK_WIDGET(gtk_builder_get_object(builder, mainwidget));
  if (!window) {
    /* DUECA graphics.

       Cannot find the main widget in the glade builder windowing
       file. Please check the file and main widget name.
    */
    E_CNF("Cannot find main widget " << mainwidget << " in " << file);
    return false;
  }

  // position requested?
  if (offset_x >= 0 && offset_y >= 0) {
    gtk_window_move(GTK_WINDOW(window), offset_x, offset_y);
  }

  // size requested?
  if (size_x > 0 && size_y > 0) {
    gtk_widget_set_size_request(window, size_x, size_y);
  }

  // connect all required buttons.
  if (client && table) {
    connectCallbacks(client, table);
  }

  // and connect signals gobject etc.
  if (connect_signals) {
    this->connectCallbackSymbols();
  }

  // done
  return true;
}

void GtkGladeWindow::connectCallbacks(gpointer client,
                                      const GladeCallbackTable *table)
{
  const GladeCallbackTable *cbl = table;
  while (cbl->widget) {

    // lookup the widget and link the function
    GObject* b = gtk_builder_get_object(builder, cbl->widget);
    if (b) {
      GtkCaller* caller = cbl->func->clone(client);
      caller->setGPointer(cbl->user_data);
      // link the callback function
      g_signal_connect(b, cbl->signal, caller->callback(),
                       caller);
    }

    else {
      // failure to find this widget
      /* DUECA graphics.

         Cannot find a widget in the gtk-builder model, when trying
         to connect the callbacks.
      */
      W_CNF("Cannot find widget " << cbl->widget);
    }

    // to next cbl.
    cbl++;
  }
}

void GtkGladeWindow::connectCallbacksAfter(gpointer client,
                                           const GladeCallbackTable *table)
{
  const GladeCallbackTable *cbl = table;
  while (cbl->widget) {

    // lookup the widget and link the function
    GObject* b = gtk_builder_get_object(builder, cbl->widget);
    if (b) {
      GtkCaller* caller = cbl->func->clone(client);
      caller->setGPointer(cbl->user_data);
      // link the callback function
      g_signal_connect_after(b, cbl->signal, caller->callback(),
                               caller);
    }

    else {
      // failure to find this widget
      /* DUECA graphics.

         Cannot find a widget in the gtk-builder model, when trying
         to connect the callbacks.
      */
      W_CNF("Cannot find widget " << cbl->widget);
      }

    // to next cbl.
    cbl++;
  }
}

void GtkGladeWindow::connectCallbackSymbols(gpointer user_data)
{
  gtk_builder_connect_signals(builder, user_data);
}

GtkWidget* GtkGladeWindow::operator [] (const char* wname)
{
  if (builder) return GTK_WIDGET(gtk_builder_get_object(builder, wname));
  return NULL;
}

GObject* GtkGladeWindow::getObject(const char* wname)
{
  if (builder) return gtk_builder_get_object(builder, wname);
  return NULL;
}

void GtkGladeWindow::show(const char* widget)
{
  if (widget) {
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, widget)));
  }
  else {
    gtk_widget_show_all(window);
  }
}

void GtkGladeWindow::hide(const char* widget)
{
  if (widget) {
    gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, widget)));
  }
  else {
    gtk_widget_hide(window);
  }
}

DUECA_NS_END

