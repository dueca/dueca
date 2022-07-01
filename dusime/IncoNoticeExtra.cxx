/* ------------------------------------------------------------------   */
/*      item            : IncoNoticeExtra.hxx
        made by         : Rene' van Paassen
        date            : 130104
        category        : additional body code
        description     :
        changes         : 130102 first version
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define __CUSTOM_COMPATLEVEL_110

void IncoNotice::appendPair(int i, float value)
{
  ivlist.push_back(IndexValuePair(i, value));
}
