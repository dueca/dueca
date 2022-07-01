/* ------------------------------------------------------------------   */
/*      item            : NumericPlaneOutput.cxx
        made by         : Rene' van Paassen
        date            : 001004
        category        : body file
        description     :
        changes         : 001004 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define NumericPlaneOutput_cc
#include "NumericPlaneOutput.hxx"
extern "C" {
#define RT
#define RTW_GENERATED_S_FUNCTION
#include "complete.h"
}
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <iomanip>
#include <errno.h>
#include <strstream>

#define DO_INSTANTIATE
#include <dusime.h>

const char* const NumericPlaneOutput::classname = "numeric-plane-output";


NumericPlaneOutput::NumericPlaneOutput(Entity* e,
                                       const char* part,
                                       const PrioritySpec& ps) :
  SimulationModule(e, classname, part, NULL),
  number(0),
  header_written(false),
  display_plus_tunnel(0),
  t_controls(getId(), NameSet(getEntity(), "PrimaryControls", part)),
  t_output(getId(), NameSet(getEntity(), "SpacePlaneY", part)),
  t_display_type(getId(), NameSet(getEntity(), "DisplaySelect", part)),
  cb(this, &NumericPlaneOutput::doStep),
  do_output(getId(), "numeric output", &cb, ps)
{
  out0.open("/dev/null");
  out1.open("/dev/null");
  do_output.setTrigger(t_output && t_controls);
}

NumericPlaneOutput::~NumericPlaneOutput()
{
  //
}

static const char* fileName(vstring& basename, int no)
{
  static char buf[256];
  strstream s(buf, 256);
  s << basename << setw(4) << setfill('0') << no << ".m" << '\000';
  return buf;
}

static const char* fileNameData(vstring& basename, int no)
{
  static char buf[256];
  strstream s(buf, 256);
  s << basename << setw(4) << setfill('0') << no << ".dat" << '\000';
  return buf;
}

void NumericPlaneOutput::checkHeader()
{
  if (header_written) return;

  time_t tim = time(NULL);
  out1 << "% Configuration data from a simulation session with the CRV\n"
      << "% experiment date and time " << ctime(&tim)
      << "\n%\n"
      << "% Display configuration\ndisplay_conf="
      << display_plus_tunnel / 3 + 1
      << "\n\n% Tunnel/path no\ntunnel_no="
      << display_plus_tunnel % 3 + 1
      << endl;
  header_written = true;
}

void NumericPlaneOutput::switchFiles()
{
  if (!header_written) {

    // make do with the present file
    cout << "Keeping output file " << fileName(basename, number)
         << endl;
  }
  else {

    // close off the present file
    out0 << endl; out0.close(); out1.close();

    // find an extension number that has not been used with this base name
    struct stat buf;
    for (; number < 10000 &&
           stat(fileName(basename, number), &buf) != -1 &&
           stat(fileNameData(basename, number), &buf) != -1 &&
           errno != ENOENT; number++);
    if (number < 10000) {

      // open the file
      out0.open(fileNameData(basename, number));
      out1.open(fileName(basename, number));
      header_written = false;
      cout << "Opening output file " << fileNameData(basename, number)
           << endl;
    }
    else {

      cerr << "Cannot find a free file among " << fileNameData(basename, 0)
           << " to " << fileNameData(basename, number - 1) << endl;
      out0.open("/dev/null");
      out1.open("/dev/null");
    }
  }
}

bool NumericPlaneOutput::setOutputFile(const vstring& f)
{
  // this carries the base name for the file
  basename = f;

  // reset the file search number to zero
  number = 0;

  // switch output files
  header_written = true; switchFiles();

  return true;
}

void NumericPlaneOutput::startModule(const TimeSpec &time)
{
  do_output.switchOn(time);
}

void NumericPlaneOutput::stopModule(const TimeSpec &time)
{
  do_output.switchOff(time);
}

bool NumericPlaneOutput::isPrepared()
{
  return t_output.isValid() && t_controls.isValid()
    && t_display_type.isValid();
}

void NumericPlaneOutput::doStep(const TimeSpec& ts)
{
  const SpacePlaneY* y;
  const PrimaryControls* u;

  // check for changes in display type
  if (t_display_type.getNumWaitingEvents(ts)) {
    const Event<DisplaySelect>* e;
    t_display_type.getNextEvent(e, ts);
    display_plus_tunnel = e->getEventData()->type;
  }

  if (getAndCheckState(ts) == SimulationState::Advance) {
    t_output.getAccess(y, ts);
    t_controls.getAccess(u, ts);

    // before sending data, write, if applicable, the header
    checkHeader();

    // read out the pilot inputs, and write to file
    out0 << ts.getValidityStart() << ' ' << ts.getValidityEnd() << ' '
         << u->stick_roll << ' ' << u->stick_pitch << ' '
         << u->roll_moment << ' ' << u->pitch_moment << ' '
         << u->roll_rate << ' ' << u->pitch_rate;

    // read out all output data, and write to file
    for (int ii = 0; ii < NOUTPUTS; ii++) {
      out0 << ' ' << y->Y[ii];
    }
    out0 << endl;
    t_output.releaseAccess(y);
    t_controls.releaseAccess(u);
  }
}

void NumericPlaneOutput::loadSnapshot(const TimeSpec& ts, const Snapshot& snap)
{
  switchFiles();
}

const ParameterTable*  NumericPlaneOutput::getMyParameterTable()
{
  static const ParameterTable table[] = {
    { "set-output-file", new MemberCall<NumericPlaneOutput,vstring>
      (&NumericPlaneOutput::setOutputFile)},
    { NULL, NULL}
  };
  return table;
}

static TypeCreator<NumericPlaneOutput>
a(NumericPlaneOutput::getMyParameterTable());



