/* ------------------------------------------------------------------   */
/*      item            : ddff-inventory.cxx
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

#define ddff_inventory_cxx
#include <FileWithInventory.hxx>
#include <iostream>
#include <dueca/fixvector.hxx>
#include "Objectx.hxx"
#include <dueca/msgpack.hxx>
#include <dueca/msgpack-unstream-iter.hxx>
#include <dueca/msgpack-unstream-iter.ixx>

using namespace dueca::ddff;

int main()
{

  // step 1, create a fresh file
  {

    FileWithInventory testlogfile("testlog-inventory.ddff",
                                  FileHandler::Mode::Truncate, 256U);

    // create a write stream
    FileStreamWrite::pointer write0(testlogfile.createNamedWrite
                                    ("write0", "label for write0", 256U));

    // create the accompanying read stream
    FileStreamRead::pointer read0(testlogfile.findNamedRead("write0", 1U));

    // object to test and write
    dueca::Objectx o1; o1.i[3] = 3;

    auto it0 = write0->iterator();
    write0->markItemStart();
    msgpack::packer<FileStreamWrite> pk(*write0); pk.pack(o1);

    // sync the file
    testlogfile.syncToFile();
    testlogfile.checkIndices();
    testlogfile.runLoads();

    dueca::Objectx o2;
    auto rit0 = read0->iterator();
    msgunpack::msg_unpack(rit0, read0->end(), o2);
    assert(o2 == o1);

    // the file goes out of scope, should write
  }

  // now re-open the file, appending?
  {
    FileWithInventory testlogfile("testlog-inventory.ddff",
                                  FileHandler::Mode::Append, 256U);

    // find or create the write stream
    FileStreamWrite::pointer write0(testlogfile.createNamedWrite
                                    ("write0", "label for write0", 256U));

    // this should load the indices from the file, and re-load the status
    // of the write stream
    //testlogfile.checkIndices();

    // write some more data
    dueca::Objectx o1; o1.i[3] = 4;

    auto it0 = write0->iterator();
    for (unsigned ii = 5; ii--; ) {
      write0->markItemStart();
      msgpack::packer<FileStreamWrite> pk(*write0); pk.pack(o1);
    }

    // and a new write stream
    FileStreamWrite::pointer write1(testlogfile.createNamedWrite
                                    ("write1", "label for write1", 256U));

    for (unsigned ii = 5; ii--; ) {
      write1->markItemStart();
      msgpack::packer<FileStreamWrite> pk(*write1); pk.pack(o1);
    }

    // sync the file
    testlogfile.syncToFile();

    // and also read it
    FileStreamRead::pointer read0(testlogfile.findNamedRead("write0", 1U));
    testlogfile.runLoads();

    dueca::Objectx o2;
    auto rit0 = read0->iterator();
    msgunpack::msg_unpack(rit0, read0->end(), o2);

    while (rit0 != read0->end()) {
      msgunpack::msg_unpack(rit0, read0->end(), o2);
      assert(o2 == o1);
    }
  }

  return 0;
}
