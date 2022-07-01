/* ------------------------------------------------------------------   */
/*      item            : IncoNoticeExtra.hxx
        made by         : Rene' van Paassen
        date            : 130104
        category        : additional header code
        description     :
        changes         : 1301002 first version
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** Iterator for accessing pairs in the ivlist */
typedef list<IndexValuePair>::iterator iterator;

/** Const version of that iterator */
typedef list<IndexValuePair>::const_iterator const_iterator;

/** Adds an index+value pair to the notice. */
void appendPair(int i, float value);

