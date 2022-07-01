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


def joindict(x, y):
    z = x.copy()
    z.update(y)
    return z

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
#include <dueca/msgpack-unstream-iter.hxx>
// #include <dueca/msgpack-unstream-iter.ixx>"""

    def printBodyInclude(self):
        """ print the lines that will be added to the body's include area
        """
        return """
#include <algorithm>
#include <ddff/DDFFDCOMetaFunctor.hxx>
#include <dueca/msgpack-unstream-iter.ixx>"""

    def printBodyCheck(self):
        """print the lines *after* a possible include of custom body code to
        check that any custom-built msgpack/ddff code is compatible with the
        version it was designed for. If the interfaces used by the
        code generation of msgpack code are changed, define a new version
        number, so this check is updated.

        """

        return r"""
#define DUECA_MSGPACK_CODEGEN_VERSION {dueca_msgpack_version}
#if defined({customdefines})
#ifndef __CUSTOM_COMPATLEVEL_MSGPACK_{dueca_msgpack_version}
#error "Verify custom msgpack code compatibility with version {dueca_msgpack_version}.\
 Then define __CUSTOM_COMPATLEVEL_MSGPACK_{dueca_msgpack_version}"
#endif
#endif
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

        res = [ """
#if defined(DUECA_CONFIG_MSGPACK) || defined(DUECA_CONFIG_DDFF)
""" ]

        # add any enum's defined in the class
        eclasses = set()
        for m in self.members:
            if not m.isEnum() or len(m.getMembers()) == 0 or \
                m.getType() in eclasses:
                continue

            eclasses.add(m.getType())
            res.append( f"""
#ifndef __CUSTOM_MSGPACK_ENUM_{self.name}_{m.getType(bare=True)}
MSGPACK_ADD_ENUM({m.getType()});
MSGPACK_ADD_ENUM_VISITOR({m.getType()});
MSGPACK_ADD_ENUM_UNSTREAM({m.getType()});
#endif""")

        # packing help for msgpack
        res.append("""
#ifndef __CUSTOM_MSGPACK_PACK_{name}
namespace msgpack {{
/// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {{
/// @endcond
namespace adaptor {{

/// msgpack pack specialization
template <>
struct pack<{nsprefix}{name}> {{
  template <typename Stream>
  msgpack::packer<Stream>& operator()(msgpack::packer<Stream>& o,
                                      const {nsprefix}{name}& v) const
  {{
    MSGPACK_DCO_OBJECT(this->n_members());
    this->pack_members<Stream>(o, v);
    return o;
  }}
  static constexpr unsigned n_members() {{
    return {len_members}{parentadd};
  }}
  template <typename Stream>
  static void pack_members(msgpack::packer<Stream>& o,
                           const {nsprefix}{name}& v)
  {{""".format(nsprefix=self.nsprefix, name=self.name,
               len_members=len(self.members),
               parentadd= (self.parent or '') and
               f' + pack<{self.parent}>::n_members()' or '',
               parentpack=(
                   self.parent and
                   f"\nthis->{self.parent}::pack_members<Stream>(o, v);") or ''))

        # pack members of the parent, if applicable
        if self.parent:
            res.append("\n    pack<{parent}>::pack_members<Stream>(o, v);".format(
                **self.__dict__))

        # append all pack actions, should automatically recurse/array/etc
        for m in self.members:
            res.append(f"\n    MSGPACK_DCO_MEMBER({m.getName()});")

        # finish off, and start the UnpackVisitor
        res.append("""
  }}
}};

}} // namespace adaptor
/// @cond
}} // MSGPACK_API_VERSION_NAMESPACE(v1)
/// @endcond
}} // namespace msgpack
#endif

namespace dueca {{
namespace messagepack {{

/** Specify the visitor class for this object */
template<> struct msgpack_visitor<{nsprefix}{name}>
{{ typedef msgpack_container_dco variant; }};

#ifndef __CUSTOM_MSGPACK_VISITOR_{name}
/** Unpackvisitor class for {name} */
template<>
struct UnpackVisitor<msgpack_container_dco,{nsprefix}{name}>:
  public {visitorparent}
{{
  /** Reference to the object currently being filled */
  {nsprefix}{name}& v;

  /** How many members in the parent */
  static constexpr unsigned parent_n_members = {parentnmembers};
  /** How many total? */
  static constexpr unsigned n_members =
    msgpack::v1::adaptor::pack<{nsprefix}{name}>::n_members();

""".format(nsprefix=self.nsprefix, name=self.name,
           parentnmembers = self.parent and f"""
    msgpack::v1::adaptor::pack<{self.parent}>::n_members();""" or '0U',
           visitorparent = self.parent and
           f"UnpackVisitor<msgpack_container_dco,{self.parent}>" or
           "DCO_VIRTUAL_VISITOR_BASE"))

        for m in self.members:
            res.append(f"""
  /** Unpackvisitor for member "{m.getName()}" */
  UnpackVisitor<typename msgpack_visitor<{m.getType()}>::variant,{m.getType()}>
    v_{m.getName()};""")

        res.append("""
  /** Generic reference to the member variable visitors */
  MemberVisitorTable visitors[{nmembers}];

  /** Constructor */
  UnpackVisitor({nsprefix}{name} &v) :
    {visitorparent}, v(v),
{smallgrut},
    visitors{{
{initvtable}
            }}
    {{ }}

  /** Select a specific virtual visitor.

      @param name    If NULL, use the (parent class) variable sel,
                     otherwise determine sel from the name */
  virtual bool setVirtualVisitor(const char* name = NULL)
  {{{callparent}
    if (name) {{{resetsel}
      for (const auto v: visitors) {{
        if (!strcmp(v.name, name)) {{ nest = v.visitor; return true; }}
        sel++;
      }}
      return false;
    }}
    if (sel < int(n_members)) {{
      nest = visitors[sel - parent_n_members].visitor;
      return true;
    }}
    return false;
  }}
}};
#endif
}} // namespace messagepack
}} // namespace dueca
""".format(
          nmembers = len(self.members),
          visitorparent = self.parent and
           f"UnpackVisitor<msgpack_container_dco,{self.parent}>(v)" or
           "DCO_VIRTUAL_VISITOR_BASE()",
          nsprefix=self.nsprefix, name=self.name,
          smallgrut = ',\n'.join(
              [ f"    v_{m.getName()}(v.{m.getName()})" for m in self.members ]),
          initvtable = ',\n'.join(
              [ f"""    {{ "{m.getName()}", &v_{m.getName()} }}"""
                 for m in self.members]),
          entries = '\n'.join(
              [ f"""      MemberVisitorTable visitor_{m.getName()};"""
                for m in self.members]),
          callparent = self.parent and f"""
    if (this->UnpackVisitor<msgpack_container_dco,{self.parent}>::
          setVirtualVisitor(name)) return true;""" or '',
          resetsel = (not self.parent) and """
      sel = 0;""" or ''))


        res.append("""

#ifndef __CUSTOM_MSGPACK_UNPACK_{name}
namespace msgunpack {{
template<typename S>
void msg_unpack(S& i0, const S& iend, {nsprefix}{name}&i)
{{
  {parentorskipsize};""".format(
  nsprefix=self.nsprefix, name=self.name,
  parentorskipsize=self.parent and
  f"msg_unpack<S>(i0, iend, *reinterpret_cast<{self.parent}*>(&i))"
    or "MSGPACK_CHECK_DCO_SIZE(0)"))

        for m in self.members:
            res.append("""
  MSGPACK_UNPACK_MEMBER(i.{name});""".format(name=m.getName()))
        res.append("""
};
}
#endif
#endif
""")

        return ''.join(res)

    def printBodyCode(self):
        """code that is inserted in the body file. After all regular
        code, assumes global namespace"""

        return """
#if defined(DUECA_CONFIG_MSGPACK) || defined(DUECA_CONFIG_DDFF)

namespace {name}_space {{

#if !defined(__CUSTOM_MSGPACK_WRITE_FUNCTOR)
struct DDFFDCOWriteFunctor: public dueca::ddff::DDFFDCOWriteFunctor {{

  // constructor
  DDFFDCOWriteFunctor(bool rtick) :
    dueca::ddff::DDFFDCOWriteFunctor(rtick)
  {{
    //
  }}

  // operation; write data from the stream to the channel
  bool operator() (void* dpointer)
  {{
    if (*it_ptr == itend) {{
      throw dueca::ddff::ddff_file_format_error();
    }}
    if (rtick) {{
      uint32_t len = msgunpack::unstream
        <dueca::ddff::FileStreamRead::Iterator>::
        unpack_arraysize(*it_ptr, itend);
      if (len != 3) throw dueca::ddff::ddff_file_format_error();
      dueca::TimeTickType dummy_tick;
      msgunpack::msg_unpack(*it_ptr, itend, dummy_tick);
      dueca::TimeTickType dummy_span;
      msgunpack::msg_unpack(*it_ptr, itend, dummy_span);
    }}
    msgunpack::msg_unpack(*it_ptr, itend,
                          *reinterpret_cast<{nsprefix}{name}*>(dpointer));
    return true;
  }}
}};
#endif

#if !defined(__CUSTOM_MSGPACK_READ_FUNCTOR)
struct DDFFDCOReadFunctor: public dueca::ddff::DDFFDCOReadFunctor {{

  // constructor
  DDFFDCOReadFunctor(dueca::ddff::FileStreamWrite::pointer wstream,
                     const dueca::DataTimeSpec* startend) :
    dueca::ddff::DDFFDCOReadFunctor(wstream, startend)
  {{
    //
  }}

  // With this call, the channel or a CommObjectReader passes data
  // for recording, this data is written to the stream
  bool operator() (const void* dpointer,
                   const dueca::DataTimeSpec& ts)
  {{
    // return true while still before a planned data recording stretch
    while(ts.getValidityEnd() <= startend->getValidityStart()) {{
      return true;
    }}

    // return false when beyond a given data writing stretch
    if (ts.getValidityStart() >= startend->getValidityEnd()) {{
      return false;
    }}

    // timing information and object in an array of three
    msgpack::packer<dueca::ddff::FileStreamWrite> pk(*wstream);
    pk.pack_array(3);

    // when completely in the recording range, as it should be
    if (ts.getValidityStart() >= startend->getValidityStart()) {{
      pk.pack(ts.getValidityStart() - startend->getValidityStart());
      pk.pack(ts.getValiditySpan());
    }}

    // not completely in the recording range, cutting of a part
    // of the range, and warn
    else {{
      pk.pack(startend->getValidityStart());
      pk.pack(ts.getValidityEnd() - startend->getValidityStart());
      /** Recording start is not aligned with data time spans; adjust
          your intervals when starting the Environment. */
      std::cerr << "Partial data span for recording, span="
                << ts << " recording start="
                << startend->getValidityStart() << std::endl;
    }}

    // pack the object
    pk.pack(*reinterpret_cast<const {nsprefix}{name}*>(dpointer));
    return true;
  }}
}};
#endif

class DDFFDCOMetaFunctor: public dueca::ddff::DDFFDCOMetaFunctor
{{
  DDFFDCOReadFunctor*
  getReadFunctor(dueca::ddff::FileStreamWrite::pointer wstream,
                 const dueca::DataTimeSpec* startend) final
  {{
    return new DDFFDCOReadFunctor(wstream, startend);
  }}

  DDFFDCOWriteFunctor*
  getWriteFunctor(bool rtick)
  {{
    return new DDFFDCOWriteFunctor(rtick);
  }}
}};

#if !defined(__DCO_STANDALONE)
  // loads the metafunctor in the table
  static dueca::LoadMetaFunctor<DDFFDCOMetaFunctor>
    load_functor_msgpack({nsprefix}functortable, "msgpack");
#endif
}} // end namespace MyBlip_space
#endif
""".format(nsprefix=self.nsprefix, name=self.name)

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
