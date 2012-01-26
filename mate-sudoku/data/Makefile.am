puzzledir = $(datadir)/mate-sudoku/puzzles/
puzzle_DATA = \
	easy \
	medium \
	hard \
	very_hard

uidir = $(datadir)/mate-sudoku
ui_DATA = \
	print_games.ui \
	puzzle_generator.ui \
	select_game.ui \
	tracker.ui

schema_in_files = mate-sudoku.schemas.in
if HAVE_MATE
schemadir = $(MATECONF_SCHEMA_FILE_DIR)
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
endif

install-schemas-local: $(schema_DATA)
if MATECONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
		for p in $^ ; do \
			MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $$p 2>&1 > /dev/null; \
		done \
	fi
endif

install-data-local: install-schemas-local

man_MANS = mate-sudoku.6

EXTRA_DIST = \
	$(puzzle_DATA)	\
	$(ui_DATA)	\
	$(schema_in_files) \
	$(man_MANS)

CLEANFILES = \
	$(schema_DATA)

DISTCLEANFILES = \
	$(schema_DATA)

@INTLTOOL_SCHEMAS_RULE@

-include $(top_srcdir)/git.mk
