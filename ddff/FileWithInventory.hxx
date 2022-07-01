/* ------------------------------------------------------------------   */
/*      item            : FileWithInventory.hxx
        made by         : Rene van Paassen
        date            : 211227
        category        : header file
        description     :
        changes         : 211227 first version
        language        : C++
        copyright       : (c) 21 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef FileWithInventory_hxx
#define FileWithInventory_hxx

#include "ddff_ns.h"
#include "FileHandler.hxx"
#include <map>
#include <string>

DDFF_NS_START

/** Derived file handler for the DDFF file format.

    This handler uses the stream#0 in a DDFF file for indicating which
    entries/other data is present in the file.

    With this, data sets can be named in the DDFF file, and additional
    information, such as type information, can be added.
 */
class FileWithInventory: public FileHandler
{
public:
  /** Pointer type to preferably use */
  typedef boost::intrusive_ptr<FileWithInventory> pointer;

  /** Entry data in the inventory */
  struct Entry {
    /** Unique identifying key */
    std::string                      key;

    /** ID of the data stream */
    unsigned                         id;

    /** Additional information field */
    std::string                      label;

    /** Constructor */
    Entry(const std::string& key="", unsigned id=0,
          const std::string& label="");
  };

private:
  /** Inventory entries, arranged by entry key */
  std::vector<Entry>                 entries;

  /** Remember when the inventory is not in line with the file */
  bool                               dirty;

  /** Write access to the inventory */
  FileStreamWrite::pointer           w_inventory;

public:
  /** Create and manage a ddff log file with inventory.

      This constructor will use the open call to open the file, and
      if applicable load the current indices from an existing file.
   */
  FileWithInventory(const std::string& fname,
                    Mode mode=Mode::Truncate,
                    unsigned blocksize=4096U);

  /** Default constructor.

      Note that opening the file and loading any existing indices needs to
      be done afterwards.
   */
  FileWithInventory();

  /** Open call, for after using a default constructor.

      Note that when opening an existing file (Read or Append mode),
      the open call needs to be followed by a loadInventory call
      to retrieve the existing inventory.

      @param fname       File to open
      @param mode        Open mode
      @param blocksize   Default blocksize for new write streams
   */
  void open(const std::string& fname, Mode mode=Mode::Truncate,
            unsigned blocksize=4096U);

  /** Load the inventory.

      @param stream0     If supplied, is the stream with inventory
                         information; when not supplied this will
                         be created, and a checkIndices call will
                         inspect the file.
   */
  void loadInventory();

  /** Destructor */
  ~FileWithInventory();

  /** check that the filer is correctly configured.

      @returns  True, if the parent is correct, and the tags can be accessed.
  */
  bool isComplete() const;

  /** Synchronize the inventory to the disk (if changes happened)

      @returns true if writing needed
   */
  bool syncInventory();

  /** Perform an intermediate flush.

      All writers are checked for partial buffers, these are pushed to
      for writing and then written to file. Further writing in these
      buffers is possible when intermediate is set to true.

      @param intermediate   Perform an intermediate flush; after this
                            further writes are still possible. */
  virtual void syncToFile(bool intermediate=true);

protected:
  /** Turn to private, to prevent unnamed streams mixed in */
  using FileHandler::createWrite;

  /** Turn to private, to prevent unnamed streams mixed in */
  using FileHandler::createRead;
public:

  /** Create a write stream and add it to the inventory.

      @param key     Unique identifying key.
      @param label   Additional information.
      @param bufsize Size of the buffers in this stream. Should be a
                     multiple of the handler's blocksize.
      @returns       FileStreamWrite object.
  */
  FileStreamWrite::pointer createNamedWrite(const std::string& key,
                                            const std::string& label,
                                            size_t bufsize=0);

  /** Access a stream based on the key given to a write stream
      @param key     Identifying key.
      @param label   Additional label to store.
      @param bufsize Block size.
      @returns       FileStreamRead object.
  */
  FileStreamRead::pointer findNamedRead(const std::string& key,
                                        unsigned num_cache=3U,
                                        bool slice_indexed=false);
};

DDFF_NS_END

PRINT_NS_START
inline ostream & operator <<
(ostream& os, const dueca::ddff::FileWithInventory::Entry& o)
{
  return os << "Entry(" << o.key << "/" << o.id << " " << o.label << ")";
}
PRINT_NS_END

#endif
