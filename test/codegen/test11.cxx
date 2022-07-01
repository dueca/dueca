/* ------------------------------------------------------------------   */
/*      item            : test9.cxx
        made by         : Rene' van Paassen
        date            : 121230
        category        : body file
        description     :
        changes         : 121230 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define test9_cxx
#include <cassert>

//#define MSGPACK_USE_DEFINE_MAP
#include <vector>
#include "Object11.hxx"
#include <AmorphStore.hxx>
#include <dueca/msgpack.hxx>
#include <dueca/MessageBuffer.hxx>


USING_DUECA_NS;

int main()
{
  char buff[1000];

  AmorphStore st(buff, 1000);

  Object11 o1(Enum10::One);
  packData(st, o1);
  Object11 o2(Enum10::Two);
  packDataDiff(st, o2, o1);
  AmorphReStore re(buff, st.getSize());
  Object11 o1c(re);
  Object11 o2c(o1c);
  unPackDataDiff(re, o2c);
  assert(o1 == o1c);
  assert(o2 == o2c);

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
