#!@Python_EXECUTABLE@

import os
import pyparsing as pp
import argparse
import sys

# When the following conditions are present
#
# - Running from node 0 on a platform
# - There is a start file in the platform folder
# - There is a ${HOME}/scripts folder
#
# Then a symbolic link is created from the ${HOME}/scripts to the start file(s)

# default 
node_number = None
number_of_nodes = None

def nodeNumber(upto, line, c):
    global node_number
    try:
        node_number = int(c[0])
    except:
        print(f"Cannot read node number from '{c[0]}', line {line}")

def numberOfNodes(upto, line, c):
    global number_of_nodes
    try:
        number_of_nodes = int(c[0])
    except:
        print(f"Cannot read node number from '{c[0]}', line {line}")


def parsepy():

    # pyparsing string to check
    nodecheck = pp.Literal('this_node_id') + pp.Literal('=') + \
        pp.Word(pp.nums).addParseAction(nodeNumber)
    numbernodescheck = pp.Literal('no_of_nodes') + pp.Literal('=') + \
        pp.Word(pp.nums).addParseAction(numberOfNodes)

    # run through all lines
    with open('dueca_cnf.py', 'r') as fn:
        for l in fn:
            nodecheck.searchString(l)

def parseguile():

    # pyparsing string to check
    nodecheck = pp.Literal('(this-node-id') + \
        pp.Word(pp.nums).addParseAction(nodeNumber) + pp.Literal(')')

    # run through all lines
    with open('dueca.cnf', 'r') as fn:
        for l in fn:
            nodecheck.searchString(l)

# Check node number
for f in os.listdir('.'):
    if f == 'dueca_cnf.py':
        parsepy()
        break
    elif f == 'dueca.cnf':
        parseguile()


def isstartfile(f):
    if not os.path.isfile(f'../{f}'):
        return False
    pattern = pp.Literal('source') + pp.Literal('`') + \
        pp.Literal('dueca-config') + pp.Literal('--path-datafiles') + \
        pp.Literal('`/data/GenericStart')
    with open(f'../{f}', 'r') as sf:
        for l in sf:
            #print( l)
            if pattern.searchString(l):
                return True
    return False

#print(node_number)

parser = argparse.ArgumentParser(
    description="Create symbolic links to DUECA start scripts")
parser.add_argument(
    '--force', action='store_true',
    help="Overwrite any existing links or files")
parser.add_argument(
    '--scriptdir', type=str, default=os.getenv('HOME', 'nohome')+'/scripts',
    help="Supply folder where scripts are created")

res = parser.parse_args(sys.argv[1:])
scriptdir = res.scriptdir
force = res.force

# see if there are start files
platformdir = os.path.realpath('..')
startfiles = []
scriptdir = os.getenv('HOME', 'nohome')+'/scripts'
if node_number == 0 and os.path.isdir(scriptdir):
    for f in os.listdir('..'):
        #print(f)
        if isstartfile(f):
            if (os.path.isfile(f'{scriptdir}/{f}') or \
                os.path.isdir(f'{scriptdir}/{f}')) and not \
                os.path.islink(f'{scriptdir}/{f}') and not force:
                print(f"Will not link start script {f}\n"
                      f"There is a file with that name in {scriptdir}")
            elif os.path.islink(f'{scriptdir}/{f}') and \
                 os.path.realpath(f'{scriptdir}/{f}') != f'{platformdir}/{f}' \
                 and not force:
                print(f"Will not link start script {f}\n"
                      f"There is already a different link in {scriptdir}")
            elif os.path.islink(f'{scriptdir}/{f}') and not force:
                # print(f"Already linked {scriptdir}/{f}")
                pass
            else:
                try:
                    os.symlink(f'{platformdir}/{f}', f'{scriptdir}/{f}')
                    print(f"Created a link to start file {f}")
                except:
                    print(f"Failed to create symlink to {f} in folder {scriptdir}")

