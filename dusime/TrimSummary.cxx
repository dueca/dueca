/* ------------------------------------------------------------------   */
/*      item            : TrimSummary.cxx
        made by         : Rene' van Paassen
        date            : 060602
        category        : body file
        description     : Instantiates the summary for the trimview
                          purposes
        changes         : 060602 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include <dueca_ns.h>
#include <Summary.hxx>
#include <TrimLink.hxx>
#include <TrimId.hxx>
#include <TrimView.hxx>
// source file with template code
#include <Summary.cxx>

DUECA_NS_START
// template instantiation
template class Summary<TrimId,TrimLink,TrimView>;
template ostream& operator << <TrimId,TrimLink,TrimView>
(ostream& os, const Summary<TrimId,TrimLink,TrimView> & o);

DUECA_NS_END
