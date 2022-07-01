/* ------------------------------------------------------------------   */
/*      item            : msgpack2.cxx
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

#define msgpack2_cxx
#include <cassert>

#define MSGPACK_USE_DEFINE_ARRAY

#include <dueca/CommObjectWriter.hxx>
#include <AmorphStore.hxx>
#include <dueca/MessageBuffer.hxx>
#include <dueca/msgpack.hxx>
#include <dueca/msgpack-unstream.hxx>

USING_DUECA_NS;

int main()
{
#if 0
  dueca::MessageBuffer buf(200);
  int ii = 1;
  msgpack::v1::adaptor::pack<dueca::MessageBuffer>(buf, ii);
  unsigned off = 0u;
  int i2 = 0;
  dueca::msgunpack::msg_unpack(buf, off, i2);
  assert(ii == i2);
#endif

  return 0;
}
