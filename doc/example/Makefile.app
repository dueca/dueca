# ============================================================================
#       item            : template Makefile dueca executable
#       made by         : René van Paassen
#       date            : 010320
# ============================================================================

# =========================================================================
#
# EDIT THIS SECTION TO CHOOSE DIFFERENT DUECA COMPONENTS OR ADD EXTRA
# LIBRARIES TO LINK IN WITH YOUR PROGRAM
#
# =========================================================================

# enter a component choice for DUECA (check dueca-config for possible values)
DCOMPONENTS =        --ip --dusime --extra --gtk2 --rtwv30

# specify which extra libs need to be included in the app
# all dueca's stuff is already handled by the DCOMPONENTS and
# dueca-config
EXTRALIBS =

# A second batch of extra libraries can be specified. This batch is
# included *after* all dueca's libs
EXTRALIBS2 =

# the rest is handled by generic rules
TAILFILE =        $(shell dueca-config --path-datafiles)/data/MakefileTail.app
include $(TAILFILE)


