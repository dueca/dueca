// -*-c++-*-
/* ------------------------------------------------------------------   */
/*      item            : ReflectoryBase.cxx
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

#ifndef ReflectoryBase_ixx
#define ReflectoryBase_ixx
#include "ReflectoryBase.hxx"

DUECA_NS_START;

template<typename TICK>
ReflectoryBase<TICK>::ReflectoryBase() :
  state(Active),
  selfid(0),
  name(""),
  path("/"),
  parent(NULL)
{
  cerr << "Creating root reflectory " << reinterpret_cast<void*>(this) << endl;
  // only root node
}

template <typename TICK>
ReflectoryBase<TICK>::
ReflectoryBase(const std::string& path) :
  state(Created),
  path(path)
{
  cerr << "Creating reflectory " << path << " "
       << reinterpret_cast<void*>(this) << endl;
}

template <typename TICK>
void ReflectoryBase<TICK>::registerWithParent(ref_pointer root)
{
  size_t idx = path.rfind("/");

  // check for name correct
  if (!path.size() || idx == 0 ||
      (idx != string::npos && path.size() - idx <= 1)) {
    throw(reflectory_incorrectname());
  }

  if (idx == string::npos) {

    // register with the root
    selfid = root->addChild
      (boost::intrusive_ptr<ReflectoryBase>(this));

    // set my name
    name = path;
  }
  else {
    // find my parent
    parent = (*root)[path.substr(0, idx)];
    if (parent) {
      selfid = parent->addChild(boost::intrusive_ptr<ReflectoryBase<TICK> >(this));
    }
    else {
      throw(reflectory_notfound());
    }

    // set my name
    name = path.substr(idx+1);
  }
}

template <typename TICK>
boost::intrusive_ptr<const ReflectoryBase<TICK> >
ReflectoryBase<TICK>::operator [] (const std::string& path) const
{
  size_t idx = path.find("/");
  if (idx == string::npos) {
    for (typename childvec_type::iterator ii = slots.begin(); ii != slots.end(); ii++) {
      if ((*ii)->getName() == path) return *ii;
    }
  }
  else {
    for (typename childvec_type::iterator ii = slots.begin(); ii != slots.end(); ii++) {
      if ((*ii)->getName() == path.substr(0, idx))
        return (*ii)->operator[](path.substr(idx+1));
    }
  }
  throw(reflectory_notfound());
}

template <typename TICK>
void ReflectoryBase<TICK>::
defaultResponseChild(const ReflectoryData::ItemState& istate,
                     const std::string& childname,
                     unsigned id,
                     boost::intrusive_ptr<const ReflectoryBase> child)
{
  //
}

template<typename TICK>
ReflectoryBase<TICK>::~ReflectoryBase()
{
  //assert(slots.size() == 0);
  cerr << "Deleting reflectory " << name << " "
       << reinterpret_cast<void*>(this) << endl;
}

template<typename TICK>
void ReflectoryBase<TICK>::addView(
     const ReflectoryViewBase<TICK>* client) const
{
  this->clients.push_back(client_type(client));
}

template<typename TICK>
void ReflectoryBase<TICK>::passConfigChange(ReflectoryData& d) const
{
  if (parent) {
    // Am not root; add my own ID and pass to parent to complete
    d.issued_path.push_front(selfid);
    parent->passConfigChange(d);
  }
  else {
    // replace by storage
    std::cout << "to config list " << d << std::endl;
    //planned_config.push_back(d);
  }
}


DUECA_NS_END;

#endif
