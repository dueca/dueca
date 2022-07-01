/* ------------------------------------------------------------------   */
/*      item            : GluiProtocol.hxx
        made by         : Rene van Paassen
        date            : 071112
        category        : header file
        description     :
        changes         : 071112 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GluiProtocol_hxx
#define GluiProtocol_hxx

#include <WindowingProtocol.hxx>

DUECA_NS_START

/** Class that implements/abstracts the connection to the glut + glui
    windowing toolkit.

    Objects of this class can be created in your dueca.mod script with
    the command
    \code
    (make-glui-protocol)
    \endcode

    You need these if you want to use windows with glui
    (http://glui.sourceforge.net/), an OpenGL/GLUT User Interface
    Library, in combination with the GLSweeper objects. GLSweeper
    objects als can be created in your dueca.mod script.
*/
class GluiProtocol: public WindowingProtocol
{
public:
  /** Constructor, script callable. */
  GluiProtocol();

  /** completion flagged. */
  bool complete();

  /** Destructor. */
  ~GluiProtocol();

  /** Init function, initialize windowing toolkit. */
  bool init();

  /** If needed, close of windowing toolkit. */
  bool close();

  /** Do one update of the windows. */
  void sweep();

  /** Parameter table. */
  static const ParameterTable* getParameterTable();

public:
  /** Default script linkage. */
  SCM_FEATURES_DEF;
};

DUECA_NS_END

#endif
