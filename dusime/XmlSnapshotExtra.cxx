/* ------------------------------------------------------------------   */
/*      item            : XmlSnapShotExtra.cxx
        made by         : Rene' van Paassen
        date            : 1301002
        category        : additional body code
        description     :
        changes         : 1301002 first version
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define __CUSTOM_COMPATLEVEL_110

XmlSnapshot::XmlSnapshot(size_t data_size,
                         const NameSet& originator) :
  Snapshot(data_size, originator, Snapshot::XML)
{
  //
}

XmlSnapshot::XmlSnapshot(const NameSet& originator) :
  Snapshot(0, originator, Snapshot::XML)
{
  //
}

XmlSnapshot::XmlSnapshot(XmlSnapshotCommand cmd, const NameSet& originator) :
    Snapshot(1, originator, Snapshot::UnSpecified)
{
  AmorphStore a(accessData(), 1);
  ::packData(a, cmd);
}

XmlSnapshot::XmlSnapshot(const string& xmldata, const NameSet& originator) :
  Snapshot(xmldata.length(), originator, Snapshot::XML)
{
  std::copy(xmldata.begin(), xmldata.end(), data.begin());
}

DUECA_NS_END;
void packData(DUECA_NS::AmorphStore&s,
              const DUECA_NS::XmlSnapshot::XmlSnapshotCommand& cmd)
{ packData(s, uint8_t(cmd)); }

void unPackData(DUECA_NS::AmorphReStore&s,
                DUECA_NS::XmlSnapshot::XmlSnapshotCommand& cmd)
{ uint8_t tmp(s); cmd = DUECA_NS::XmlSnapshot::XmlSnapshotCommand(tmp); }
DUECA_NS_START;
