/* ------------------------------------------------------------------   */
/*      item            : DuecaGtkInteraction.hxx
        made by         : Rene van Paassen
        date            : 151013
        category        : header file
        description     :
        changes         : 151013 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DuecaGtkInteraction_hxx
#define DuecaGtkInteraction_hxx

#include <gtk/gtk.h>

#include <dueca_ns.h>
DUECA_NS_START;

/** This class provides feedback from keypresses and mouse movements
    on a Gtk3 widget. It is normally used in combination with GL
    widgets. */
class DuecaGtkInteraction
{
  GtkWidget *wdgt;

protected:
  int width;
  int height;
  int x;
  int y;
  bool dopass;

protected:
  /** Helper function, initialization */
  void init(GtkWidget* widget = NULL);
public:
  void newSize(int w, int h);

  /** Constructor */
  DuecaGtkInteraction(GtkWidget* widget = NULL, int w=0, int h=0);

  /** Destructor */
  ~DuecaGtkInteraction();

  /** Information function, retrieve width */
  int getWidth();

  int getHeight();

  int getXOffset();

  int getYOffset();

  /** pass a previous/default shape */
  void passShape();

  /** callback function, override to get notified of shape changes */
  virtual void reshape(int x, int y);

  /** callback function, override to get notified of keypresses */
  virtual void keyboard(char c, int x, int y);

  /** callback function, override to get notified of special key presses */
  virtual void special(int c, int x, int y);

  /** mouse motion, with pressed key */
  virtual void motion(int x, int y);

  /** mouse motion, nothing pressed */
  virtual void passive(int x, int y);

  /** mouse button press actions */
  virtual void mouse(int button, int state, int x, int y);
};

DUECA_NS_END;
#endif
