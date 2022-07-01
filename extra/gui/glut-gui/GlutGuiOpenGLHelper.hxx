/* ------------------------------------------------------------------   */
/*      item            : GlutGuiOpenGLHelper.hxx
        made by         : Rene van Paassen
        date            : 060531
        category        : header file
        description     : Common interface class to glut, gtk or
                          whatever-based opengl windowing code
        changes         : 060531 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GlutGuiOpenGLHelper_hxx
#define GlutGuiOpenGLHelper_hxx

#include <OpenGLHelper.hxx>

/*# forward declaration */
class GLUI;

DUECA_NS_START;

/*# forward declaration */
class DuecaGLWindow;
/*# forward declaration */
class GlutGuiOpenGLHelper;


/** This is a class derived from the generic GLWindowHelper, and it
    implements GL connection to a glui + glui based interface.

    All its calls are intended for internal DUECA classes, except for
    one, which returns the glui object on which it is based. Use with
    caution!
*/
class GlutGuiGLWindowHelper: public GLWindowHelper
{
  friend class DuecaGLWindow;
  friend class GlutGuiOpenGLHelper;

  /** Window id in GLUT. */
  int glut_win_id;

  /** Pointer to the GLUI subwindow. */
  GLUI *glui;

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
  GlutGuiGLWindowHelper();
  /** Open call. */
  void open(const std::string &title, DuecaGLWindow* master,
            int offset_x, int offset_y, int width, int height,
            bool fullscreen = false);

  /** Delete the window. */
  ~GlutGuiGLWindowHelper();

public:
};

/** This class provides an interface to duecaGLwindow for creation of
    windows under different windowing (toolkit) regimes. */
class GlutGuiOpenGLHelper: public OpenGLHelper
{
public:
  /** Constructor. */
  GlutGuiOpenGLHelper(const std::string &name);

  /** Create window. */
  GLWindowHelper* newWindow();

  /** Destructor. */
  ~GlutGuiOpenGLHelper();
};

DUECA_NS_END;

#endif
