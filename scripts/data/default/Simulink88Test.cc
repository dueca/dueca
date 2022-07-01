/* ------------------------------------------------------------------   */
/*      item            : @Module@Test.cc
        made by         : @author@ (automatically generated)
        date            : @date@
        category        : test file with main
        description     : Encapsulation of the Simulink/rtw model
                          @rtwmodel@
        changes         : @date@ first version
        template changes: 070116 RvP 6.1 version doubles for version 6.4                                  070125 RvP Call sequence for rtw65 is
                                 identical; code also doubles for rtw65
                          160421 RvP Switching over to ert target for
                                 newer RTW.
        language        : C++
*/

#include <cmath>
extern "C" {
#define RTW_GENERATED_S_FUNCTION
#include "rtwtypes.h"
#include "rt_nonfinite.h"
#include <simstruc.h>
#include "@rtwmodel@.h"

#include <rtw_prototypes.h>
#include <rt_sim.h>
}


struct modelSet {
    /** RTW model */
    RT_MODEL_@rtwmodel@_T *M;
    /** Inputs to the model */
    ExtU_@rtwmodel@_T      U;
    /** Outputs of the model */
    ExtY_@rtwmodel@_T      Y;
};

// verify this; assuming all real_T inputs/outputs
const unsigned NOUTPUTS = sizeof(ExtY_@rtwmodel@_T)/sizeof(real_T);
const unsigned NINPUTS = sizeof(ExtU_@rtwmodel@_T)/sizeof(real_T);
const unsigned NCSTATES = sizeof(X_@rtwmodel@_T)/sizeof(real_T);

// the @rtwmodel@.h file defines the assert macro to be null. This is
// not wat we want, we need assert to check the dimensions of the
// model against those of the DUECA simulation. Therefore:
#undef assert
#include <cassert>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <strstream>

using namespace std;

void Usage(int argc, char* argv[])
{
  cout << "Usage:" << endl;
  cout << argv[0] << " [-i n] [-x state_file] [input_file]" << endl;
  cout << "  Without arguments, this calculates a response of the model "
       << "@rtwmodel@" << endl
       << "  for 100 time steps, with an input of all zeros" << endl
       << "  When an input file is given, this file is read, it must contain"
       << "a number of" << endl
       << "  columns equal to the input size of the model." << endl
       << "  The resulting output is printed on the standard output" << endl
       << "  -i n: do n times the output calculation before calling the"
       << " updates" << endl
       << "  -x state_file: output the state into a file" << endl
       << "  -t dt, use time step dt" << endl;
}

class InputData
{
  ifstream fd;
  bool havefile;
  int lcount;
public:
  InputData();
  InputData(const char* fname);

  bool read(double* U);
};

InputData::InputData() :
  havefile(false),
  lcount(100)
{
  //
}

InputData::InputData(const char* fname) :
  fd(fname),
  havefile(fd.good()),
  lcount(0)
{
  if (fd.bad()) {
    cerr << "Error opening file \"" << fname << "\"" << endl;
    exit(1);
  }
}

bool InputData::read(double* U)
{
  if (havefile) {
    for (unsigned jj = 0; jj < NINPUTS; jj++) {
      fd >> U[jj];
      if (fd.eof()) {
        cerr << "End of file, total read " << lcount << " lines" << endl;
        return false;
      }
      else if (!fd.good()) {
        cerr << "File read error at data line " << lcount + 1
             << ", column " << jj+1 << endl;
        return false;
      }
    }
    lcount++;
    return true;
  }
  else if (lcount--) {
    for (unsigned jj = 0; jj < NINPUTS; jj++) {
      U[jj] = 0.0;
    }
    return true;
  }
  return false;
}

const char* getId()
{
  return "";
}
#define E_MOD(A) cerr << A << endl;

int main(int argc, char* argv[])
{
  InputData* idata;
  int initrun = 10;
  ofstream fx;
  extern char* optarg;
  extern int optind;
  double dt = 1.0;

  // -------- argument parsing --------
  while (1) {

    int c = getopt(argc, argv, "i:x:t");
    if (c == -1)
      break;

    switch (c) {
    case 'i': {
      istrstream is(optarg, std::strlen(optarg));
      is >> initrun;
      cerr << "Doing " << initrun
           << " output calls before simulation" << endl;
      break;
    }

    case 'x': {
      fx.open(optarg);
      if (!fx.good()) {
        cerr << "Error opening state output file \"" << optarg
             << '"' << endl;
        exit(1);
      }
      break;
    }
    case 't': {
      istrstream is(optarg, std::strlen(optarg));
      is >> dt;
      cerr << "Using time step " << dt << endl;
      break;
    }
    case 'h': {
      Usage(argc, argv);
      exit(0);
    }
    case '?': {
      Usage(argc, argv);
      exit(1);
    }
    default: {
      cerr << "getopt returned code" << c << endl;
      exit(1);
    }
    }
  }

  if (optind < argc) {
    idata = new InputData(argv[optind]);
  }
  else {
    idata = new InputData();
  }

  // ------ initialisation, copy from constructor -----

  // initialise global stuff
  rt_InitInfAndNaN(sizeof(real_T));

  modelSet *S = new modelSet();
  S->M = @rtwmodel@(&(S->U), &(S->Y));
  if (S->M == NULL) {
    E_MOD(getId() << " Cannot create model \"@rtwmodel@\"");
    exit(1);
  }
  if (rtmGetErrorStatus(S->M) != NULL) {
    E_MOD(getId() << " Error creating model \"@rtwmodel@\"");
    exit(1);
  }

  // further initialisation
  @rtwmodel@_initialize(S->M, &(S->U), &(S->Y));

  // ------ read input file, and run through, producing output ------
  real_T* U = reinterpret_cast<real_T*>(&(S->U));
  double tnext = 0.0;

  while (idata->read(U)) {

    @rtwmodel@_output(S->M, &(S->U), &(S->Y));
    @rtwmodel@_update(S->M, &(S->U), &(S->Y));

    real_T* Y = reinterpret_cast<real_T*>(&(S->Y));

    cout << setw(14) << tnext;
    for (int ii = 0; ii < NOUTPUTS; ii++) {
      cout << setw(14) << Y[ii];
    }
    cout << endl;
    tnext += dt;

    if (fx.good()) {
      real_T *X = reinterpret_cast<real_T*>(S->M->contStates);
      for (unsigned ii = 0; ii < NCSTATES; ii++) fx << setw(14) << X[ii];
      fx << endl;
    }
  }
}

