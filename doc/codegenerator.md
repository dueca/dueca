# DUECA code generator {#codegenerator}

The C++ objects that are sent over event or stream channels need to
obey certain rules so that they can be packed and unpacked. A code
generator (dueca-codegen) is available to produce this code from
simple specification input. A code generator file may contain:

- Type definitions, which declare certain C++ types to the code
  generator as being "available" for use in the objects that the code
  generator makes. For example:

  ~~~~{.scm}
  (Type double)
  ~~~~

  The code generator assumes that methods to pack and unpack objects of
  type double (a basic C++ type) are available, and will after this
  declaration accept member variables of that type for the new objects.

  The types that you declare may be more complex than basic C++ types,
  in that case the code generator can generate a statement to read an
  include file where the type is defined, for example:

  ~~~~{.scm}
  (Type GlobalId "#include <GlobalId.hxx>")
  ~~~~

  With this you can send objects of the GlobalId class of DUECA
  around. The GlobalId objects are sent around by DUECA as well, and
  therefore the methods to pack their data, unpack their data and create
  them from amorphous storage are defined. Because a GlobalId is itself
  also a type that can be sent around, the code generator knows that the
  object can be recursively inspected if need be.

  Note that the string argument given after defining GlobalId is simply
  emitted as code before the class for the communicatable object is
  created. In this case it defines the inclusion of a header, but in
  principle you can write any set of valid c++ or preprocessor
  statements here. You can also have a multi-line string, and if you
  need string quotes in that string (""), you enter these by escaping
  them with a backslash (\\).

- Type definitions for enumerated types, for example:

  ~~~~{.scm}
  (Enum MyEnum uint8_t One Two Three Four Five Six)
  ~~~~

  The first item after MyEnum is the integer type with which it will be
  represented when packed for the net. Here an 8-bit unsigned integer is
  sufficient (this has room for 256 elements). After that, the different
  values for MyEnum are listed. This Enum definition will be given
  *inside* the transportable class.

  The Enum may also be defined outside the transportable class, and you
  can again include a string -- instead of the definition of the enum
  members. In that case, you need to ensure that you have methods to
  print the Enum type to an `std::ostream`, and to read it from an
  `std::istream`.

  You can now (from DUECA 3.1.3) also give the numeric presentation for
  your enum types:

  ~~~~{.scm}
  ;; And we can give a comment that will be ported to C++
  (Enum MyEnum uint8_t
    ;; the very first element
    One = 1
    ;; the second (and skipping the other comments)
    Two = 3 Three = 5 Four = 8 Five = 9 Six = 11)
  ~~~~

  This might be useful in case you need to communicate with hardware,
  and want to use symbolic names that map to the correct integer
  labels. And yes, the comments are parsed, and they carry over in the
  c++ generated code.

  A further possiblity is using the new c++-11 "class type" enums

  ~~~~{.scm}
  (EnumClass MyClassEnum uint16_t
    One Two Three)
  ~~~~

- Defines that you can later use as the argument for array
  size. As an example:

  ~~~~{.scm}
  (ArraySize NOUTPUTS "#include \"some-header.h\"")
  ~~~~

  The variable NOUTPUTS can now be used instead of an integer size for
  the arrays. Note that NOUTPUTS (which may be a preprocessor define or
  an integer constant) must evaluate to an integer. A small C++ program
  will be generated and then run by the code generator to determine the
  size of NOUTPUTS at the time of code generation.

- The "Type" declaration can be used to include
  more complex types:

  1. It is possible to use standard standard stl or stl-like
     containers, for example with:
    
     ~~~~{.scm}
     (Type std::list<int32_t> "#include <list>")
     (Type std::vector<int32_t> "#include <vector>")

     (MyObject
     (std::list<int32_t> a)
      ; you can also initialize the vector with a size and default
      ; value; easy!
      (std::vector<int32_t> b (Default 0) (DefaultSize 5)))
     ~~~~

     The lists or vectors will be correctly packed and unpacked. Their
     size is coded in a 4 byte unsigned number.

     Note that in old code (code generator version 110), this needed
     to be specified explicitly as an iterable type:

     ~~~~{.scm}	  
     (IterableType std::list<int32_t> "#include <list>")
     ~~~~
      
     Or, if you wanted to specify the length of the object beforehand:
      
     ~~~~{.scm}
     (IterableVarSizeType dueca::varvector<float>
       "#include <dueca/varvector.hxx>")
     ~~~~
      
     This code is still accepted, but it is no longer necessary to
     mark types as iterable.

  2. DUECA also provides three simplified vector types that in some
     cases may be more appropriate. The `varvector` type defines a type
     that is efficiently given a length once, and can then be
     used. `varvector` is less efficient than the `std::vector` when the
     length of the vector changes over its lifetime, but otherwise it
     has less overhead than `std::vector`.

     Another option is `limvector`, which has variable size, but a limit
     for this size. This is efficient if you want to fill a small
     vector which you know has a limited size. It is not (memory)
     efficient if you often fill only a small portion of a potentially
     large vector. The `limvector`'s data will be placed within the
     communication object that you create, so use of the `limvector`
     saves one allocation/de-allocation pair, generally making the
     real-time performance of your code better.

     The final option is the `fixvector`, that takes its size as a
     template parameter and sticks to this fixed size. This one has
     the least overhead of all, but the disadvantage is that its size
     is not variable, some examples:

     @code{.scm}
     (Type dueca::varvector<std::string>
      "#include <dueca/varvector.hxx>
       #include <string>")

     (Type dueca::limvector<5,std::string>
      "#include <dueca/limvector.hxx>
       #include <string>")
     @endcode
     
     Notice that limvector needs a specification of its size
     limit. This also shows an "include definition" with multiple
     lines.
     
     It is possible (if the type permits it), to give a default value
     and/or a default size:
     
     @code{.scm}
     (MyObject
       ;; first value, then size
       (varvector<std::string> b (Default "somestring") (DefaultSize 3))
     )
     @endcode

     Here is an example of an stl-like type
     that has a fixed size.

     @code{.scm}
     (Type dueca::fixvector<6,int> "#include <dueca/fixvector.hxx>")

     ;; with in the object definition
     (Object MyObject
     ;; it is possible to initialise with a default value
     (dueca::fixvector<6,int> a (Default 1.0)))
     @endcode

     The objects in the (vector, fixvector, list, etc.) containers
     may themselves also be transmittable objects, so generated by the code
     generator. For example:

     @code{.scm}
     (Type dueca::fixvector<6,dueca::GlobalId>
      "#include <dueca/fixvector.hxx>
       #include <dueca/GlobalId.hxx>")
     @endcode

- Definitions of classes to be generated, for example:

  ~~~~{.scm}
  (Object MyClass (MyEnum num)
                  (double f)
                  (dueca::fixvector<10,double> r))
  ~~~~

  This will be written as a pair of files, MyClass.cxx and
  MyClass.hxx. MyClass will have a member num, of type MyEnum and a
  member f, of type double. It will also have a fixvector r, with
  size 10. The fixvector elements may be accessed much as an std::vector is
  used. Extending or shortening the vector is however not possible.

  Old code often used the following format:

  ~~~~{.scm}
  (Object MyClass (MyEnum num)
                  (double f)
                  (double r 10))
  ~~~~

  With the old code generator, that produced a c-style array `r[10]` as
  member, and a const member `r_fixed_size`, which gave the size of the
  array `r`. The new code generator accepts this format, emits a warning,
  and replaces the array by a fixvector. If you have old code that read
  the size of the vector (`r_fixed_size`), that code will now fail. As a
  shortcut, you may test for the version of the code generator, and
  create a dirty preprocessor define, to use the more standard `.size()`
  member function of the fixvector:

  ~~~~{.scm}
  #if GENCODEGEN >= 110
  #define r_fixed_size r.size()
  #endif
  ~~~~

  For the old code generator an index -1 meant a variable sized
  array. This is replaced by a varvector, and the corresponding fix for
  your old code is:

  ~~~~
  #if GENCODEGEN >= 110
  #define r_variable_size r.size()
  #endif
  ~~~~

  Note that variable sized arrays should be avoided for high-rate
  data, since they need heap memory allocation and de-allocation, which
  leads to reduced real-time performance.

There is also an option to expand your generated class with methods
that you have programmed in C++. I must stress here that these must be
methods (functions) only, not data, since data you add yourself will not
be packed by the code generator. For example, you might want to send
over quaternions (as a four-element dueca::fixvector), and add the
operations for the quaternions to the class. You must supply a .hxx
file, which is going to be included in the class definition, and a
.cxx file, which is added to the body. Example:

~~~~{.scm}
(Object MyClass (IncludeFile MyClassExtra)
                (MyEnum num (Default One))
                (double f (Default 2.0f))
                )
~~~~

The IncludeFile keyword indicates that files need to be added.  You
can add these files, MyClassExtra.cxx and MyClassExtra.hxx to the
comm-objects directory. The alternative would be to implement your own
transportable classes, which is a very error-prone habit, and breaks
when there is a change to the code generator. Hopefully the default
values and the additional methods provide enough flexibility to avoid
this habit.

Note that the MyClassExtra.hxx gets included in the *body* of
the generated class (in the MyClass.hxx file), and it gets included
*last*.  MyClassExtra.cxx gets included *before* other
parts in the MyClass.cxx file. You may also use MyClassExtra.cxx to
override a number of functions that are produced by default. A number
of preprocessor defines are available to "switch off" these default
implementations. You can use:

~~~~{.cpp}
// override the print function
#define __CUSTOM_FUNCTION_PRINT
// override the assignment operator
#define __CUSTOM_OPERATOR_ASSIGN
// override the equality test operator
#define __CUSTOM_OPERATOR_EQUAL

// if you defined an enum, e.g. MyEnum, you can also add:
#define __CUSTOM_GETSTRING_MyEnum
// or
#define __CUSTOM_READFROMSTRING_MyEnum
~~~~

If you override a function with a custom function, your code may of
course break if the DUECA code generator is updated to generate a newer
version of its code. To guard against that, you must also add a define
that acknowledges the code generation version you are writing custom
code for. So somewhere in your MyClassExtra.cxx, define:
~~~~{.scm}
#define __CUSTOM_COMPATLEVEL_111
~~~~

If the code generation version changes in the future, you should check
whether your custom code is still compatible, adapt if needed, and
define a new acknowledgement. You will get a compiler warning when it
is time to do so.

Another possibility is the addition of some c++ code to the default
constructor (the one without arguments) or the full constructor (the
one with arguments). This gives you the possiblity to initialize the
arrays that cannot be initialized by default value arguments.

~~~~{.scm}
(Object MyClass
        (IncludeFile MyClassExtra)
        (ConstructorCode
         "std::cout << \"making a MyClass object\" << std::endl;" )
                (MyEnum num (Default One))
                (double f (Default 2.0f))
                (dueca::fixvector<10,double> r))
~~~~
A third way to extend funcionality of the generated class is to have it
inherit from a parent class. Example:

~~~~{.scm}
(Object MyClass (Inherits MyParentClass)
                (MyEnum num (Default One))
                (double f (Default 2.0f))
                (double r 10))
~~~~

The Inherits keyword here indicates that MyClass should derive from
MyParentClass. You need to define the MyParentClass as a type, and
provide the code to include the appropriate file.

Take care that the parent that you inherit from is also a full-fledged
packable and unpackable object. It works best if you also created the
parent with the code generator.

Sometimes it is useful to be able to send an object over a channel
*and* be able to specify it as a complete object in the Scheme or
Python script that you use to define the simulation. In that case
you can use a ScriptCreatable option:

~~~~{.scm}
(Object MyClass (Option ScriptCreatable)
                (double f (Default 2.0f))
                (double r 10))
~~~~

The code generator adds a templated class,

~~~~{.scm}
ScriptCreatableDataHolder<MyClass>
~~~~

to the generated code. That class is creatable from your scheme
script, given that the data members are simple enough to be specified
in the script language (generally, simple floats, integers, strings
and vectors of these). You can create it with the "lowercased" name of
your datatype, and "set-" commands to set the data. Note that you are
limited to the datatypes that you can set in the scheme script. So in
the python startup script you will be able to say things like:

~~~~{.py}
x = dueca.MyClass().param(
  f = 5.0, r = np.arange(10)
) 

# .... and later ...

  dueca.Module('my-module', '', sim_priority).param(
    set_my_data = x
  )
~~~~

Assuming you linked "set-my-data" to a function for your class in the
parameter table, and you added this function to your class:

~~~~{.cpp}
bool MyModule::setMyData(ScriptCreatable& d, bool in)
{
  if (in == true) {
    ScriptCreatableDataHolder<MyClass> *ptrx =
      dynamic_cast<ScriptCreatableDataHolder<ScriptCreatableTest>*>(&d);

    // check the pointer, if the cast failed, then the user supplied
    // the wrong type in the script
    if (!ptrx) {
      W_MOD("You supplied the wrong type in the script");
      return false;
    }

    // now you can copy the data
    localdata = ptrx->data();
  }
  return true;
}
~~~~

When you use Python scripting, the case of the class name will be kept
for the Python version.

If you use a combination of these above options, use the following
order: Inherits, Option, IncludeFile, ConstructorCode,
FullArgsConstructorCode, AddToHeader. You can not combine Inherits
with the ScriptCreatable option.

There used to be an option of generating different kinds of code:

~~~~{.scm}
(Stream MyClassA (MyEnum num))
(EventAndStream MyClassB (double f))
(Event MyClassC (double f))
~~~~

The current code generator generates the same code for all these
types, and they can be used for all types of channel. It is customary
to use the keyword `Object` now instead.

## External use of the generated code {#codegen_external}

You might want to communicate with DUECA from another program, and use
for example the `UDPWriter` and `UDPReader` modules to transmit this
data. In that case it would be handy to use the generated code objects
also in a non-dueca program. To do this, there are two preprocessor
defines that select how the connection to DUECA is compiled for a DCO
object:

~~~~{.cpp}
#define __DCO_STANDALONE
#define __DCO_NOPACK
~~~~

The first one generates standalone code, which does not need to be
connected to a DUECA executable. This code does have packing and
unpacking routines for the generated class, and you will need to link
the AmorphStore and AmorphReStore (a set of classes in DUECA for
packing and unpacking) code with your program. This enables you to
pack data into net representation and send it over, or unpack.

If you also do not need packing and unpacking, also define
`__DCO_NOPACK`, and the code compiles to a fully standalone class.

##  Comments in the generated code {#codegen_comment}

You can add comments to the .dco file by starting the comment with a
';'. The semicolon and anything it, until the end of the line is
considered a comment. The code generator tries to be smart about these
comments, and transport them to the generated code. The following
comments are preserved:

- Comment lines in front of the definition of an enumerated
  type. As the enumerated type becomes element of the class definition,
  the corresponding comment is put in front of the "enum" definition.
- Comment lines in front of an element of an enumerated type. These
  are copied into the c++ file as belonging to that element.
- Comment lines in front of the definition of a new class (Object). 
  These are copied into the c++ file in front of that class definition.
- Comment lines in front of the definition of elements in a
  class. These are put in front of the corresponding elements in the
  class definition.

All comments in the header file are compatible with doxygen
documentation generation. So a "make doc" command in the directory
with communication objects produces pretty html pages, with the
comments you originally put in the .dco files, or, if you did not put
comments in, generally silly comments about your class and elements in
it.

Comments in front of type definitions or array size definitions are
not copied into the c++ file.

##  Comment header {#codegen_header}

With a Header statement, you can add a header to the generated
code. The header is a multi-line string, which is copied verbatim into
the standard header for the generated code, here is a small example.

~~~~{.scm}
(Header "
        original item   : CycleCounter.dco
        made by         : Rene' van Paassen
        date            : 200612
        description     : Repeating, possibly overflowing counter
                          for message cycles
        copyright       : (c) 2020 TUDelft-AE-C&S - Rene van Paassen")
~~~~

##  Enumerated types only {#codegen_enumerated}

Sometimes you might want to share your enumerated types across
multiple code generated objects. By default, the code generator will
add the enumerated type as a type for the class you generate. But you
can also let the code generator produce stand-alone code for the
enumerated type, by defining an enumerator. Given a MyEnum type you
defined previously, you can then write:

~~~~{.scm}
(Enumerator MyEnum)
~~~~

This will produce a `MyEnum.hxx` and `MyEnum.cxx` file that has the code
to pack and unpack the enum. This can then be included as an external
enum when you want to generate a DCO object.

#  Additional options {#codegen_option}

The code generator is extensible with several options, which add or
modify the code generation when used, to be selected with the Option
keyword. Currently two different additions are available:

- hdf5; this adds code for storing and unpacking DCO objects from hdf5
  datafiles. The hdf5 format is a bit tricky; it is based on old Fortran
  code and not everything is possible. The following variants therefore
  exist:

  o hdf5: The DCO object can be packed and unpacked from hdf5.

  o hdf5nest: The object can be packed and unpacked, and also be used as
              an element within other DCO objects. For this to work, you
	      cannot use variable-sized containers within the contained
	      objects; so no stl::map, stl::vector, dueca::varvector,
	      dueca::limvector etc.

  o hdf5enum: A special variant for creating an unconnected enumerated
    value that can be stored and retrieved, only needed in combination with
    Enumerator (from DUECA 3.1.4 onwards)

- msgpack; this adds code to storing and retrieving from the
  MessagePack format, see https://msgpack.org/index.html .
  For Enumerator types, use option msgpackenum

##  Generating the code for your dco file {#codegen_codegen}

To help you create nicely formatted dco files, there is a little
script, new-dco, that creates a dco file for you.
You can use it to create a simple template to edit, or
let it enter a lot of the code for you already. A little example:

~~~~{.sh}
new-dco object --name MyData --description "Test object" --type float \
	--member float a --default 0.0f
~~~~

##  Example {#codegen_example}

Finally, here is an [example](@ref primarycontrols_code) of the
generated code. 

