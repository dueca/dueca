/* ------------------------------------------------------------------   */
/*      item            : test12.cxx
        made by         : Rene' van Paassen
        date            : 230613
        category        : body file
        description     :
        changes         : 230613 first version
        language        : C++
        copyright       : (c) 2023 René van Paassen
        license         : EUPL-1.2
*/

#define test12_cxx
#include <cassert>

//#define MSGPACK_USE_DEFINE_MAP
#include <vector>
#include "Object12.hxx"
#include <AmorphStore.hxx>
#include <dueca/msgpack.hxx>
#include <dueca/MessageBuffer.hxx>

USING_DUECA_NS;

int main()
{
  char buff[1000];

  AmorphStore st(buff, 1000);

  Object12 o1(2, -0.1);
  assert(o1.i1.valid == true);
  assert(o1.f2.valid == true);
  assert(o1.v3.valid == false);
  assert(o1.i1.value == 2);
  

  // msgpack
  dueca::MessageBuffer buf(200);
  msgpack::packer<dueca::MessageBuffer> pk(buf);
  pk.pack(o2);
  std::size_t off = 0;
  Object11 o1d;
  dueca::messagepack::UnpackVisitor
    <dueca::messagepack::msgpack_container_dco,Object11> v(o1d);
  bool res = msgpack::v2::parse(buf.data(), buf.size(), off, v);
  assert(o2 == o1d);
  cout << o2 << o1d << std::endl;
  assert(res);
  return 0;
}
