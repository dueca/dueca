#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Extract a list of debug and error messages from DUECA code.

Created on Mon Jul 20 17:24:01 2020

@author: repa
"""

import os
import sys
from pyparsing import Literal, OneOrMore, printables, Combine, \
    ParseException, Word, Regex, ParseResults, c_style_comment, \
        ZeroOrMore, MatchFirst, LineEnd, LineStart
import re
from argparse import ArgumentParser
import xlwt
import html

_debug = True
def dprint(*args, **kwargs):
    if _debug:
        print(*args, **kwargs)

class XLFile:
    """Excel file"""

    def __init__(self, name):
        """
        Create an excel file writing object

        Parameters
        ----------
        name : str
            Name for the file

        Returns
        -------
        None.

        """
        self.fname = name
        self.book = xlwt.Workbook(encoding='utf-8')
        self.sheet = self.book.add_sheet("Messages")
        self.row = 1
        self.rheight = self.sheet.row(0).height
        cwidth = self.sheet.col(0).width
        for col, hdr in enumerate(('File', 'Line', 'Message Type',
                                   'Chapter', 'Description')):
            self.sheet.write(0, col, hdr)
        self.sheet.col(0).width = 3*cwidth
        self.sheet.col(3).width = 2*cwidth
        self.sheet.col(4).width = 5*cwidth

    def extend(self, msglist):
        for m in msglist.messages:
            for i, txt in enumerate(m):
                self.sheet.write(self.row, i, txt)
            if txt.count('\n') > 0:
                self.sheet.row(self.row).height = \
                    (txt.count('\n') + 1)*self.rheight
            self.row += 1

    def close(self):
        self.book.save(self.fname)


class DoxyFile:
    """Doxygen file"""

    def __init__(self, name):
        """
        Create an doxygen file writing object

        Parameters
        ----------
        name : str
            Name for the file

        Returns
        -------
        None.

        """
        self.fp = open(name, 'w')
        self.fp.write("""
// -*-c++-*-
/**
\page loglist List of DUECA's log messages

This page provides a list of log messages, extracted from DUECA's source
code. Comments accompanying the messages can help identify why a message
was generated.

Error messages are printed to the console, listed in DUECA's error log
window (if you have that configured), and also assembled on node 0 and
printed in dueca.messagelog. The error log window and dueca.messagelog
also list the file name and line number for the error messages, you
can use these to find the error message here (verify that you use the
same DUECA version!), and look up additional information on the error.

Note that the table is (should be) sortable; click on a label to sort
the table alphabetically according to the requested column.

@htmlonly
<script src="sorttable.js"></script>
<table class="doxtable sortable">
  <tr>
    <th>File</th>
    <th>Line</th>
    <th>Message Type</th>
    <th>Section</th>
    <th>Description</th>
  </tr>
""")

    def extend(self, msglist):
        for m in msglist.messages:
            self.fp.write("  <tr>\n")
            for txt in m:
                self.fp.write(f"    <td>{html.escape(str(txt))}</td>\n")
            self.fp.write("  </tr>\n")

    def close(self):
        self.fp.write("""
</table>
@endhtmlonly
*/
""")
        self.fp.close()


class Folder:
    """File directory."""

    # ignored files and folders
    ignored = set(('__pycache__', '.git', 'newlog.hxx', 'scripts',
                   'test', 'doc', 'tests', 'ScramNetAccessor.cxx'))

    def __init__(self, name):
        """
        Check given name is a directory, and get full path.

        Parameters
        ----------
        name : str
            Directory name.

        Raises
        ------
        ValueError
            Given name does not represent a file directory.

        Returns
        -------
        None.

        """
        if os.path.isdir(name):
            self.path = os.path.abspath(name)
        else:
            raise ValueError(f"{name} is not a valid directory")

    def getFiles(self, suffix="xx"):
        """
        Return a list of matching file names.

        Parameters
        ----------
        suffix : str, optional
            File type to search for. The default is ".cxx".

        Returns
        -------
        list of file names : list[str].

        """
        files = []
        for f in os.listdir(self.path):
            ffull = self.path + os.sep + f
            if f[0] == '.' or f in Folder.ignored:
                pass
            elif os.path.isdir(ffull) and f[0] != '.':
                files.extend(Folder(ffull).getFiles(suffix))
            elif f.endswith(suffix):
                files.append(ffull)
        return files


logstart = Combine(
    (Literal('D_') |
     Literal('I_') |
     Literal('W_') |
     Literal('E_')) +
    (Literal('CNF') |
     Literal('SYS') |
     Literal('ACT') |
     Literal('CHN') |
     Literal('SHM') |
     Literal('TIM') |
     Literal('NET') |
     Literal('MOD') |
     Literal('STS') |
     Literal('TRM') |
     Literal('MEM') |
     Literal('INT') |
     Literal('XTR'))) + Literal('(')


class FileMessages:
    """Inventory of messages in a file."""

    def __init__(self, fname):
        """
        Open a file and parse it for error messages.

        Parameters
        ----------
        fname : str
            Source code file.

        Returns
        -------
        None.

        """
        comment = ''
        self.messages = []
        self.fname = fname[fname.find('/dueca')+1:]
        self._fname = fname

    def runparse(self):

        with open(self._fname, 'r') as f:
            #code = f.read()
            parse_file.parseFile(f, parse_all=True)
        #parse_file.parseString(code, True)

    def combination(self, s: str, loc: int, toks: ParseResults):
        global currentline
        print(s)
        if isinstance(toks[0], Comment) and isinstance(toks[1], LogMessage):
            self.messages.append(
                (self.fname, currentline, toks[1].logcode, toks[0].comment))
        return toks

    def _parse(self, *args):
        print(args)

    def __bool__(self):
        return bool(self.messages)

    def title(self):
        titles = set()
        for m in self.messages:
            titles.add(m[-1].split('\n')[0])
        if len(titles) > 1:
            print(f'file {fname} had multiple titles: {titles}')
        elif len(titles) < 1:
            print(f'file {fname} had no title')
            return "Untitled"
        return titles.pop()

    def __str__(self):
        return str(self.messages)

class Comment:
    def __init__(self, line, lines):
        self.comment = line
        self.lines = lines

class LogMessage:
    def __init__(self, logcode, lines):
        self.logcode = logcode
        self.lines = lines

currentline = 0
def make_comment(s: str, loc: int, toks: ParseResults):
    global currentline
    #currentline += toks[0].count('\n') + 1
    dprint("Comment", toks[0])
    return Comment(toks[0], toks[0].count('\n')+1)

def make_logmessage(s: str, loc: int, toks: ParseResults):
    global currentline
    #currentline += toks[1].count('\n') + 1
    logcode = toks[0]
    dprint("Logmesg", toks)
    return LogMessage(logcode, toks[2].count('\n') + 1)

def make_logmiss(s: str, loc: int, toks: ParseResults):
    global currentline
    print(f"Log message without comment in line {currentline+1}:\n{''.join(toks)}",
          file=sys.stderr)
    currentline += toks[1].count('\n') + 1
    return None

def read_otherline(s: str, loc: int, toks: ParseResults):
    global currentline
    currentline += 1
    dprint("AnyLine", currentline, toks)
    return None

def read_emptyline(s: str, loc: int, toks: ParseResults):
    global currentline
    currentline += 1
    dprint("EmptyLine", currentline, toks)
    return None

def read_combined(s: str, loc: int, toks: ParseResults):
    global currentline
    currentline += toks[0].lines + toks[1].lines
    dprint("Combination", toks)
    return "combination"

parse_comment = c_style_comment.copy() # Literal('/*') + Regex(r".*?\*\/", re.DOTALL) + Literal('*/')
parse_comment.add_parse_action(make_comment)
parse_lmessage = logstart + Regex(r".*?\)[ ]*;", re.DOTALL)
parse_lmiss = parse_lmessage.copy()
parse_lmessage.add_parse_action(make_logmessage)
parse_lmiss.set_parse_action(make_logmiss)
parse_combined = parse_comment + parse_lmessage
parse_combined.set_parse_action(read_combined)
parse_otherline = Regex(r".+") + LineEnd()
parse_otherline.add_parse_action(read_otherline)
parse_emptyline = LineStart() + LineEnd()
parse_emptyline.set_parse_action(read_emptyline)
parse_elt = MatchFirst(
    (parse_combined, parse_lmiss, parse_emptyline, parse_otherline))
parse_file = OneOrMore(parse_elt)

if __name__ == '__main__':
    parser = ArgumentParser(
        description="""Extract descriptions for debug and error messages.

        Parse cxx files files found in a submitted folder for DUECA
        style error and debug messages and their descriptions, listing
        these in one of several formats.""")
    parser.add_argument('--base', type=Folder,
                        help="""Folder with the source code.""")
    parser.add_argument('--output', type=XLFile,
                        help="""Output file.""")
    parser.add_argument('--outputdoc', type=DoxyFile,
                        help="""Doxygen output file.""")

    # default value helps in testing
    args = parser.parse_args(sys.argv[1:] or (
        '--base', '/home/repa/dueca/dueca',
        '--output', '/tmp/messagelist.xlsx',
        '--outputdoc', '/tmp/messagelist.doc'))

    if len(sys.argv) <= 1:

        fm = FileMessages('/home/repa/dueca/dueca/XMLtoDCO.cxx')
        fm.runparse()
        sys.exit(0)

    allmsg = []
    for fname in args.base.getFiles():
        currentline = 0
        fm = FileMessages(fname)
        parse_combined.set_parse_action(fm.combination)
        fm.runparse()

        if fm:
            args.output.extend(fm)
        if args.outputdoc:
            args.outputdoc.extend(fm)

    args.output.close()
    if args.outputdoc:
        args.outputdoc.close()
