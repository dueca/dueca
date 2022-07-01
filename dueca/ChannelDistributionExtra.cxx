/* ------------------------------------------------------------------   */
/*      item            : ChannelDistributionExtra.cxx
        made by         : Rene' van Paassen
        date            : 130102
        category        : additional body code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110

const ChannelDistribution operator |
(const ChannelDistribution& d1, const ChannelDistribution& d2)
{
  if (d2.distribution == ChannelDistribution::MULTI_SEND &&
      d1.distribution == ChannelDistribution::MULTI_SEND)
    return ChannelDistribution(d1);
  if (d2.distribution == ChannelDistribution::NO_OPINION)
    return ChannelDistribution(d1);
  if (d1.distribution == ChannelDistribution::NO_OPINION)
    return ChannelDistribution(d2);
  return ChannelDistribution(ChannelDistribution::CONFLICT);
}
