/* ------------------------------------------------------------------   */
/*      item            : DuecaGLWidget.hxx
        made by         : Joost Ellerbroek
        date            : 100625
        category        : header file
        description     :
        changes         : 100625 first version
                          180612 RvP, update for gtkmm3
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
*/

#ifndef DuecaGLWidget_hxx
#define DuecaGLWidget_hxx
#include <gtkmm/drawingarea.h>
#include <DuecaGLCanvas.hxx>
//#include "GtkGLWidgetHelper.hxx"

DUECA_NS_START;

class GtkGLWidgetHelper;

// this define is for hmilib; compatibility with gtk2 and gtk3
class DuecaGLWidgetArea;

class DuecaGLWidget : public Gtk::GLArea, public DuecaGLCanvas
{
  /** Defines a class for implementation-dependent data. The data
      needed depends on the GUI toolset(s) supported by this build of
      DUECA. */
  GtkGLWidgetHelper *gtkgl_helper;

public:
  /// Constructor
  DuecaGLWidget(GtkWidget* ctype);

  /// Destructor
  ~DuecaGLWidget();

  /// Initialization for after GtkWidget assignment
  void InitArea();
};

DUECA_NS_END;
#endif
