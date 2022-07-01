/* ------------------------------------------------------------------   */
/*      item            : GlutOpenGLHelper.hxx
        made by         : Rene van Paassen
        date            : 060531
        category        : header file
        description     : Common interface class to glut, gtk or
                          whatever-based opengl windowing code
        changes         : 060531 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GlutOpenGLHelper_hxx
#define GlutOpenGLHelper_hxx

#include <OpenGLHelper.hxx>

DUECA_NS_START;

/** This class defines the basic opengl operations. Derived classes
    implement these for specific windowing interfaces. */

class GlutGLWindowHelper: public GLWindowHelper
{
  /** Window id in FLTK. */
  int glut_win_id;

public:
  /** Hide the window. */
  void hide();
  /** Show the window. */
  void show();
  /** Make the window's gl context current. */
  void current();
  /** Change the window's cursor type. */
  void cursor(int i);
  /** Schedule a redraw of the window. */
  void redraw();
  /** Schedule buffer swap after redraw. */
  void swap();
  /** Return the width of the window. */
  int width();
  /** Return the height of the window. */
  int height();
  /** Return the x offset of the window. */
  int xoffset();
  /** Return the y offset of the window. */
  int yoffset();
  /** Constructor. */
  GlutGLWindowHelper();
  /** Open call. */
  void open(const std::string &title, DuecaGLWindow* master,
            int offset_x, int offset_y, int width, int height,
            bool fullscreen = false);

  /** Delete the window. */
  ~GlutGLWindowHelper();
};

/** This class provides an interface to duecaGLwindow for creation of
    windows under different windowing (toolkit) regimes. */
class GlutOpenGLHelper: public OpenGLHelper
{
public:
  /** Constructor. */
  GlutOpenGLHelper(const std::string &name);

  /** Create window. */
  GLWindowHelper* newWindow();

  /** Destructor. */
  ~GlutOpenGLHelper();
};

DUECA_NS_END;

#endif
