SUBDIRS =

if WITH_INCLUDED_GNUCHESS
SUBDIRS += gnuchess
endif

SUBDIRS += data help src

desktopdir = $(datadir)/applications
desktop_in_files = glchess.desktop.in.in
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)

CLEANFILES = $(desktop_DATA)

@INTLTOOL_DESKTOP_RULE@

-include $(top_srcdir)/git.mk
