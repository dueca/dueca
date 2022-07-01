/* ------------------------------------------------------------------   */
/*      item            : ReflectoryTest.cxx
        made by         : Rene' van Paassen
        date            : 160928
        category        : body file
        description     :
        changes         : 160928 first version
        language        : C++
        copyright       : (c) 16 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define ReflectoryTest_cxx
#include "Reflectory.ixx"
#include "ReflectoryBase.hxx"
#include "ReflectoryView.ixx"
#include "ReflectoryViewBase.hxx"
#include "ReflectoryRemote.hxx"
#include <TimeSpec.hxx>
#include <Object.hxx>

USING_DUECA_NS;

struct cchange
{
  void operator() (ReflectoryData::ItemState istate, unsigned id)
  {
    std::cout << "child change" << istate << ' ' << id << std::endl;
  }
};

struct nchange
{

  void operator() (ReflectoryData::ItemState istate, unsigned id)
  {
    std::cout << "node change" << istate << ' ' << id << std::endl;
  }
};

//template<>
ReflectoryBase<dueca::TimeTickType>::child_change ccx = cchange();
//template<>
ReflectoryBase<dueca::TimeTickType>::child_change ncx = nchange();

struct mydetect
{
  bool operator () (const ReflectoryView<dueca::Object,TimeTickType>& o)
  {
    cout << "mydetect" << endl;
    return true;
  }
};


int main()
{
  ReflectoryBase<dueca::TimeTickType>::ref_pointer root0
    (new Reflectory<dueca::Object,dueca::TimeTickType>);
  ReflectoryBase<dueca::TimeTickType>::ref_pointer root1
    (new ReflectoryRemote<dueca::TimeTickType>);

  Reflectory<dueca::Object,dueca::TimeTickType>::ref_pointer nnode
    (new Reflectory<dueca::Object,dueca::TimeTickType>(root0, "nodes", ccx, ncx));
  Reflectory<dueca::Object,dueca::TimeTickType>::ref_pointer net
    (new Reflectory<dueca::Object,unsigned>(root0, "net", ccx, ncx));
  Reflectory<dueca::Object,dueca::TimeTickType>::ref_pointer node0
    (new Reflectory<dueca::Object,TimeTickType>(root0, "nodes/0", ccx, ncx));
  node0->newData(new dueca::Object(), 1U);

  // create a reflectoryview
  ReflectoryView<dueca::Object,dueca::TimeTickType>::data_change dchange = mydetect();
  ReflectoryView<dueca::Object,dueca::TimeTickType> view0(root0, "nodes/0", dchange);
  // read its data
  cout << view0.data(1U) << endl;

  // read off root0's changes



  //  ReflectoryBase::root().addChild(nnode);
}
