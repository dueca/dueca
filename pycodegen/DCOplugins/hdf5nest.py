# AddOn for hdf5 option
"""     item            : hdf5nest.py
        made by         : RvP
        date            : 2017
        category        : python program
        description     : Code generation of DUECA Communication Objects (DCO)
                          hdf5 extension
        language        : python
        changes         : 1704xx RvP Added a plugin system, to enable
                                     extension of code generation
        copyright       : TUDelft-AE-C&S

AddOn objects extend the code generation by the dueca-codegen program

Create a file named after the Option you want to add to the code
generation, and install it in the DCOplugins directory. This file adds
(Option hdf5nest) to DCO files, adding ability to read DCO objects
from a DUECA channel and to store this in an HDF5 format file. Objects
with this option must be directly packable in HDF5 type, this excludes
objects with std::string members, and stl containers. DUECA's containers
(fixvector, limvector, varvector) can be used.

"""
from DCOplugins.hdf5 import AddOn as AddOnNest

class AddOn(AddOnNest):
    """Print HDF5 interaction code for a DCO object.

     This version also emits the getHdfDataType method for the DCO
     object, where by the object itself can be nested again. However,
     this is possible for a more limited class of DCO objects than the
     regular (not enabling nesting) hdf packing

    - include section of header
    - in class definition (at end, but just before extra include)
    - in header, outside class
    - include in body file
    - in body

    """
    def __init__(self, namespace, name, parent, members):
        """ Initialisation of an AddOn object

        namespace -  name space of the DCO object
        name      -  class name of DCO object
        parent    -  parent class name
        members   -  list with dicts describing data members,
                     dict(name=, klass=, mtype=, ctype=, enums=)
                     here klass is the C class, mtype is Type,
                     IterableType or Enum, ctype is c type for enums,
                     enums is the list of enum members
        """
        super(AddOn,self).__init__(namespace, name, parent, members,
                                   nest=True)
