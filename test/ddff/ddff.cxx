/* ------------------------------------------------------------------   */
/*      item            : ddff.cxx
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

#define ddff_cxx
#include <FileHandler.hxx>
#include <FileStreamWrite.hxx>
#include <iostream>

using namespace dueca::ddff;

int main()
{
  {
    // should open the file, blocks of 128 bytes, with 12-byte headers
    FileHandler::pointer testlogfile(new FileHandler());

    testlogfile->open("testlog.ddff", FileHandler::Mode::Truncate, 128U);

    // create two write streams
    FileStreamWrite::pointer write0(testlogfile->createWrite());
    FileStreamWrite::pointer write1(testlogfile->createWrite());
    assert(write0->getStreamId() == 0);
    assert(write1->getStreamId() == 1);

    // create iterators on the stream
    {
      auto it0 = write0->iterator();
      auto it1 = write1->iterator();
      const char jack[] = "All work and no play makes Jack a dull boy!\n";
      for (unsigned ii = 4; ii--; ) {
        it0 = std::copy(jack, jack + sizeof(jack), it0);
      }
      // intermediate writing
      unsigned nblocks =  testlogfile->processWrites();
      std::cout << "written, # blocks " << nblocks << std::endl;

      const char jill[] = "All work and no play makes Jill a dull gal!\n";
      for (unsigned ii = 3; ii--; ) {
        it1 =  std::copy(jill, jill + sizeof(jill), it1);
      }
    }
    // next writing
    unsigned nblocks = testlogfile->processWrites();
    std::cout << "written, # blocks " << nblocks << std::endl;
    assert(nblocks==1);

    // removes the streams, ensures sending remaining blocks
    write0.reset(); write1.reset();

    // destructor of the handler ensures writing file
  }

  {
    // now open a read file
    FileHandler::pointer testlogfile
      (new FileHandler("testlog.ddff", FileHandler::Mode::Read, 128U));

    // create two read streams
    FileStreamRead::pointer read0(testlogfile->createRead(0));
    FileStreamRead::pointer read1(testlogfile->createRead(1));
    assert(read0->getStreamId() == 0);
    assert(read1->getStreamId() == 1);

    // run indices on the streams
    testlogfile->checkIndices();

    // process initial loads
    testlogfile->runLoads();

    // create iterators on the stream
    {
      auto it0 = read0->iterator();
      auto it1 = read1->iterator();
      std::string jack;
      std::copy(it0, read0->end(), std::back_inserter(jack));
      std::string jill(it1, read1->end());
      //std::copy(it1, read1->end(), std::back_inserter(jill));
      std::cout << jack << jill;
    }
  }

  // write and read at the same time, with an interruption and resume of
  // further write
  {
    // should open the file, blocks of 128 bytes, with 12-byte headers
    FileHandler::pointer testlogfile
      (new FileHandler("testlogi.ddff", FileHandler::Mode::Truncate, 128U));

    // create two write streams
    FileStreamWrite::pointer write0(testlogfile->createWrite());
    FileStreamWrite::pointer write1(testlogfile->createWrite());
    assert(write0->getStreamId() == 0);
    assert(write1->getStreamId() == 1);

    // create two read streams
    FileStreamRead::pointer read0(testlogfile->createRead(0));
    FileStreamRead::pointer read1(testlogfile->createRead(1));
    assert(read0->getStreamId() == 0);
    assert(read1->getStreamId() == 1);

    // create write iterators on the stream
    {
      auto it0 = write0->iterator();
      auto it1 = write1->iterator();
      const char jack[] = "All work and no play makes Jack a dull boy!\n";
      for (unsigned ii = 4; ii--; ) {
        write0->markItemStart();
        it0 = std::copy(jack, jack + sizeof(jack), it0);
      }
      // intermediate writing
      //unsigned nblocks =  testlogfile->processWrites();
      //std::cout << "written, # blocks " << nblocks << std::endl;

      const char jill[] = "All work and no play makes Jill a dull gal!\n";
      for (unsigned ii = 3; ii--; ) {
        write1->markItemStart();
        it1 =  std::copy(jill, jill + sizeof(jill), it1);
      }
    }

    // next writing
    unsigned nblocks;// =  testlogfile->processWrites();
    //std::cout << "written, # blocks " << nblocks << std::endl;

    // now write all we have to file
    testlogfile->syncToFile();

    // process initial loads for the read streams
    testlogfile->runLoads();

    // create iterators on the read streams and read
    {
      std::string jack {read0->iterator(), read0->end()};
      std::string jill {read1->iterator(), read1->end()};
      std::cout << jack << jill;
    }

    // now write more
    {
      std::cout << "writing additional data" << std::endl;
      auto it0 = write0->iterator();
      auto it1 = write1->iterator();
      const char jack[] = "All work and no play makes Jack a dull boy!\n";
      for (unsigned ii = 2; ii--; ) {
        write0->markItemStart();
        it0 = std::copy(jack, jack + sizeof(jack), it0);
      }

      // intermediate writing
      // unsigned nblocks =  testlogfile->processWrites();
      //std::cout << "written, # blocks " << nblocks << std::endl;

      const char jill[] = "All work and no play makes Jill a dull gal!\n";
      for (unsigned ii = 2; ii--; ) {
        write1->markItemStart();
        it1 =  std::copy(jill, jill + sizeof(jill), it1);
      }
    }

    nblocks =  testlogfile->processWrites();
    std::cout << "written, # blocks " << nblocks << std::endl;

    // now write all we have to file
    testlogfile->syncToFile();

    // process initial loads for the read streams
    testlogfile->runLoads();

    // create new iterators on the read streams
    {
      std::cout << "reading additional data" << std::endl;
      std::string jack {read0->iterator(), read0->end()};
      std::string jill {read1->iterator(), read1->end()};
      std::cout << jack << jill;
    }

    // and again
    std::cout << "Rewind to 0x180" << std::endl;
    testlogfile->checkIndices(0x180);

    testlogfile->runLoads();
    // create new iterators on the read streams
    {
      std::cout << "reading additional data" << std::endl;
      std::string jack {read0->iterator(), read0->end()};
      std::string jill {read1->iterator(), read1->end()};
      std::cout << jack << jill;
    }

    // removes the streams, ensures sending remaining blocks
    write0.reset(); write1.reset();

    // destructor of the handler ensures writing file

  }
  return 0;
}
