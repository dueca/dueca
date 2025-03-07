/* ------------------------------------------------------------------   */
/*      item            : DuecaGLWidget.hxx
        made by         : Joost Ellerbroek
        date            : 100625
        category        : header file
        description     :
        changes         : 100625 first version
                          151013 gtkmm3 variant
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 Joost Ellerbroek/René van Paassen
        license         : EUPL-1.2
*/

#ifndef DuecaGLWidget_hxx
#define DuecaGLWidget_hxx
#include <gtk/gtk.h>
#include <dueca_ns.h>

typedef GtkGLArea DuecaGLWidgetArea;

#include "DuecaGtkInteraction.hxx"

DUECA_NS_START;

/** Provides a DUECA shell around a Gtk3 GtkGlarea. If Gtk3 does not
    have GtkGLarea, tough luck (GTK3 < 3.16).

    After deriving from this class, you should implement
    the GL drawing routine.

    You can also implement callbacks for reshape, initialisation and
    mouse and keyboard events.
*/
class DuecaGLWidget: public DuecaGtkInteraction
{
  /** pointer to the underlying GL area */
  DuecaGLWidgetArea   *area;

  /** initialisation method */
  void _init();
public:
  /// Constructor
  DuecaGLWidget(DuecaGLWidgetArea* ctype = NULL);

  /// Destructor
  ~DuecaGLWidget();

  /** initialisation function. This is only needed if you did not call the
      DuecaGLWidget constructor with a pointer to the DuecaGLWidgetArea */
  void init(DuecaGLWidgetArea* ctype = NULL);

  void selectCursor(int cursor);
  void redraw();
  void makeCurrent();

public:
  //virtual void reshape(int x, int y);

  /** Function to implement in a derived class. Can assume that the GL
      context is current, do not need a swap call at the end! Only draw! */
  virtual void display() = 0;

  /** Function to implement in a derived class. Called with the GL
      context current. */
  virtual void initGL();
};

DUECA_NS_END;

#endif
