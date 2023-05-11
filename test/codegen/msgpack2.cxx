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

#define MSGPACK_USE_DEFINE_MAP

#include "PupilRemotePupil.hxx"
#include "PupilRemoteGaze2.hxx"
#include <dueca/CommObjectWriter.hxx>
#include <AmorphStore.hxx>
#include <dueca/MessageBuffer.hxx>

USING_DUECA_NS;

int main()
{
#if 0
  {
    // a small one first
    PupilRemote2DEllipse eo, eoc;
    eo.angle = 3.0f;
    eo.center[1] = 1.0f;
    eo.axes[0] = 0.2f;
    dueca::MessageBuffer buf(200);
    {
      msgpack::packer<dueca::MessageBuffer> pk(buf);
      pk.pack(eo);
      dueca::messagepack::UnpackVisitor
        <dueca::messagepack::msgpack_container_dco,PupilRemote2DEllipse> v(eoc);
      std::size_t off = 0;
      bool res = msgpack::v2::parse(buf.data(), buf.size(), off, v);
      cout << "small one " << res << endl;
      cout << "original" << eo << endl;
      cout << "unpacked" << eoc << endl;
    }
  }
#endif
#if 0
  {
    // this has int, strings, floats, a fixvector and two simple nested objects
    // all fixed-size
    PupilRemotePupil eo, eoc;
    eo.id = 2;
    eo.circle_3d.radius = 5.0f;
    dueca::MessageBuffer buf(1000);
    {
      msgpack::packer<dueca::MessageBuffer> pk(buf);
      pk.pack(eo);
      dueca::messagepack::UnpackVisitor
        <dueca::messagepack::msgpack_container_dco,PupilRemotePupil> v(eoc);
      std::size_t off = 0;
      bool res = msgpack::v2::parse(buf.data(), buf.size(), off, v);
      cout << "PupilRemotePupil " << res << endl;
      cout << "original" << eo << endl;
      cout << "unpacked" << eoc << endl;
      cout << "buffer size " << buf.size() << endl;
      cout << "equal? " << (eo == eoc) << endl;
    }
    buf.release();
  }
#endif
#if 0
  {
    // this has int, strings, floats, a fixvector and two simple nested objects
    // all fixed-size
    PupilRemoteGaze eo, eoc;
    PupilRemotePupil p;
    p.id = 1; eo.base_data.push_back(p);
    p.id = 2; eo.base_data.push_back(p);
    dueca::MessageBuffer buf(1000);
    {
      msgpack::packer<dueca::MessageBuffer> pk(buf);
      pk.pack(eo);
      dueca::messagepack::UnpackVisitor
        <dueca::messagepack::msgpack_container_dco,PupilRemoteGaze> v(eoc);
      std::size_t off = 0;
      bool res = msgpack::v2::parse(buf.data(), buf.size(), off, v);
      cout << "PupilRemoteGaze " << res << endl;
      cout << "original " << eo << endl;
      cout << "unpacked " << eoc << endl;
      cout << "buffer size " << buf.size() << endl;
      cout << "equal? " << (eo == eoc) << endl;
    }
    buf.release();
  }
#endif
#if 0
  {
    // this has int, strings, floats, a fixvector and two simple nested objects
    // all fixed-size
    PupilRemoteGaze2 eo, eoc;
    PupilRemotePupil p;
    p.id = 3; eo.base_data.push_back(p);
    p.id = 4; eo.base_data.push_back(p);
    eo.gaze_normals_3d["0"] = dueca::fixvector<3,float>(0.1);
    eo.gaze_normals_3d["1"] = dueca::fixvector<3,float>(1.0);
    eo.eye_centers_3d["0"] = dueca::fixvector<3,float>(1.0);
    eo.eye_centers_3d["1"] = dueca::fixvector<3,float>(0.1);
    dueca::MessageBuffer buf(2000);
    {
      msgpack::packer<dueca::MessageBuffer> pk(buf);
      pk.pack(eo);
      dueca::messagepack::UnpackVisitor
        <dueca::messagepack::msgpack_container_dco,PupilRemoteGaze2> v(eoc);
      std::size_t off = 0;
      bool res = msgpack::v2::parse(buf.data(), buf.size(), off, v);
      cout << "PupilRemoteGaze2 " << res << endl;
      cout << "original " << eo << endl;
      cout << "unpacked " << eoc << endl;
      cout << "buffer size " << buf.size() << endl;
      cout << "equal? " << (eo == eoc) << endl;
      assert(eo == eoc);
   }
    buf.release();
  }
#endif

#if 0 // keep
  {
    // alternative unpack
    PupilRemoteGaze2 eo, eoc;
    PupilRemotePupil p;
    p.id = 3; eo.base_data.push_back(p);
    p.id = 4; eo.base_data.push_back(p);
    eo.gaze_normals_3d["0"] = dueca::fixvector<3,float>(0.1);
    eo.gaze_normals_3d["1"] = dueca::fixvector<3,float>(1.0);
    eo.eye_centers_3d["0"] = dueca::fixvector<3,float>(1.0);
    eo.eye_centers_3d["1"] = dueca::fixvector<3,float>(0.1);
    dueca::MessageBuffer buf(2000);
    {
      msgpack::packer<dueca::MessageBuffer> pk(buf);
      pk.pack(eo);
      std::size_t off = 0;
      dueca::msgunpack::msg_unpack(buf, off, eoc);
      cout << "PupilRemoteGaze2 " << endl;
      cout << "original " << eo << endl;
      cout << "unpacked " << eoc << endl;
      cout << "buffer size " << buf.size() << endl;
      cout << "equal? " << (eo == eoc) << endl;
      assert(eo == eoc);
   }
    buf.release();
  }
#endif
  
#if 1
  {
    // now add some data not in the original object
    PupilRemote2DEllipse eo, eoc;
    eo.angle = 3.0f;
    eo.center[1] = 1.0f;
    eo.axes[0] = 0.2f;
    dueca::MessageBuffer buf(200);
    {
      msgpack::packer<dueca::MessageBuffer> pk(buf);
      pk.pack_map(4);
      pk.pack_str(strlen("angle"));
      pk.pack_str_body("angle", strlen("angle"));
      pk.pack(3.0f);
      pk.pack_str(strlen("center"));
      pk.pack_str_body("center", strlen("center"));
      pk.pack_array(2);
      pk.pack(0.0f);
      pk.pack(1.0f);
      pk.pack_str(strlen("extra"));
      pk.pack_str_body("extra", strlen("extra"));
      pk.pack_array(2);
      pk.pack(1.0f);
      pk.pack(1.0f);
      pk.pack_str(strlen("axes"));
      pk.pack_str_body("axes", strlen("axes"));
      pk.pack_array(2);
      pk.pack(0.2f);
      pk.pack(0.0f);
       dueca::messagepack::UnpackVisitor
        <dueca::messagepack::msgpack_container_dco,PupilRemote2DEllipse> v(eoc);
      std::size_t off = 0;
      bool res = msgpack::v2::parse(buf.data(), buf.size(), off, v);
      cout << "with additional stuff " << res << endl;
      cout << "original" << eo << endl;
      cout << "unpacked" << eoc << endl;
    }
  }
#endif



  return 0;
}
