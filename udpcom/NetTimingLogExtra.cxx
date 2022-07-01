/* ------------------------------------------------------------------   */
/*      item            : NetTimingLogExtra.cxx
        made by         : Rene' van Paassen
        date            : 170205
        category        : Additional code for NetTimingLog
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
#include <debug.h>

DUECA_NS_START;

void NetTimingLog::histoLog(unsigned cycletime, unsigned span)
{
  unsigned idx = std::min((cycletime*20) /span, unsigned(19));
  times[idx]++;
  n_points++;
  if (cycletime > t_max) t_max = cycletime;
}

float NetTimingLog::histTime(unsigned idx) const
{
  return float(times[idx])/float(n_points);
}

void NetTimingLog::printhead(std::ostream& s, const std::string& label)
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
    /* DUECA status.

       Network use information is given in NetTimingLog messages. The
       entry label for these messages contains information on the
       condition with which this data is generated. Decoding that
       information failed, indicates an error in DUECA, or incorrect
       use of the timing information channel. */
    W_NET("Cannot decode label, error " << e.what());
  }

  // the label contains the time step; histogram is scaled to this step
  s << "Net cycle time use; cycle period " << dt
    << " number of samples " << nstep << " packet size " << packsize
    << std::endl;
  s << "        tick   tmax [us] t0/msg [us] t/byte [us]";
  for (int ii = 5; ii < 100; ii+=5) {
    s << std::setw(5) << ii << "%";
  }
  s << " >=100%" << std::endl;
}

void NetTimingLog::printline(std::ostream& s, TimeTickType tick) const
{
  s << std::setw(12) << tick << std::setw(12) << t_max
    << std::setw(12) << std::fixed << std::setprecision(1) << net_permessage
    << std::setw(12) << std::fixed << std::setprecision(3) << net_perbyte
    << std::fixed << std::setprecision(3);
  for (unsigned ii = 0; ii < times.size(); ii++) { s << std::setw(6) << histTime(ii); }
  s << std::endl;
}
