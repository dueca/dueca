/* ------------------------------------------------------------------   */
/*      item            : msgpack.cxx
        made by         : Rene van Paassen
        date            : 181027
        category        : body implementation
        description     :
        changes         : 181027 first version
        language        : C++
        copyright       : (c) 2021 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

#include "msgpack.hxx"
#include <dueca_ns.h>
#include <debug.h>
#define DEBPRINTLEVEL 2
#include <debprint.h>

PRINT_NS_START;

ostream& operator<<(ostream&os, const dueca::messagepack::VVMode mode)
{
  static const char* names[] = {
    "Init",
    "Key",
    "Value",
    "Next",
    "Exit"
  };
  return (os << names[unsigned(mode)]);
}

ostream& operator<<(ostream&os, const dueca::messagepack::MMode mode)
{
  static const char* names[] = {
    "Init",
    "Key",
    "Value",
    "Exit"
  };
  return (os << names[unsigned(mode)]);
}
PRINT_NS_END;

DUECA_NS_START;
namespace messagepack {

msgpack_obj_mode_mismatch::msgpack_obj_mode_mismatch
(const char* operation, VVMode mode)
{
  std::stringstream mxs;
  mxs << "MSGPack mode error, in mode " << mode
      << " no operation " << operation;
  msg = mxs.str();
}

const char* msgpack_obj_mode_mismatch::what() const noexcept
{ return msg.c_str(); }
const char* msgpack_dco_key_too_long::what() const noexcept
{ return "DCO member name too long"; }

DCOVirtualVisitor::DCOVirtualVisitor() :
  sel(-1), depth(-1), nest(NULL), mode(VVMode::Init) { DEB3("DCOVirtualVisitor constructor");}

bool DCOVirtualVisitor::visit_nil()
{ DEB1("DCO visit_nil nest=" << bool(nest));
  if (nest) return nest->visit_nil();
  return false; }

bool DCOVirtualVisitor::visit_boolean(bool v)
{ DEB1("DCO visit_boolean nest=" << bool(nest) << " v=" << v);
 if (nest) return nest->visit_boolean(v);
 return false; }

bool DCOVirtualVisitor::visit_positive_integer(uint64_t v)
{ DEB1("DCO visit_positive_integer nest=" << bool(nest) << " v=" << v);
 if (nest) return nest->visit_positive_integer(v);
 return false; }

bool DCOVirtualVisitor::visit_negative_integer(int64_t v)
{ DEB1("DCO visit_negative_integer nest=" << bool(nest) << " v=" << v);
 if (nest) return nest->visit_negative_integer(v);
 return false; }

bool DCOVirtualVisitor::visit_float32(float v)
{ DEB1("DCO visit_float32 nest=" << bool(nest) << " v=" << v);
 if (nest) return nest->visit_float32(v);
 return false; }

bool DCOVirtualVisitor::visit_float64(double v)
{ DEB1("DCO visit_float64 nest=" << bool(nest) << " v=" << v);
 if (nest) return nest->visit_float64(v);
 return false; }

  bool GobbleVisitor::visit_nil()
  { DEB1("Gobble visit_nil"); return true; }
  
  bool GobbleVisitor::visit_boolean(bool v)
  { DEB1("Gobble visit_boolean v=" << v); return true; }

  bool GobbleVisitor::visit_positive_integer(uint64_t v)
  { DEB1("Gobble visit_positive_integer v=" << v); return true; }

  bool GobbleVisitor::visit_negative_integer(int64_t v)
  { DEB1("Gobble visit_negative_integer v=" << v); return true; }

  bool GobbleVisitor::visit_float32(float v)
  { DEB1("Gobble visit_float32 v=" << v); return true; }

  bool GobbleVisitor::visit_float64(double v)
  { DEB1("Gobble visit_float64 v=" << v); return true; }
 
  bool GobbleVisitor::visit_str(const char* v, unsigned len)
  { DEB1("Gobble visit_str v=" << v << " l=" << len); return true; }

  bool GobbleVisitor::visit_bin(const char* v, unsigned len)
  { DEB1("Gobble visit_bin l=" << len); return true; }

  bool GobbleVisitor::visit_ext(const char* v, unsigned len)
  { DEB1("Gobble visit_ext l=" << len); return true; }
 
  bool GobbleVisitor::start_array(uint32_t num_elements)
  { DEB1("Gobble start_array num_elements=" << num_elements); return true; }

  bool GobbleVisitor::start_array_item()
  { DEB1("Gobble start_array_item"); return true; }

  bool GobbleVisitor::end_array_item()
  { DEB1("Gobble end_array_item"); return true; }

  bool GobbleVisitor::end_array()
  { DEB1("Gobble end_array"); return true; }

  bool GobbleVisitor::start_map(uint32_t num_elements)
  { DEB1("Gobble start_map num_elements=" << num_elements); return true; }

  bool GobbleVisitor::start_map_key()
  { DEB1("Gobble start_map_key"); return true; }

  bool GobbleVisitor::end_map_key()
  { DEB1("Gobble end_map_key"); return true; }

  bool GobbleVisitor::start_map_value()
  { DEB1("Gobble start_map_value"); return true; }

  bool GobbleVisitor::end_map_value()
  { DEB1("Gobble end_map_value"); return true; }

  bool GobbleVisitor::end_map()
  { DEB1("Gobble end_map"); return true; }
  
  void GobbleVisitor::parse_error(size_t parsed_offset, size_t error_offset)
  { DEB1("Gobble parse error");}

  void GobbleVisitor::insufficient_bytes(size_t parsed_offset, size_t error_offset)
  { DEB1("Gobble insifficient bytes");}

bool DCOVirtualVisitorArray::visit_str(const char* v, uint32_t size)
{ DEB1("A_DCO visit_str nest=" << bool(nest) << " v=" << v);
 if (nest) return nest->visit_str(v, size);
 return false; }

bool DCOVirtualVisitorMap::visit_str(const char* v, uint32_t size)
{ DEB1("M_DCO visit_str nest=" << bool(nest) << " v=" <<
      std::string().assign(v, size));
  switch(mode) {
  case VVMode::Key:
    if (size >= key.max_size()) { throw msgpack_dco_key_too_long(); }
    DEB("M_DCO visit_str assigning key " << std::string().assign(v, size));
    key.assign(v, size); return true;
  case VVMode::Value:
    if (nest) return nest->visit_str(v, size);
  default: ; }
  return false; }

bool DCOVirtualVisitor::visit_bin(const char* v, uint32_t size)
{ DEB1("DCO visit_bin nest=" << bool(nest) << " v=" << v);
 if (nest) return nest->visit_bin(v, size);
 return false; }

bool DCOVirtualVisitor::visit_ext(const char* v, uint32_t size)
{ DEB1("DCO visit_ext nest=" << bool(nest) << " v=" << v);
 if (nest) return nest->visit_ext(v, size);
 return false; }


DCOVirtualVisitorArray::DCOVirtualVisitorArray() :
  DCOVirtualVisitor() { DEB2("DCOVirtualVisitorArray constructor");}
DCOVirtualVisitorMap::DCOVirtualVisitorMap() :
  DCOVirtualVisitor(), key() {DEB2("DCOVirtualVisitorMap constructor");}

bool DCOVirtualVisitorArray::start_array(uint32_t num_elements)
{
  DEB1("A_DCO start_array mode=" << mode << " nest=" << bool(nest) <<
      " depth=" << depth << " num_elements=" << num_elements);
  switch(mode) {
  case VVMode::Init:
    DEB("A_DCO start_array initial array start");
    mode = VVMode::Value;
    sel = -1;
    depth = 0;
    return true;
  case VVMode::Value:
    depth++; return nest->start_array(num_elements);
  default:
    return false;
  }
}
bool DCOVirtualVisitorMap::start_array(uint32_t num_elements)
{
  DEB1("M_DCO start_array mode=" << mode << " nest=" << bool(nest) <<
      " depth=" << depth << " num_elements=" << num_elements );
  if (nest) { return nest->start_array(num_elements); }
  return false;
}

  bool DCOVirtualVisitorArray::start_array_item()
  {
    DEB1("A_DCO start_array_item mode=" << mode << " nest=" << bool(nest) <<
	" depth=" << depth);
    if (depth) {
      return nest->start_array_item();
    }
    // next member of the object
    sel++;  // next member
    DEB("A_DCO start_array_item, selecting member " << sel);
    return setVirtualVisitor();
  }

  bool DCOVirtualVisitorMap::start_array_item()
  {
    DEB1("M_DCO start_array_item mode=" << mode << " nest=" << bool(nest) <<
	" depth=" << depth);
    if (nest) { return nest->start_array_item(); }
    return false;
  }

  bool DCOVirtualVisitorArray::end_array_item()
  {
    DEB1("A_DCO end_array_item mode=" << mode << " nest=" << bool(nest) <<
         " depth=" << depth);
    if (mode != VVMode::Value)
      throw msgpack_obj_mode_mismatch("A_DCO end_array_item", mode);
    if (depth) return nest->end_array_item();
    nest = NULL;
    return true;
  }
  bool DCOVirtualVisitorMap::end_array_item()
  {
    DEB1("M_DCO end_array_item mode=" << mode << " nest=" << bool(nest) <<
         " depth=" << depth);
    if (nest) { return nest->end_array_item(); }
    return false;
  }

  bool DCOVirtualVisitorArray::end_array()
  {
    DEB1("A_DCO end_array mode=" << mode << " nest=" << bool(nest) <<
         " depth=" << depth);
    if (mode != VVMode::Value)
      throw msgpack_obj_mode_mismatch("A_DCO end_array_item", mode);
    if (depth--) return nest->end_array();
    DEB("A_DCO end_array, to Exit");
    mode = VVMode::Exit; return true;
  }
  bool DCOVirtualVisitorMap::end_array()
  {
    DEB1("M_DCO end_array mode=" << mode << " nest=" << bool(nest) <<
         " depth=" << depth);
   if (nest) { return nest->end_array(); }
    return false;
  }

  bool DCOVirtualVisitorArray::start_map(uint32_t num_kv_pairs)
  {
    DEB1("A_DCO start_map mode=" << mode << " nest=" << bool(nest) <<
         " depth=" << depth << " num_kv_pairs=" << num_kv_pairs);
    if (nest) return nest->start_map(num_kv_pairs);
    throw msgpack_obj_mode_mismatch("start_map no nest", mode);
  }
  bool DCOVirtualVisitorMap::start_map(uint32_t num_kv_pairs)
  {
    DEB1("M_DCO start_map mode=" << mode << " nest=" << bool(nest) <<
         " depth=" << depth << " num_kv_pairs=" << num_kv_pairs);
    if (++depth) {
      switch(mode) {
      case VVMode::Value: return nest->start_map(num_kv_pairs);
      default: throw msgpack_obj_mode_mismatch("M_DCO start_map, depth!=0", mode);}}
    if (mode == VVMode::Init) {
      DEB("M_DCO start_map, Init->Next");
      mode = VVMode::Next; return true;
    }
    throw msgpack_obj_mode_mismatch("M_DCO start_map, depth=0", mode);
  }

  bool DCOVirtualVisitorArray::start_map_key()
  {
    DEB1("A_DCO start_map_key mode=" << mode << " nest=" << bool(nest) <<
         " depth=" << depth);
    if (mode != VVMode::Value)
      throw msgpack_obj_mode_mismatch("start_map_key", mode);
    return nest->start_map_key();
  }
  bool DCOVirtualVisitorMap::start_map_key()
  {
    DEB1("M_DCO start_map_key mode=" << mode << " nest=" << bool(nest) <<
        " depth=" << depth);
    switch(mode) {
    case VVMode::Next:
      DEB("M_DCO start_map_key, entering Key");
      mode = VVMode::Key; return true;
    case VVMode::Value: return nest->start_map_key();
    default: ; }
    throw msgpack_obj_mode_mismatch("start_map_key", mode);
  }

  bool DCOVirtualVisitorArray::end_map_key()
  {
    DEB1("A_DCO end_map_key mode=" << mode << " nest=" << bool(nest) <<
	" depth=" << depth);
    if (mode !=VVMode::Value)
      throw msgpack_obj_mode_mismatch("end_map_key", mode);
    return nest->end_map_key();
  }
  bool DCOVirtualVisitorMap::end_map_key()
  {
    DEB1("M_DCO end_map_key mode=" << mode << " nest=" << bool(nest) <<
         " depth=" << depth);
    switch(mode) {
    case VVMode::Key: {
      bool setting = setVirtualVisitor(key.c_str());
      DEB("M_DCO end_map_key, now selected member " << key << " #" << sel);
      if (!setting) {
	DEB("Key " << key << " not in current object, need to gobble");
        throw msgpack_obj_mode_mismatch("end_map_key, no member found", mode);
      }
      return (depth == 0); }
    case VVMode::Value: return nest->end_map_key();
    default:
      throw msgpack_obj_mode_mismatch("end_map_key", mode);
    }
  }

  bool DCOVirtualVisitorArray::start_map_value()
  {
    DEB1("A_DCO start_map_value mode=" << mode << " nest=" << bool(nest) <<
         " depth=" << depth);
    if (mode !=VVMode::Value)
      throw msgpack_obj_mode_mismatch("start_map_value", mode);
    return nest->start_map_value();
  }
  bool DCOVirtualVisitorMap::start_map_value()
  {
    DEB1("M_DCO start_map_value mode=" << mode << " nest=" << bool(nest) <<
	" depth=" << depth);
    switch(mode) {
    case VVMode::Key:
      DEB("M_DCO start_map_value, Key->Value");
      mode = VVMode::Value; return true;
    case VVMode::Value: return nest->start_map_value();
    default:
      throw msgpack_obj_mode_mismatch("start_map_value", mode);
    }
  }

  bool DCOVirtualVisitorArray::end_map_value()
  {
    DEB1("A_DCO end_map_value mode=" << mode << " nest=" << bool(nest) <<
	" depth=" << depth);
    if (mode !=VVMode::Value)
      throw msgpack_obj_mode_mismatch("end_map_value", mode);
    return nest->end_map_value();
  }
  bool DCOVirtualVisitorMap::end_map_value()
  {
    DEB1("M_DCO end_map_value mode=" << mode << " nest=" << bool(nest) <<
         " depth=" << depth);
    switch(mode) {
    case VVMode::Value:
      if (depth) return nest->end_map_value();
      DEB("M_DCO end_map_value, passing to Next");
      mode = VVMode::Next; return true;
    default:
      throw msgpack_obj_mode_mismatch("end_map_value", mode);
    }
  }

  bool DCOVirtualVisitorArray::end_map()
  {
    DEB1("A_DCO end_map mode=" << mode << " nest=" << bool(nest) <<
         " depth=" << depth);
    if (mode !=VVMode::Value)
      throw msgpack_obj_mode_mismatch("A_DCO end_map", mode);
    return nest->end_map();
  }
  bool DCOVirtualVisitorMap::end_map()
  {
    DEB1("M_DCO end_map mode=" << mode << " nest=" << bool(nest) <<
         " depth=" << depth);
    if (depth--) {
      switch (mode) {
      case VVMode::Value: return nest->end_map();
      default: throw msgpack_obj_mode_mismatch("M_DCO end_map", mode);
      }
    }
    DEB("M_DCO end_map, returning to Exit");
    mode = VVMode::Exit; return true;
  }

void DCOVirtualVisitor::parse_error(size_t parsed_offset,
                                    size_t error_offset)
{
  /* DUECA extra.

     Cannot parse a msgpack packed message.
  */
  E_XTR("Error parsing MSGPack, at " << error_offset);
  //throw(msgpack_dco_error("parse error"));
}

void DCOVirtualVisitor::insufficient_bytes(size_t parsed_offset,
                                           size_t error_offset)
{
  /* DUECA extra.

     End of data when parsing a msgpack packed message.
  */
  E_XTR("End of data parsing MSGPack, at " << error_offset);
  //throw(msgpack_dco_error("dataerror"));
}

} // namespace messagepack
DUECA_NS_END;
