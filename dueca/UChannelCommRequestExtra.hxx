/* ------------------------------------------------------------------   */
/*      item            : UChannelCommRequestExtra.hxx
        made by         : Rene van Paassen
        date            : 140101
        category        : header file
        description     :
        changes         : 140101 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

/** Test whether a command is targeted at the entry */
inline bool isForEntry() { return type <= TimeJump; }

/** Test whether a command is targeted at the master */
inline bool isForMaster() { return type >= NewEntryReq; }

/** Test whether a command is targeted at the channel (changing
    configuration etc) */
inline bool isForChannel()
{ return type >= CleanEntryCmd && type <= DeleteEntryCmd; }

