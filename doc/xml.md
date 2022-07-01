# XML representation of DCO objects {#xmlrep}

XML is widely used as a data interchange format. This chapter describes how to encode DCO objects as XML data.

## Introduction

The XML specification useful for storing and transporting data. Data
structures can be converted to string or file format, and reliably
re-read. The data files are also human-readable (although not always
pleasant to look at). Dueca Communication Objects are simple C++ data
structure objects that can be packed and transported efficiently, but
the packing and unpacking is by default to a binary format, that is
less suitable for inspection and storage in text files.

Conversion to XML is useful for communication with external code that
has no direct means of unpacking or packing DCO data, and for
storing/retrieving DCO objects in and from files. Note that in this
implementation good real-time performance is not possible when
converting to and from XML, so avoid this for high-frequency data
communication when possible. Also don't store your experiment data in
XML format, it can generate really huge files. It is better to use the
hdf5 logger to store your data.

## Representation of DCO objects in XML format

A DCO object is started with the tag `<object>`. The DCO type is
indicated with an attribute `class`. Each object can only have
"members", for each of the variables in the DCO object, indicated with
`<member>` tags. When the member data are basic C++ variables, or
containers such as vectors, lists, a member has zero or more `<value>`
tags with the value of the objects.

When the member data are nested DCO objects, the contents of the
`<member>` are `<object>` tags with the enclosed objects.

A compact version of the DCO coding assumes that the decoding uses
knowledge about the structure of the DCO object. An extended coding
includes data on the class or type of the member data, with the
`class` attribute.

A small example:

    <object class="VNAVSetting">
      <member name="target_alt">
        <value>28000.0</value>
      </member>
      <member name="target_fmc_speed">
        <value>270.0</value>
      </member>
      <member name="target_vs_descent">
        <value>-995.0</value>
      </member>
      <member name="target_vs_climb">
        <value>1200.0</value>
      </member>
      <member name="target_throttle_n1">
        <value>95.0</value>
      </member>
      <member name="initial_on">
        <value>1</value>
      </member>
    </object>

With the extended encoding, this would become:

    <object class="VNAVSetting">
      <member name="target_alt" class="float">
        <value>28000.0</value>
      </member>
      <!-- ... etcetera -->
    </object>

## Easy encoding and decoding

Encoding to an XML format can be done with the function
dueca::DCOtoXML . This function accepts an XML writer from the
`pugixml` library, and the encoding can be started either from a
dueca::CommObjectReader (of which a dueca::DCOReader is a subclass),
or from a void pointer to a DCO object and information on the DCO
object class.

For example for reading a DCO object from a channel, without knowledge
on the DCO object

    // somewhere at the start of the file
    #include <string>
    #include <smartreader>
    #include <dueca.h>
    // ... etc.

    // snip...


    // create a generic reader
    DCOReader rdr(r_token, ts);

    // 1st step, an xml document
    pugi::xml_document doc;

    // conversion to xml
    DCOtoXML(doc, rdr);

    // need a pugixml writer, smartstring provides one
    std::string result;
    smartstring::xml_string_writer writer(result);

    // save the document to the string
    doc.save(writer);

    // check the result, or do something else with it
    std::cout << result << std::endl;

For writing it back into the channel, you first need a pugixml
document. It is also possible to have a larger document, and know that
one of the xml nodes is a DCO object, and then convert that one.

    // read a document from a string (or file, or whatever,
    // read pugi's documentation).
    pugi::xml_document doc;
    doc.load_string(this->c_str());

    {
      // create a generic writer on a token
      DCOWriter wrt(w_token, ts);

      // suppose we navigated to the node which has a dco object, and it is
      // called our_dco_node
      XMLtoDCO(our_dco_node, wrt);

      // all done, when this scope is closed, the destructor of 'wrt'
      // is called and our data is written into the channel
    }

As a side note, the string class dueca::smartstring is a derivative of
std::string that has some facilities added for conversion to and from
XML and JSON. It can also be used in a [DCO](@ref codegenerator) object.
