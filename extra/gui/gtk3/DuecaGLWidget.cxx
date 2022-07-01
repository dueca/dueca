/* ------------------------------------------------------------------   */
/*      item            : DuecaGLWidget.cxx
        made by         : Rene van Paassen
        date            : 100625
        category        : body file
        description     :
        changes         : 100625 first version
                          16xxxx copy&modification to gtk3 version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "DuecaGLWidget.hxx"
#include <dueca/Environment.hxx>

#define E_CNF
#define W_CNF
#include "debug.h"

DUECA_NS_START;

extern GdkGLContext *DUECA_GTK3GL_common_gc;

DuecaGLWidget::DuecaGLWidget(DuecaGLWidgetArea* area) :
  DuecaGtkInteraction(GTK_WIDGET(area)),
  area(area)
{
  if (area)
    this->_init();
}

static gboolean on_render(DuecaGLWidgetArea *area,
                          GdkGLContext* context, gpointer self)
{
  reinterpret_cast<DuecaGLWidget*>(self)->display();
  return TRUE;
}

static GdkGLContext *on_context(DuecaGLWidgetArea *area, gpointer self)
{
  return DUECA_GTK3GL_common_gc;
}

static void on_realize(DuecaGLWidgetArea *area, gpointer self)
{
  gtk_gl_area_make_current(area);
  if (gtk_gl_area_get_error(area) != NULL) {
    return;
  }
  reinterpret_cast<DuecaGLWidget*>(self)->initGL();
}

void DuecaGLWidget::_init()
{
  // connect to the render callback
  gtk_gl_area_set_auto_render(area, FALSE);
  if (CSE.getGraphicDepthBufferSize()) {
    gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(area), TRUE);
  }
  if (CSE.getGraphicStencilBufferSize()) {
    gtk_gl_area_set_has_stencil_buffer(GTK_GL_AREA(area), TRUE);
  }
  gtk_gl_area_set_has_alpha(GTK_GL_AREA(area), TRUE);

  g_signal_connect(area, "render", G_CALLBACK(on_render), this);
  g_signal_connect(area, "realize", G_CALLBACK(on_realize), this);
  if (DUECA_GTK3GL_common_gc) {
    g_signal_connect(area, "create-context", G_CALLBACK(on_context), this);
  }

  DuecaGtkInteraction::init(GTK_WIDGET(area));
}

void DuecaGLWidget::redraw()
{
  gtk_gl_area_queue_render(area);
}

void DuecaGLWidget::makeCurrent()
{
  gtk_gl_area_make_current(area);
}

void DuecaGLWidget::init(DuecaGLWidgetArea* area2)
{
  if (area == NULL && area2 != NULL) {
    area = area2;
    this->_init();
  }
  else {
    std::cerr << "DuecaGLWidget improper initialisation" << std::endl;
  }
}


DuecaGLWidget::~DuecaGLWidget()
{

}

void DuecaGLWidget::initGL()
{
  // default implementation; noop
}

DUECA_NS_END;
