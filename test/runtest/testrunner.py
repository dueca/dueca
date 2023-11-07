#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Sep 27 20:32:23 2021

@author: repa
"""

from PIL import ImageGrab, ImageDraw
import pynput
from pynput.keyboard import Key
from wmctrl import Window
import subprocess
import os
import asyncio
import time
import argparse
import pathlib
import sys
from lxml import etree
from duecautils.xmlutil import XML_comment, XML_tag, XML_interpret_bool

base = '/tmp/tmp.runner'
x11display = os.environ.get("DISPLAY")

the_mouse = pynput.mouse.Controller()
the_keyboard = pynput.keyboard.Controller()

class Translation:
    def __init__(self, offset_x=0, offset_y=0, extra_y=0):
        self.offset_x = offset_x
        self.offset_y = offset_y
        self.extra_y = extra_y

    def inWindow(self, x, y, w):
        if x < w.x - self.offset_x: return False
        if x > w.x + w.w - self.offset_x: return False
        if y < w.y - self.offset_y - self.extra_y: return False
        if y > w.y + w.h - self.offset_y: return False
        #print(f"In window {w.wm_name} at {w.x},{w.y} size {w.w}x{w.h}")
        return True

    def toWindow(self, x, y, w):
        #print(f"To window {w.wm_name} at {w.x},{w.y} size {w.w}x{w.h}")
        return x - w.x + self.offset_x, y - w.y + self.offset_y

    def toScreen(self, x, y, w):
        #print(f"From window {w.wm_name} at {w.x},{w.y} size {w.w}x{w.h}")
        return w.x - self.offset_x + x, w.y - self.offset_y + y

translation = None

known_windows = dict()

def findWindow(name: str):
    for w in Window.list():
        if w.wm_name not in known_windows:
            known_windows[w.wm_name] = (w.x, w.y)
            print(f"{w.wm_name} {w.wm_state} at {w.x},{w.y} size {w.w},{w.h}")
        elif known_windows[w.wm_name] != (w.x, w.y):
            known_windows[w.wm_name] = (w.x, w.y)
            print(f"{w.wm_name} {w.wm_state} to {w.x},{w.y} size {w.w},{w.h}")
        if w.wm_name == name:
            return w
    return None


def findWindowUnder(wlist, x: int, y: int, recording=False):
    global translation
    foundwin = None
    for w in Window.list():
        if w.wm_name not in known_windows:
            known_windows[w.wm_name] = (w.x, w.y)
            if recording:
                wlist.append(w.wm_name)
            print(f"{w.wm_name} {w.wm_state} at {w.x},{w.y} size {w.w},{w.h}")
        elif known_windows[w.wm_name] != (w.x, w.y):
            known_windows[w.wm_name] = (w.x, w.y)
            print(f"{w.wm_name} {w.wm_state} to {w.x},{w.y} size {w.w},{w.h}")
        if w.wm_name in wlist and 'focused' in w.wm_state and \
            translation.inWindow(x, y, w):
            #print(f"focus window {w.wm_name} at {w.x},{w.y} size {w.w}x{w.h}")
            foundwin = w
    if foundwin is not None:
        return w

    for w in Window.list():
        if translation.inWindow(x, y, w):
            #print(f"found window {w.wm_name} at {w.x},{w.y} size {w.w}x{w.h}")
            foundwin = w
    return foundwin

class Project:
    def __init__(self, xmlnode):

        self.name = xmlnode.get('name', None)
        self.windows = []
        for elt in xmlnode:
            self.windows.append(elt.text.strip())
        self.xmlnode = xmlnode

    def save(self):
        tosave = set(self.windows)
        for elt in self.xmlnode:
            try:
                tosave.remove(elt.text.strip())
            except KeyError:
                pass
        for n in tosave:
            etree.SubElement(self.xmlnode, 'window').text = n

class Execute:
    def __init__(self, platform=None, node=None, xmlnode=None, xmlroot=None):

        if xmlroot is not None and platform and node:
            self.xmlnode = etree.SubElement(xmlroot, 'execute')
            etree.SubElement(self.xmlnode, 'platform').text = platform
            self.platform = platform
            etree.SubElement(self.xmlnode, 'node').text = node
            self.node = node

        elif xmlnode is not None:
            self.platform, self.node = None, None
            for elt in xmlnode:
                if XML_tag(elt, 'platform'):
                    self.platform = elt.text.strip()
                elif XML_tag(elt, 'node'):
                    self.node = elt.text.strip()
                elif XML_comment(elt):
                    pass
                else:
                    print(f"Cannot interpret {elt.tag}")
            if self.platform is None or self.node is None:
                raise ValueError("xml file incomplete")


class Click:
    buttonmap = {'Button.left': pynput.mouse.Button.left,
                 'Button.right': pynput.mouse.Button.right,
                 'Button.middle': pynput.mouse.Button.middle}

    def __init__(self, xmlroot=None, xmlnode=None, x=0, y=0, window=None,
                 button=None, pressed=False, wait=0.2):
        if xmlroot is not None:
            xmlnode = etree.SubElement(xmlroot, 'click')
            if window is not None:
                x, y = translation.toWindow(x, y, window)
                xmlnode.set('window', window.wm_name)
            xmlnode.set('x', str(x))
            xmlnode.set('y', str(y))
            xmlnode.set('button', str(button))
            xmlnode.set('pressed', str(pressed).lower())
            xmlnode.set('wait', str(wait))
        elif xmlnode is not None:
            self.window = xmlnode.get('window', '')
            self.x = int(xmlnode.get('x', '0'))
            self.y = int(xmlnode.get('y', '0'))
            self.button = Click.buttonmap[xmlnode.get('button')]
            self.wait = float(xmlnode.get('wait', 0.1))
            self.pressed = XML_interpret_bool(xmlnode.get('pressed', 'false'))

    async def execute(self):
        global the_mouse
        global translation

        print(f"click {self.window} {self.x},{self.y} {self.button}, {self.pressed}")

        if self.window:
            w = findWindow(self.window)
            if w is None:
                raise ValueError(f"Cannot find window {self.window}")
            x, y = translation.toScreen(self.x, self.y, w)
        else:
            x, y = self.x, self.y

        the_mouse.position = x, y
        if self.wait > 0.0:
            await asyncio.sleep(self.wait)

        if self.pressed:
            the_mouse.press(self.button)
        else:
            the_mouse.release(self.button)

class KeyPress:

    def __init__(self, xmlroot=None, xmlnode=None, x=0, y=0, window=None,
                 key=None, wait=0.1):
        if xmlroot is not None:
            xmlnode = etree.SubElement(xmlroot, 'key')
            if window is not None:
                x, y = translation.toWindow(x, y, window)
                xmlnode.set('window', window.wm_name)
            xmlnode.set('x', str(x))
            xmlnode.set('y', str(y))
            xmlnode.set('key', str(key))
            xmlnode.set('wait', str(wait))
        elif xmlnode is not None:
            self.window = xmlnode.get('window', '')
            self.x = int(xmlnode.get('x', '0'))
            self.y = int(xmlnode.get('y', '0'))
            keystring = xmlnode.get('key')
            if keystring.startswith('Key.'):
                try:
                    self.key = Key[keystring[4:]]
                except KeyError:
                    self.key = keystring[4]
            elif keystring[0] == "'" and keystring[-1] == "'":
                self.key = keystring[1]
            self.wait = float(xmlnode.get('wait', 0.1))

    async def execute(self):
        global the_mouse
        global the_keyboard
        global translation

        print(f"key {self.window} {self.x},{self.y} '{self.key}'")

        if self.window:
            w = findWindow(self.window)
            if w is None:
                raise ValueError(f"Cannot find window {self.window}")
            x, y = translation.toScreen(self.x, self.y, w)
        else:
            x, y = self.x, self.y

        the_mouse.position = x, y
        if self.wait:
            await asyncio.sleep(self.wait)

        the_keyboard.press(self.key)
        the_keyboard.release(self.key)


class Check:

    errcnt = 0

    def __init__(self, xmlroot=None, xmlnode=None, x=0, y=0,
                 timeout=10.0, window='', wait=0.5):

        global translation

        if xmlroot is not None:
            xmlnode = etree.SubElement(xmlroot, 'check')

            if window:
                _x, _y = translation.toWindow(x, y, window)
                xmlnode.set('window', window.wm_name)
            else:
                _x, _y = x, y

            under_cursor = ImageGrab.grab(bbox=(x-2, y-2, x-1, y-1),
                                          xdisplay=x11display)
            col = under_cursor.getcolors(1)[0][1]
            print(f"Found {col} at {x},{y}")
            if window:
                print(f"rel to window {window.wm_name} at {_x},{_y}")

            xmlnode.set('x', str(_x))
            xmlnode.set('y', str(_y))
            xmlnode.set('r', str(col[0]))
            xmlnode.set('g', str(col[1]))
            xmlnode.set('b', str(col[2]))
            xmlnode.set('timeout', str(timeout))
            xmlnode.set('wait', str(wait))
            self.x, self.y, self.color, self.timeout, self.window = \
                x, y, col, timeout, window

        elif xmlnode is not None:
            self.window, self.x, self.y, self.timeout, self.wait = (
                xmlnode.get('window', ''),
                int(xmlnode.get('x', 1)),
                int(xmlnode.get('y', 1)),
                float(xmlnode.get('timeout', 0.0)),
                float(xmlnode.get('wait', 0.0)))
            if xmlnode.get('r', None) is not None:
                self.color = (
                    int(xmlnode.get('r')),
                    int(xmlnode.get('g')),
                    int(xmlnode.get('b')))
            else:
                self.color = None

    async def execute(self):

        print(f"check {self.window} {self.x},{self.y} {self.wait}+{self.timeout} {self.color}")
        # simply run in 100 sleep increments

        # move the mouse, since that may change color
        global the_mouse

        if self.wait:
            await asyncio.sleep(self.wait)
        moved = False

        for cnt in range(20):

            if not moved:
                if self.window:

                    # check the window is present
                    w = findWindow(self.window)

                    # when no window there, wait some more
                    if w is None and self.timeout > 0.0:
                        await asyncio.sleep(0.05*self.timeout)
                        continue

                    # pull the window up and to focus if needed
                    w.activate()

                    # translate the coordinates for test to screen
                    x, y = translation.toScreen(self.x, self.y, w)

                else:
                    x, y = self.x, self.y

                # set the mouse position
                the_mouse.position = x, y
                moved = True


            # wait part of the timeout, to see if the interface reacts
            if self.timeout > 0.0:
                await asyncio.sleep(0.05*self.timeout)

            # exit when we found the requested color
            if self.color is not None:
                under_cursor = ImageGrab.grab(bbox=(x-2, y-2, x-1, y-1),
                                              xdisplay=x11display)
                col = under_cursor.getcolors(1)[0][1]
                if col == self.color:
                    return True

            # no color was specified, exit if just looked for a window
            elif self.window is not None:
                return True

        # no window or color found, create a snapshot and increase errror count
        img = ImageGrab.grab(xdisplay=x11display)
        if self.window and w is None:
            print(f"Failed to find window {self.window} after {cnt+1} checks")
            img.save(f'{scenario.name}-error{Check.errcnt:03d}-no-win-{self.window}.png'.replace('/', '_'))
        elif self.color is not None:
            draw = ImageDraw.Draw(img)
            draw.rectangle(((x-3, y-3),(x, y)), outline=(0,0,0))
            img.save(f'{scenario.name}-error{Check.errcnt:03d}-no-col-{",".join(map(str, self.color))}-at{self.x},{self.y}.png')
            print(f"Failed to find the right color {self.color} at "
                  f"{x}, {y} after {cnt+1} checks, found {col}")
        if self.window is None and self.color is None:
            # no check, just timeout and wait
            return True
        
        Check.errcnt += 1
        return False

class Snap:
    def __init__(self, xmlroot=None, xmlnode=None, name=''):

        if xmlroot is not None:
            xmlnode = etree.SubElement(xmlroot, 'snap')
            xmlnode.set('name', name)
            self.name = name

        elif xmlnode is not None:
            self.name = xmlnode.get('name')

    async def execute(self):
        print(f"Snapshot to {self.name}")
        img = ImageGrab.grab(xdisplay=x11display)
        img.save(self.name)


class Scenario:
    def __init__(self, fname=None):
        self.clean = None
        self.fname = fname
        self.name = (fname and pathlib.Path(fname).stem) or 'anonymous'
        self._sync()

    def parseActions(self, actionroot):
        for node in actionroot:
            if XML_comment(node):
                pass
            elif XML_tag(node, 'check'):
                self.actions.append(Check(xmlnode=node))
            elif XML_tag(node, 'click'):
                self.actions.append(Click(xmlnode=node))
            elif XML_tag(node, 'snap'):
                self.actions.append(Snap(xmlnode=node))
            elif XML_tag(node, 'key'):
                self.actions.append(KeyPress(xmlnode=node))
            else:
                raise ValueError(f"Cannot process node {node.tag}")

    def _sync(self):
        if self.clean:
            return

        elif self.clean is None:

            print(f"Reading {self.fname}")
            # empty slate
            self.processes = []
            self.actionnode = None
            self.project = None
            self.actions = []

            # read from file
            try:
                parser = etree.XMLParser(remove_blank_text=True)
                with open(self.fname, 'rb') as mds:

                    self.xmltree = etree.XML(mds.read(), parser=parser)
                    for node in self.xmltree:
                        if XML_comment(node):
                            pass
                        elif XML_tag(node, 'project'):
                            self.project = Project(xmlnode=node)
                        elif XML_tag(node, 'repository'):
                            self.repository = node.text.strip()
                        elif XML_tag(node, 'execute'):
                            self.processes.append(Execute(xmlnode=node))
                        elif XML_tag(node, 'actions'):
                            self.actionnode = node
                            self.parseActions(node)
                        else:
                            raise ValueError(f"Cannot process node {node.tag}")
            except Exception as e:
                print("Failed to parse scenario information"
                      f" from {self.fname}: {e}")
                raise e

            if self.actionnode is None:
                self.actionnode = etree.SubElement(self.xmltree, 'actions')

            self.clean = True

            return

        # try a rename of the current file
        try:
            os.rename(self.fname, self.fname + '~')
        except FileNotFoundError:
            pass

        # write back to file
        print(f"Re-writing {self.fname}")
        self.project.save()
        etree.ElementTree(self.xmltree).write(
                self.fname, pretty_print=True, encoding='utf-8',
                xml_declaration=True)
        self.clean = True

    def pass_click(self, x, y, button, pressed):

        window = findWindowUnder(self.project.windows, x, y, True)
        self.actions.append(Click(xmlroot=self.actionnode,
                                  window=window, x=x, y=y,
                                  button=button, pressed=pressed))
        self.clean = False

    def pass_move(self, x, y):
        self.x, self.y = x, y

    def pass_key(self, key):
        print(f"Key press {key}")
        if key in (Key.f1,):

            # get color spot here
            window = findWindowUnder(self.project.windows, self.x, self.y, True)
            self.actions.append(Check(xmlroot=self.actionnode,
                                      x=self.x, y=self.y, window=window))
            return True

        elif key in (Key.f2,):

            self.actions.append(Snap(xmlroot=self.actionnode,
                                     name=f'snapshot{self.snapnum:04d}.png'))
            return True

        elif key in (Key.esc, ):
            return False

        else:
            window = findWindowUnder(self.project.windows, self.x, self.y, True)
            self.actions.append(KeyPress(xmlroot=self.actionnode, key=key,
                                         x=self.x, y=self.y, window=window))
            return True

    async def run(self, runner, record: bool=False):

        if runner.allComplete():
            print("Run exit before starting actions")
            return False

        for a in self.actions:
            await a.execute()
            if runner.allComplete():
                print("Run exit")
                return False

        if record:
            self.snapnum = 0
            loop = asyncio.get_event_loop()
            queue = asyncio.Queue()
            def on_press(key):
                #print("on_press called")
                loop.call_soon_threadsafe(queue.put_nowait, key)
            pynput.keyboard.Listener(on_press=on_press).start()
            def pass_move(x, y):
                #print("on_move called")
                loop.call_soon_threadsafe(queue.put_nowait, (x, y))
            def pass_click(x, y, button, pressed):
                #print("on_click called")
                loop.call_soon_threadsafe(queue.put_nowait, (x, y, button, pressed))
            pynput.mouse.Listener(on_click=pass_click, on_move=pass_move).start()

            while True:
                val = await queue.get()
                #print(f"Have queue value {val}")
                if isinstance(val, tuple) and len(val) == 2:
                    self.pass_move(*val)
                elif isinstance(val, tuple) and len(val) == 4:
                    self.pass_click(*val)
                else:
                    if not self.pass_key(val):
                        break
            self._sync()


class DuecaRunner:

    def __init__(self, project, base, repository):
        print(f"Creating runner {project}")
        self.project = project
        self.pdir = f'{base}/{project}/{project}'
        self.base = base
        self.repository = repository
        self.running = []

    def prepare(self):

        if not os.path.isdir(self.base):
            print(f"Preparing base dir {self.base}")
            os.mkdir(self.base)

        if not os.path.isdir(self.pdir):
            print(f"Preparing project dir {self.pdir}")



            # environment variables
            envdict = dict(os.environ)
            envdict['DAPPS_GITROOT_pub']='https://github.com/dueca/'
            #envdict['DAPPS_GITROOT'] = f'file:///{self.base}/repo/'
            #envdict['DUECA_CVSTOGITPATCHES'] = \
            #    '/home/repa/TUDelft/servers/cvstogit/patches'

            c1 = subprocess.run((
                'dueca-gproject', 'clone', '--remote',
                 f'{self.repository}{self.project}.git',
                 '--node', 'solo'), cwd=self.base, stderr=subprocess.PIPE)
            if c1.returncode != 0:
                raise RuntimeError(
                    f'Failing dueca-gproject for {self.project}\n', c1.stderr)
            c1 = subprocess.run(
                'dueca-gproject policies', shell=True,
                env=envdict, cwd=self.pdir, stderr=subprocess.PIPE)
            if c1.returncode != 0:
                raise RuntimeError(
                    f'Failing dueca-gproject policies for {self.project}\n'
                    f'{c1.stdout}/{c1.stderr}')

            c1 = subprocess.run('dueca-gproject refresh', shell=True,
                                env=envdict, cwd=self.pdir,
                                stderr=subprocess.PIPE)
            if c1.returncode != 0:
                raise RuntimeError(
                    f'Failing dueca-gproject refresh for {self.project}:\n{c1.stderr}')

            c1 = subprocess.run('cmake ..', cwd=f'{self.pdir}/build',
                                shell=True, stderr=subprocess.PIPE)
            if c1.returncode != 0:
                raise RuntimeError(
                    f'Failing cmake for {self.project}:\n{c1.stderr}')

            c1 = subprocess.run('make', cwd=f'{self.pdir}/build', shell=True,
                                stderr=subprocess.PIPE)
            if c1.returncode != 0:
                raise RuntimeError(
                    f'Failing make for {self.project}:\n{c1.stderr}')
        else:
            print(f"Project dir {self.pdir} already present")

    async def _runDueca(self, platform='solo', node='solo'):
        print(f"runDueca for project {self.project}, p:{platform} n:{node}")

        rdir = f'{self.pdir}/run/{platform}/{node}'

        cmpl = subprocess.run(('source', 'links.script'),
                              executable='/usr/bin/bash',
                              cwd=rdir, text=True,
                              stderr=subprocess.PIPE)
        if cmpl.returncode != 0:
            print(f'Failure to run links.script:\n{cmpl.stderr}')

        # hackety hack, someone scrubbed my LD_LIBRARY_PATH
        envdict = dict(os.environ)
        # print(envdict)
        if envdict.get('LD_LIBRARY_PATH', None) is None:
            envdict['LD_LIBRARY_PATH'] = '/tmp/lib64:/tmp/lib'
        for p in envdict['PATH'].split(':'):
            if os.path.exists(f"{p}/dueca-gproject"):
                break
        else:
            # development version in /tmp?
            envdict['PATH'] = envdict['PATH']+":/tmp/bin"

        print(f"using display {envdict.get('DISPLAY')}")
        duecaprocess = await asyncio.create_subprocess_shell(
            f"../../../build/dueca_run.x",
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE, cwd=rdir, env=envdict)
        stdout, stderr = await duecaprocess.communicate()
        print(f"Dueca task for {self.project} on {node} ended, {duecaprocess.returncode}")
        print(f'\nNormal out {node}\n{stdout.decode()}')
        print(f'\nError out {node}\n{stderr.decode()}')

    def runDueca(self, platform='solo', node='solo'):
        self.running.append(
            #asyncio.ensure_future(self._runDueca(platform, node)))
            self._runDueca(platform, node))

    def allComplete(self):
        #for f in self.running:
        #    if not f.done():
        #        return False
        return False
        #return True

    def pass_click(self, x, y, button, release):

        # check that we still have running windows
        if self.allComplete():
            self.follower.stop()

            return

        # search for the receiving window
        win = None
        for w in Window.list():
             if 'focused' in w.wm_state:
                 win = w
                 break

        self.clicks.append((w.wm_name, x, y, x-w.x, y-w.y, button, release))
        print(w.wm_name, x, y, x-w.x, y-w.y, button, release)


parser = argparse.ArgumentParser(
    description="""
Run and control one or more DUECA executables""")
parser.add_argument('--base', default='/tmp/tmp.runner',
                    help="Folder for extracting and running.")
parser.add_argument('--control', default='testscenario.xml',
                    help="File with start options and event inputs")
parser.add_argument('--learn', action='store_true',
                    help="Learning mode, record mouse clicks")
parser.add_argument('--offset-x', default=0, type=int,
                    help="Offset of the x origin given by the window manager")
parser.add_argument('--offset-y', default=22, type=int,
                    help="Offset of the y origin given by the window manager")
parser.add_argument('--extra-x', default=0, type=int,
                    help="Additional x size (borders) by window manager")
parser.add_argument('--extra-y', default=32, type=int,
                    help="Additional y size (borders, header) by window manager")
parser.add_argument('--timelimit', default=3600, type=int,
                    help="Time limit for running the test")
pres = parser.parse_args(sys.argv[1:])

t0 = time.time()

translation = Translation(pres.offset_x, pres.offset_y,
                          pres.extra_y)

# read the scenario
scenario = Scenario(fname=pres.control)

# prepare the executable
runner = DuecaRunner(scenario.project.name, pres.base, scenario.repository)
runner.prepare()

tprep = int(round(time.time() - t0))
print(f"Code preparation took {tprep}s") 

# run the different dueca processes
for p in scenario.processes:
    runner.runDueca(p.platform, p.node)

async def main():
    doall = asyncio.gather(scenario.run(runner, pres.learn), *runner.running)
    await asyncio.wait_for(doall, pres.timelimit - tprep)

asyncio.run(main())

sys.exit(Check.errcnt)
