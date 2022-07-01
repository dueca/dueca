/* ------------------------------------------------------------------   */
/*      item            : ddff-segments.cxx
        made by         : Rene' van Paassen
        date            : 211218
        category        : body file
        description     :
        changes         : 211218 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ddff_segments_cxx
#include <FileWithSegments.hxx>
#include <iostream>
#include <dueca/fixvector.hxx>
#include "Objectx.hxx"
#include <DDFFDataRecorder.hxx>
#include <dueca/msgpack.hxx>
#include <dueca/msgpack-unstream-iter.hxx>
#include <dueca/msgpack-unstream-iter.ixx>

using namespace dueca::ddff;
using namespace dueca;

int main()
{

  // step 1, create a fresh file, should be created through the DDFFDataRecorder
  {
    std::cout << "-- Opening a fresh file" << std::endl;

    DDFFDataRecorder rec1, rec2;

    bool res = false;

    // make a filer
    FileWithSegments::findFiler("entity")->
      openFile(std::string("recorder-test0.ddff"), std::string(), 128U);

    while (res == false) {
      res = true;
      rec1.complete("entity", "key for rec1", "Objectx");
      rec2.complete("entity", "key for rec2", "Objectx");

      res = res && rec1.isValid();
      res = res && rec2.isValid();

      if (res == false) {
        std::cerr << "Recorders not valid yet" << std::endl;
      }
    }

    auto filer =  FileWithSegments::findFiler("entity", false);

    // initiate a recording, gives name and inco
    filer->nameRecording("record1", "inco1");

    // start command tick, time default
    filer->startStretch(100);

    std::cout << "-- Writing data" << std::endl;

    // do some writes, at least a buffer full
    DataTimeSpec ts(80, 100);
    for (unsigned ii = 40; ii--; ) {
      Objectx data; data.i[0] = ii;
      rec1.record(ts, data);
      rec2.record(ts, data);
      ts += 20;
      if (ii % 10 == 0) {
        filer->processWrites();
      }

      // write until 400 complete?
      if (filer->completeStretch(400U)) {
        std::cout << "Completion after writing " << (40 - ii) << std::endl;
        break;
      }
    }

    // now close off, sync to file
    filer->syncToFile();

    std::cout << "-- Replaying" << std::endl;

    // load this segment
    filer->spoolForReplay(0);

    // start tick somewhere off
    filer->startTickReplay(1000);

    DataTimeSpec tsr(1000,1020), tso;
    unsigned ii = 0;
    while (tsr.getValidityEnd() <= 1320) {
      Objectx data;

      if (rec1.replay(tsr, data, tso)) {
        std::cout << "Data replay, " << tso << " verif "
                  << data.i[0] << " count=" << ii << std::endl;
      }
      else {
        std::cout << "False for attempt at " << tsr << std::endl;
      }
      rec2.replay(tsr, data, tso);
      tsr += 20;
      if (++ii % 5 == 0) {
       filer->replayLoad();
      }
    }

    // make sure we lose the filer, it is currently referenced in a map
    FileWithSegments::findFiler("entity", filer.get());

    std::cout << "-- Closing" << std::endl;
  }

  // step 2, retrieve the old file, and use that data
  {
    std::cout << "-- Opening existing file" << std::endl;

    DDFFDataRecorder rec1, rec2;
    FileWithSegments::findFiler("entity")->
      openFile(std::string("recorder-test1.ddff"),
               std::string("recorder-test0.ddff"), 128U);

    bool res = false;
    while (res == false) {
      res = true;
      rec1.complete("entity", "key for rec1", "Objectx");
      rec2.complete("entity", "key for rec2", "Objectx");

      res = res && rec1.isValid();
      res = res && rec2.isValid();

      if (res == false) {
        std::cerr << "Recorders not valid yet" << std::endl;
      }
    }

    std::cout << "-- Replaying 0" << std::endl;

    // check indices to load existing data
    FileWithSegments::findFiler("entity")->checkIndices();

    // can I replay my recording?
    FileWithSegments::findFiler("entity")->spoolForReplay(0);

    // start tick somewhere off
    FileWithSegments::findFiler("entity")->startTickReplay(100);

    DataTimeSpec tsr(100,120), tso;
    unsigned ii = 0;
    while (tsr.getValidityEnd() <= 1300) {
      Objectx data;

      if (rec1.replay(tsr, data, tso)) {
        std::cout << "Data replay, " << tso << " verif "
                  << data.i[0] << " count=" << ii << std::endl;
      }
      else {
        std::cout << "False for attempt at " << tsr << std::endl;
      }
      rec2.replay(tsr, data, tso);
      tsr += 20;
      if (++ii % 5 == 0) {
        FileWithSegments::findFiler("entity")->replayLoad();
      }
    }

    std::cout << "-- Write additional recording" << std::endl;

    // create an additional recording
    FileWithSegments::findFiler("entity")->nameRecording("record2", "inco1");

    // start command tick, time default
    FileWithSegments::findFiler("entity")->startStretch(100);

    // do some writes, at least a buffer full
    DataTimeSpec ts(100, 120);
    for (unsigned ii = 10; ii--; ) {
      Objectx data; data.i[0] = ii;
      rec1.record(ts, data);
      rec2.record(ts, data);
      ts += 20;
      if (ii % 10 == 0) {
        FileWithSegments::findFiler("entity")->processWrites();
      }

      // write until 400 complete?
      if (FileWithSegments::findFiler("entity")->completeStretch(200U)) {
        std::cout << "Completion, cycles left " << ii << std::endl;
        break;
      }
    }

    // now close off, sync to file
    FileWithSegments::findFiler("entity")->syncToFile();

    std::cout << "-- Replaying 1" << std::endl;

    // load this segment
    FileWithSegments::findFiler("entity")->spoolForReplay(1);

    // start tick somewhere off
    FileWithSegments::findFiler("entity")->startTickReplay(1000);

    DataTimeSpec tsr2(1000,1020);
    ii = 0;
    while (tsr.getValidityEnd() <= 1200) {
      Objectx data;

      if (rec1.replay(tsr2, data, tso)) {
        std::cout << "Data replay, " << tso << " verif "
                  << data.i[0] << " count=" << ii << std::endl;
      }
      else {
        std::cout << "False for attempt at " << tsr << std::endl;
      }
      rec2.replay(tsr2, data, tso);
      tsr2 += 20;
      if (++ii % 5 == 0) {
        FileWithSegments::findFiler("entity")->replayLoad();
      }
    }
  }

  return 0;
}
