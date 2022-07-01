# -*-makefile-*-
# ============================================================================
#       item            : Makefile "tail" dueca executable
#       made by         : René van Paassen
#       date            : 010320
#       changes         : 040408 RvP added documentation generation
# ============================================================================


MACHINE :=	$(shell cat .machine)
SUBDIRS :=      $(shell dueca-filtmods modules.$(MACHINE))

# check that all components are present/correct
CGOOD :=	$(shell dueca-config --libs $(DCOMPONENTS) >/dev/null \
                  && echo "good")
ifneq ($(CGOOD),good)
      $(error dueca-config problem)
endif

DUECALIBS :=	$(shell dueca-config --libs $(DCOMPONENTS))
DUECAINC := 	$(shell dueca-config --cflags $(DCOMPONENTS)) \
	 	$(COMMON_INCLUDES)
DUECAVER :=	$(shell dueca-config --version)
CODEGEN =	dueca-codegen
COMMDIRS :=    	$(shell ls -d ../*/comm-objects)
OWNDIRS := 	$(shell dueca-filtmods --own modules.$(MACHINE))
OPTBASE = 	/opt/dueca

# loader definitions
LD=		g++

MODULES = 	$(SUBDIRS:=/module.o) $(COMMDIRS:=/module.o)
MODULEFRESH = 	$(SUBDIRS:=/module_fresh) $(COMMDIRS:=/module_fresh)

# =========================================================================
#
#	targets and rules
#
# =========================================================================

.PHONY: all subdirs depend suid doc clean mrproper help generated \
	$(SUBDIRS) $(COMMDIRS) buildenv cleanenv

all : dueca_run.x buildenv

cleanenv:
	rm -f dueca-buildenv-*

# restore build environment
ifneq ("$(wildcard dueca-buildenv-*)", "")
    $(info using PATH from $(wildcard dueca-buildenv-*))
    include $(wildcard dueca-buildenv-*)
    SHELL := env PATH=$(PATH) /bin/bash
endif

dueca_run.x : $(SUBDIRS) $(COMMDIRS)
	$(LD) $(LDFLAGS) $(MODULES) \
	$(EXTRALIBS) $(DUECALIBS) $(EXTRALIBS2) -o $@

$(SUBDIRS) $(COMMDIRS):
	@if test -f $@/Makefile.$(MACHINE); then \
	    	echo "make in $@ using Makefile.$(MACHINE)"; \
		$(MAKE) -C $@ DUECAINC="$(DUECAINC)" CODEGEN=$(CODEGEN) \
	 	    -f Makefile.$(MACHINE) top-all ; \
	else \
	    	echo "make in $@"; \
		$(MAKE) -C $@ DUECAINC="$(DUECAINC)" CODEGEN=$(CODEGEN) \
		    top-all ; \
	fi

generated:
	@echo "making generated code"
	@(for i in $(SUBDIRS) $(COMMDIRS); do \
	    (if test -f $$i/Makefile.$(MACHINE); then \
	     	echo "make \"$@\" in $$i using Makefile.$(MACHINE)"; \
	        $(MAKE) -C $$i -f Makefile.$(MACHINE) DUECAINC="$(DUECAINC)" \
	          generated; \
	     else \
	     	echo "make \"$@\" in $$i"; \
		$(MAKE) -C $$i DUECAINC="$(DUECAINC)" generated; \
	     fi ) \
         done)

depend: generated
	@echo "calculating dependencies";
	(for i in $(SUBDIRS) $(COMMDIRS); do \
	    (if test -f $$i/Makefile.$(MACHINE); then \
	        echo "make \"depend\" in $$i using Makefile.$(MACHINE)"; \
	        $(MAKE) -C $$i -f Makefile.$(MACHINE) DUECAINC="$(DUECAINC)" \
	          depend; \
	     else \
	     	echo "make \"$@\" in $$i"; \
		$(MAKE) -C $$i DUECAINC="$(DUECAINC)" depend; \
	     fi ) \
         done)

suid:
	chown root dueca_run.x
	chmod 4770 dueca_run.x

doc: dueca_run.x
	@echo "making documentation"
	@(for i in $(SUBDIRS) $(COMMDIRS); do \
	    (if test -f $$i/Makefile.$(MACHINE); then \
	     	echo "make \"$@\" in $$i using Makefile.$(MACHINE)"; \
	        $(MAKE) -C $$i -f Makefile.$(MACHINE) DUECAINC="$(DUECAINC)" \
	          $@; \
	     else \
	     	echo "make \"$@\" in $$i"; \
		$(MAKE) -C $$i DUECAINC="$(DUECAINC)" $@; \
	     fi ) \
         done)

PYTHON_MAMI= $(shell python3 -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')")

buildenv:
	@(if (dueca-config --prefix | grep -q $(OPTBASE)) && \
	  test \! -f dueca-buildenv-$(DUECAVER) ; then \
	    rm -f dueca-buildenv-*; \
	    echo "# -*-makefile-*-" >dueca-buildenv-$(DUECAVER); \
	    echo "# Alternative build environment" \
		>>dueca-buildenv-$(DUECAVER); \
	    echo "export PATH := $(shell dueca-config --prefix)/bin:\$$(PATH)" \
		>>dueca-buildenv-$(DUECAVER); \
	    echo "export PKG_CONFIG_PATH := $(shell dueca-config --prefix)/lib/pkgconfig:$(shell dueca-config --prefix)/lib64/pkgconfig:\$$(PKG_CONFIG_PATH)" \
		>>dueca-buildenv-$(DUECAVER); \
	    echo "export PYTHONPATH := $(shell dueca-config --prefix)/lib/python$(PYTHON_MAMI)/site-packages" \
		>>dueca-buildenv-$(DUECAVER); \
	  echo "Created scriptlet for alternative DUECA $(DUECAVER)"; \
	  fi)

# =========================================================================
#
#	clean-up
#
# =========================================================================

clean:
	(for i in $(SUBDIRS) $(COMMDIRS); do \
	 (cd $$i && $(MAKE) clean); \
	 done)

mrproper:
	(for i in $(SUBDIRS) $(COMMDIRS); do \
	 (cd $$i && $(MAKE) mrproper); \
	 done)
	rm -f dueca_run.x *~

# =========================================================================
#
#	help!!
#
# =========================================================================

help:
	@cat `dueca-config --path-datafiles`/data/MakefileTailApp.doc
