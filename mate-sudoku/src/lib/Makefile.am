SUBDIRS = gtk_goodies

# Don't distribute generated defaults.py. If you know a simpler way that works
# e.g. nodist_matesudoku_PYTHON=defaults.py then please fix this
dist-hook:
	rm $(distdir)/defaults.py

matesudokudir = $(pythondir)/mate_sudoku
matesudoku_PYTHON = \
	colors.py	\
	dancer.py	\
	defaults.py	\
	dialog_swallower.py	\
	game_selector.py	\
	mate_sudoku.py	\
	gsudoku.py	\
	__init__.py	\
	main.py 	\
	number_box.py 	\
	pausable.py	\
	printing.py	\
	saver.py	\
	simple_debug.py	\
	sudoku_generator_gui.py	\
	sudoku_maker.py	\
	sudoku_thumber.py \
	sudoku.py	\
	timer.py

-include $(top_srcdir)/git.mk
