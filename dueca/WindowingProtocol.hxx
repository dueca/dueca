/* ------------------------------------------------------------------   */
/*      item            : WindowingProtocol.hxx
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

#ifndef WindowingProtocol_hxx
#define WindowingProtocol_hxx

#include <ScriptCreatable.hxx>
#include <stringoptions.h>
#include <dueca_ns.h>

DUECA_NS_START

/** Helper base class for windowing code. Currently this is
    implementing the windowing code for the "GLSweeper" module, but
    there is no objection against using it for the rest of DUECA's
    windowing stuff, which is currently in "Handler" classes, but
    could be easily split off and re-used. */
class WindowingProtocol: public ScriptCreatable
{
  /** Name of the protocol */
  const vstring name;

public:
  /** Creator
      \param name   Name of the new protocol, must be a permanent
      string. */
  WindowingProtocol(const char* name);

  /** Destructor. */
  virtual ~WindowingProtocol();

  /** Type name information */
  const char* getTypeName();

  /** return the name of this protocol. */
  inline const vstring& getName() const { return name; }

  /** Init function, initialize windowing toolkit. */
  virtual bool init() = 0;

  /** If needed, close of windowing toolkit. */
  virtual bool close() = 0;

  /** Do one update of the windows. */
  virtual void sweep() = 0;

public:
  /** Default script linkage. */
  SCM_FEATURES_DEF;
};

DUECA_NS_END

#endif
