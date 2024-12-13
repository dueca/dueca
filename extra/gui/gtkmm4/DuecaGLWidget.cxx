/* ------------------------------------------------------------------   */
/*      item            : DuecaGLWidget.cxx
        made by         : Joost Ellerbroek
        date            : 100625
        category        : body file
        description     :
        changes         : 100625 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
*/

#include "DuecaGLWidget.hxx"
#define E_CNF
#define W_CNF
#include "debug.h"
#include "GtkGLWidgetHelper.hxx"
#include <gtkmm/container.h>

DUECA_NS_START;


DuecaGLWidget::DuecaGLWidget(GtkWidget* ctype) :
  Gtk::DrawingArea(GTK_DRAWING_AREA(ctype)),
  DuecaGLCanvas()
{
  gtkgl_helper = new GtkGLWidgetHelper;
  helper = gtkgl_helper;
}


DuecaGLWidget::~DuecaGLWidget()
{
  //
}

void DuecaGLWidget::InitArea()
{
  // DrawingArea needs to be unrealized to enable GL
  if (is_realized()) unrealize();

  Gtk::Container* parent = get_parent();
  if (parent) parent->set_reallocate_redraws(true);

  parent->hide();
  gtk_widget_unrealize(GTK_WIDGET(parent->gobj()));// Protected in Gtk::Widget

  //GtkGLWidgetHelper* h = dynamic_cast<GtkGLWidgetHelper*>(helper);
  gtkgl_helper->gtk_glwidget_id = GTK_WIDGET(gobj());

  gtkgl_helper->init_gl_area(this);

  parent->show();
}

DUECA_NS_END;
