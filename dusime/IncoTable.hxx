/* ------------------------------------------------------------------   */
/*      item            : IncoTable.hh
        made by         : Rene' van Paassen
        date            : 000223
        category        : header file
        description     :
        changes         : 000223 first version
        language        : C++
        copyright       : (c) 2016 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
        documentation   : DUECA_API
*/

#ifndef IncoTable_hh
#define IncoTable_hh

#ifdef IncoTable_cc
#endif

#include <dueca_ns.h>
#include <IncoNotice.hxx>
#include <GenericVarIO.hxx>
#include <IncoVariable.hxx>
#include <dassert.h>

DUECA_NS_START;

/** This struct is helpful in constructing a table of pointers to
    IncoVariable objects, combined with pointers to GenericVarIO
    objects. Using this table, the SimulationModule base class of some
    SimulationModule derived class can send trim data to the trim
    engine or obtain control/constraint data from this engine and
    insert it into the proper variables. Close off such a table with a
    {NULL, NULL} entry. */
struct IncoTable
{
  /// Pointer to the IncoVariable object
  IncoVariable* incovar;

  /// Pointer to the GenericVarIO object which will be used to
  /// probe/insert data
  GenericVarIO* probe;
};

/** Create the contents from a parameter table */
template<typename O>
unsigned fill_inco_notice(IncoNotice& notice, const IncoTable* table, O* obj)
{
  unsigned idx = 0; double value;
  while (table->incovar) {
    if (table->incovar->queryInsertForThisMode(notice.mode)) {
      table->probe->peek(obj, value);
      notice.ivlist.emplace_back(idx, value);
    }
    idx++; table++;
  }
  return idx;
}

template<typename O>
void process_inco_notice(IncoNotice& notice, const IncoTable* table, O* obj)
{
#ifdef ASSERT_ACTIVE
  unsigned tsize = 0; const IncoTable *t2 = table;
  while (t2->incovar) { tsize++; t2++; }
#endif
  for (const auto &ivpair: notice.ivlist) {
#ifdef ASSERT_ACTIVE
    assert(ivpair.index < tsize);
#endif
    table[ivpair.index].probe->poke(obj, ivpair.value);
  }
}

DUECA_NS_END;

#endif
