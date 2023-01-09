/* ------------------------------------------------------------------   */
/*      item            : HDF5DCOMetaFunctor.hxx
        made by         : Rene van Paassen
        date            : 170327
        category        : header file
        api             : DUECA_API
        description     :
        changes         : 170327 first version
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef HDF5DCOMetaFunctor_hxx
#define HDF5DCOMetaFunctor_hxx

#include <H5Cpp.h>
#include <DCOMetaFunctor.hxx>
#include <DataTimeSpec.hxx>
#include <HDF5DCOWriteFunctor.hxx>
#include <HDF5DCOReadFunctor.hxx>

STARTHDF5LOG;

USING_DUECA_NS;


/** metafunctor to access DCO HDF5 facilities. Two modes of interaction

    - Get a datatype for writing the whole DCO object as a compound
      HDF5 datatype. Note that this only works for DCO objects with
      fixed-length strings, fixed length arrays (fixvector) and
      variable length arrays (limvector, varvector). DCO with fancier
      stuff like variable length strings, and stl containers do not
      allow this, and throw an exception.

    - Get a functor for reading a channel and writing the data into
      hdf5 format. This creates an hdf5 dataset for each variable in
      the DCO object. Again, limitations apply, the variables may not
      be dco objects for which a datatype cannot be obtained as
      described above. The hdf5 datasets are written as follows:

      @code
      path/tick            - for DUECA tick values
      path/data/<member1>  - first data member
      path/data/<membern>  - datasets for all data members

      for the future, if the DCO has a parent class, the structure
      is deepened further; note that currently the parent data is not
      written.
      path/<Parent>        - ONLY for DCO with parent class
      path/<Parent>/data   - all the members of this parent

      and possibly:
      path/<Parent>/<Parent2>/data   etc.
      @endcode
*/
class HDF5DCOMetaFunctor: public DCOMetaFunctor
{

public:
  /** Constructor */
  HDF5DCOMetaFunctor();

  /** Destructor */
  ~HDF5DCOMetaFunctor();

  /** Get a HDF5 datatype; not always defined! */
  virtual const H5::DataType* operator() ();

  /** Get an arrayed HDF5 channel reading & file writing functor

      @param file      HDF5 file on which to write
      @param path      Base path for writing the data.
      @param chunksize Size of data chunks.
      @param label     Label to be written to the file
      @param startend  Pointer to a time spec controlling run/pause in
                       logging
      @param compress  You can guess
      @param writeticks If true, time is also logged

      @returns         A functor object, that can be accepted by a
                       channel token to apply the operation, in this
                       case writing data to HDF5 file
   */
  virtual HDF5DCOWriteFunctor* getWriteFunctor(std::weak_ptr<H5::H5File> file,
                                               const std::string& path,
                                               size_t chunksize,
                                               const std::string& label,
                                               const dueca::DataTimeSpec*
                                               startend,
                                               bool compress=false,
                                               bool writeticks=true) = 0;


  /** Get an HDF5 channel writing & file reading functor

      @param file      HDF5 file from which to read.
      @param path      Base path for reading the data.
      @param readticks If true, time also read from the file

      @returns         A functor object, that can be accepted by a
                       channel token to apply the operation, in this
                       case writing data to HDF5 file
  */
  virtual HDF5DCOReadFunctor* getReadFunctor(std::weak_ptr<H5::H5File> file,
                                             const std::string& path,
                                             bool readticks=true) = 0;
};

ENDHDF5LOG;

#endif
