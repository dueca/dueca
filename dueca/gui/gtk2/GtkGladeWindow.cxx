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
#include <fstream>

#include <dueca-conf.h>
#include "GtkGladeWindow.hxx"
#include "GtkDuecaButtons.hxx"
// on rh9, this still has a 'new' in it, and thus compilation fails
extern "C" {
#define new gnew
#include "glade/glade-build.h"
#undef new
}
#include "DuecaPath.hxx"
#define E_CNF
#define W_CNF
#include "debug.h"

DUECA_NS_START

bool GtkGladeWindow::initialised_glade = false;
bool GtkGladeWindow::initialised_gtkmm = false;

GtkGladeWindow::GtkGladeWindow() :
  window(reinterpret_cast<GtkWidget*>(NULL)),
  xmltree(reinterpret_cast<GladeXML*>(NULL)),
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
  g_object_unref(G_OBJECT(xmltree));
}

bool GtkGladeWindow::readGladeFile(const char* file,
                                   const char* mainwidget,
                                   gpointer client,
                                   const GladeCallbackTable *table,
                                   bool main_only)
{

  // glade or builder?
  std::ifstream testfile; testfile.open(file);
  if (!testfile.is_open()) {
    /* DUECA graphics.

       Cannot open the file specified as interface file. Please check
       your code or configuration.
    */
    E_CNF("GtkGladeWindow cannot open file " << file);
    return false;
  }
  bool chooseglade = false;
  std::string line;
  while (!testfile.eof()) {
    std::getline(testfile, line);
    if (line.find("<glade-interface>") != string::npos) {
      chooseglade = true;
      break;
    }
  }
  testfile.close();

  if (chooseglade) {
    if (!initialised_glade) {
      glade_init();
      initialised_glade = true;
    }

    // try to build the interface
    xmltree = glade_xml_new(file, (main_only ? mainwidget : NULL), NULL);

    if (xmltree == NULL) {
      /* DUECA graphics.

         Problem reading the glade file with glade. Please check its
         validity.
       */
      E_CNF("libglade error " << " for file " << file);
      return false;
    }
    g_object_ref_sink(G_OBJECT(xmltree));

    // get the main widget
    window = glade_xml_get_widget(xmltree, mainwidget);
  }
  else {
    if (builder == NULL) {
      builder = gtk_builder_new();
      g_object_ref_sink(G_OBJECT(builder));
    }
    GError *gerror = NULL;
    if (gtk_builder_add_from_file(builder, file, &gerror) != 0) {
      if (gerror) {
        /* DUECA graphics.

           Problem reading the interface file with gtk-builder. Please
           check its validity.
         */
        E_CNF("gtk builder error " << gerror->message <<
              " for file " << file);
        return false;
      }
    }

    // get the main widget
    window = GTK_WIDGET(gtk_builder_get_object(builder, mainwidget));
  }

  if (!window) {
    /* DUECA graphics.

       Cannot find the main widget in the glade windowing file. Please
       check the file and main widget name.
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

  // done
  return true;
}

void GtkGladeWindow::connectCallbacks(gpointer client,
                                      const GladeCallbackTable *table)
{
  const GladeCallbackTable *cbl = table;

  while (cbl->widget) {

    if (builder) {
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
    }
    else {
      // lookup the widget and link the function
      GtkWidget* b = glade_xml_get_widget(xmltree, cbl->widget);
      if (b) {
        GtkCaller* caller = cbl->func->clone(client);
        caller->setGPointer(cbl->user_data);
        // link the callback function
        g_signal_connect(G_OBJECT(b), cbl->signal, caller->callback(),
                         caller);
      }

      else {
        // failure to find this widget
        /* DUECA graphics.

           Cannot find a widget in the glade model, when trying
           to connect the callbacks.
        */
        W_CNF("Cannot find widget " << cbl->widget);
      }
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

    if (builder) {
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
    }
    else {
      // lookup the widget and link the function
      GtkWidget* b = glade_xml_get_widget(xmltree, cbl->widget);
      if (b) {
        GtkCaller* caller = cbl->func->clone(client);
        caller->setGPointer(cbl->user_data);
        // link the callback function
        g_signal_connect_after(G_OBJECT(b), cbl->signal, caller->callback(),
                               caller);
      }

      else {
        // failure to find this widget
        /* DUECA graphics.

           Cannot find a widget in the glade model, when trying
           to connect the callbacks.
        */
        W_CNF("Cannot find widget " << cbl->widget);
      }
    }
    // to next cbl.
    cbl++;
  }
}

GtkWidget* GtkGladeWindow::operator [] (const char* wname)
{
  if (xmltree) return glade_xml_get_widget(xmltree, wname);
  if (builder) return GTK_WIDGET(gtk_builder_get_object(builder, wname));
  return NULL;
}

void GtkGladeWindow::show()
{
  gtk_widget_show(window);
}

void GtkGladeWindow::hide()
{
  gtk_widget_hide(window);
}

DUECA_NS_END

