#include "TypeNameDef.hxx"
#include <sstream>

TypeNameDef::TypeNameDef(const char* type, const char* name, const int size, const char* comm) :
    BaseObject(type, name),
    syslevel(1),
    size(size)
{
  if (string(comm) != "")
    addComment(comm);
}

TypeNameDef::TypeNameDef(const char* type, const char* name, const char* comm) :
  BaseObject(type, name),
  syslevel(1),
  size(1)
{
  if (string(comm) != "")
    addComment(comm);
}

TypeNameDef::~TypeNameDef()
{
}

int TypeNameDef::parseComment()
{
  string cedit = getComment();
  while (cedit[0] == ' ') cedit.erase(0, 1);
  if (cedit.find("Computed Parameter: ", 0) == 0) {
    cedit.erase(0, 20);
    while (cedit[0] == ' ') cedit.erase(0, 1);
    while (cedit[0] != '\'') {
      if (cedit[0] != ' ' && cedit[0] != '\'' && cedit[0] != '\n' && cedit[0] != '\t' && cedit[0] != '\0' && cedit[0] != '\r') var += cedit[0];
      cedit.erase(0, 1);
    }
    cedit.erase(0, 1);
    if (cedit[0] == '<') {
      cedit.erase(0, 2);
      string snum;
      while (cedit[0] != '>') {
        snum += cedit[0];
        cedit.erase(0, 1);
      }
      cedit.erase(0, 2);
      if (snum == "oot") { /* syslevel is Root (R char has already been removed) */
        syslevel = 1;
      } else {
        istringstream stm;
        stm.str(snum);
        stm >> skipws; // skip whitespace!
        stm >> syslevel;
        syslevel++; // Level 1 is the model root
      }
    }
    while (cedit[0] != '\'') {
      block += cedit[0];
      cedit.erase(0, 1);
    }
    return HAS_COMPUTED_PARAM;

  } else if (cedit.find("Expression: ", 0) == 0) {
    cedit.erase(0, 12);
    while (cedit[0] == ' ') cedit.erase(0, 1);
    if (cedit[0] == '[') {
      while (cedit[0] != ']') {
        expr += cedit[0];
        cedit.erase(0, 1);
      }
      expr += ']';
    } else {
      while (cedit[0] != ' ' && cedit[0] != '\n' && cedit[0] != '\t' && cedit[0] != '\0' && cedit[0] != '\r') {
        expr += cedit[0];
        cedit.erase(0, 1);
      }
    }
    return HAS_EXPRESSION;
  } else if (cedit[0] == '\'') {
    cedit.erase(0, 3);
    string snum;
    while (cedit[0] != '>') {
      snum += cedit[0];
      cedit.erase(0, 1);
    }
    cedit.erase(0, 2);
    if (snum == "oot") { /* syslevel is Root (R char has already been removed) */
      syslevel = 1;
    } else {
      istringstream stm;
      stm.str(snum);
      stm >> skipws; // skip whitespace!
      stm >> syslevel;
      syslevel++; // Level 1 is the model root: model levels start at 1
    }
    while (cedit[0] != '\'') {
      block += cedit[0];
      cedit.erase(0, 1);
    }
    return IS_NOT_PARAM;

  } else return ERROR_NO_COMMENT;
}
