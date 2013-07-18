TEMPLATE = subdirs
SUBDIRS = libraries plugins tools

plugins.depends = libraries
tools.depends = libraries
