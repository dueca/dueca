/* ------------------------------------------------------------------   */
/*      item            : PackTraits.hxx
        made by         : Rene van Paassen
        date            : 131220
        category        : header file
        description     : For the new Comm object reading, the properties
                          of the member classes must be known, iterable,
                          direct, map-like, etcetera. These traits

                          summarise these, and are used for template
                          adaptation
        changes         : 131220 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef PackTraits_hxx
#define PackTraits_hxx

#include <vector>
#include <list>
#include <map>
#include <string>
#include <dueca_ns.h>

DUECA_NS_START;

/* traits, capturing an element for reading */

/** iterable has a constant fixed size, size does not need to be
    packed or unpacked */
struct pack_constant_size { };
/** iterable size is variable and has to be packed */
struct pack_var_size { };
/** at unpacking, the size is unpacked first and a resize is
    performed */
struct unpack_resize { };
/** at unpacking, the size is unpacked first, the list or array is
    cleared, elements are added to the container */
struct unpack_extend { };
/** at unpacking, the size is unpacked first, the map is
    cleared, key/value elements are added to the container */
struct unpack_extend_map { };

template <typename T>
struct pack_traits: public pack_var_size, unpack_extend { };

template <typename D>
struct pack_traits<std::vector<D> >: public pack_var_size, unpack_resize { };

template <typename K, typename D>
struct pack_traits<std::map<K,D> >: public pack_var_size, unpack_extend_map
{ };

/** Compare elements and size, insert/modify/contract
    as needed */
struct diffpack_vector { };
/** Compare elements, size is given, modify as needed */
struct diffpack_fixedsize { };
/** No specific method, simply do a complete pack or not */
struct diffpack_complete { };

template <typename T>
struct diffpack_traits: public diffpack_complete { };
template <typename D>
struct diffpack_traits<std::vector<D> >: public diffpack_vector { };

DUECA_NS_END;

#endif
