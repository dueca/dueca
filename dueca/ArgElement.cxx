/* ------------------------------------------------------------------   */
/*      item            : ArgElement.cxx
        made by         : Rene' van Paassen
        date            : 180322
        category        : body file
        description     :
        changes         : 180322 first version
        language        : C++
        copyright       : (c) 18 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ArgElement_cxx
#include "ArgElement.hxx"

DUECA_NS_START

ArgElement::ArgElement(unsigned idx, boost::any value) :
  idx(idx), value(value) { }

ArgElement::ArgElement(const ArgElement& o) :
  idx(o.idx), value(o.value) { }

ArgElement&
ArgElement::operator= (const ArgElement& o)
{
  if (this != &o) {
    this->idx = o.idx; this->value = o.value;
  }
  return *this;
}

DUECA_NS_END
