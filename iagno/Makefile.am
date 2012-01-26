SUBDIRS =

if BUILD_HELP
SUBDIRS += help
endif

NULL =

bin_PROGRAMS = iagno

iagno_SOURCES = \
	gnothello.c \
	gnothello.h \
	othello.c \
	othello.h \
	properties.c \
	properties.h \
	$(NULL)

iagno_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

iagno_CFLAGS = \
	$(GTK_CFLAGS) \
	$(AM_CFLAGS)

iagno_LDADD = \
	$(top_builddir)/libgames-support/libgames-support.la \
	$(GTK_LIBS) \
	$(INTLLIBS)

if HAVE_MATE
iagno_CFLAGS += $(MATE_CFLAGS)
iagno_LDADD += $(MATE_LIBS)
endif

if HAVE_RSVG
iagno_CFLAGS += $(RSVG_CFLAGS)
iagno_LDADD += $(RSVG_LIBS)
endif

if WITH_GTHREAD
iagno_CFLAGS += $(GHTREAD_CFLAGS)
iagno_LDADD += $(GTHREAD_LIBS)
endif

pixmapdir = $(pkgdatadir)/iagno/pixmaps
pixmap_DATA = \
	classic.png \
	woodtrim.png \
	small.png

Gamesdir = $(datadir)/applications
Games_in_files = iagno.desktop.in.in
Games_DATA = $(Games_in_files:.desktop.in.in=.desktop)

schema_in_files = iagno.schemas.in
if HAVE_MATE
schemadir = $(MATECONF_SCHEMA_FILE_DIR)
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
endif

man_MANS = iagno.6

EXTRA_DIST = \
	AUTHORS \
	$(pixmap_DATA)	\
	$(schema_in_files) \
	$(man_MANS)

CLEANFILES = \
	$(NULL)

DISTCLEANFILES = $(Games_DATA) $(schema_DATA)

install-data-local:
if MATECONF_SCHEMAS_INSTALL
	MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(top_builddir)/iagno/$(schema_DATA)
endif

@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_SCHEMAS_RULE@
@INTLTOOL_SOUNDLIST_RULE@

-include $(top_srcdir)/git.mk
