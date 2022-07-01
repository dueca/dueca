#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Feb 22 15:49:30 2021

@author: repa
"""

import git
import tempfile
import subprocess
import os

class GitHandler(git.Repo):
    def __init__(self, project=''):
        super(GitHandler, self).__init__('.')
        self.my_urlbase = None
        try:
            self.my_urlbase = \
                '/'.join(self.remote().url[:-4].split('/')[:-1]) + \
                    '/{project}.git'
        except:
            pass

    def copyModule(self, project, module, newname, version, url=None):
        tf = tempfile.mkstemp(suffix='.tar')
        url = url or self.remote()
        self.git.archive(
            '--format=tar', f'--remote={url}',
            f'{version}:{module}/',
            f'--prefix={newname or module}/', f'--output={tf[1]}')
        res = subprocess.call(['tar', '-xvf', tf[1]])
        self.addFolder(newname)
        if res:
            raise Exception(
                f"Unable to unpack copied {project}/{module}")

    def borrowModule(self, project, module, version, url=None):

        # do we not have the project?
        if not os.path.isdir(f'../{project}'):


            os.mkdir(f'../{project}')
            if url is None:
                url = self.remote()
            base = git.Repo(f'{self.gitbase}/{project}')
            # clone from remote repo, but only one level
            repo = base.clone(f'../{project}', multi_options=(
                '--no-checkout',))
            # set sparse config
            with repo.config_writer() as cw:
                cw.set('core.sparseCheckout', 'true')

        # do we not have the module?
        if not os.path.isdir(f'../{project}/{module}'):
            with open(f'../{project}/.git/info/sparse-checkout', 'a') as f:
                f.write(f'{module}/\n')
            # git pull origin master

        pass

    def addFiles(self, files):
        self.index.add(f)
        #for f in files:
        #    self.index.add(f)

    def addFolder(self, f):
        self.index.add((f,))

    def getUrlBase(self, defltbase=None):
        if defltbase:
            return defltbase + '/{project}.git'
        if self.my_urlbase is None:
            raise Exception("project does not have an upstream origin"
                            " specify --remote-base")
        return self.my_urlbase

    def getUrl(self):
        return self.remote().url
