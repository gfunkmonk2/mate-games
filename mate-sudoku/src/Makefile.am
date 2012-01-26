SUBDIRS = lib

#################################################################

## Executable
bin_SCRIPTS = mate-sudoku
CLEANFILES  = mate-sudoku mate-sudoku.in

mate-sudoku: mate-sudoku.in
	$(AM_V_GEN) cp mate-sudoku.in $@ && chmod u+x $@

-include $(top_srcdir)/git.mk
