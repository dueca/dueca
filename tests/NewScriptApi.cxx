/* ------------------------------------------------------------------   */
/*      item            : NewScriptApi.cxx
        made by         : Rene' van Paassen
        date            : 030509
        category        : body file
        description     :
        changes         : 030509 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define NewScriptApi_cxx
#include "ParameterTable.hxx"
#include "SchemeData.hxx"
#include "VarProbe.hxx"

#define DO_INSTANTIATE
#include "CoreCreator.hxx"
#include "VarProbe.hxx"

class ToBeCreated
{
  int par;
public:

  SCM_FEATURES_DEF;

  /** This contains the per-object Scheme information. */
  mutable SchemeObject      scheme_id;


  ToBeCreated() { cout << "constructor" << endl; }

  ~ToBeCreated() {cout << "destructor" << endl; }

  static const ParameterTable* getParameterTable() {
    static const ParameterTable table[] = {
      { "par" , new VarProbe<ToBeCreated,int>(&ToBeCreated::par) },
      { NULL, NULL} };
    return table;
  }

  bool complete() { cout << "complete, param" << par << endl; }
};

SCM_FEATURES_NOIMP(ToBeCreated, "to-be-created");

int main() {

  CoreCreator<ToBeCreated> c(ToBeCreated::getParameterTable());
}
