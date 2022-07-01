/* ------------------------------------------------------------------   */
/*      item            : DDFFDCOReadFunctor.hxx
        made by         : Rene van Paassen
        date            : 211230
        category        : header file
        description     :
        changes         : 211230 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DDFFDCOReadFunctor_hxx
#define DDFFDCOReadFunctor_hxx

#include <dueca_ns.h>
#include <dueca/DCOFunctor.hxx>
#include "ddff_ns.h"
#include "FileStreamWrite.hxx"
#include <dueca/DataTimeSpec.hxx>

DDFF_NS_START

/** Base class for writing DCO data to file. */
class DDFFDCOReadFunctor: public ::dueca::DCOFunctor
{
protected:
  /** The file stream to be written to */
  FileStreamWrite::pointer wstream;

  /** Time span for writing */
  const dueca::DataTimeSpec  *startend;

public:
  /** Constructor */
  DDFFDCOReadFunctor(FileStreamWrite::pointer wstream,
                     const dueca::DataTimeSpec* startend);

  /** Destructor */
  ~DDFFDCOReadFunctor();
};

DDFF_NS_END

#endif
