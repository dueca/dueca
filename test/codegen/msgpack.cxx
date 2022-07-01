/* ------------------------------------------------------------------   */
/*      item            : msgpack.cxx
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

#include <cassert>
#define MSGPACK_USE_DEFINE_MAP
#include "Object1.hxx"
#include "Object2.hxx"
#include "Object3.hxx"
#include "Object4.hxx"
#include "Object5.hxx"
#include <dueca/CommObjectWriter.hxx>
#include <AmorphStore.hxx>
#include <dueca/MessageBuffer.hxx>



USING_DUECA_NS;

int main()
{
  Object1 o1; o1.i[3] = 3;
  Object2 o2; o2.a[3] = 5.0;
  Object3 o3; o3.a.push_back(1.9);
  Object4 o4; o4.a.push_back(1.9);
  Object5 o5; o5.a.push_back(5.0); o5.extra = 1.0;

  dueca::MessageBuffer buf(2000);
  { msgpack::packer<dueca::MessageBuffer> pk(buf); pk.pack(o1); }
  { msgpack::packer<dueca::MessageBuffer> pk(buf); pk.pack(o2); }
  { msgpack::packer<dueca::MessageBuffer> pk(buf); pk.pack(o3); }
  { msgpack::packer<dueca::MessageBuffer> pk(buf); pk.pack(o4); }
  { msgpack::packer<dueca::MessageBuffer> pk(buf); pk.pack(o5); }

  Object1 o1r;
  Object2 o2r;
  Object3 o3r;
  Object4 o4r;
  Object5 o5r;

  dueca::messagepack::UnpackVisitor
    <dueca::messagepack::msgpack_container_dco,Object1> v1(o1r);
  dueca::messagepack::UnpackVisitor
    <dueca::messagepack::msgpack_container_dco,Object2> v2(o2r);
  dueca::messagepack::UnpackVisitor
    <dueca::messagepack::msgpack_container_dco,Object3> v3(o3r);
  dueca::messagepack::UnpackVisitor
    <dueca::messagepack::msgpack_container_dco,Object4> v4(o4r);
  dueca::messagepack::UnpackVisitor
    <dueca::messagepack::msgpack_container_dco,Object5> v5(o5r);

  std::size_t off = 0;
  bool res1 = msgpack::v2::parse(buf.data(), buf.size(), off, v1);
  cout << "1 result " << res1 << " offset" << off << endl;
  bool res2 = msgpack::v2::parse(buf.data(), buf.size(), off, v2);
  cout << "2 result " << res2 << " offset" << off << endl;
  bool res3 = msgpack::v2::parse(buf.data(), buf.size(), off, v3);
  cout << "3 result " << res3 << " offset" << off << endl;
  bool res4 = msgpack::v2::parse(buf.data(), buf.size(), off, v4);
  cout << "4 result " << res4 << " offset" << off << endl;
  bool res5 = msgpack::v2::parse(buf.data(), buf.size(), off, v5);
  cout << "5 result " << res5 << " offset" << off << endl;
  buf.release();

  assert (o1 == o1r);
  assert (o2 == o2r);
  assert (o3 == o3r);
  assert (o4 == o4r);
  assert (o5 == o5r);
  return 0;
}
