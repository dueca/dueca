#!@Python_EXECUTABLE@

import os
import pyparsing as pp

# When the following conditions are present
#
# - Running from node 0 on a platform
# - There is one start file in the platform folder
# - There is a ${HOME}/scripts folder
#
# Then a symbolic link is created from the ${HOME}/scripts to the start file

# default
node_number = None

def nodeNumber(upto, line, c):
    global node_number
    try:
        node_number = int(c[0])
    except:
        print(f"Cannot read node number from '{c[0]}', line {line}")

def parsepy():

    # pyparsing string to check
    nodecheck = pp.Literal('this_node_id') + pp.Literal('=') + \
        pp.Word(pp.nums).addParseAction(nodeNumber)

    # run through all lines
    with open('dueca_cnf.py', 'r') as fn:
        for l in fn:
            nodecheck.search_string(l)

def parseguile():

    # pyparsing string to check
    nodecheck = pp.Literal('(this-node-id') + \
        pp.Word(pp.nums).addParseAction(nodeNumber) + pp.Literal(')')

    # run through all lines
    with open('dueca.cnf', 'r') as fn:
        for l in fn:
            nodecheck.search_string(l)

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
            if pattern.search_string(l):
                return True
    return False

#print(node_number)

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
                os.path.islink(f'{scriptdir}/{f}'):
                print(f"Will not link start script {f}\n"
                      f"There is a file with that name in {scriptdir}")
            elif os.path.islink(f'{scriptdir}/{f}') and \
                 os.path.realpath(f'{scriptdir}/{f}') != f'{platformdir}/{f}':
                print(f"Will not link start script {f}\n"
                      f"There is already a different link in {scriptdir}")
            elif os.path.islink(f'{scriptdir}/{f}'):
                # print(f"Already linked {scriptdir}/{f}")
            else:
                try:
                    os.symlink(f'{platformdir}/{f}', f'{scriptdir}/{f}')
                    print(f"Created a link to start file {f}")
                except:
                    print(f"Failed to create symlink to {f} in folder {scriptdir}")
        



