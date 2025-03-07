/* ------------------------------------------------------------------   */
/*      item            : LogLevelCommandExtra.cxx
        made by         : Rene' van Paassen
        date            : 250306
        category        : body file
        description     :
        changes         : 250306 first version
        language        : C++
        copyright       : (c) 2025 DUECA Authors
*/

#define __CUSTOM_MEMBERACCESS_TABLE

static ::dueca::CommObjectMemberAccess
  <LogLevelCommand,int32_t >
  LogLevelCommand_member_node(&LogLevelCommand::node, "node");
static ::dueca::CommObjectMemberAccess
  <LogLevelCommand,LogLevel >
  LogLevelCommand_member_level(&LogLevelCommand::level, "level");
// modified, to directly give access to the (single) Dstring5 member
// in the nested LogCategory union, simplifies writing of LogLevelCommand
// xml/json files.
static ::dueca::CommObjectMemberAccess
  <LogLevelCommand,Dstring<5> >
  LogLevelCommand_member_category(MEMBER_POINTER(LogLevelCommand, category.name), "category");

// assemble the above entries into a table in the order in which they
// appear in the LogLevelCommand object
static const ::dueca::CommObjectDataTable entriestable[] = { 
  { &LogLevelCommand_member_node },
  { &LogLevelCommand_member_level },
  { &LogLevelCommand_member_category },
  { NULL }
};
