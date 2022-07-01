/* ------------------------------------------------------------------   */
/*      item            : DDFFDCOWriteFunctor.hxx
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

#ifndef DDFFDCOWriteFunctor_hxx
#define DDFFDCOWriteFunctor_hxx

#include <dueca_ns.h>
#include <dueca/DCOFunctor.hxx>
#include "FileStreamRead.hxx"
#include <dueca/DataTimeSpec.hxx>
#include "DDFFExceptions.hxx"

DDFF_NS_START

/** Base class for reading DCO data from file. */
class DDFFDCOWriteFunctor: public ::dueca::DCOFunctor
{
protected:

  /** Always the end iterator. */
  FileStreamRead::Iterator itend;

  /** Stream iterator for reading. */
  FileStreamRead::Iterator *it_ptr;

  /** Read also tick, or read clean */
  bool  rtick;

public:
  /** Constructor */
  DDFFDCOWriteFunctor(bool rtick);

  /** Set the iterator to a certain position/copy. 

      Do this before starting to read from the file, and each time the
      reading position jumps.
      
      @param it    Iterator. A copy is made that shares the same position
                   as the supplied iterator.
  */
  void setIterator(FileStreamRead::Iterator& it);
  
  /** Destructor */
  virtual ~DDFFDCOWriteFunctor();
};

DDFF_NS_END

#endif
