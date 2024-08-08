#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Sep 23 15:02:32 2021

@author: repa
"""

def _ccprefix(s):
    if s:
        return f'{s}::'
    return s

class MemberSummary:
    def __init__(self, name: str, klass: str, iterable: bool,
                 mclass: str, mspace: str=''):
        """
        Essential information about a class data member

        Parameters
        ----------
        name : str
            Name of the member.
        klass : str
            Type of the member. Assuming this is defined elsewhere.
        iterable : bool
            Iterable member
        mclass : str
            Name of the enclosing object.
        mspace : str, optional
            Namespace for the enclosing object. The default is ''.

        Returns
        -------
        None.

        """
        self.name = name
        self.klass = klass
        self.mclass = mclass
        self.mspace = mspace

    def getType(self, bare: bool=False, in_class: bool=False,
                in_namespace: bool=False, **args):
        """
        Return the klass/type of the object.

        Parameters
        ----------
        bare : bool, optional
            Returns the type without possible prefixes.
            The default is False.
        **args : TYPE
            Filler for compatibility.

        Returns
        -------
        str.

        """
        if bare:
            return self.klass.split('::')[-1]
        return self.getPrefix(bare=bare, **args) + self.klass

    def getName(self, with_this: bool=False, **args):
        if with_this:
            return f"this->{self.name}"
        return self.name

    def getMembers(self):
        return tuple()

    def getPrefix(self, **args):
        return ''

    def isEnum(self):
        return False


class EnumMemberSummary(MemberSummary):
    def __init__(self, name: str, klass: str, iterable: bool,
                 mclass: str, ctype: str, mspace: str='', members: list=None):
        """
        Essential information about an enum class data member

        Parameters
        ----------
        name : str
            Name of the member.
        klass : str
            Type of the member. Assuming this is defined elsewhere.
        mclass : str
            Class of the enclosing object.
        ctype : str
            C-type equivalent for packing the data (uint8_t or the like)
        mspace : str, optional
            Namespace for the enclosing object. The default is ''.
        members : list, optional
            List of enumeration members. Non-empty if the
            enum was defined in the class. The default is None.

        Returns
        -------
        None.

        """
        super().__init__(name, klass, iterable, mclass, mspace)
        if members:
            self.members = tuple(members)
        else:
            self.members = tuple()
        self.ctype = ctype
        #print("new enum member",
        #      self.getType(), self.getCType(), self.getMembers())

    def getPrefix(self, bare: bool=False, in_class: bool=False,
                  in_namespace: bool=False):

        if bare:
            return ''
        if in_class:
            return ''
        if in_namespace and len(self.members):
            return __ccprefix(self.mclass)
        elif len(self.members):
            return f'{_ccprefix(self.mspace)}{_ccprefix(self.mclass)}'
        return ''

    def getMemberPrefix(self, bare: bool=False, in_class: bool=False,
                  in_namespace: bool=False):
        return self.getPrefix(bare=bare, in_class=in_class,
                              in_namespace=in_namespace)

    def getMembers(self, bare: bool=False, **args):
        if bare:
            return [ m.split('::')[-1] for m in self.members ]
        return [ self.getMemberPrefix(**args) + m for m in self.members ]

    def getCType(self):
        return self.ctype

    def isEnum(self):
        return True


class EnumClassMemberSummary(EnumMemberSummary):

    def __init__(self, **args):
        super().__init__(**args)

    def getMemberPrefix(self, bare: bool=False, in_class: bool=False,
                        in_namespace: bool=False):
        if bare:
            return ''
        if in_class:
            return f'{self.klass}::'
        if in_namespace and len(self.members):
            return f'{_ccprefix(self.mclass)}{self.klass}::'
        elif len(self.members):
            return f'{_ccprefix(self.mspace)}{_ccprefix(self.mclass)}{self.klass}::'
        return ''

def summarise_member(m, mclass, namespace):

    if m.klassref.getType() == 'Enum':
        if m.klassref.classenum:
            return EnumClassMemberSummary(
                name=m.name, klass=m.klassref.name,
                iterable=m.klassref.isIterable(),
                ctype=m.klassref.getCType(), mclass=mclass,
                mspace=namespace, members=m.klassref.getEnums())
        else:
            return EnumMemberSummary(
                name=m.name, klass=m.klassref.name,
                iterable=m.klassref.isIterable(),
                ctype=m.klassref.getCType(),
                mclass=mclass, mspace=namespace,
                members=m.klassref.getEnums())
    return MemberSummary(name=m.name, klass=m.klassref.name,
                         iterable=m.klassref.isIterable(),
                         mclass=mclass, mspace=namespace)
