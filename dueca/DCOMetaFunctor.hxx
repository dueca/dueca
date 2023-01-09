/* ------------------------------------------------------------------   */
/*      item            : DCOFunctorFactory.hxx
        made by         : Rene van Paassen
        date            : 170326
        category        : header file
        description     : Base class for a factory table.
        changes         : 170326 first version
        api             : DUECA_API
        language        : C++
        copyright       : (c) 2017 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#ifndef DCOMetaFunctor_hxx
#define DCOMetaFunctor_hxx

#include <string>
#include <map>
#include <iostream>
#include <memory>
#include <dueca_ns.h>

DUECA_NS_START;

/**
    Base class for key-based access to helper classes for DCO
    objects. These classes can be accessed through the
    DataClassRegistry. For typical use, decide on the following:

    * Create a class that derives from this DCOMetaFunctor. Give that
      class a set of virtual methods, or typically only define the
      operator(), that are const (i.e. the class may never be
      changed).

    * In the DCO opjects that you want to extend, add extra code (with
      the IncludeFile option). Another possibility is extending the
      code generator with a plugin to add this code. The extra code
      defines a class derived from the class defined in the previous
      step. This class can then access a specific dco object's meta
      information (since you don't have an actual object to work
      with).

    * If you want to work with data in a channel, and have access (or
      want to have, since you are making a generic module) to only the
      data class name, you can add a functor derived (directly or
      indirectly) from DCOFunctor. This functor can then be created
      through your DCOMetaFunctor, and once you have it, you can use
      it with a channel read token, or a channel write token.

    * Note that you can hand-write a metafunctor for a specific
      datatype, but that the most efficient (if you need this a lot),
      is extending the code generator with a plugin. This has been
      done for HDF5 functors.

    Since this is all very theoretical, let's assume we want to write
    DCO objects to some SQL-type database. The derived metafunctor
    gets a call with a pointer to the database and the name for the
    table. It creates a derivative of DCOFunctor that can write its
    data in the table.

    @code
    // the functor that writes sql, when given a pointer to the data
    class DCOSQLFunctor_MyData: public DCOFunctor
    {
      // constructor
      DCOSQLFunctor_MyData(sqlptrtype database, std::string tablename)
      {
        // create a table in the database, and table entries for all
        // my members!
      }
      // destructor
      ~DCOSQLFunctor_MyData()
      {
        // properly close off the table
      }
      // override the operator definition from DCOFunctor, will be used by
      // the channel code
      bool operator() (const void* dpointer, const DataTimeSpec& ts)
      {
        // dpointer contains the data; convert and write another row
        // to the table
        return true;
      }
    };

    // the generic MetaFunctor class for sql capable DCO objects,
    // returns an sql-writing functor (defined elsewhere, for all DCO objects)
    class DCOSQLMetaFunctor: public DCOMetaFunctor
    {
      // note that the arguments are basically those needed for the
      // constructor of the DCOSQLFunctor
      virtual DCOSQLFunctor* operator()
        (sqlptrtype database, const std::string& tablename) const = 0;
    };

    // derived MetaFunctor class for this sql-capable DCO object
    // typically in the extra (IncludeFile) code for the
    // data type (possibly you can also template stuff??)
    class DCOSQLMetaFunctor_MyData: public DCOSQLMetaFunctor
    {
      DCOSQLFunctor* operator()
        (sqlptrtype database, std::string tablename) const
      {
        return new DCOSQLFunctor_MyData(database, tablename);
      }
    };

    // we need to add one MetaFunctor to the DCO functortable
    // this trick does it at loading time
    static dueca::LoadMetaFunctor<DCOSQLMetaFunctor_MyData>
      load_sql_functor(functortable, "sql");
    @endcode
*/
class DCOMetaFunctor
{
public:
  /** Constructor */
  DCOMetaFunctor();

  /** Destructor */
  virtual ~DCOMetaFunctor();
};

/** type definition for metafunctor tables */
typedef std::map<std::string,std::shared_ptr<dueca::DCOMetaFunctor> >
functortable_type;

/** Struct to declare a MetaFunctor to the DCO type system
    @tparam F    Metafunctor */
template<typename F>
struct LoadMetaFunctor {
  /** Constructor adds an entry to the metafunctor table.
      @param functortable map with metafunctors
      @param key          Key for the new entry */
  LoadMetaFunctor(functortable_type& functortable, const char* key) {
    functortable[key] = std::shared_ptr<DCOMetaFunctor>(new F());
  }
};


DUECA_NS_END
#endif
