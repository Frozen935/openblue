############################################################################
# platform/openvela/Makefile
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.
############################################################################

include $(APPDIR)/Make.defs

OPENBLUEDIR = $(APPDIR)/external/openblue/openblue

CFLAGS += ${INCDIR_PREFIX}$(OPENBLUEDIR)
CFLAGS += ${INCDIR_PREFIX}$(OPENBLUEDIR)/include
CFLAGS += ${INCDIR_PREFIX}$(OPENBLUEDIR)/bluetooth
CFLAGS += ${INCDIR_PREFIX}$(OPENBLUEDIR)/base
CFLAGS += ${INCDIR_PREFIX}$(OPENBLUEDIR)/osdep/posix
CFLAGS += ${INCDIR_PREFIX}$(OPENBLUEDIR)/shim/include

include $(APPDIR)/Application.mk
