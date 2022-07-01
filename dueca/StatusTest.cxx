/* ------------------------------------------------------------------   */
/*      item            : StatusTest.cxx
        made by         : Rene' van Paassen
        date            : 010824
        category        : body file
        description     :
        changes         : 010824 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define StatusTest_cxx

#include <StatusKeeper.hxx>
#include <StatusT1.hxx>
using namespace dueca;

int main()
{
  StatusKeeper<StatusT1> k;

  // add a leaf node to k
  list<vstring> nameparts;
  nameparts.push_back("root");
  nameparts.push_back("second");
  nameparts.push_back("Third");
  ModuleId id(nameparts, GlobalId(2,5));
  StatusT1 c;
  c.setModuleState(ModuleState::On);
  c.setSimulationState(SimulationState::HoldCurrent);
  k.addNode(id, c);

  // add a leaf
  nameparts.pop_back(); nameparts.push_back("leaf");
  ModuleId id2(nameparts, GlobalId(2,6));
  k.addNode(id2, c);

  // print a node, found on the basis of the id
  cout << k.findSummary(id2) << endl;

  // recreate a moduleid on the basis of a global id
  cout << k.findSummary(ModuleId::find(GlobalId(2,5))) << endl;

  // update a status
  StatusT1 c2(c);
  c2.setModuleState(ModuleState::InitialPrep);
  k.getTop().updateStatus(ModuleId::find(GlobalId(2,6)), c2);

  cout << k.getTop() << endl;
  cout << k.getTop().getOrCalculateStatus() << endl << endl;

  // print a node, found on the basis of the id
  cout << k.findSummary(id2) << endl;

  // recreate a moduleid on the basis of a global id
  cout << k.findSummary(ModuleId::find(GlobalId(2,5))) << endl;
  cout << k.findSummary(ModuleId::find(GlobalId(2,6))) << endl;


}
