# AddOn for msgpack option
"""     item            : msgpack.py
        made by         : RvP
        date            : 2017
        category        : python program
        description     : Code generation of DUECA Communication Objects (DCO)
                          msgpack extension
        language        : python
        changes         : 1704xx RvP Added a plugin system, to enable
                                     extension of code generation
                          171010 RvP Added option for compress
        copyright       : TUDelft-AE-C&S

AddOn objects extend the code generation by the dueca-codegen program

Create a file named after the Option you want to add to the code generation,
and install it in the DCOplugins directory. This file adds (Option msgpack) to
DCO files, adding ability to convert DCO objects to and from msgpack.

The same pattern can be followed to add other capabilities.

- base, define, map vs array for objects; Inherited is mapped with
  class name as key!
  https://github.com/msgpack/msgpack-c/wiki/v1_1_cpp_adaptor

- visitor
  https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_visitor

- manual packing
  https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_packer#pack-manually
"""

from jinja2 import Environment, BaseLoader

env = Environment(loader=BaseLoader)

headercode_tmpl = """
#if defined(DUECA_CONFIG_MSGPACK) || defined(DUECA_CONFIG_DDFF)

{#- For any enum members, implement the msgpack conversion #}
{%- for m in members %}
{%- if m.isEnum() and m.getMembers()|length != 0 and eclasses.firstSeen(m.getType()) %}
# ifndef __CUSTOM_MSGPACK_ENUM_{{ self.name }}_{{ m.getType(bare=True) }}
MSGPACK_ADD_ENUM({{ m.getType() }});
MSGPACK_ADD_ENUM_VISITOR({{ m.getType() }});
MSGPACK_ADD_ENUM_UNSTREAM({{ m.getType() }});
# endif
{%- endif %}
{%- endfor %}

#ifndef __CUSTOM_MSGPACK_PACK_{{ name }}
namespace msgpack {
/// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
/// @endcond
namespace adaptor {

/// msgpack pack specialization
template <>
struct pack<{{ nsprefix }}{{ name }}> {
  template <typename Stream>
  msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o,
                                      const {{ nsprefix }}{{ name }}& v) const
  {
    MSGPACK_DCO_OBJECT(this->n_members());
    this->pack_members<Stream>(o, v);
    return o;
  }
  static constexpr unsigned n_members() {
    {%- if parent %}
    return {{ members|length }}U + pack<{{ parent }}>::n_members();
    {%- else %}
    return {{ members|length }}U;
    {%- endif %}
  }
  template <typename Stream>
  static void pack_members(msgpack::packer<Stream>& o,
                           const {{ nsprefix }}{{ name }}& v)
  {
    {%- if parent %}
    pack<{{ parent }}>::pack_members<Stream>(o, v);
    {%- endif %}
    {%- for m in members %}
    MSGPACK_DCO_MEMBER({{ m.getName() }});
    {%- endfor %}
  }
};
} // namespace adaptor
/// @cond
} // MSGPACK_API_VERSION_NAMESPACE(v1)
/// @endcond
} // namespace msgpack
#endif

namespace dueca {
namespace messagepack {

/** Specify the visitor class for this object */
template<> struct msgpack_visitor<{{ nsprefix }}{{ name }}>
{ typedef msgpack_container_dco variant; };

#ifndef __CUSTOM_MSGPACK_VISITOR_{{ name }}

/** Gobble visitor, for uncoded "members" */
GobbleVisitor& v_gobble_{{ name }}();

/** Unpackvisitor class for {{ name }} */
template<>
struct UnpackVisitor<msgpack_container_dco,{{ nsprefix }}{{ name }}>:
  {%- if parent %}
  public UnpackVisitor<msgpack_container_dco,{{ parent }}>
  {%- else %}
  public DCOUnpackVisitor
  {%- endif %}
{
  /** Reference to the object currently being filled */
  {{ nsprefix }}{{ name }}& v;

  /** How many members in the parent */
  {%- if parent %}
  static constexpr unsigned parent_n_members =
    msgpack::v1::adaptor::pack<{{ parent }}>::n_members();
  {%- else %}
  static constexpr unsigned parent_n_members = 0U;
  {%- endif %}
  /** How many total? */
  static constexpr unsigned n_members =
    msgpack::v1::adaptor::pack<{{ nsprefix }}{{ name }}>::n_members();

  {%- for m in members %}
  /** Unpackvisitor for member "{{ m.getName() }}" */
  UnpackVisitor<typename msgpack_visitor<{{ m.getType() }}>::variant,{{ m.getType() }}>
    v_{{ m.getName() }};
  {%- endfor %}

  /** Generic reference to the member variable visitors */
  MemberVisitorTable visitors[{{ members|length }}];

  /** Constructor */
  UnpackVisitor({{ nsprefix }}{{ name }} &v) :
    {%- if parent %}
    UnpackVisitor<msgpack_container_dco,{{ parent }}>(v),
    {%- else %}
    DCOUnpackVisitor(),
    {%- endif %}
    v(v),
    {%- for m in members %}
    v_{{ m.getName() }}(v.{{ m.getName() }}),
    {%- endfor %}
    visitors{
      {%- for m in members %}
      { "{{ m.getName() }}", &v_{{ m.getName() }} }{{ m != members|last and ',' or '' }}
      {%- endfor %}
    }
  { }

  /** Select a specific virtual visitor.

      @param name    If NULL, use the (parent class) variable sel,
                     otherwise determine sel from the name */
  virtual bool setVirtualVisitor(const char* name = NULL, bool isparent=false)
  {
    {%- if parent %}
    if (this->UnpackVisitor<msgpack_container_dco,{{ parent }}>::
          setVirtualVisitor(name, true)) return true;

    {%- endif %}
    if (name) {
      {%- if not parent %}
      sel = 0;
      {%- endif %}
      for (const auto v: visitors) {
        if (!strcmp(v.name, name)) { nest = v.visitor; return true; }
        sel++;
      }
      if (isparent) return false;
      nest = v_gobble_{{ name }}().missingMember(name);
      return true;
    }
    if (sel < int(n_members)) {
      nest = visitors[sel - parent_n_members].visitor;
      return true;
    }
    if (isparent) return false;
    throw msgpack_excess_array_members("{{ name }}");
  }
};
# endif
} // namespace messagepack
} // namespace dueca

namespace msgunpack {
template<typename S>
void msg_unpack(S& i0, const S& iend, {{ nsprefix }}{{ name }}&i);
} // namespace msgunpack

# ifndef __CUSTOM_MSGPACK_UNPACK_{{ name }}
namespace msgunpack {
template<typename S>
void msg_unpack(S& i0, const S& iend, {{ nsprefix }}{{ name }}&i)
{
  {%- if parent %}
  msg_unpack<S>(i0, iend, *reinterpret_cast<{{ parent }}*>(&i));
  {%- else %}
  MSGPACK_CHECK_DCO_SIZE(0);
  {%- endif %}
  {%- for m in members %}
  MSGPACK_UNPACK_MEMBER(i.{{ m.getName() }});
  {%- endfor %}
};
} // namespace msgunpack
# endif
# endif
"""

bodycode_tmpl = """
#ifndef __CUSTOM_MSGPACK_VISITOR_{{ name }}
namespace dueca {
namespace messagepack {
GobbleVisitor& v_gobble_{{ name }}()
{
  static GobbleVisitor _v("{{ name }}");
  return _v;
}
} // namespace messagepack
} // namespace dueca
#endif

#if defined(DUECA_CONFIG_MSGPACK) || defined(DUECA_CONFIG_DDFF)

namespace {{ name }}_space {
#if !defined(__CUSTOM_MSGPACK_WRITE_FUNCTOR)
struct DDFFDCOWriteFunctor: public dueca::ddff::DDFFDCOWriteFunctor {

  // constructor
  DDFFDCOWriteFunctor(bool rtick) :
    dueca::ddff::DDFFDCOWriteFunctor(rtick)
  {
    //
  }

  // operation; write data from the stream to the channel
  bool operator() (void* dpointer)
  {
    if (*it_ptr == itend) {
      throw dueca::ddff::ddff_file_format_error();
    }
    if (rtick) {
      uint32_t len = msgunpack::unstream
        <dueca::ddff::FileStreamRead::Iterator>::
        unpack_arraysize(*it_ptr, itend);
      if (len != 3) throw dueca::ddff::ddff_file_format_error();
      dueca::TimeTickType dummy_tick;
      msgunpack::msg_unpack(*it_ptr, itend, dummy_tick);
      dueca::TimeTickType dummy_span;
      msgunpack::msg_unpack(*it_ptr, itend, dummy_span);
    }
    msgunpack::msg_unpack(*it_ptr, itend,
                          *reinterpret_cast<{{ nsprefix }}{{ name }}*>(dpointer));
    return true;
  }
};
#endif

#if !defined(__CUSTOM_MSGPACK_READ_FUNCTOR)
struct DDFFDCOReadFunctor: public dueca::ddff::DDFFDCOReadFunctor {

  // constructor
  DDFFDCOReadFunctor(dueca::ddff::FileStreamWrite::pointer wstream,
                     const dueca::DataTimeSpec* startend) :
    dueca::ddff::DDFFDCOReadFunctor(wstream, startend)
  {
    //
  }

  // With this call, the channel or a CommObjectReader passes data
  // for recording, this data is written to the stream
  bool operator() (const void* dpointer,
                   const dueca::DataTimeSpec& ts)
  {
    // return true while still before a planned data recording stretch
    while(ts.getValidityEnd() <= startend->getValidityStart()) {
      return true;
    }

    // return false when beyond a given data writing stretch
    if (ts.getValidityStart() >= startend->getValidityEnd()) {
      return false;
    }

    // timing information and object in an array of three
    msgpack::packer<dueca::ddff::FileStreamWrite> pk(*wstream);
    pk.pack_array(3);

    // when completely in the recording range, as it should be
    if (ts.getValidityStart() >= startend->getValidityStart()) {
      pk.pack(ts.getValidityStart() - startend->getValidityStart());
      pk.pack(ts.getValiditySpan());
    }

    // not completely in the recording range, cutting of a part
    // of the range, and warn
    else {
      pk.pack(startend->getValidityStart());
      pk.pack(ts.getValidityEnd() - startend->getValidityStart());
      /** Recording start is not aligned with data time spans; adjust
          your intervals when starting the Environment. */
      std::cerr << "Partial data span for recording, span="
                << ts << " recording start="
                << startend->getValidityStart() << std::endl;
    }

    // pack the object
    pk.pack(*reinterpret_cast<const {{ nsprefix }}{{ name }}*>(dpointer));
    return true;
  }
};
#endif

class DDFFDCOMetaFunctor: public dueca::ddff::DDFFDCOMetaFunctor
{
  DDFFDCOReadFunctor*
  getReadFunctor(dueca::ddff::FileStreamWrite::pointer wstream,
                 const dueca::DataTimeSpec* startend) final
  {
    return new DDFFDCOReadFunctor(wstream, startend);
  }

  DDFFDCOWriteFunctor*
  getWriteFunctor(bool rtick)
  {
    return new DDFFDCOWriteFunctor(rtick);
  }
};

#if !defined(__DCO_STANDALONE)
  // loads the metafunctor in the table
  static dueca::LoadMetaFunctor<DDFFDCOMetaFunctor>
    load_functor_msgpack({{ nsprefix }}functortable, "msgpack");
#endif
} // end namespace {{ name }}_space
#endif
"""


def joindict(x, y):
    z = x.copy()
    z.update(y)
    return z


class CheckSeen:
    """ Remember seen objects, return true on first occurrence
    """

    def __init__(self):
        self.seen = set()

    def firstSeen(self, s: str):
        if s in self.seen:
            return False
        self.seen.add(s)
        return True


class AddOn(object):
    """ Print MSGPACK interaction code for a DCO object.

    - include section of header (printHeaderInclude)
    - in class definition, at end, but just before extra include
      (printHeaderClassCode)
    - in header, outside class (printHeaderCode)
    - include in body file (printBodyInclude)
    - in the body file, in a check section executed when custom body
      code is included (printBodyCheck)
    - in the body (printBodyCode)

    """

    def __init__(self, namespace, name, parent, members, nest=False):
        """ Initialisation of an AddOn object

        namespace -  name space of the DCO object
        name      -  class name of DCO object
        parent    -  parent class name
        members   -  list with MemberSummary objects describing data members,
                     options:
                         getName()
                         getType(bare=, in_class=, in_namespace=)
                         isEnum()
                         isIterable()
                         and for enums
                         getCType()
                         getMembers(bare=, in_class=, in_namespace=)
        """

        self.namespace = namespace
        self.nsprefix = (namespace and namespace+'::') or ''
        self.nsopen = (
            namespace and "\nnamespace %s {" % namespace) or ''
        self.nsclose = (
            namespace and "\n} // end namespace %s" % namespace) or ''
        self.name = name
        self.parent = parent
        self.members = members
        self.nest = nest
        self.dueca_msgpack_version = 1

    def printHeaderInclude(self):
        """ print the lines that will be added to the header's include area
        """
        return """
#include <dueca/msgpack.hxx>
#ifndef NESTED_DCO
#include <dueca/msgpack-unstream-iter.hxx>
#endif"""

    def printBodyInclude(self):
        """ print the lines that will be added to the body's include area
        """
        return """
# include <algorithm>
# include <ddff/DDFFDCOMetaFunctor.hxx>
# include <dueca/msgpack-unstream-iter.ixx>"""

    def printBodyCheck(self):
        """print the lines *after* a possible include of custom body code to
        check that any custom-built msgpack/ddff code is compatible with the
        version it was designed for. If the interfaces used by the
        code generation of msgpack code are changed, define a new version
        number, so this check is updated.

        """

        return r"""
# define DUECA_MSGPACK_CODEGEN_VERSION {dueca_msgpack_version}
# if defined({customdefines})
# ifndef __CUSTOM_COMPATLEVEL_MSGPACK_{dueca_msgpack_version}
# error "Verify custom msgpack code compatibility with version {dueca_msgpack_version}.\
 Then define __CUSTOM_COMPATLEVEL_MSGPACK_{dueca_msgpack_version}"
# endif
# endif
""".format(dueca_msgpack_version=self.dueca_msgpack_version,
           customdefines=r""") || \
    defined(""".join(self.getCustomDefines()))

    def printHeaderClassCode(self):
        """code that will be inserted in the definition of the class. Assumes
        public, and inserted at the end"""

        return ""

    def printHeaderCode(self):
        """code that will be inserted in the header after the class
        definition. Starts outside any namespace directive.
        """
        return env.from_string(headercode_tmpl).render(
            eclasses=CheckSeen(),
            members=self.members,
            name=self.name,
            parent=self.parent,
            nsprefix=self.nsprefix
        )

    def printBodyCode(self):
        """code that is inserted in the body file. After all regular
        code, assumes global namespace"""

        return env.from_string(bodycode_tmpl).render(
            nsprefix=self.nsprefix,
            name=self.name
        )

    def getCustomDefines(self):
        """these defines guard the implementation in the body, and can be used
        to override standard implementation. When the standard
        implementation is overridden, define
        __CUSTOM_COMPATLEVEL_MSGPACK_# to indicate compatibility with a
        specific version of the hdf code. These are also checked for
        the general code generation compatibility level.

        """

        emembers = set()
        res = []
        for m in self.members:
            if m.isEnum() and len(m.getMembers()) > 0 and \
                    m.getType() not in emembers:
                res.append('__CUSTOM_MSGPACK_ENUM_' + self.name + '_'
                           + m.getType(bare=True))
        res.append(f'__CUSTOM_MSGPACK_PACK_{self.name}')
        res.append(f'__CUSTOM_MSGPACK_VISITOR_{self.name}')
        res.append('__CUSTOM_MSGPACK_WRITE_FUNCTOR')
        res.append('__CUSTOM_MSGPACK_READ_FUNCTOR')

        return res
