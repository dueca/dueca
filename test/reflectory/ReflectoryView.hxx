/* ------------------------------------------------------------------   */
/*      item            : ReflectoryView.hxx
        made by         : Rene van Paassen
        date            : 160928
        category        : header file
        description     :
        changes         : 160928 first version
        language        : C++
        copyright       : (c) 16 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef ReflectoryView_hxx
#define ReflectoryView_hxx

#include "ReflectoryViewBase.hxx"

DUECA_NS_START;


/** Creates an observation of a reflectory entry.

    Access the data for reading. A client is responsible for keeping a supplied
    callback function alive for the lifetime of this observation. */
template <class DATA, typename TICK>
class ReflectoryView: public ReflectoryViewBase<TICK>
{
public:
  typedef TICK ticktype;
  typedef DATA datatype;
  typedef typename boost::intrusive_ptr<ReflectoryView<DATA,TICK> > ref_pointer;
  typedef ReflectoryView<DATA,TICK>* pointer;
  typedef ReflectoryViewBase<TICK> parenttype;

public:
  /** Callback function type,

      All changes in data or state are communicated through this
      callback object */
  typedef typename boost::function<bool(const ReflectoryView<DATA,TICK>&)>  data_change;

private:
  /** Callback function object. */
  data_change    cb_change;

public:

  /** Constructor for a ReflectoryView

      This accesses the data at the location indicated by the path
      @param root       Root object for the requested reflectory
      @param path       Location of the data, e.g. /nodes
      @param cb_change  Callback function object
  */
  ReflectoryView(typename ReflectoryBase<TICK>::ref_pointer root,
                 const std::string& path,
                 data_change& cb_change = data_change());

  /** Destructor */
  ~ReflectoryView();

  /** Access to the data representation. */
  inline const DATA& data(TICK tick=0) const
  { return *reinterpret_cast<const DATA*>(this->backend->data(tick)); }

protected:
  /** Notify new data, if applicable */
  virtual void notify(const TICK& tick)
  {
    // notified of change at tick
  }
};

DUECA_NS_END;

#endif
