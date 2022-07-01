// -*-c++-*-
/* ------------------------------------------------------------------   */
/*      item            : Callback.ixx
        made by         : Rene' van Paassen
        date            : 061221
        category        : template implementation file
        description     : Function object, to be derived for various
                          callback types.
        changes         : 061221 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef Callback_ixx
#define Callback_ixx

#include <Callback.hxx>

DUECA_NS_START

template <class T> void Callback<T>::
operator() (const TimeSpec &t)
{
  ((*obj) .* h) (t);
}

template <class T> void Callback<T>::
print(ostream& os) const
{
  os << "Callback of " << obj->getId();
}

template <class T> Callback<T>::
Callback(T *obj, void (T::*h)(const TimeSpec &t)) :
  obj(obj), h(h)
{
  // no more
}

template <class T> Callback<T>::
~Callback()
{
  // no more
}

DUECA_NS_END

#endif



