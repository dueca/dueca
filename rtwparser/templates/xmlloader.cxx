/* ------------------------------------------------------------------   */
/*      item            : XMLLOADER.cxx
        template made by: Joost Ellerbroek
        date            : 080129
        category        : body file
        description     :
        changes         : 080129 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/
#include "XMLLOADER.hxx"
#include <string>
#include <iomanip>
#include <iostream>

// Xml array maximal row width
#ifndef MAX_ROW_WIDTH
#define MAX_ROW_WIDTH 6
#endif

using namespace std;

template<class T> bool getData(TiXmlElement* node, string& varname, T* data, unsigned short arraysize=1)
{
  TiXmlNode* el = node->FirstChild();
  if (el) {
    string stringdata;

    // Catch multirow data
    while (el) {
      if (el->Type() == TiXmlNode::TEXT)
        stringdata += el->ToText()->ValueStr();
      else
        cerr << "Wrong xml type at element " << varname << endl;

      el = el->NextSiblingElement();
      if (el)
        stringdata += ' ';
    }

    istringstream stm;
    stm.exceptions ( istringstream::badbit );
    stm.str(stringdata);
    stm >> skipws; // skip whitespace!
    unsigned short count = arraysize;
    try {
      for (; count--; )
        stm >> *data++;
      return true;
    } catch (istringstream::failure e) {
      cerr << "Error retrieving xml data for " << varname << " at array index " << arraysize -count << endl;
    }
  } else {
    cerr << "Xml element " << varname << " has no data!" << endl;
  }
  return false;
}

template<class T> bool putData(TiXmlElement* node, string& varname, T* data, unsigned short arraysize=1)
{
  int count = arraysize;
  node->Clear();
  do
  {
    string s;
    for (unsigned short i = 0; i < (count > MAX_ROW_WIDTH ? MAX_ROW_WIDTH : count) ; i++) {
      ostringstream stm;
      stm << (i > 0 ? " " : "") << data[i];
      s += stm.str();
    }
    node->LinkEndChild( new TiXmlText(s) );
    count -= MAX_ROW_WIDTH;
  } while (count >= MAX_ROW_WIDTH);
  return true;
}

XMLLOADER::XMLLOADER():
  TiXmlDocument(),
  print_complete(true)
{
}

XMLLOADER::XMLLOADER(const char* documentname):
  TiXmlDocument(documentname),
  print_complete(true)
{
}

XMLLOADER::~XMLLOADER()
{
}

bool XMLLOADER::load()
{
  return TiXmlDocument::LoadFile();
}

bool XMLLOADER::load(const char* filename)
{
  return TiXmlDocument::LoadFile(filename);
}

bool XMLLOADER::save() const
{
  return TiXmlDocument::SaveFile();
}

bool XMLLOADER::save(const char* filename) const
{
  return TiXmlDocument::SaveFile(filename);
}

bool XMLLOADER::AcceptXmlSnapshot(const XmlSnapshot & snap)
{
  if (snap.data) {
    Clear();
    Parse(snap.data);
    return true;
  }
  return false;
}

bool XMLLOADER::FillXmlSnapshot(XmlSnapshot & snap)
{
  // Traverse the document, calling the Printer callback for each node.
  // This will dump a copy of the XML tree in the Printer's buffer.
  TiXmlPrinter printer;
  Accept(&printer);

  // Allocate XmlSnapshot data, and copy from printer
  char* snapdata = snap.AllocAndAccessData(printer.Size());
  memcpy(snapdata, printer.CStr(), printer.Size());

  return true;
}

void XMLLOADER::saveCompleteData(bool do_complete)
{
  print_complete = do_complete;
}

bool XMLLOADER::getFromInputStruct(const RTW_INPUT_STRUCT* s)
{
  TiXmlElement* inputs = FirstChildElement("InitialInputs");
  if (inputs) {
    TiXmlElement* el = inputs->FirstChildElement();
    while (el)
    {
      if (el->Attribute("name")) {
        string varname = el->Attribute("name");
        SET_INPUT
      } else {
          cerr << "Malformed xml element" << endl;
          return false;
      }
      el = el->NextSiblingElement();
    }
    return true;
  }
  return false;
}

bool XMLLOADER::getFromParamStruct(const RTW_PARAM_STRUCT* s)
{
  TiXmlElement* params = FirstChildElement("ModelParameters");
  if (params) {
    TiXmlElement* el = params->FirstChildElement();
    while (el)
    {
      if (el->Attribute("name")) {
        string varname = el->Attribute("name");
        SET_PARAM
      } else {
          cerr << "Malformed xml element" << endl;
          return false;
      }
      el = el->NextSiblingElement();
    }
    return true;
  }
  return false;
}

#ifdef NCSTATES
bool XMLLOADER::getFromContStates(const RTW_CSTATES_STRUCT* s)
{
  TiXmlElement* contstates = FirstChildElement("InitialContinuousStates");
  if (contstates) {
    TiXmlElement* el = contstates->FirstChildElement();
    while (el)
    {
      if (el->Attribute("name")) {
        string varname = el->Attribute("name");
        SET_CONT_STATES
      } else {
          cerr << "Malformed xml element" << endl;
          return false;
      }
      el = el->NextSiblingElement();
    }
    return true;
  }
  return false;
}
#endif

#ifdef NDSTATES
bool XMLLOADER::getFromDicsStates(const RTW_DSTATES_STRUCT* s)
{
  TiXmlElement* discstates = FirstChildElement("InitialDiscreteStates");
  if (discstates) {
    TiXmlElement* el = discstates->FirstChildElement();
    while (el)
    {
      if (el->Attribute("name")) {
        string varname = el->Attribute("name");
        SET_DISC_STATES
      } else {
          cerr << "Malformed xml element" << endl;
          return false;
      }
      el = el->NextSiblingElement();
    }
    return true;
  }
  return false;
}
#endif

bool XMLLOADER::putAllStates(const real_T* s)
{
  bool ret        = true;
  const real_T* p = s;

#ifdef NCSTATES
  ret &= getFromContStates( (RTW_CSTATES_STRUCT*) p);
  p += NCSTATES;
#endif

#ifdef NDSTATES
  ret &= getFromDiscStates( (RTW_CSTATES_STRUCT*) p);
#endif
  return ret;
}

bool XMLLOADER::setInputStruct(RTW_INPUT_STRUCT* s)
{
  TiXmlElement* inputs = FirstChildElement("InitialInputs");
  if (inputs) {
    TiXmlElement* el = inputs->FirstChildElement();
    while (el)
    {
      if (el->Attribute("name")) {
        string varname = el->Attribute("name");
        GET_INPUT
      } else {
        cerr << "Malformed xml element" << endl;
        return false;
      }
      el = el->NextSiblingElement();
    }
    return true;
  }
  return false;
}

bool XMLLOADER::setParamStruct(RTW_PARAM_STRUCT* s)
{
  TiXmlElement* params = FirstChildElement("ModelParameters");
  if (params) {
    TiXmlElement* el = params->FirstChildElement();
    while (el)
    {
      if (el->Attribute("name")) {
        string varname = el->Attribute("name");
        GET_PARAM
      } else {
        cerr << "Malformed xml element" << endl;
        return false;
      }
      el = el->NextSiblingElement();
    }
    return true;
  }
  return false;
}

#ifdef NCSTATES
bool XMLLOADER::setContStates(RTW_CSTATES_STRUCT* s)
{
  TiXmlElement* contstates = FirstChildElement("InitialContinuousStates");
  if (contstates) {
    TiXmlElement* el = contstates->FirstChildElement();
    while (el)
    {
      if (el->Attribute("name")) {
        string varname = el->Attribute("name");
        GET_CONT_STATES
      } else {
        cerr << "Malformed xml element" << endl;
        return false;
      }
      el = el->NextSiblingElement();
    }
    return true;
  }
  return false;
}
#endif

#ifdef NDSTATES
bool XMLLOADER::setDiscStates(RTW_DSTATES_STRUCT* s)
{
  TiXmlElement* discstates = FirstChildElement("InitialDiscreteStates");
  if (discstates) {
    TiXmlElement* el = discstates->FirstChildElement();
    while (el)
    {
      if (el->Attribute("name")) {
        string varname = el->Attribute("name");
        GET_DISC_STATES
      } else {
        cerr << "Malformed xml element" << endl;
        return false;
      }
      el = el->NextSiblingElement();
    }
    return true;
  }
  return false;
}
#endif

bool XMLLOADER::importFromModel(const RTW_MODEL_STRUCT* m)
{
  // If no previous xml structure exists, create base structure
  if (NoChildren() || print_complete)
  {
    // If necessary, clear current xml tree, and populate with new data tree
    Clear();

    // Header
    LinkEndChild( new TiXmlDeclaration( "1.0", "", "" ) );

    // Comment
    TiXmlComment* comment = new TiXmlComment();
    string s_comment = "XML structure describing an initial condition for a RTW model of type MODEL_NAME";
    comment->SetValue(s_comment);
    LinkEndChild( comment );

    // Body
    TiXmlElement *el, *var;

    // Inputs
    el = new TiXmlElement( "InitialInputs" );
    INPUT_XML_TAGS
    LinkEndChild(el);

    // Model Parameters
    el = new TiXmlElement( "ModelParameters" );
    PARAM_XML_TAGS
    LinkEndChild(el);
#ifdef NCSTATES
    // Continuous states
    el = new TiXmlElement( "InitialContinuousStates" );
    CONT_XML_TAGS
    LinkEndChild(el);
#endif
#ifdef NDSTATES
    // Discrete states
    el = new TiXmlElement( "InitialDiscreteStates" );
    DISC_XML_TAGS
    LinkEndChild(el);
#endif
  }

  bool ret = true;

#ifdef NCSTATES
  ret &= getFromContStates( (RTW_CSTATES_STRUCT*) m->ModelData.contStates );
#endif
#ifdef NDSTATES
  ret &= getFromDiscStates( (RTW_DSTATES_STRUCT*) m->Work.dwork );
#endif
  ret &= getFromParamStruct( (RTW_PARAM_STRUCT*) m->ModelData.defaultParam );
  ret &= getFromInputStruct( (RTW_INPUT_STRUCT*) m->ModelData.inputs );

  return ret;
}

bool XMLLOADER::exportToModel(RTW_MODEL_STRUCT* m)
{
  bool ret = true;

#ifdef NCSTATES
  ret &= setContStates( (RTW_CSTATES_STRUCT*) m->ModelData.contStates );
#endif
#ifdef NDSTATES
  ret &= setDiscStates( (RTW_DSTATES_STRUCT*) m->Work.dwork );
#endif
  ret &= setParamStruct( (RTW_PARAM_STRUCT*) m->ModelData.defaultParam );
  ret &= setInputStruct( (RTW_INPUT_STRUCT*) m->ModelData.inputs );

  return ret;
}
