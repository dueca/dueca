import os
import importlib

plugins = {}
for module in os.listdir(os.path.dirname(__file__)):
    if module == '__init__.py' or module[-3:] != '.py':
        continue
    module = module[:-3]
    plugins[module] = importlib.import_module('EnumPlugins.'+module)
del module
del os
