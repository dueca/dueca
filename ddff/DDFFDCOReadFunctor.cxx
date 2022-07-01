/* ------------------------------------------------------------------   */
/*      item            : DDFFDCOReadFunctor.cxx
        made by         : Rene' van Paassen
        date            : 211230
        category        : body file
        description     :
        changes         : 211230 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define DDFFDCOReadFunctor_cxx
#include "DDFFDCOReadFunctor.hxx"

DDFF_NS_START

DDFFDCOReadFunctor::DDFFDCOReadFunctor(FileStreamWrite::pointer wstream,
                                       const dueca::DataTimeSpec* startend) :
  wstream(wstream),
  startend(startend)
{
  // 
}


DDFFDCOReadFunctor::~DDFFDCOReadFunctor()
{

}

DDFF_NS_END
