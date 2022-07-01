/* ------------------------------------------------------------------   */
/*      item            : NetCapacityLogExtra.cxx
        made by         : Rene' van Paassen
        date            : 170205
        category        : Additional code for NetCapacityLog
        description     :
        changes         : 170205 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

DUECA_NS_END;
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <dueca/debug.h>
DUECA_NS_START;

NetCapacityLog::NetCapacityLog(const uint16_t& node_id) :
    node_id(node_id),
    n_points(0),
    regular(0),
    total(0)
{
  //
}


void NetCapacityLog::histoLog(unsigned regulart, unsigned fill, unsigned capacity)
{
  unsigned idx = std::min((regulart*10) /capacity, unsigned(9));
  regular[idx]++;
  idx = std::min((fill*10) /capacity, unsigned(9));
  total[idx]++;
  n_points++;
}

float NetCapacityLog::histRegular(unsigned idx) const
{
  return float(regular[idx])/float(n_points);
}

float NetCapacityLog::histTotal(unsigned idx) const
{
  return float(total[idx])/float(n_points);
}

void NetCapacityLog::printhead(std::ostream& s, const std::string& label)
{
  // label is coded as nnnnn*dt
  // where nnnnn is the number of samples, dt is the (nominal) time step
  unsigned nstep = 0;
  double dt = 0.01;
  unsigned packsize;
  try {
    std::stringstream ldata(label);
    ldata >> nstep >> dt >> packsize;
  }
  catch (const std::exception& e) {
    /* DUECA network.

       The channel entry label for a network capacity message cannot be
       decoded. Internal DUECA error, or someone is using the channel for
       messages that are not related to network capacity.
    */
    W_NET("Cannot decode label, error " << e.what());
  }

  // the label contains the time step; histogram is scaled to this step
  s << "Net cycle bandwidth use; cycle period " << dt
    << " number of samples " << nstep << " packet size " << packsize
    << std::endl
    << setw(10*6+18) << "regular message size"
    << setw(10*6+2) << "total message size" << std::endl;
  s << "        tick  node";
  for (int ii = 0; ii < 10; ii++) {
    s << std::setw(5) << (ii+1)*10 << "%";
  }
  s << "  ";
  for (int ii = 0; ii < 10; ii++) {
    s << std::setw(5) << (ii+1)*10 << "%";
  }
  s << std::endl;
}

void NetCapacityLog::printline(std::ostream& s, TimeTickType tick) const
{
  s << std::setw(12) << tick << std::setw(6) << node_id
    << std::fixed << std::setprecision(3);
  for (unsigned ii = 0; ii < regular.size(); ii++) {
    s << std::setw(6) << histRegular(ii);
  }
  s << "  ";
  for (unsigned ii = 0; ii < total.size(); ii++) {
    s << std::setw(6) << histTotal(ii);
  }
 s << std::endl;
}

#include <dueca/undebug.h>
