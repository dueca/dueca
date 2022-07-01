/* ------------------------------------------------------------------   */
/*      item            : XMLLOADER.hxx
        template made by: Joost Ellerbroek
        date            : 080129
        category        : header file
        description     :
        changes         : 080129 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/
#ifndef XMLLOADER_HXX
#define XMLLOADER_HXX
DO_DSTATES
#include <tinyxml.h>
#include <Snapshot.hxx>

extern "C" {
#include "MODEL_NAME.h"
}
USING_DUECA_NS

class XMLLOADER: public TiXmlDocument
{
  public: //Construction / destruction
    /// Default constructor
    XMLLOADER();

    /** Constructor accepting a name for the root document,
      * this is then also the default filename
      * \param documentname Name of this document
      */
    XMLLOADER(const char* documentname);

    /// Destructor
    ~XMLLOADER();

  public: //File access methods
    /** Load an xml file using the current document value,
      * returns true if succesful
      */
    bool load();

    /** Load an xml file 'filename'
      * returns true if succesful
      * \param filename Name of the file to be loaded
      */
    bool load(const char* filename);

    /** Save current xml tree to a file,
      * uses the current documentname as filename,
      * returns true if succesful
      */
    bool save() const;

    /** Save current xml tree to a file,
      * returns true if succesful
      * \param filename File to save xml tree to
      */
    bool save(const char* filename) const;

    /** Flag whether to store all variables present in the model,
      * or only the variables present in a previously loaded
      * xml file. true by default.
      */
    void saveCompleteData(bool do_complete = true);

    /** Accept and parse an incoming XmlSnapshot. \see XmlSnapshot */
    bool AcceptXmlSnapshot(const XmlSnapshot & snap);

    /** Fill an outgoing XmlSnapshot. \see XmlSnapshot */
    bool FillXmlSnapshot(XmlSnapshot & snap);

  public: //Get/set RTW structs
    /** Copy data from Input struct to the xml tree,
      * returns true if succesful
      */
    bool getFromInputStruct(const RTW_INPUT_STRUCT* s);

    /** Copy data from Parameter struct to the xml tree,
      * returns true if succesful
      */
    bool getFromParamStruct(const RTW_PARAM_STRUCT* s);

#ifdef NCSTATES
    /** Copy data from Continuous State struct to the xml tree,
      * returns true if succesful
      */
    bool getFromContStates(const RTW_CSTATES_STRUCT* s);
#endif

#ifdef NDSTATES
    /** Copy data from Discrete State struct to the xml tree,
      * returns true if succesful
      */
    bool getFromDicsStates(const RTW_DSTATES_STRUCT* s);
#endif

    /** Copy data from a real_T array to continuous and/or discrete stores in xml tree */
    bool putAllStates(const real_T* s);

    /** Copy data from xml tree to Input struct,
     * returns true if succesful
     */
    bool setInputStruct(RTW_INPUT_STRUCT* s);

    /** Copy data from xml tree to Parameter struct,
     * returns true if succesful
     */
    bool setParamStruct(RTW_PARAM_STRUCT* s);

#ifdef NCSTATES
    /** Copy data from xml tree to Continuous State struct,
     * returns true if succesful
     */
    bool setContStates(RTW_CSTATES_STRUCT* s);
#endif

#ifdef NDSTATES
    /** Copy data from xml tree to Discrete State struct,
     * returns true if succesful
     */
    bool setDiscStates(RTW_DSTATES_STRUCT* s);
#endif

  public: //Get/set data from RTW model
    /** Copy data from RTW model to xml tree,
      * returns true if succesful
      */
    bool importFromModel(const RTW_MODEL_STRUCT* m);

    /** Copy data from xml tree to RTW model,
     * returns true if succesful
     */
    bool exportToModel(RTW_MODEL_STRUCT* m);

  private:
    bool print_complete;
};

#endif
