/* ------------------------------------------------------------------   */
/*      item            : msgpack4.cxx
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

#define msgpack4_cxx
#include <cassert>

#define MSGPACK_USE_DEFINE_MAP

#include "PupilRemoteHeadPose.hxx"
#include <dueca/CommObjectWriter.hxx>
#include <dueca/DCOtoJSON.hxx>
#include <dueca/JSONtoDCO.hxx>
#include <dueca/smartstring.hxx>
#include <AmorphStore.hxx>
#include <dueca/MessageBuffer.hxx>
#include <limits>
#include <cmath>

USING_DUECA_NS;

int main()
{
#if 1
  {
    // a small one first
    PupilRemoteHeadPose hp, hpc, hps;
    hp.topic = "head_pose";
    hp.timestamp = 0.4;
    dueca::MessageBuffer buf(600);
    {
      msgpack::packer<dueca::MessageBuffer> pk(buf);
      pk.pack(hp);

      // visitor unpack
      dueca::messagepack::UnpackVisitor
        <dueca::messagepack::msgpack_container_dco,PupilRemoteHeadPose> v(hpc);
      std::size_t off = 0;
      bool res = msgpack::v2::parse(buf.data(), buf.size(), off, v);
      
      // stream unpack
      auto i0 = buf.begin();
      auto iend = buf.end();
      msgunpack::msg_unpack(i0, iend, hps);
      assert(res);
      assert(hp == hpc);
      assert(hp == hps);
       
      cout << "base headpose " << res << endl;
      cout << "original" << hp << endl;
      cout << "unpacked" << hpc << endl;
      cout << "unstreamed" << hps << endl;
    }
  }
#endif
 
 
#if 1
  {
    // JSON cycle, with some NaN
    PupilRemoteHeadPose hp, hpc;
    hp.topic = "head_pose";
    hp.timestamp = 0.4;
    hp.camera_trace[0] = std::numeric_limits<double>::quiet_NaN();

    dueca::smartstring smrt;
    smrt.encodejson(hp);
    smrt.decodejson(hpc);
    cout << "JSON: " << smrt << endl;
    cout << "original" << hp << endl;
    cout << "unpacked" << hpc << endl;
    assert(isnan(hp.camera_trace[0]));
    hp.camera_trace[0] = hpc.camera_trace[0] = -1.0;
    assert(hp == hpc);
  }
#endif

#if 1
  {
    // XML cycle, with some NaN
    PupilRemoteHeadPose hp, hpc;
    hp.topic = "head_pose";
    hp.timestamp = 0.4;
    hp.camera_trace[0] = std::numeric_limits<double>::quiet_NaN();

    dueca::smartstring smrt;
    smrt.encodexml(hp);
    smrt.decodexml(hpc);
    cout << "XML: " << smrt << endl;
    cout << "original" << hp << endl;
    cout << "unpacked" << hpc << endl;
    assert(isnan(hp.camera_trace[0]));
    hp.camera_trace[0] = hpc.camera_trace[0] = -1.0;
    assert(hp == hpc);
  }
#endif

  return 0;
}
