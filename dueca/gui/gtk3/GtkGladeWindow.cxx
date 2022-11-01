/* ------------------------------------------------------------------   */
/*      item            : GtkGladeWindow.cxx
        made by         : Rene' van Paassen
        date            : 051017
        category        : body file
        description     :
        changes         : 051017 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#define GtkGladeWindow_cxx

#include <list>

#include <dueca-conf.h>
#include "GtkGladeWindow.hxx"
#include "GtkDuecaButtons.hxx"
#include "DuecaPath.hxx"
#include <dueca/CommObjectReader.hxx>
#include <dueca/CommObjectWriter.hxx>
#include <dueca/CommObjectElementReader.hxx>
#include <dueca/CommObjectElementWriter.hxx>
#include <dueca/DataClassRegistry.hxx>
#include <dueca/CommObjectMemberAccess.hxx>
#define E_CNF
#define W_CNF
#include "debug.h"
#include <boost/lexical_cast.hpp>

DUECA_NS_START

bool GtkGladeWindow::initialised_glade = false;
bool GtkGladeWindow::initialised_gtkmm = false;

GtkGladeWindow::GtkGladeWindow() :
  window(NULL),
  builder(NULL),
  offset_x(-1),
  offset_y(-1),
  size_x(0),
  size_y(0)
{
  //
}

GtkGladeWindow::~GtkGladeWindow()
{
  hide();
  g_object_unref(builder);
}

bool GtkGladeWindow::readGladeFile(const char* file,
                                   const char* mainwidget,
                                   gpointer client,
                                   const GladeCallbackTable *table,
                                   bool connect_signals)
{
  if (builder == NULL) builder = gtk_builder_new();

  gtk_builder_add_from_file(builder, file, NULL);

  // get the main widget
  window = GTK_WIDGET(gtk_builder_get_object(builder, mainwidget));
  if (!window) {
    /* DUECA graphics.

       Cannot find the main widget in the glade builder windowing
       file. Please check the file and main widget name.
    */
    E_CNF("Cannot find main widget " << mainwidget << " in " << file);
    return false;
  }

  // position requested?
  if (offset_x >= 0 && offset_y >= 0) {
    gtk_window_move(GTK_WINDOW(window), offset_x, offset_y);
  }

  // size requested?
  if (size_x > 0 && size_y > 0) {
    gtk_widget_set_size_request(window, size_x, size_y);
  }

  // connect all required buttons.
  if (client && table) {
    connectCallbacks(client, table);
  }

  // and connect signals gobject etc.
  if (connect_signals) {
    this->connectCallbackSymbols();
  }

  // done
  return true;
}

void GtkGladeWindow::connectCallbacks(gpointer client,
                                      const GladeCallbackTable *table)
{
  const GladeCallbackTable *cbl = table;
  while (cbl->widget) {

    // lookup the widget and link the function
    GObject* b = gtk_builder_get_object(builder, cbl->widget);
    if (b) {
      GtkCaller* caller = cbl->func->clone(client);
      caller->setGPointer(cbl->user_data);
      // link the callback function
      g_signal_connect(b, cbl->signal, caller->callback(),
                       caller);
    }

    else {
      // failure to find this widget
      /* DUECA graphics.

         Cannot find a widget in the gtk-builder model, when trying
         to connect the callbacks.
      */
      W_CNF("Cannot find widget " << cbl->widget);
    }

    // to next cbl.
    cbl++;
  }
}

void GtkGladeWindow::connectCallbacksAfter(gpointer client,
                                           const GladeCallbackTable *table)
{
  const GladeCallbackTable *cbl = table;
  while (cbl->widget) {

    // lookup the widget and link the function
    GObject* b = gtk_builder_get_object(builder, cbl->widget);
    if (b) {
      GtkCaller* caller = cbl->func->clone(client);
      caller->setGPointer(cbl->user_data);
      // link the callback function
      g_signal_connect_after(b, cbl->signal, caller->callback(),
                               caller);
    }

    else {
      // failure to find this widget
      /* DUECA graphics.

         Cannot find a widget in the gtk-builder model, when trying
         to connect the callbacks.
      */
      W_CNF("Cannot find widget " << cbl->widget);
      }

    // to next cbl.
    cbl++;
  }
}

void GtkGladeWindow::connectCallbackSymbols(gpointer user_data)
{
  gtk_builder_connect_signals(builder, user_data);
}

GtkWidget* GtkGladeWindow::operator [] (const char* wname)
{
  if (builder) return GTK_WIDGET(gtk_builder_get_object(builder, wname));
  return NULL;
}

GObject* GtkGladeWindow::getObject(const char* wname)
{
  if (builder) return gtk_builder_get_object(builder, wname);
  return NULL;
}

void GtkGladeWindow::show(const char* widget)
{
  if (widget) {
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(builder, widget)));
  }
  else {
    gtk_widget_show_all(window);
  }
}

void GtkGladeWindow::hide(const char* widget)
{
  if (widget) {
    gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, widget)));
  }
  else {
    gtk_widget_hide(window);
  }
}

bool GtkGladeWindow::_setValue(const char* wname, double value, bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      W_MOD("Could not find gtk object with id \"" << wname << "\"");
    }
    return false;
  }

  // try 1, adjustment
  if (GTK_IS_ADJUSTMENT(o)) {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(o), value);
    return true;
  }

  // try 2, range
  if (GTK_IS_RANGE(o)) {
    gtk_range_set_value(GTK_RANGE(o), value);
    return true;
  }

  // try 3, spinbutton
  if (GTK_IS_SPIN_BUTTON(o)) {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(o), value);
    return true;
  }

  // try 3, entry, after spinbutton, bc that inherits from entry
  if (GTK_IS_ENTRY(o)) {
    gtk_entry_set_text(GTK_ENTRY(o),
                       boost::lexical_cast<std::string>(value).c_str());
    return true;
  }
  if (warn) {
    W_MOD("Setting double/float for gtk object \"" << wname <<
          "\" not implemented");
  }
  return false;
}

bool GtkGladeWindow::_setValue(const char* wname, const char* value, bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      W_MOD("Could not find gtk object with id \"" << wname << "\"");
    }
    return false;
  }

  if (GTK_IS_COMBO_BOX(o)) {

    // find the model, and determine where the value is (should be there!)
    GtkTreeModel *mdl = gtk_combo_box_get_model(GTK_COMBO_BOX(o));
    GtkTreeIter it;
    gboolean itvalid = gtk_tree_model_get_iter_first(mdl, &it);
    gchararray val = NULL; gtk_tree_model_get(mdl, &it, 0, &val, -1);

    while (itvalid && !strcmp(val, value)) {
      itvalid = gtk_tree_model_iter_next(mdl, &it);
      if (itvalid) gtk_tree_model_get(mdl, &it, 0, &val, -1);
    }
    if (itvalid) {
      gtk_combo_box_set_active_iter(GTK_COMBO_BOX(o), &it);
      return true;
    }

    // no valid match found
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(o), NULL);
    if (warn) {
      W_MOD("No matching item for gtk combo \"" << wname <<
	    "\", missing \"" << value << '"');
    }
    return false;
  }

  if (GTK_IS_ENTRY(o)) {
    GtkEntry *e = GTK_ENTRY(o);
    if (e != NULL) {
      gtk_entry_set_text(e, value);
      return true;
    }
  }

  if (warn) {
    W_MOD("Setting text for gtk object \"" << wname << "\" not implemented");
  }
  return false;
}

bool GtkGladeWindow::_setValue(const char* wname, bool value, bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      W_MOD("Could not find gtk object with id \"" << wname << "\"");
    }
    return false;
  }

  {
    GtkToggleButton *t = GTK_TOGGLE_BUTTON(o);
    gtk_toggle_button_set_active(t, value ? TRUE : FALSE);
    return true;
  }

  if (warn) {
    W_MOD("Setting state for gtk object \"" << wname << "\" not implemented");
  }
  return false;
}

bool GtkGladeWindow::_setValue(const char* wname, const char* mname,
                              boost::any& b, bool warn)
{
  if (b.type() == typeid(double)) {
    return _setValue(wname, boost::any_cast<double>(b), warn);
  }
  else if (b.type() == typeid(float)) {
    return _setValue(wname, boost::any_cast<float>(b), warn);
  }
  else if (b.type() == typeid(int32_t)) {
    return _setValue(wname, double(boost::any_cast<int32_t>(b)), warn);
  }
  else if (b.type() == typeid(int64_t)) {
    return _setValue(wname, double(boost::any_cast<int32_t>(b)), warn);
  }
  else if (b.type() == typeid(int64_t)) {
    return _setValue(wname, double(boost::any_cast<int64_t>(b)), warn);
  }
  else if (b.type() == typeid(uint64_t)) {
    return _setValue(wname, double(boost::any_cast<int32_t>(b)), warn);
  }
  else if (b.type() == typeid(uint64_t)) {
    return _setValue(wname, double(boost::any_cast<int64_t>(b)), warn);
  }
  else if (b.type() == typeid(bool)) {
    return _setValue(wname, boost::any_cast<bool>(b), warn);
  }
  else if (b.type() == typeid(std::string)) {
    return _setValue(wname, boost::any_cast<std::string>(b).c_str(), warn);
  }
  if (warn) {
    W_MOD("Could not interpret type of member " << mname);
  }
  return false;
}

template<class T>
bool GtkGladeWindow::__getValue(const char* wname, boost::any& b, bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      W_MOD("Could not find gtk object with id \"" << wname << "\"");
    }
    return false;
  }

  // try 1, adjustment
  if (GTK_IS_ADJUSTMENT(o)) {
    b = T(gtk_adjustment_get_value(GTK_ADJUSTMENT(o)));
    return true;
  }

  // try 2, range
  if (GTK_IS_RANGE(o)) {
    b = T(gtk_range_get_value(GTK_RANGE(o)));
    return true;
  }

  // try 3, spinbutton
  if (GTK_IS_SPIN_BUTTON(o)) {
    b = T(gtk_spin_button_get_value(GTK_SPIN_BUTTON(o)));
    return true;
  }

  // try 3, entry, after spinbutton, bc that inherits from entry
  if (GTK_IS_ENTRY(o)) {
    b = boost::lexical_cast<T>(gtk_entry_get_text(GTK_ENTRY(o)));
    return true;
  }

  if (warn) {
    W_MOD("Setting double/float for gtk object \"" << wname <<
          "\" not implemented");
  }
  return false;
}

template<>
bool GtkGladeWindow::__getValue<bool>(const char* wname,
                                      boost::any& b, bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      W_MOD("Could not find gtk object with id \"" << wname << "\"");
    }
    return false;
  }

  if (GTK_IS_TOGGLE_BUTTON(o)) {
    b = bool(gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(o)));
    return true;
  }

  if (warn) {
    W_MOD("Getting state for gtk object \"" << wname << "\" not implemented");
  }
  return false;
}

template<>
bool GtkGladeWindow::__getValue<std::string>(const char* wname,
                                             boost::any& b, bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      W_MOD("Could not find gtk object with id \"" << wname << "\"");
    }
    return false;
  }

  {
    GtkEntry *e = GTK_ENTRY(o);
    if (e != NULL) {
      b = std::string(gtk_entry_get_text(e));
      return true;
    }
  }
  if (warn) {
    W_MOD("Getting text for gtk object \"" << wname << "\" not implemented");
  }
  return false;

}

bool GtkGladeWindow::_getValue(const char* wname, const char* mname,
                               const char* klass, boost::any& value, bool warn)
{
  if (!strcmp(klass, "double")) {
    return __getValue<double>(wname, value, warn);
  }
  if (!strcmp(klass, "float")) {
    return __getValue<float>(wname, value, warn);
  }
  if (!strcmp(klass, "int32_t")) {
    return __getValue<int32_t>(wname, value, warn);
  }
  if (!strcmp(klass, "uint32_t")) {
    return __getValue<uint32_t>(wname, value, warn);
  }
  if (!strcmp(klass, "int64_t")) {
    return __getValue<int64_t>(wname, value, warn);
  }
  if (!strcmp(klass, "uint64_t")) {
    return __getValue<uint64_t>(wname, value, warn);
  }
  if (!strcmp(klass, "bool")) {
    return __getValue<bool>(wname, value, warn);
  }
  if (!strcmp(klass, "std::string")) {
    return __getValue<std::string>(wname, value, warn);
  }
  if (warn) {
    W_MOD("Could not interpret type of member \"" << mname
          << "\" with class \"" << klass << '"');
  }
  return false;
}

unsigned GtkGladeWindow::setValues(CommObjectReader& dco,
                                   const char* format,
                                   const char* arrformat,
                                   bool warn)
{
  unsigned nset = 0;
  char gtkid[128];
  for (size_t ii = dco.getNumMembers(); ii--; ) {
    if (dco.getMemberArity(ii) == Single) {
      snprintf(gtkid, sizeof(gtkid), format, dco.getMemberName(ii));
      boost::any b; dco[ii].read(b);
      if (_setValue(gtkid, dco.getMemberName(ii), b, warn)) { nset++; }
    }
    else if (dco.getMemberArity(ii) == Iterable ||
             dco.getMemberArity(ii) == FixedIterable) {
      if (arrformat != NULL) {
        auto ereader = dco[ii]; unsigned idx = 0;
        while (!ereader.isEnd()) {
          snprintf(gtkid, sizeof(gtkid), arrformat, dco.getMemberName(ii), idx);
          boost::any b; ereader.read(b);
          if (_setValue(gtkid, dco.getMemberName(ii), b, warn)) { nset++; }
        }
      }
      else {
        W_MOD("No format specified for array member "
              << dco.getMemberName(ii));
      }
    }
    else {
      W_MOD("Could not interpret organisation of member "
            << dco.getMemberName(ii));
    }
  }
  return nset;
}

unsigned GtkGladeWindow::getValues(CommObjectWriter& dco,
                                   const char* format,
                                   const char* arrformat,
                                   bool warn)
{
  unsigned nset = 0;
  char gtkid[128];
  for (size_t ii = dco.getNumMembers(); ii--; ) {
    if (dco.getMemberArity(ii) == Single) {
      snprintf(gtkid, sizeof(gtkid), format, dco.getMemberName(ii));
      boost::any b;
      if (_getValue(gtkid, dco.getMemberName(ii), dco.getMemberClass(ii),
                    b, warn)) {
        nset++;
        dco[ii].write(b);
      }
    }
    else if (dco.getMemberArity(ii) == FixedIterable) {
      if (arrformat != NULL) {
        auto ewriter = dco[ii]; unsigned idx = 0;
        while (!ewriter.isEnd()) {
          snprintf(gtkid, sizeof(gtkid), format, dco.getMemberName(ii), idx++);
          boost::any b;
          if (_getValue(gtkid, dco.getMemberName(ii), dco.getMemberClass(ii),
                        b, warn)) {
            nset++;
            ewriter.write(b);
          }
          else {
            ewriter.skip();
          }
        }
      }
      else {
        W_MOD("No format specified for array member "
              << dco.getMemberName(ii));
      }
    }
    else if (dco.getMemberArity(ii) == Iterable) {
      if (arrformat != NULL) {
        auto ewriter = dco[ii]; unsigned idx = 0;
        while (true) {
          snprintf(gtkid, sizeof(gtkid), format, dco.getMemberName(ii), idx++);
          boost::any b;
          if (_getValue(gtkid, dco.getMemberName(ii), dco.getMemberClass(ii),
                        b, warn)) {
             ewriter.write(b);
          }
          else {
            break;
          }
        }
      }
      else {
        W_MOD("No format specified for array member "
              << dco.getMemberName(ii));
      }
    }
    else {
      W_MOD("Could not interpret organisation of member "
            << dco.getMemberName(ii));
    }
  }
  return nset;
}

static const char* _searchInMap(GtkGladeWindow::OptionMapping* mapping,
				const char* key, bool warn)
{
  for (auto m = mapping; m->ename != NULL; m++) {
    if (!strcmp(m->ename, key)) {
      if (m->representation != NULL) {
	return m->representation;
      }
      else {
	return key;
      }
    }
  }
  if (warn) {
    /* DUECA graphics.

       In the given key is missing from the option string mapping for
       selecting an Enum with a ComboBox. Check the mapping against
       the DCO definition for the enum.
    */
    W_XTR("Key \"" << key << "\" not given in options mapping");
  }
  return key;
}


bool GtkGladeWindow::_fillOptions(const char* wname,
				  ElementWriter& writer,
				  ElementReader& reader,
				  OptionMapping* mapping,
				  bool warn)
{
  GObject *o = getObject(wname);
  if (o == NULL) {
    if (warn) {
      W_MOD("Could not find gtk object with id \"" << wname << "\"");
    }
    return false;
  }

  if (!GTK_IS_COMBO_BOX(o)) {
    if (warn) {
      W_XTR("Cannot fill options, object not a ComboBox \"" << wname << '"');
    }
    return false;
  }

  GtkTreeModel* treemodel = gtk_combo_box_get_model(GTK_COMBO_BOX(o));
  if (treemodel == NULL) {
    treemodel = GTK_TREE_MODEL
      (mapping != NULL ?
       gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING) :
       gtk_list_store_new(1, G_TYPE_STRING));
    gtk_combo_box_set_model(GTK_COMBO_BOX(o), treemodel);
  }

  GtkListStore* store = GTK_LIST_STORE(treemodel);
  if (store == NULL ) {
    if (warn) {
      W_XTR("ComboBox object \"" << wname << "\", store is not compatible");
    }
    return false;
  }
  gtk_list_store_clear(store);

  // iterate through the values
  writer.setFirstValue(); GtkTreeIter it;
  do {
    std::string value;
    reader.read(value);
    gtk_list_store_insert(store, &it, -1);
    if (mapping) {
      gtk_list_store_set(store, &it, 0, value.c_str(), 1,
			 _searchInMap(mapping, value.c_str(), warn), -1);
    }
    else {
      gtk_list_store_set(store, &it, 0, value.c_str(), -1);
    }
  } while (writer.setNextValue());
  return true;
}

bool GtkGladeWindow::fillOptions(const char* dcoclass,
				 const char* format, const char* arrformat,
				 const OptionMappings* mappings, bool warn)
{
  auto eclass = DataClassRegistry::single().getEntryShared(dcoclass);
  if (!eclass.get()) {
    /** DUECA Graphics.

	When trying to fill selections for combobox entries in a GUI,
	(GtkGladeWindow::fillOptions), the specified dco data class is
	not available. Check spelling, or add the class to the
	executable.
    */
    E_XTR("GtkGladeWindow cannot access data class " << dcoclass);
    return false;
  }

  // work variable
  char gtkid[128];

  /** Run through all members. */
  for (size_t im = 0; im < DataClassRegistry::single().
	 getNumMembers(eclass.get()); im++) {
    auto access = DataClassRegistry::single().
      getMemberAccessor(eclass.get(), im);

    // only the enums
    if (access->isEnum()) {
      if (access->getArity() == FixedIterable) {
	if (arrformat != NULL) {
	  for (unsigned idx = access->getSize(); idx--; ) {
	    snprintf(gtkid, sizeof(gtkid), arrformat, access->getName(), idx);
	    // now need to get the enum values?

	  }
	}
	else {
	  /** DUECA Graphics.

	      There is an enum array specified, but no array format
	      available for finding it in the interface.
	  */
	  W_XTR("GtkGladeWindow::fillOptions missing array format");
	}
      }
      else if (access->getArity() == Single) {

	snprintf(gtkid, sizeof(gtkid), format, access->getName());
	// again, enum values

      }
    }
  }
  return true;
}

DUECA_NS_END
