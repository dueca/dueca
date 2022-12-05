/* ------------------------------------------------------------------   */
/*      item            : GtkMMGladeWindow.cxx
        made by         : Joost Ellerbroek
        date            : 100811
        category        : body file
        description     :
        changes         : 100811 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
*/

#include "GtkGladeWindow.hxx"
#include <gtkmm.h>

DUECA_NS_START

Gtk::Widget* GtkGladeWindow::getWidget(const std::string& name)
{
  if (!initialised_gtkmm) initGtkMM();

  std::map<std::string, Gtk::Widget*>::iterator it = widgets.find(name);
  if (it != widgets.end()) {
    return it->second;
  }
  else {
    if (GtkWidget* cw =
        GTK_WIDGET(gtk_builder_get_object(builder, name.c_str()))) {
      Gtk::Widget* wmm = Glib::wrap(cw);
      if (wmm) {
        widgets[name] = wmm;
        return wmm;
      }
      return NULL;
    }
    else {
      throw (GladeException("Widget \"" + name + "\" does not exist!"));
    }
  }
}

void GtkGladeWindow::initGtkMM()
{
  // Not needed??
  // Gtk::Main::init_gtkmm_internals();
  initialised_gtkmm = true;
}

DUECA_NS_END
