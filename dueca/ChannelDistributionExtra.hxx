/* ------------------------------------------------------------------   */
/*      item            : ChannelDistributionExtra.hxx
        made by         : Rene' van Paassen
        date            : 130102
        category        : additional header code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** Combine two distribution types. May produce a CONFLICT value. */
  friend const ChannelDistribution operator |
  (const ChannelDistribution& d1, const ChannelDistribution& d2);
