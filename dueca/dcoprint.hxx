/* ------------------------------------------------------------------   */
/*      item            : dcoprint.hxx
        made by         : Rene van Paassen
        date            : 230614
        category        : header file
        description     :
        changes         : 230614 first version
        language        : C++
	api             : DUECA_API
        copyright       : (c) 2023 Rene van Paassen
*/

#pragma once

#include <dueca/CommObjectTraits.hxx>

DUECA_NS_START;

/** @file Print templates for printing dco member variables */

/** Default, print a standard object using standard iostream

    @param s   Stream to print to
    @param obj Object to print
    @returns   Reference to the stream */
template<typename S, typename T>
inline S &dcoprint(S& s, const T& obj, const dco_print_single&)
{ return s << obj; }

/** Print a list or sequence of someting
    
    @param s   Stream to print to
    @param obj Object to print
    @returns   Reference to the stream */
template<typename S, typename T>
inline S &dcoprint(S& s, const T& obj, const dco_print_iterable&)
{
  s << "{";
  size_t size = obj.size();
  for (const auto &e: obj) {
    dcoprint(s, e, typename dco_traits<typename T::value_type>::ptype());
    if (--size) { s << ","; }
  }
  return s << "}";
}

/** Print a pair
    
    @param s   Stream to print to
    @param obj Object to print
    @returns   Reference to the stream */
template<typename S, typename T>
inline S &dcoprint(S& s, const T& obj, const dco_print_pair&)
{
  s << "(";
  dcoprint(s, obj.first, typename dco_traits<typename T::first_type>::ptype()) << ":";
  dcoprint(s, obj.second, typename dco_traits<typename T::second_type>::ptype());
  return s << ")";
}

/** Optionally filled value
    @param s   Stream to print to
    @param obj Object to print
    @returns   Reference to the stream */
template<typename S, typename T>
inline S &dcoprint(S& s, const T& obj, const dco_print_optional&)
{
  if (obj.valid) {
    dcoprint(s, obj.value, typename dco_traits<typename T::value_type>::ptype());
  }
  else {
    s << "(nil)";
  }
  return s;
}

DUECA_NS_END;
