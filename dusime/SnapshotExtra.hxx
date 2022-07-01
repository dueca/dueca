/* ------------------------------------------------------------------   */
/*      item            : SnapshotExtra.hxx
        made by         : Rene' van Paassen
        date            : 130104
        category        : additional header code
        description     :
        changes         : 130102 first version
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

  /** Constructor. The Snapshot constructor at this point allocates
      room for the data you want to send. Use the accessData() method
      to access this room. Careful! There is no protection here,
      please don't mis-use or your application might fail. */
  Snapshot(size_t data_size, SnapCoding coding=UnSpecified);

  /** Constructor. The Snapshot constructor at this point allocates
      room for the data you want to send. Use the accessData() method
      to access this room. Careful! There is no protection here,
      please don't mis-use or your application might fail. */
  Snapshot(dueca::DataWriterArraySize data_size, SnapCoding coding=UnSpecified);

  /** Constructor. The Snapshot constructor at this point allocates
      room for the data you want to send. Use the accessData() method
      to access this room. Careful! There is no protection here,
      please don't mis-use or your application might fail. */
  Snapshot(size_t data_size, const NameSet& originator,
	   SnapCoding coding=UnSpecified);

  /** Constructor from a toml-parsed object */
  Snapshot(const toml::value& coded);

  /** Create a toml object */
  toml::value tomlCode(const std::string& fname=std::string()) const;

  /** return true if the snapshot is to be saved in an external file */
  bool saveExternal() const;

  /** Return an appropriate extension for an external file */
  const char* fileExtension() const;

  /** This directly accesses the data area of the snapshot. Please be
      careful, there is no protection here. */
  inline const char* accessData() const {return data.data();}

  /** This directly accesses the data area of the snapshot. Please be
      careful, there is no protection here. */
  inline char* accessData() {return const_cast<char*>(data.data());}

  /** This initializes, and subsequently accesses the data area of the
  snapshot. Please be careful, there is no protection here. */
  inline char* AllocAndAccessData(uint32_t dsize)
  {
    data.resize(dsize);
    return const_cast<char*>(data.data());
  }

  /** Read back the maximum size of the data */
  inline uint32_t getDataSize() const {return data.size();}

  /** Return the complete snapshot, or a sample. */
  std::string getSample(unsigned size=20) const;

  /** For convenience, additional data_size member */
  size_t data_size;


