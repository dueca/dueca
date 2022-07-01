/* ------------------------------------------------------------------   */
/*      item            : ddff-msgpack.cxx
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

#define ddff_msgpack_cxx
#include <FileHandler.hxx>
#include <FileStreamWrite.hxx>
#include <iostream>
#include <dueca/fixvector.hxx>
#include "Objectx.hxx"
#include <dueca/msgpack.hxx>
#include <dueca/msgpack-unstream-iter.hxx>
#include <dueca/msgpack-unstream-iter.ixx>

using namespace dueca::ddff;

int main()
{
  FileHandler testlogfile("testlog-diffpack.ddff",
                          FileHandler::Mode::Truncate, 128U);

  // create a write stream
  FileStreamWrite::pointer write0(testlogfile.createWrite());
  // create the read stream
  FileStreamRead::pointer read0(testlogfile.createRead(0, 1U));


  // object to test and write
  dueca::Objectx o1; o1.i[3] = 3;

  auto it0 = write0->iterator();
  write0->markItemStart();
  msgpack::packer<FileStreamWrite> pk(*write0); pk.pack(o1);

  // sync the file
  testlogfile.syncToFile();

  // get indices to any readers
  // testlogfile.checkIndices();
  testlogfile.runLoads();

  dueca::Objectx o2;
  auto rit0 = read0->iterator();
  msgunpack::msg_unpack(rit0, read0->end(), o2);
  assert(o2 == o1);

  return 0;
}
