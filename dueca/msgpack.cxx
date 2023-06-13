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
#include <debug.h>
#include <dueca_ns.h>
#define DEBPRINTLEVEL 2
#include <debprint.h>

PRINT_NS_START;

ostream &operator<<(ostream &os, const dueca::messagepack::VVMode mode)
{
  static const char *names[] = { "Init", "Map", "Array", "Exit" };
  return (os << names[unsigned(mode)]);
}

ostream &operator<<(ostream &os, const dueca::messagepack::MMode mode)
{
  static const char *names[] = { "Init", "Key", "Value", "Exit" };
  return (os << names[unsigned(mode)]);
}

PRINT_NS_END;

DUECA_NS_START;
namespace messagepack {

  msgpack_obj_mode_mismatch::msgpack_obj_mode_mismatch(const char *operation,
						       VVMode      mode)
  {
    std::stringstream mxs;
    mxs << "MSGPack mode error, in mode " << mode << " no operation "
	<< operation;
    msg = mxs.str();
  }

  msgpack_excess_array_members::msgpack_excess_array_members(const char* dconame)
  {
    std::stringstream mxs;
    mxs << "Too many array members to unpack for " << dconame;
    msg = mxs.str();
  }

  const char *msgpack_obj_mode_mismatch::what() const noexcept
  {
    return msg.c_str();
  }
  const char *msgpack_dco_key_too_long::what() const noexcept
  {
    return "DCO member name too long";
  }
  const char *msgpack_excess_array_members::what() const noexcept
  {
    return msg.c_str();
  }
  DCOUnpackVisitor::DCOUnpackVisitor() :
    sel(-1), depth(-1), nest(NULL), mode(VVMode::Init), key()
  {
    DEB3("DCOUnpackVisitor constructor");
  }

  bool DCOUnpackVisitor::visit_nil()
  {
    DEB1("DCO visit_nil nest=" << bool(nest));
    if (nest)
      return nest->visit_nil();
    return false;
  }

  bool DCOUnpackVisitor::visit_boolean(bool v)
  {
    DEB1("DCO visit_boolean nest=" << bool(nest) << " v=" << v);
    if (nest)
      return nest->visit_boolean(v);
    return false;
  }

  bool DCOUnpackVisitor::visit_positive_integer(uint64_t v)
  {
    DEB1("DCO visit_positive_integer nest=" << bool(nest) << " v=" << v);
    if (nest)
      return nest->visit_positive_integer(v);
    return false;
  }

  bool DCOUnpackVisitor::visit_negative_integer(int64_t v)
  {
    DEB1("DCO visit_negative_integer nest=" << bool(nest) << " v=" << v);
    if (nest)
      return nest->visit_negative_integer(v);
    return false;
  }

  bool DCOUnpackVisitor::visit_float32(float v)
  {
    DEB1("DCO visit_float32 nest=" << bool(nest) << " v=" << v);
    if (nest)
      return nest->visit_float32(v);
    return false;
  }

  bool DCOUnpackVisitor::visit_float64(double v)
  {
    DEB1("DCO visit_float64 nest=" << bool(nest) << " v=" << v);
    if (nest)
      return nest->visit_float64(v);
    return false;
  }

  GobbleVisitor::GobbleVisitor(const char *klass) :
    seen(), classname(klass), missing_lock("gobbler")
  {
    missing_lock.leaveState();
  }

  bool GobbleVisitor::visit_nil()
  {
    DEB1("Gobble visit_nil");
    return true;
  }

  bool GobbleVisitor::visit_boolean(bool v)
  {
    DEB1("Gobble visit_boolean v=" << v);
    return true;
  }

  bool GobbleVisitor::visit_positive_integer(uint64_t v)
  {
    DEB1("Gobble visit_positive_integer v=" << v);
    return true;
  }

  bool GobbleVisitor::visit_negative_integer(int64_t v)
  {
    DEB1("Gobble visit_negative_integer v=" << v);
    return true;
  }

  bool GobbleVisitor::visit_float32(float v)
  {
    DEB1("Gobble visit_float32 v=" << v);
    return true;
  }

  bool GobbleVisitor::visit_float64(double v)
  {
    DEB1("Gobble visit_float64 v=" << v);
    return true;
  }

  bool GobbleVisitor::visit_str(const char *v, unsigned len)
  {
    DEB1("Gobble visit_str v=" << v << " l=" << len);
    return true;
  }

  bool GobbleVisitor::visit_bin(const char *v, unsigned len)
  {
    DEB1("Gobble visit_bin l=" << len);
    return true;
  }

  bool GobbleVisitor::visit_ext(const char *v, unsigned len)
  {
    DEB1("Gobble visit_ext l=" << len);
    return true;
  }

  bool GobbleVisitor::start_array(uint32_t num_elements)
  {
    DEB1("Gobble start_array num_elements=" << num_elements);
    return true;
  }

  bool GobbleVisitor::start_array_item()
  {
    DEB1("Gobble start_array_item");
    return true;
  }

  bool GobbleVisitor::end_array_item()
  {
    DEB1("Gobble end_array_item");
    return true;
  }

  bool GobbleVisitor::end_array()
  {
    DEB1("Gobble end_array");
    return true;
  }

  bool GobbleVisitor::start_map(uint32_t num_elements)
  {
    DEB1("Gobble start_map num_elements=" << num_elements);
    return true;
  }

  bool GobbleVisitor::start_map_key()
  {
    DEB1("Gobble start_map_key");
    return true;
  }

  bool GobbleVisitor::end_map_key()
  {
    DEB1("Gobble end_map_key");
    return true;
  }

  bool GobbleVisitor::start_map_value()
  {
    DEB1("Gobble start_map_value");
    return true;
  }

  bool GobbleVisitor::end_map_value()
  {
    DEB1("Gobble end_map_value");
    return true;
  }

  bool GobbleVisitor::end_map()
  {
    DEB1("Gobble end_map");
    return true;
  }

  void GobbleVisitor::parse_error(size_t parsed_offset, size_t error_offset)
  {
    DEB1("Gobble parse error");
  }

  void GobbleVisitor::insufficient_bytes(size_t parsed_offset,
					 size_t error_offset)
  {
    DEB1("Gobble insufficient bytes");
  }

  VirtualVisitor *GobbleVisitor::missingMember(const char *name)
  {
    ScopeLock   l(missing_lock);
    std::string _name(name);
    if (seen.count(_name))
      return this;
    seen.insert(_name);
    /* DUECA msgpack.

       A mgpack message contained a data member that was not present
       in your DCO type. This message is generated only once for this
       type.
    */
    W_XTR("msgpack visitor, object of type " << classname << " no member "
	  << name);
    return this;
  }

  bool DCOUnpackVisitor::visit_str(const char *v, uint32_t size)
  {
    DEB1("DCO visit_str nest=" << bool(nest)
	 << " v=" << std::string().assign(v, size));
    if (nest) {
      return nest->visit_str(v, size);
    }
    if (mode == VVMode::Map) {
      if (size >= key.max_size()) {
	throw msgpack_dco_key_too_long();
      }
      DEB("DCO visit_str assigning key " << std::string().assign(v, size));
      key.assign(v, size);
      return true;
    }
    throw msgpack_obj_mode_mismatch("DCO visit_str, no nest", mode);
  }

  bool DCOUnpackVisitor::visit_bin(const char *v, uint32_t size)
  {
    DEB1("DCO visit_bin nest=" << bool(nest) << " v=" << v);
    if (nest)
      return nest->visit_bin(v, size);
    return false;
  }

  bool DCOUnpackVisitor::visit_ext(const char *v, uint32_t size)
  {
    DEB1("DCO visit_ext nest=" << bool(nest) << " v=" << v);
    if (nest)
      return nest->visit_ext(v, size);
    return false;
  }

  bool DCOUnpackVisitor::start_array(uint32_t num_elements)
  {
    DEB1("DCO start_array mode=" << mode << " nest=" << bool(nest)
	 << " depth=" << depth << " num_elements=" << num_elements);
    if (nest) {
      depth++;
      return nest->start_array(num_elements);
    }

    if (mode == VVMode::Init) {
      DEB("DCO start_array initial");
      mode = VVMode::Array;
      sel = -1;
      depth = 0;
      return true;
    }
    throw msgpack_obj_mode_mismatch("DCO start_array, no nest", mode);
  }


  bool DCOUnpackVisitor::start_array_item()
  {
    DEB1("DCO start_array_item mode=" << mode << " nest=" << bool(nest)
	 << " depth=" << depth);
    if (nest) {
      return nest->start_array_item();
    }
    if (mode == VVMode::Array) {
      sel++;   // to next member of the object
      DEB("DCO start_array_item, selecting member " << sel);
      // this sets nest, returns true
      return setVirtualVisitor();
    }
    throw msgpack_obj_mode_mismatch("DCO start_array_item, no nest", mode);
  }


  bool DCOUnpackVisitor::end_array_item()
  {
    DEB1("DCO end_array_item mode=" << mode << " nest=" << bool(nest)
	 << " depth=" << depth);
    if (mode == VVMode::Array && depth == 0) {
      nest = NULL;
      return true;
    }
    if (nest) {
      return nest->end_array_item();
    }
    throw msgpack_obj_mode_mismatch("DCO end_array_item, no nest", mode);
  }

  bool DCOUnpackVisitor::end_array()
  {
    DEB1("DCO end_array mode=" << mode << " nest=" << bool(nest)
	 << " depth=" << depth);
    if (nest) {
      depth--;
      return nest->end_array();
    }

    if (mode == VVMode::Array) {
      DEB("DCO end_array, to Exit");
      mode = VVMode::Exit;
      return true;
    }

    throw msgpack_obj_mode_mismatch("DCO end_array", mode);
  }

  bool DCOUnpackVisitor::start_map(uint32_t num_kv_pairs)
  {
    DEB1("DCO start_map mode=" << mode << " nest=" << bool(nest) << " depth="
	 << depth << " num_kv_pairs=" << num_kv_pairs);
    if (nest) {
      depth++;
      return nest->start_map(num_kv_pairs);
    }
    if (mode == VVMode::Init) {
      mode = VVMode::Map;
      sel = -1;
      depth = 0;
      return true;
    }
    throw msgpack_obj_mode_mismatch("DCO start_map, no nest", mode);
  }


  bool DCOUnpackVisitor::start_map_key()
  {
    DEB1("DCO start_map_key mode=" << mode << " nest=" << bool(nest)
	 << " depth=" << depth);
    if (nest) {
      return nest->start_map_key();
    }
    if (mode == VVMode::Map) {
      return true;
    }
    throw msgpack_obj_mode_mismatch("DCO start_map_key, no nest", mode);
  }


  bool DCOUnpackVisitor::end_map_key()
  {
    DEB1("DCO end_map_key mode=" << mode << " nest=" << bool(nest)
	 << " depth=" << depth);
    if (nest) {
      return nest->end_map_key();
    }
    if (mode == VVMode::Map) {
      return true;
    }
    throw msgpack_obj_mode_mismatch("end_map_key, no nest", mode);
  }


  bool DCOUnpackVisitor::start_map_value()
  {
    if (nest) {
      return nest->start_map_value();
    }
    else if (mode == VVMode::Map) {
      return setVirtualVisitor(key.c_str());
    }
    throw msgpack_obj_mode_mismatch("start_map_value, no nest", mode);
  }


  bool DCOUnpackVisitor::end_map_value()
  {
    DEB1("DCO end_map_value mode=" << mode << " nest=" << bool(nest)
	 << " depth=" << depth);
    if (mode == VVMode::Map && depth == 0) {
      nest = NULL;
      return true;
    }
    if (nest) {
      return nest->end_map_value();
    }
    throw msgpack_obj_mode_mismatch("end_map_value, no nest", mode);
  }


  bool DCOUnpackVisitor::end_map()
  {
    DEB1("DCO end_map mode=" << mode << " nest=" << bool(nest)
	 << " depth=" << depth);
    if (nest) {
      depth--;
      return nest->end_map();
    }
    if (mode == VVMode::Map) {
      DEB("DCO end_array, to Exit");
      mode = VVMode::Exit;
      return true;
    }
    throw msgpack_obj_mode_mismatch("DCO end_map, no nest", mode);
  }

  void DCOUnpackVisitor::parse_error(size_t parsed_offset, size_t error_offset)
  {
    /* DUECA extra.

       Cannot parse a msgpack packed message.
    */
    E_XTR("Error parsing MSGPack, at " << error_offset);
    // throw(msgpack_dco_error("parse error"));
  }

  void DCOUnpackVisitor::insufficient_bytes(size_t parsed_offset,
					     size_t error_offset)
  {
    /* DUECA extra.

       End of data when parsing a msgpack packed message.
    */
    E_XTR("End of data parsing MSGPack, at " << error_offset);
    // throw(msgpack_dco_error("dataerror"));
  }

} // namespace messagepack
DUECA_NS_END;
