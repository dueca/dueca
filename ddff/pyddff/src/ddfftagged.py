#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Apr  8 17:16:10 2022

@author: repa
"""

from .ddffinventoried import DDFFInventoried

class DDFFTagStream:

    def __init__(self, ddffs):
        self.base = ddffs
        self.stretches = {}
        for st in self.base:
            self.stretches[st[-1]] = (st[2], st[3], st[4])

    def __getitem__(self, key):
        return self.stretches[key]


class DDFFTagged(DDFFInventoried):

    def __init__(self, name, mode='r', *args, **kwargs):

        # analyse with base DDFF read
        super().__init__(name, mode=mode, *args, **kwargs)

        # replace/parse stream 1
        self.streams[1] = DDFFTagStream(self.streams[1])

    def index(self):
        return self.streams[1]

if __name__ == '__main__':

    stuff = DDFFTagged(
        '/home/repa/dapps/TestMultiStream/TestMultiStream/run/solo/solo/' +
        'PHLAB-record-20220410_1831.ddff')

    print(stuff.index()['Unnamed'])
