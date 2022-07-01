#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

#include "rtwparser.hxx"
#include "NamedStruct.hxx"
#include "TypeNameDef.hxx"
#include <DuecaPath.ixx>

using namespace std;
extern "C" {
  int yywrap();
  void yyerror(const char* c);
  int yylex();
  extern char *yytext;
  extern int yylineno;
  extern FILE* yyin;
}

// Stream just about anything into a string
template <typename T>
    string& operator<<(string& s, const T& t)
{
  ostringstream os;
  os <<  t;
  s+=os.str();
  return s;
}

static map<string,bool> flags;
int yyparse();

int yywrap()
{
  return 1;
}

void yyerror(const char* c)
{
  cerr << "At :" << yytext << ":" << c << endl;
}

int main(int argc, char** argv)
{
  // Possible command arguments
  flags["--list"]                   = false;
  flags["--parse-discrete-states"]  = false;
  flags["--help"]                   = false;
  ++argv, --argc; // skip past app name
  string filename = "";
  if ( argc > 0 ) {
    for (;argc--;argv++) {
      map<string,bool>::iterator i = flags.begin();
      for (; i != flags.end(); ++i) {
        if (i->first == argv[0]) {
          i->second = true;
          break;
        }
      }
      if (i == flags.end()) filename = argv[0];
    }
  }
  if ( filename != "" )
    yyin = fopen( filename.c_str(), "r");

  if (flags["--help"]) {
    cout  << "Usage: " << "rtwparser [OPTIONS] [FILENAME]" << endl
          << "Generate C++ and MATLAB xml parsers for Real-Time Workshop modules." << endl << endl
          << "--help\t\t\t\tPrint this help message" << endl
          << "--list\t\t\t\tOnly print a list of parsed RTW structs, don't generate code" << endl
          << "--parse-discrete-states\t\tAlso generate code to store RTW discrete states." << endl
          << "\t\t\t\tThis only works if all discrete blocks in your simulation have states!" << endl << endl
          << "If no filename is given, rtwparser will read its input from stdin." << endl;
  } else {
    return yyparse();
  }
}

void writeFromTemplate(ObjectMap *list, const string& templatefile, const char* targetfile, map<string,string> &keymap)
{
  ifstream ifile;
  ofstream ofile;

  ifile.exceptions( ifstream::badbit );
  ofile << noskipws;

  try {
    ifile.open(templatefile.c_str());
    ofile.open(targetfile);
    string chunk;
    while (!ifile.eof())
    {
      getline(ifile, chunk);
      size_t pos;
      for (map<string,string>::iterator i = keymap.begin(); i != keymap.end(); ++i)
        while ((pos = chunk.find(i->first)) != string::npos)
          chunk.replace(pos, i->first.length(), i->second);

      ofile << chunk << endl;
    }
  } catch (const ifstream::failure &e) {
    cout << "Exception reading file" << endl;
  }
  ifile.close();
  ofile.close();
}

string printGetFromXml(NamedStruct *in)
{
  string ret;
  for (ObjectList::const_iterator i = in->children()->begin(); i != in->children()->end(); ++i) {
    ret << "if (varname == \""  << (*i)->getName() << "\") {\n"
        << "          "   << "getData(el, varname, "<< ((*i)->getSize() == 1 ? "&" : "") << "s->" << (*i)->getName() << ", " << (*i)->getSize() << ");\n"
        << "        } else ";
  }
  ret << "{\n"
      << "          cout << \"Unknown parameter \" << varname << endl;\n"
      << "        }";
  return ret;
}

string printPutXml(NamedStruct *in)
{
  string ret;
  for (ObjectList::const_iterator i = in->children()->begin(); i != in->children()->end(); ++i) {
    ret << "if (varname == \""  << (*i)->getName() << "\") {\n"
        << "          "   << "putData(el, varname, "<< ((*i)->getSize() == 1 ? "&" : "") << "s->" << (*i)->getName() << ", " << (*i)->getSize() << ");\n"
        << "        } else ";
  }
  ret << "{\n"
      << "          cout << \"Unknown parameter \" << varname << endl;\n"
      << "        }";
  return ret;
}

string printXmlTags(NamedStruct *in)
{
  string ret;
  for (ObjectList::const_iterator i = in->children()->begin(); i != in->children()->end(); ++i) {
    ret << "\n    var = new TiXmlElement( \""
        << (*i)->getType() << "\" ); var->SetAttribute( \"name\", \""
        << (*i)->getName() << "\" ); el->LinkEndChild(var);";
  }
  ret.erase(0,5);
  return ret;
}

string printPutM(NamedStruct *in, int indent = 0)
{
  string ret, ind, varname;
  if (indent) for (;indent--;) ind << ' '; // Create indent spacing string
  for (ObjectList::const_iterator i = in->children()->begin(); i != in->children()->end(); ++i) {
    varname.clear();
    TypeNameDef* tn = dynamic_cast<TypeNameDef*>(*i);
    if (tn) {
      int count = 1;
      switch (tn->parseComment())
      {
        case TypeNameDef::ERROR_NO_COMMENT:
          cout << "Variable " << tn->getName() << " has no suitable comment to parse, resulting m-file may be useless." << endl;
          return string("");
          break;
        case TypeNameDef::HAS_COMPUTED_PARAM:
          ret << ind << "eval([ 's = num2str(reshape(' get_param(["
              << "subs{" << tn->syslevel << "} '/"
              << tn->block << "' ], '" << tn->var << "') ''', 1, []), 15);' ]);\n";
          varname << '\'' << tn->getName() << '\'';
          break;
        case TypeNameDef::HAS_EXPRESSION:
          ret << ind << "s = num2str(reshape(" << tn->expr << "', 1, []), 15);\n";
          varname << '\'' << tn->getName() << '\'';
          break;
        case TypeNameDef::IS_NOT_PARAM:
          if ( (count = tn->getSize()) == 1)
            ret << ind << "  " << "s = num2str(current_xml_data(count + count_init), 15);\n";
          varname << "[ subs{" << tn->syslevel << "} '/" << tn->block << "' ]";
          break;
      }

      if (count > 1) {
        ret << ind << "for i = 1:" << count << '\n'
            << ind << "  " << "s = num2str(current_xml_data(count + count_init), 15);\n"
            << ind << "  " << "el = docNode.createElement('" << tn->getType() << "');\n"
            << ind << "  " << "el.setAttribute('name', " << varname << ");\n"
            << ind << "  " << "el.setAttribute('index', num2str(count));\n"
            << ind << "  " << "el.appendChild(docNode.createTextNode(s));\n"
            << ind << "  " << "currentNode.appendChild(el);\n"
            << ind << "  " << "count = count + 1;\n"
            << ind << "end\n";
      } else {
        ret << ind << "el = docNode.createElement('" << tn->getType() << "');\n"
            << ind << "el.setAttribute('name', " << varname << ");\n"
            << ind << "el.appendChild(docNode.createTextNode(s));\n"
            << ind << "currentNode.appendChild(el);\n";
      }
    }
  }
  ret.erase(0, ind.length());
  return ret;
}

string printGetM(NamedStruct *in)
{
  string ret;
  for (ObjectList::const_iterator i = in->children()->begin(); i != in->children()->end(); ++i) {
  }
  return ret;
}

void printResult(ObjectMap *list)
{
  map<string,string> c_keymap, m_keymap;
  if (flags["--list"]) {
    for (ObjectMap::const_iterator i = list->begin(); i != list->end(); ++i) {
      cout << i->second->getName() << endl;
      NamedStruct* s = dynamic_cast<NamedStruct*>(i->second);
      if (s) {
        for (ObjectList::const_iterator ii = s->children()->begin(); ii != s->children()->end(); ++ii)
          cout << "    " << (*ii)->getName() << "    Comment: "<< (*ii)->getComment() << endl;
      }
      cout << endl;
    }
  } else {
    string project_name = BaseObject::getModelName();

    NamedStruct *in=0, *param=0, *cont=0, *disc=0;

    // RealTimeWorkshop 6.x
    NamedStruct* base   = dynamic_cast<NamedStruct*>((*list)[string("rtModel_") + project_name]);
    // RealTimeWorkshop 5.x
    if (!base) {
      base   = dynamic_cast<NamedStruct*>((*list)[string("_rtModel_") + project_name + "_Tag"]);
      if (base) {
        in     = dynamic_cast<NamedStruct*>((*list)[string("_ExternalInputs_") + project_name]);
        param  = dynamic_cast<NamedStruct*>((*list)[string("_Parameters_") + project_name]);
        cont   = dynamic_cast<NamedStruct*>((*list)[string("_ContinuousStates_") + project_name]);
        disc   = dynamic_cast<NamedStruct*>((*list)[string("_D_Work_") + project_name]);
        c_keymap["RTW_INPUT_STRUCT"]    = string("_ExternalInputs_") + project_name;
        c_keymap["RTW_PARAM_STRUCT"]    = string("_Parameters_") + project_name;
        c_keymap["RTW_CSTATES_STRUCT"]  = string("_ContinuousStates_") + project_name;
        c_keymap["RTW_DSTATES_STRUCT"]  = string("_D_Work_") + project_name;
        c_keymap["RTW_MODEL_STRUCT"]    = string("_rtModel_") + project_name;
      }
    } else {
      in     = dynamic_cast<NamedStruct*>((*list)[string("ExternalInputs_") + project_name]);
      param  = dynamic_cast<NamedStruct*>((*list)[string("Parameters_") + project_name]);
      cont   = dynamic_cast<NamedStruct*>((*list)[string("ContinuousStates_") + project_name]);
      disc   = dynamic_cast<NamedStruct*>((*list)[string("D_Work_") + project_name]);
      c_keymap["RTW_INPUT_STRUCT"]    = string("ExternalInputs_") + project_name;
      c_keymap["RTW_PARAM_STRUCT"]    = string("Parameters_") + project_name;
      c_keymap["RTW_CSTATES_STRUCT"]  = string("ContinuousStates_") + project_name;
      c_keymap["RTW_DSTATES_STRUCT"]  = string("D_Work_") + project_name;
      c_keymap["RTW_MODEL_STRUCT"]    = string("rtModel_") + project_name;
    }
    if (base) {
      c_keymap["XMLLOADER"]           = project_name + "XmlLoader";
      c_keymap["MODEL_NAME"]          = project_name;

      m_keymap["XMLLOAD"]             = project_name + "XmlLoad";
      m_keymap["XMLSAVE"]             = project_name + "XmlSave";
      m_keymap["MODEL_NAME"]          = project_name;

      m_keymap["PUT_INPUT"]         = (in ? printPutM(in, 4) : "");
      m_keymap["PUT_PARAM"]         = (param ? printPutM(param) : "");
      m_keymap["PUT_CONT_STATES"]   = (cont ? printPutM(cont, 2) : "");
      m_keymap["PUT_DISC_STATES"]   = (disc && flags["--parse-discrete-states"] ? printPutM(disc, 2) : "");

      m_keymap["GET_INPUT"]         = (in ? printGetM(in) : "");
      m_keymap["GET_PARAM"]         = (param ? printGetM(param) : "");
      m_keymap["GET_CONT_STATES"]   = (cont ? printGetM(cont) : "");
      m_keymap["GET_DISC_STATES"]   = (disc && flags["--parse-discrete-states"] ? printGetM(disc) : "");

      c_keymap["GET_INPUT"]         = (in ? printGetFromXml(in) : "");
      c_keymap["GET_PARAM"]         = (param ? printGetFromXml(param) : "");
      c_keymap["GET_CONT_STATES"]   = (cont ? printGetFromXml(cont) : "");
      c_keymap["GET_DISC_STATES"]   = (disc && flags["--parse-discrete-states"] ? printGetFromXml(disc) : "");

      c_keymap["SET_INPUT"]         = (in ? printPutXml(in) : "");
      c_keymap["SET_PARAM"]         = (param ? printPutXml(param) : "");
      c_keymap["SET_CONT_STATES"]   = (cont ? printPutXml(cont) : "");
      c_keymap["SET_DISC_STATES"]   = (disc && flags["--parse-discrete-states"] ? printPutXml(disc) : "");

      c_keymap["INPUT_XML_TAGS"]    = (in ? printXmlTags(in) : "");
      c_keymap["PARAM_XML_TAGS"]    = (param ? printXmlTags(param) : "");
      c_keymap["CONT_XML_TAGS"]     = (cont ? printXmlTags(cont) : "");
      c_keymap["DISC_XML_TAGS"]     = (disc && flags["--parse-discrete-states"] ? printXmlTags(disc) : "");

      c_keymap["DO_DSTATES"]        = (disc && flags["--parse-discrete-states"] ? "#ifndef NDSTATES\n#define NDSTATES\n#endif" : "");
      m_keymap["DO_DSTATES"]        = (disc && flags["--parse-discrete-states"] ? "true" : "false");

      cout << "Generating MATLAB and C++ XML loaders for RTW model \"" << project_name << "\" :" << endl;
      if (in)     cout << in->getSize()     << " Input"             << (in->getSize() > 1 ? "s" : "")     << endl;
      if (param)  cout << param->getSize()  << " Parameter"         << (param->getSize() > 1 ? "s" : "")  << endl;
      if (cont)   cout << cont->getSize()   << " Continuous state"  << (cont->getSize() > 1 ? "s" : "")   << endl;
      if (disc && flags["--parse-discrete-states"])
                  cout << disc->getSize()   << " Discrete state"    << (disc->getSize() > 1 ? "s" : "")   << endl;

      // Write the output files
      string base(install_path); base = base + "/rtwxml/";
      writeFromTemplate(list, base + "xmlloader.cxx", (project_name + "XmlLoader.cxx").c_str(), c_keymap);
      writeFromTemplate(list, base + "xmlloader.hxx", (project_name + "XmlLoader.hxx").c_str(), c_keymap);
      writeFromTemplate(list, base + "xmlload.m", (project_name + "XmlLoad.m").c_str(), m_keymap);
      writeFromTemplate(list, base + "xmlsave.m", (project_name + "XmlSave.m").c_str(), m_keymap);
    } else {
      cerr << "Failed reading RTW struct rtModel: incomplete header?" << endl;
    }
  }
}
