/* ------------------------------------------------------------------   */
/*      item            : GlutProtocol.hxx
        made by         : Rene van Paassen
        date            : 071112
        category        : header file
        description     :
        changes         : 071112 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef GlutProtocol_hxx
#define GlutProtocol_hxx

#include <WindowingProtocol.hxx>

DUECA_NS_START

/** Class that implements/abstracts the connection to the glut
    windowing toolkit. */
class GlutProtocol: public WindowingProtocol
{
public:
  /** Constructor, script callable. */
  GlutProtocol();

  /** completion flagged. */
  bool complete();

  /** Destructor. */
  ~GlutProtocol();

  /** Init function, initialize windowing toolkit. */
  bool init();

  /** If needed, close off windowing toolkit. */
  bool close();

  /** Do one update of the windows. */
  void sweep();

public:
  /** Default script linkage. */
  SCM_FEATURES_DEF;
};

DUECA_NS_END

#endif
