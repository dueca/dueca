/* ------------------------------------------------------------------   */
/*      item            : FileWithInventory.cxx
        made by         : Rene' van Paassen
        date            : 211227
        category        : body file
        description     :
        changes         : 211227 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define FileWithInventory_cxx
#include "FileWithInventory.hxx"
#include <msgpack.hpp>
#include <dueca/msgpack-unstream-iter.hxx>
#include "DDFFExceptions.hxx"
#define DEBPRINTLEVEL -1
#include <debprint.h>

namespace msgpack {
  /// @cond
  MSGPACK_API_VERSION_NAMESPACE(v1) {
    /// @endcond
    namespace adaptor {

      /// msgpack pack specialization
      template <>
      struct pack<dueca::ddff::FileWithInventory::Entry> {
        template <typename Stream>
        msgpack::packer<Stream>& operator()
          (msgpack::packer<Stream>& o,
           const dueca::ddff::FileWithInventory::Entry& v) const
        {
          o.pack_array(3);
          o.pack(v.key);
          o.pack(v.id);
          o.pack(v.label);
          return o;
        }
      };
}}}


MSGPACKUS_NS_START;

template<typename S>
  inline void msg_unpack(S& i0, const S& iend,
                         dueca::ddff::FileWithInventory::Entry& e)
{
  uint32_t len = unstream<S>::unpack_arraysize(i0, iend);
  assert(len == 3);
  msg_unpack(i0, iend, e.key);
  msg_unpack(i0, iend, e.id);
  msg_unpack(i0, iend, e.label);
}

MSGPACKUS_NS_END;

DDFF_NS_START

FileWithInventory::FileWithInventory(const std::string& fname, Mode mode,
                                     unsigned blocksize) :
  FileHandler(),
  entries(),
  dirty(true),
  w_inventory(NULL)
{
  this->open(fname, mode, blocksize);
  loadInventory();
}

FileWithInventory::FileWithInventory() :
  FileHandler(),
  entries(),
  dirty(true),
  w_inventory(NULL)
{
  //
}

void FileWithInventory::open(const std::string& fname, Mode mode,
                             unsigned blocksize)
{
  FileHandler::open(fname, mode, blocksize);

  // create the write streams for entry information
  w_inventory = attachWrite(0, blocksize);
}

void FileWithInventory::loadInventory()
{
  // was there an inventory?
  if (file_existing && open_mode != Mode::Truncate) {

    // access the entries definition
    ddff::FileStreamRead::pointer stream0 = createRead(0, 1U);

    // get the data
    runLoads();

    // read current entries
    Entry e;
    for (auto i0 = stream0->iterator(); i0 != stream0->end(); ) {
      msgunpack::msg_unpack(i0, stream0->end(), e);
      DEB1("Loaded entry " << e);
      entries.push_back(e);
    }

    // release the stream from the handler
    requestFileStreamReadRelease(stream0);

    // destruction of the smart pointer will release the object now
  }
}

FileWithInventory::~FileWithInventory()
{
  //
}

bool FileWithInventory::isComplete() const
{
  return bool(w_inventory) && FileHandler::isComplete();
}

FileWithInventory::Entry::
Entry(const std::string& key, unsigned id, const std::string& label) :
  key(key), id(id), label(label) { }

FileStreamWrite::pointer FileWithInventory::
createNamedWrite(const std::string& key,
                 const std::string& label,
                 size_t bufsize)
{
  for (auto const &e: entries) {
    if (e.key == key) {

      // existing entry, check file mode and determine that there is not
      // yet a writer for this
      if (streams.size() <= e.id) { streams.resize(e.id+1); }

      // no writer claimed yet, issue on existing stream, but uninitialised
      if (streams[e.id].writer.get() == NULL) {
        streams[e.id].setWriter(this, e.id, bufsize, file);
        //FileStreamWrite::pointer writer(new FileStreamWrite(this, e.id, 0U));

        //streams[e.id].writer = writer;
        return streams[e.id].writer;
      }

      // writer already claimed
      throw entry_exists();
    }
  }

  // new write stream
  FileStreamWrite::pointer writer = createWrite(bufsize);

  // update the entries array
  entries.emplace_back(key, writer->getStreamId(), label);

  // and add the entry to the inventory
  msgpack::packer<FileStreamWrite> pk(*w_inventory);
  w_inventory->markItemStart(); pk.pack(entries.back());
  dirty = true;
  return writer;
}

bool FileWithInventory::syncInventory()
{
  if (dirty) {
    w_inventory->closeOff(true);
    dirty = false;
    return true;
  }
  return false;
}

FileStreamRead::pointer FileWithInventory::
findNamedRead(const std::string& key, unsigned num_cache, bool slice_indexed)
{
  for (auto const &e: entries) {
    if (e.key == key) {
      return createRead(e.id, num_cache, slice_indexed);
    }
  }
  throw entry_notfound();
}

void FileWithInventory::syncToFile(bool intermediate)
{
  DEB("FileHandler, syncing, im=" << intermediate);

  // collect all remaining buffers
  for (auto w: streams) {
    if (w.writer == w_inventory) {
      syncInventory();
    }
    else if (w.writer.get() != NULL) {
      w.writer->closeOff(intermediate);
    }
  }

  // process the write jobs
  processWrites();
}

DDFF_NS_END
