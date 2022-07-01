import os
import importlib

plugins = {}
for module in os.listdir(os.path.dirname(__file__)):
    if module == '__init__.py' or module[-3:] != '.py':
        continue
    module = module[:-3]
    #plugins[module[:-3]] = __import__(module[:-3], locals(), globals())
    plugins[module] = importlib.import_module('DCOplugins.'+module)
    #print "plugin", module
    #plugins[module[:-3]] = locals()[module[:-3]]
del module
del os
