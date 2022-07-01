/* ------------------------------------------------------------------   */
/*      item            : UCDataclassLink.hxx
        made by         : Rene van Paassen
        date            : 141015
        category        : header file
        description     :
        changes         : 141015 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef UCDataclassLink_hxx
#define UCDataclassLink_hxx

#include "dueca_ns.h"
#include "UCClientHandle.hxx"
#include <map>

DUECA_NS_START

/** Entry map, linking reading clients and available entries for a
    specific Dataclass name. */
struct UCDataclassLink
{
    /** Linked list of entries providing a specific class of data */
    UCEntryLinkPtr entries;

    /** Linked list of clients reading this data */
    UCClientHandleLinkPtr clients;

    UCDataclassLink() : entries(NULL), clients(NULL) {}
};

/** define a shortcut to the entry map type */
typedef std::map<std::string,UCDataclassLink> dataclassmap_type;
typedef UCDataclassLink* UCDataclassLinkPtr;

DUECA_NS_END

#endif
