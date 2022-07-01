/* ------------------------------------------------------------------   */
/*      item            : TrimView.cxx
        made by         : Rene' van Paassen
        date            : 010826
        category        : body file
        description     :
        changes         : 010826 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define TrimView_cxx

#include "TrimView.hxx"
#include "IncoTable.hxx"

DUECA_NS_START

TrimView* TrimView::singleton = NULL;

TrimView::TrimView()
{
  /* Ensure only a single instance active. */
  if (singleton == NULL) {
    singleton = this;
  }
  else if (singleton->isRootClass() && !this->isRootClass()) {
    // replace the root class duecaview with a specialized one
    TrimView* to_delete = singleton;
    singleton = this;
    delete to_delete;
  }
}

TrimView* TrimView::single()
{
  if (!singleton) singleton = new TrimView();
  return singleton;
}


TrimView::~TrimView()
{
  if (this == singleton) singleton = NULL;
}

bool TrimView::isRootClass()
{
  return true;
}

TrimMode TrimView::getMode() const
{
  return NoIncoModes;
}

int TrimView::addEntity(const std::string& s, IncoCalculator *calculator)
{
  return 0;
}

void TrimView::removeEntity(const std::string& s)
{
  //
}

bool TrimView::addVariable(const vector<vstring>& names,
                 int cal, int tvar,
                 const IncoVariableWork& ivar)
{
  return false;
}

void TrimView::refreshView()
{
  //
}

IncoVariableWork& TrimView::getIncoVariable(unsigned int calculator,
                              unsigned int variable)
{
  static IncoVariableWork dummy(IncoVariable("noname", 0.0, 0.0));
  return dummy;
}

void* TrimView::insertEntityNode(const char* name, void* parent,
                                 int dueca_node, TrimLink* obj)
{
  return NULL;
}

DUECA_NS_END
