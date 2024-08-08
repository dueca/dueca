#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Jun 21 21:05:36 2021

@author: repa
"""
from lxml import etree

def XML_interpret_bool(val):
    """
    Interpret the argument as a boolean

    Parameters
    ----------
    val : str
        input value, "true", "yes", "1" will be interpreted as true,
        "false", "no", "0" as false, anything else None.

    Returns
    -------
    bool
        Result.

    """
    if isinstance(val, str):
        if val.lower() in ("true", "yes"):
            return True
        if val.lower() in ("false", "no"):
            return False
    try:
        return bool(int(val))
    except ValueError:
        pass
    return None


def XML_tag(elt, tag):
    """
    Check match with xml tag

    Parameters
    ----------
    elt : etree.Element
        Element to check.
    tag : str
        Tag name.

    Returns
    -------
    bool
        True if the tag (excluding namespace) matches.

    """
    return isinstance(elt.tag, str) and elt.tag.split('}')[-1] == tag


def XML_comment(elt):
    """
    Check whether element is of comment type

    Parameters
    ----------
    elt : etree.Element
        Element to check

    Returns
    -------
    bool
        True if this is a comment element.

    """
    return isinstance(elt, etree._Comment)

_xmlns='{https://dueca.tudelft.nl}'

class XML_TagUnknown(ValueError):
    def __init__(self, elt):
        if isinstance(elt.tag, str):
            self.tag = elt.tag.split('}')[-1]
        else:
            self.tag = "NO TAG!"

    def __str__(self):
        return f"Unhandled tag <{self.tag}>"
