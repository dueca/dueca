/* ------------------------------------------------------------------   */
/*      item            : DCOFunctor.hxx
        made by         : Rene van Paassen
        date            : 170326
        category        : header file
        api             : DUECA_API
        description     : Base class for a functor type, that can be
                          passed to channel tokens for data operation
        changes         : 170326 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DCOFunctor_hxx
#define DCOFunctor_hxx

#include <dueca_ns.h>
#include <DataTimeSpec.hxx>

DUECA_NS_START;

/** Base class for service objects that can handle DCO data in channels.
 */
class DCOFunctor
{

public:
  /** Constructor */
  DCOFunctor();

  /** Destructor */
  virtual ~DCOFunctor();

  /** function base, with const pointer (for channel reading)

      @param dpointer  Pointer to the data object
      @param ts        Time specification for which writing to be done
      @returns         True if the read/inspect operation was successful */
  virtual bool operator() (const void* dpointer, const DataTimeSpec& ts);

  /** function base, updates data object

      @param dpointer  Is filled with a new data object having a copy of the
                       latest data point in the channel, or a blank copy
                       if this is the first write. Update/overwrite this data
      @returns         True if writer successful */
  virtual bool operator() (void* dpointer);
};

DUECA_NS_END
#endif
