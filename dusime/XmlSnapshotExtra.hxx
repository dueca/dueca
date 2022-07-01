/* ------------------------------------------------------------------   */
/*      item            : XmlSnapShotExtra.hxx
        made by         : Rene' van Paassen
        date            : 1301002
        category        : additional header code
        description     :
        changes         : 1301002 first version
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

  /// Enumerated values of Xml snapshot commands
  enum XmlSnapshotCommand { PrepareXmlSnapshot, SendXmlSnapshot };

  /** Constructor. Allocates room for an xml snapshot command.
  Use this constructor to send commands to RTW modules to
  prepare for an xml snapshot or to send an xml snapshot. */
  XmlSnapshot(XmlSnapshotCommand cmd, const NameSet& originator);

  /** Constructor. The Snapshot constructor at this point allocates
  room for the data you want to send. Use the accessData() method
  to access this room. Careful! There is no protection here,
  please don't mis-use or your application might fail. */
  XmlSnapshot(size_t data_size, const NameSet& originator);

  /** Constructor. Does not allocate room for data yet. */
  XmlSnapshot(const NameSet& originator);

  /** Constructor. Copies buffer from a string. Useful for instance
      when loading an xml snapshot from a file.
      \param xmldata String containing the xml data
      \param originator NameSet of the RTW module you want to send to */
  XmlSnapshot(const std::string& xmldata, const NameSet& originator);
};

DUECA_NS_END;
void packData(DUECA_NS::AmorphStore&s,
              const DUECA_NS::XmlSnapshot::XmlSnapshotCommand& cmd);

void unPackData(DUECA_NS::AmorphReStore&s,
                DUECA_NS::XmlSnapshot::XmlSnapshotCommand& cmd);
DUECA_NS_START;
struct dum {
