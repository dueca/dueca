#define __CUSTOM_COMPATLEVEL_110
#define __CUSTOM_COMPATLEVEL_111

#if defined(DUECA_CONFIG_HDF5)
#define __CUSTOM_COMPATLEVEL_HDF_1


// guarded in a separate namespace
namespace PupilRemoteHeadPose_space {

// replacement definition, simply write hdf data in a 16-long array
static dueca::fixvector<16,double> examplefix;

#define __CUSTOM_HDF5_WRITE_FUNCTOR
  
  HDF5DCOWriteFunctor::
  HDF5DCOWriteFunctor(std::weak_ptr<H5::H5File> file,
                      const std::string& path,
                      size_t chunksize,
                      const std::string& label,
                      bool compress, bool writeticks,
                      const dueca::DataTimeSpec* startend) :
    dueca::hdf5log::HDF5DCOWriteFunctor(file, path, chunksize, label,
					6, compress, writeticks,
					startend)
  {
    // add memspaces for all elements
    this->configureDataSet(0, "/data/topic",
                           HOFFSET(PupilRemoteHeadPose, topic),
                           dueca::get_hdf5_elt_type(example.topic),
                           dueca::get_hdf5_elt_length(example.topic));

    this->configureDataSet(1, "/data/camera_extrinsics",
                           HOFFSET(PupilRemoteHeadPose, camera_extrinsics),
                           dueca::get_hdf5_elt_type(example.camera_extrinsics),
                           dueca::get_hdf5_elt_length(example.camera_extrinsics));
    
    this->configureDataSet(2, "/data/camera_poses",
                           HOFFSET(PupilRemoteHeadPose, camera_poses),
                           dueca::get_hdf5_elt_type(example.camera_poses),
                           dueca::get_hdf5_elt_length(example.camera_poses));

    this->configureDataSet(3, "/data/camera_trace",
                           HOFFSET(PupilRemoteHeadPose, camera_trace),
                           dueca::get_hdf5_elt_type(example.camera_trace),
                           dueca::get_hdf5_elt_length(example.camera_trace));

    this->configureDataSet(4, "/data/camera_pose_matrix",
                           HOFFSET(PupilRemoteHeadPose, camera_pose_matrix),
                           dueca::get_hdf5_elt_type(examplefix),
                           dueca::get_hdf5_elt_length(examplefix));

    this->configureDataSet(5, "/data/timestamp",
                           HOFFSET(PupilRemoteHeadPose, timestamp),
                           dueca::get_hdf5_elt_type(example.timestamp),
                           dueca::get_hdf5_elt_length(example.timestamp));

    if (writeticks) {
      dueca::TimeTickType tex;
      this->configureDataSet(6, "/tick", 0,
                             dueca::get_hdf5_elt_type(tex), 1);
    }
  }

  // the functor member used by channel reading code, writes data in HDF5 file
  bool HDF5DCOWriteFunctor::operator() (const void* dpointer,
                                        const dueca::DataTimeSpec& ts)
  {
    while (ts.getValidityEnd() <= startend->getValidityStart()) {
      return true;
    }
    if (ts.getValidityStart() >= startend->getValidityEnd()) {
      return false;
    }
    this->prepareRow();
    unsigned ii = 0;
    this->sets[0].writeNew(dpointer, chunkidx, example.topic);

    this->sets[1].writeNew(dpointer, chunkidx, example.camera_extrinsics);
    
    this->sets[2].writeNew(dpointer, chunkidx, example.camera_poses);

    this->sets[3].writeNew(dpointer, chunkidx, example.camera_trace);

    this->sets[4].writeNew(dpointer, chunkidx, example.camera_pose_matrix);

    this->sets[5].writeNew(dpointer, chunkidx, example.timestamp);

    if (writeticks) {
      this->sets[6].writeNew(&ts);
    }
    return true;
  }

#define __CUSTOM_HDF5_READ_FUNCTOR
  HDF5DCOReadFunctor::
  HDF5DCOReadFunctor(std::weak_ptr<H5::H5File> file,
                     const std::string& path,
                     bool readticks) :
    dueca::hdf5log::HDF5DCOReadFunctor(file, path,
                              6, readticks)
  {
    // add memspaces for all elements
    this->configureDataSet(0, "/data/topic",
                           HOFFSET(PupilRemoteHeadPose, topic),
                           dueca::get_hdf5_elt_type(example.topic),
                           dueca::get_hdf5_elt_length(example.topic));

    this->configureDataSet(1, "/data/camera_extrinsics",
                           HOFFSET(PupilRemoteHeadPose, camera_extrinsics),
                           dueca::get_hdf5_elt_type(example.camera_extrinsics),
                           dueca::get_hdf5_elt_length(example.camera_extrinsics));
    
    this->configureDataSet(2, "/data/camera_poses",
                           HOFFSET(PupilRemoteHeadPose, camera_poses),
                           dueca::get_hdf5_elt_type(example.camera_poses),
                           dueca::get_hdf5_elt_length(example.camera_poses));

    this->configureDataSet(3, "/data/camera_trace",
                           HOFFSET(PupilRemoteHeadPose, camera_trace),
                           dueca::get_hdf5_elt_type(example.camera_trace),
                           dueca::get_hdf5_elt_length(example.camera_trace));

    this->configureDataSet(4, "/data/camera_pose_matrix",
                           HOFFSET(PupilRemoteHeadPose, camera_pose_matrix),
                           dueca::get_hdf5_elt_type(examplefix),
                           dueca::get_hdf5_elt_length(examplefix));

    this->configureDataSet(5, "/data/timestamp",
                           HOFFSET(PupilRemoteHeadPose, timestamp),
                           dueca::get_hdf5_elt_type(example.timestamp),
                           dueca::get_hdf5_elt_length(example.timestamp));

    if (readticks) {
      dueca::TimeTickType tex;
      this->configureDataSet(6, "/tick", 0,
                             dueca::get_hdf5_elt_type(tex), 1);
    }
  }

  bool HDF5DCOReadFunctor::operator() (void* dpointer)
  {
    this->sets[0].readObjectPart(dpointer, example.topic);

    this->sets[1].readObjectPart(dpointer, example.camera_extrinsics);
    
    this->sets[2].readObjectPart(dpointer, example.camera_poses);

    this->sets[3].readObjectPart(dpointer, example.camera_trace);

    this->sets[4].readObjectPart(dpointer, example.camera_pose_matrix);

    this->sets[5].readObjectPart(dpointer, example.timestamp);

    return true;
  }
} // end namespace PupilRemoteHeadPose_space

#endif

namespace dueca {
namespace messagepack {
GobbleVisitor& v_gobble_PupilRemoteHeadPose()
{
  static GobbleVisitor _v("PupilRemoteHeadPose");
  return _v;
}
} // namespace messagepack
} // namespace dueca

