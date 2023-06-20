/* ------------------------------------------------------------------   */
/*      item            : ActivityLogExtra.cxx
        made by         : Rene' van Paassen
        date            : 1301002
        category        : additional body code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

//#include <dueca/debug.h>

// code originally written for this codegen version
#define __CUSTOM_COMPATLEVEL_110
#define __CUSTOM_COMPATLEVEL_111

// these constructors will not be used
#define __CUSTOM_FULL_CONSTRUCTOR
ActivityLog::ActivityLog(const uint8_t& node_id,
                         const uint8_t& manager_number,
                         const TimeTickType& base_tick,
                         const double& fraction_mult,
                         const ActivityBitPtr& bit_list) :
  node_id(node_id),
  manager_number(manager_number),
  base_tick(base_tick),
  fraction_mult(fraction_mult),
  bit_list(bit_list),
  bit_tail(bit_list),
  no_of_bits(1)
{
  //
}

#define __CUSTOM_COPY_CONSTRUCTOR
ActivityLog::ActivityLog(const ActivityLog& o) :
  node_id(o.node_id),
  manager_number(o.manager_number),
  base_tick(o.base_tick),
  fraction_mult(o.fraction_mult),
  bit_list(NULL),
  bit_tail(NULL),
  no_of_bits(0)
{
  cerr << "You should not clone an ActivityLog!" << endl;

  // copy the first
  if (o.bit_list) {
    bit_tail = bit_list = new ActivityBit(*o.bit_list);
    no_of_bits = 1;
  }
  ActivityBitPtr current = o.bit_list->getNext();
  while (current != NULL) {
    appendActivityBit(new ActivityBit(*current));
    current = current->getNext();
  }
}

#define __CUSTOM_FUNCTION_UNPACKDATA
void ActivityLog::unPackData(::dueca::AmorphReStore& s)
{
  DOBS("unPackData ActivityLog");
  //{ amorphunpackfirst }
  //{ amorphunpacksecond }
  ::dueca::unpackobject(s, this->node_id,
                        dueca::dco_traits<uint8_t>());
  ::dueca::unpackobject(s, this->manager_number,
                        dueca::dco_traits<uint8_t>());
  ::dueca::unpackobject(s, this->base_tick,
                        dueca::dco_traits<TimeTickType>());
  ::dueca::unpackobject(s, this->fraction_mult,
                        dueca::dco_traits<double>());
  ::unPackData(s, no_of_bits);
}


#define __CUSTOM_AMORPHRESTORE_CONSTRUCTOR
ActivityLog::ActivityLog(AmorphReStore& s) :
  bit_list(NULL),
  bit_tail(NULL)
{
  unPackData(s);
  if (!no_of_bits) {
    bit_list = NULL;
    return;
  }
  else {
    bit_tail = bit_list = new ActivityBit(s);
  }

  // reads out the remaining bits. Note --ii, first decrement, then test, so
  // this loop is run (no_of_bits - 1) times
  int listsize = no_of_bits; no_of_bits = 1;
  for (int ii = listsize; --ii; ) {
    appendActivityBit(new ActivityBit(s));
  }
  assert(no_of_bits == listsize);
}

#define __CUSTOM_DESTRUCTOR
ActivityLog::~ActivityLog()
{
  while (bit_list != NULL) {
    ActivityBit* to_delete = bit_list;
    bit_list = bit_list->getNext();
    delete(to_delete);
  }
}

#define __CUSTOM_FUNCTION_PACKDATA
void ActivityLog::packData(AmorphStore& s) const
{
  //cerr << "Packing " << *this << endl;
  ::packData(s, node_id);
  ::packData(s, manager_number);
  ::packData(s, base_tick);
  ::packData(s, fraction_mult);

  // remmember where we packed num bits
  int s_nbits = s.getSize();
  ::packData(s, no_of_bits);

  ActivityBit* current = bit_list; uint16_t nbits = 0;
  int storesize = s.getSize();
  try {
    while (current != NULL) {
      ::packData(s, *current);
      storesize = s.getSize();
      nbits++;
      current = current->getNext();
    }
    assert(nbits == no_of_bits);
  }
  catch (AmorphStoreBoundary &e) {
    // At least one bit needs to be packed, otherwise subsequent use
    // will fail. So
    if (!nbits) throw(e);

    // the store is full. Pity, but we are not jeopardizing DUECA
    // running for the log. Truncate to latest full bit packed, and
    // adjust size.
    /* DUECA UI.

       An activity log has been requested, and in packing the size of
       the buffer to transmit the data has been reached. Rather than
       forcing a re-pack later, the non-packed bits are discarded.
    */
    //W_STS("Truncate pack activity log for manager " << int(manager_number) <<
    //      " from " << no_of_bits << " to " << nbits);
    s.reUse();           // this resets the choked flag.
    s.setSize(s_nbits);
    ::packData(s, nbits);
    s.setSize(storesize);
    // s.setChoked();
    // throw(IncompletePack(GlobalId(0,0), GlobalId(0,0)));
  }
}

#define __CUSTOM_FUNCTION_PRINT
ostream & ActivityLog::print (ostream& s) const
{
  s << "ActivityLog(node_id=" << int(node_id) << ','
    << "manager_number=" << int(manager_number) << ','
    << "no_of_bits=" << no_of_bits << ','
    << "base_tick=" << base_tick << ','
    << "fraction_mult=" << fraction_mult;
  ActivityBit* current = bit_list;
  while (current != NULL) {
    s << endl << "  ,bit=" << *current;
    current = current->getNext();
  }
  s << ")" << endl;

  return s;
}

//#include <dueca/undebug.h>
