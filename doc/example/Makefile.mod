# =========================================================================
#       item            : template Makefile dueca module
#       made by         : René van Paassen
#       date            : 010320
# =========================================================================

# =========================================================================
#
# EDIT THIS SECTION ONLY TO ADJUST INCLUDE PATHS/DEFINES OR TO CHANGE
# THE DEFINITION OF WHAT YOUR SOURCE FILES ARE
#
# =========================================================================

# if needed, add the path to extra header files you need
EXTRA_INCLUDES =

# add all c++-files here (.cxx files), except the ones generated from .dco
# files. The line that is in there now simply finds all .cxx files. Change
# to : CPPSRC = File1.cxx File2.cxx
# if you don't want that behaviour
CPPSRC :=        $(shell ls *.cxx 2>/dev/null)

# and all c-files here (.c files)
CSRC :=                $(shell ls *.c 2>/dev/null)

# version of this makefile
MAKEFILEVERSION = 1

################ including generic rules and targets #######################
TAILFILE =        $(shell dueca-config --path-datafiles)/data/MakefileTail.mod
include $(TAILFILE)
################ end generic rules and targets #############################

# ==========================================================================
#
# EDIT THIS SECTION ONLY IF YOU HAVE SPECIAL NEEDS (GENERATING INTERFACE
# CODE FROM GLADE ETC) THAT ARE NOT HANDLED BY THE NORMAL TARGETS/RULES
#
# ==========================================================================

# Put additional commands to be executed for "make all" here
local-all:

# Put additional commands to be executed for "make depend" here, for example
# interface files generated with glade?
local-depend:

# Put additional commands to be executed for "make clean" here
local-clean:

# Put additional commands to be executed for "make mrproper" here
local-mrproper:



