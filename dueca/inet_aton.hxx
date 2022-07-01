/* ------------------------------------------------------------------   */
/*      item            : inet_aton.hxx
        made by         : Rene' van Paassen
        date            : 1301002
        category        : additional header code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/
#ifndef INET_ATON_HH
#define INET_ATON_HH

inline int inet_aton(const char* address, in_addr* to)
{
  to->s_addr = inet_addr(address);
  return to->s_addr;
}

#endif
