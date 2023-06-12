/* ------------------------------------------------------------------   */
/*      item            : msgpackx.cxx
        made by         : Rene' van Paassen
        date            : 230612
        category        : body file
        description     :
        changes         : 230612 first version
        language        : C++
        copyright       : (c) 2023 René van Paassen
        license         : EUPL-1.2
*/

#include <cassert>

#define MSGPACK_USE_DEFINE_MAP
#include <dueca/MessageBuffer.hxx>
#include <dueca/msgpack.hxx>
#include "PupilRemoteGaze2.hxx"
#include <iostream>
#include <fstream>

USING_DUECA_NS;

int main()
{
  {
    // load data from a binary file
    ifstream is;
    std::string fname("PupilRemoteGaze2.msgpack");
    is.open(fname.c_str(), ios::binary);
    is.seekg(0, ios::end); size_t length = is.tellg(); is.seekg(0, ios::beg);
    dueca::MessageBuffer buf(length);
    is.read(buf.data(), length); buf.fill = length;
    is.close();
    std::cout << "Read data from " << fname << std::endl;

    std::size_t off = 0;
    for (unsigned ii = 6; ii--; ) {

      // create an object and a visitor
      PupilRemoteGaze2 obj;
      dueca::messagepack::UnpackVisitor
	<dueca::messagepack::msgpack_container_dco,PupilRemoteGaze2> v(obj);
      bool res = msgpack::v2::parse(buf.data(), buf.size(), off, v);
      std::cout << "Object " << ii << ": " << obj << std::endl;
      assert(res);
    }
    assert(buf.size() == off);
  }

  return 0;
}
