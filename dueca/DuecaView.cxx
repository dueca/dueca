/* ------------------------------------------------------------------   */
/*      item            : DuecaView.cxx
        made by         : Rene' van Paassen
        date            : 000721
        category        : body file
        description     :
        changes         : 000721 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/


#define DuecaView_cc
#include <dueca-conf.h>
#include "DuecaView.hxx"
#define E_CNF
#include "debug.h"
#include <dassert.h>

DUECA_NS_START

DuecaView* DuecaView::singleton = NULL;
bool DuecaView::is_base = false;

DuecaView::DuecaView(bool derived)
{
  if (derived && is_base) {
    // replace the root class duecaview with a specialized one
    DuecaView* to_delete = singleton;
    singleton = this;
    delete to_delete;
    is_base = false;
  }
  else if (!derived && singleton == NULL) {
    // automatic base creation
    singleton = this;
    is_base = true;
  }
  else if (derived) {
    /* DUECA UI.

       Attempt to create a second dueca view. */
    E_CNF("Attempt to create a second dueca view");
  }
  else {
    assert(0);
  }
}

DuecaView::~DuecaView()
{
  if (this == singleton) singleton = NULL;
}

DuecaView* DuecaView::single()
{
  if (!singleton) singleton = new DuecaView();
  return singleton;
}

void DuecaView::updateEntityButtons(const ModuleState& confirmed_state,
                                    const ModuleState& command_state,
                                    bool emergency_flag)
{
  // only action in derived class
}

void DuecaView::refreshNodesView()
{
  // only action in derived class
}


void DuecaView::refreshEntitiesView()
{
  // only action in derived class
}

void* DuecaView::insertEntityNode(const char* name, void* parent,
                                  int dueca_node, StatusT1* obj)
{
  return NULL;
}

DUECA_NS_END;


