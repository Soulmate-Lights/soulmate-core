#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

# CFLAGS += -I$(abspath ..)
# CPPFLAGS += -I$(abspath ..) -Wno-error=switch -Wno-error=maybe-uninitialized

COMPONENT_SRCDIRS := .

CPPFLAGS += -Wno-error=switch -Wno-error=maybe-uninitialized

EXTRA_COMPONENT_DIRS += $(abspath ../components)

PROJECT_NAME := example

include $(IDF_PATH)/make/project.mk

