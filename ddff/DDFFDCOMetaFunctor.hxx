/* ------------------------------------------------------------------   */
/*      item            : DDFFDCOMetaFunctor.hxx
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

#ifndef DDFFDCOMetaFunctor_hxx
#define DDFFDCOMetaFunctor_hxx

#include <dueca_ns.h>
#include "DDFFDCOReadFunctor.hxx"
#include "DDFFDCOWriteFunctor.hxx"
#include <dueca/DCOMetaFunctor.hxx>

DDFF_NS_START

/** MetaFunctor to access DCO DDFF facilities

    Gets functors for reading a channel or writing a channel with DDFF
    streams.

*/
class DDFFDCOMetaFunctor: public DCOMetaFunctor
{

public:
  /** Constructor */
  DDFFDCOMetaFunctor();

  /** Destructor */
  ~DDFFDCOMetaFunctor();

  /** Get a channel reading and DDFF file writing functor

      @param stream    Write stream for receiving the data
      @param startend  Pointer to a time spec controlling run/pause in
                       logging
      @returns         A functor object, that can be accepted by a
                       channel token to apply the operation, in this
                       case writing data to ddff stream, while
                       reading it from the channel.
  */
  virtual DDFFDCOReadFunctor*
  getReadFunctor(FileStreamWrite::pointer wstream,
                 const dueca::DataTimeSpec* startend) = 0;

  /** Get a DDFF file reading and DCO channel writing functor

      @param stream    Read stream for reading the data
      @returns         A functor object, that can be accepted by a
                       channel token to apply the operation, in this case
                       reading data from a ddff stream, and writing it
                       to the channel.
  */
  virtual DDFFDCOWriteFunctor*
  getWriteFunctor(bool rtick) = 0;
};

DDFF_NS_END

#endif
