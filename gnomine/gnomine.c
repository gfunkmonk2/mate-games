/* -*- mode:C; tab-width:8; c-basic-offset:8; indent-tabs-mode:true -*- */

/*
 * Mines for MATE
 * Author:        Pista <szekeres@cyberspace.mht.bme.hu>
 *
 * Score support: horape@compendium.com.ar
 * Mine Resizing: djb@redhat.com
 *
 * This game is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 */

#include <config.h>
#include <stdlib.h>

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libgames-support/games-clock.h>
#include <libgames-support/games-conf.h>
#include <libgames-support/games-frame.h>
#include <libgames-support/games-gtk-compat.h>
#include <libgames-support/games-help.h>
#include <libgames-support/games-runtime.h>
#include <libgames-support/games-scores.c>
#include <libgames-support/games-scores-dialog.h>
#include <libgames-support/games-stock.h>

#ifdef WITH_SMCLIENT
#include <libgames-support/eggsmclient.h>
#endif /* WITH_SMCLIENT */

#include "minefield.h"

#define APP_NAME "gnomine"
#define APP_NAME_LONG N_("Mines")

/* Limits for various minefield properties */
#define XSIZE_MIN 4
#define XSIZE_MAX 100
#define YSIZE_MIN 4
#define YSIZE_MAX 100
#define WIDTH_DEFAULT  300
#define HEIGHT_DEFAULT 300

/* GamesConf key paths */
#define KEY_GEOMETRY_GROUP "geometry"
#define KEY_XSIZE "xsize"
#define KEY_YSIZE "ysize"
#define KEY_NMINES "nmines"
#define KEY_MODE "mode"

#define KEY_USE_QUESTION_MARKS "use_question_marks"
#define KEY_USE_OVERMINE_WARNING "use_overmine_warning"
#define KEY_USE_AUTOFLAG "use_autoflag"

static GtkWidget *mfield;
static GtkWidget *pref_dialog = NULL;
static GtkWidget *resume_button;
static GtkWidget *resume_container;
static GtkWidget *mfield_container;
GtkWidget *window;
GtkWidget *flabel;
GtkWidget *mentry;
GtkWidget *mbutton;
GtkWidget *cframe;
GtkWidget *clk;
GtkWidget *pm_win, *pm_sad, *pm_smile, *pm_cool, *pm_worried, *pm_current;
gint ysize = -1, xsize = -1;
gint nmines = -1;
gint fsize = -1;
gboolean use_question_marks = TRUE;
gboolean use_overmine_warning = TRUE;
gboolean use_autoflag = FALSE;
gboolean game_paused = FALSE;

GtkAction *hint_action;
GtkAction *fullscreen_action;
GtkAction *leavefullscreen_action;
GtkAction *pause_action;
GtkAction *resume_action;

/*GstElement *sound_player;*/

static const GamesScoresCategory scorecats[] = {
  {"Small",  NC_("board size", "Small")  },
  {"Medium", NC_("board size", "Medium") },
  {"Large",  NC_("board size", "Large")  },
  {"Custom", NC_("board size", "Custom") }
};

GamesScores *highscores;

/* It's a little ugly, but it stops the hint dialogs triggering the
 * hide-the-window-to-stop cheating thing. */
gboolean disable_hiding = FALSE;

#if 0
static void
play_sound (int id)
{
  /* FIXME: We ignore the id */

  /* To play, do this: */
  gst_element_set_state (sound_player, GST_STATE_PLAYING);
}
#endif

static GtkWidget *
image_widget_setup (char *name)
{
  GtkWidget *image = NULL;
  char *filename = NULL;
  const char *dname;

  image = gtk_image_new ();
  dname = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);
  filename = g_build_filename (dname, name, NULL);

  if (filename != NULL)
    gtk_image_set_from_file (GTK_IMAGE (image), filename);

  g_free (filename);

  return image;
}

static void
show_face (GtkWidget * pm)
{
  if (pm_current == pm)
    return;

  if (pm_current) {
    gtk_widget_hide (pm_current);
  }
  gtk_action_set_sensitive (hint_action, (pm == pm_cool) || (pm == pm_smile));
  gtk_widget_show (pm);

  pm_current = pm;
}

static void
quit_game (void)
{
  gtk_main_quit ();
}

static void
set_flabel (GtkMineField * mfield)
{
  char *val;

  val =
    g_strdup_printf (_("Flags: %d/%d"), mfield->flag_count, mfield->mcount);
  gtk_label_set_text (GTK_LABEL (flabel), val);
  g_free (val);
}

/* Show the high scores dialog - creating it if necessary. If pos is 
 * greater than 0 the appropriate score is highlighted. If the score isn't
 * a high score and this isn't a direct request to see the scores, we 
 * only show a simple dialog. */
static gint
show_scores (gint pos, gboolean endofgame)
{
  gchar *message;
  static GtkWidget *scoresdialog = NULL;
  static GtkWidget *sorrydialog = NULL;
  GtkWidget *dialog;
  gint result;

  if (endofgame && (pos <= 0)) {
    if (sorrydialog != NULL) {
      gtk_window_present (GTK_WINDOW (sorrydialog));
    } else {
      sorrydialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
							GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_INFO,
							GTK_BUTTONS_NONE,
							"<b>%s</b>\n%s",
							_
							("The Mines Have Been Cleared!"),
							_
							("Great work, but unfortunately your score did not make the top ten."));
      gtk_dialog_add_buttons (GTK_DIALOG (sorrydialog), GTK_STOCK_QUIT,
			      GTK_RESPONSE_REJECT, _("_New Game"),
			      GTK_RESPONSE_ACCEPT, NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (sorrydialog),
				       GTK_RESPONSE_ACCEPT);
      gtk_window_set_title (GTK_WINDOW (sorrydialog), "");
    }
    dialog = sorrydialog;
  } else {

    if (scoresdialog != NULL) {
      gtk_window_present (GTK_WINDOW (scoresdialog));
    } else {
      scoresdialog = games_scores_dialog_new (GTK_WINDOW (window), highscores, _("Mines Scores"));
      games_scores_dialog_set_category_description (GAMES_SCORES_DIALOG
						    (scoresdialog),
						    _("Size:"));
    }

    if (pos > 0) {
      games_scores_dialog_set_hilight (GAMES_SCORES_DIALOG (scoresdialog),
				       pos);
      message = g_strdup_printf ("<b>%s</b>\n\n%s",
				 _("Congratulations!"),
				 pos == 1 ? _("Your score is the best!") :
                                 _("Your score has made the top ten."));
      games_scores_dialog_set_message (GAMES_SCORES_DIALOG (scoresdialog),
				       message);
      g_free (message);
    } else {
      games_scores_dialog_set_message (GAMES_SCORES_DIALOG (scoresdialog),
				       NULL);
    }

    if (endofgame) {
      games_scores_dialog_set_buttons (GAMES_SCORES_DIALOG (scoresdialog),
				       GAMES_SCORES_QUIT_BUTTON |
				       GAMES_SCORES_NEW_GAME_BUTTON);
    } else {
      games_scores_dialog_set_buttons (GAMES_SCORES_DIALOG (scoresdialog), 0);
    }
    dialog = scoresdialog;
  }

  result = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);

  return result;
}

static void
scores_callback (void)
{
  show_scores (0, FALSE);
}

static void
new_game (void)
{
  gint width, height, w_diff, h_diff;
  guint size;
  gint x, y;
  static gint size_table[3][3] = { {8, 8, 10}, {16, 16, 40}, {30, 16, 99} };
  GtkMineField *mf = GTK_MINEFIELD (mfield);
  GtkAllocation allocation;

  games_clock_stop (GAMES_CLOCK (clk));
  games_clock_reset (GAMES_CLOCK (clk));
  show_face (pm_smile);


  /* get window size and mine square size (gtk_minefield_restart() may change it) */
  gtk_window_get_size (GTK_WINDOW (window), &width, &height);
  size = mf->minesizepixels;
  gtk_widget_get_allocation (mfield, &allocation);
  w_diff = width - allocation.width;
  h_diff = height - allocation.height;

  if (fsize == 3) {
    x = xsize;
    y = ysize;
    mf->mcount = nmines;
  } else {
    x = size_table[fsize][0];
    y = size_table[fsize][1];
    mf->mcount = size_table[fsize][2];
  }

  games_scores_set_category (highscores, scorecats[fsize].key);
  gtk_minefield_set_size (GTK_MINEFIELD (mfield), x, y);
  gtk_minefield_restart (GTK_MINEFIELD (mfield));

  set_flabel (GTK_MINEFIELD (mfield));

  gtk_action_set_visible (pause_action, TRUE);
  gtk_action_set_sensitive (pause_action, TRUE);
  gtk_action_set_visible (resume_action, FALSE);
  gtk_action_set_sensitive (resume_action, FALSE);
  gtk_widget_hide (resume_container);
  gtk_widget_show (mfield_container);
}

/* Add a penalty for a successful hint. */
static void
hint_used (GtkWidget * widget, gpointer data)
{
  /* There is a ten second penalty for accepting a hint. */
  games_clock_add_seconds (GAMES_CLOCK (clk), 10);
}

static void
hint_callback (void)
{
  int result;
  gchar *message;
  GtkWidget *dialog;

  result = gtk_minefield_hint (GTK_MINEFIELD (mfield));

  /* Successful hints are handled by the callback. */
  if (result == MINEFIELD_HINT_ACCEPTED)
    return;

  if (result == MINEFIELD_HINT_NO_GAME)
    message = _("Click a square, any square");
  else
    message = _("Maybe they're all mines ...");

  dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                               message, NULL);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  disable_hiding = TRUE;
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  disable_hiding = FALSE;

}

static void
pause_callback (GtkWidget * widget, GdkEventFocus * event, gpointer data)
{
  if (games_clock_is_started (GAMES_CLOCK (clk))
      && (!disable_hiding)) {
    gtk_widget_hide (mfield_container);
    gtk_widget_show (resume_container);
    gtk_widget_grab_focus (resume_button);

    gtk_action_set_sensitive (hint_action, FALSE);
    games_clock_stop (GAMES_CLOCK (clk));

    gtk_action_set_visible (pause_action, FALSE);
    gtk_action_set_sensitive (pause_action, FALSE);

    gtk_action_set_visible (resume_action, TRUE);
    gtk_action_set_sensitive (resume_action, TRUE);

    game_paused = TRUE;
  }
}

static void
resume_game_cb (GtkButton * widget, gpointer data)
{
  gtk_widget_hide (resume_container);
  gtk_widget_show (mfield_container);
  gtk_action_set_sensitive (hint_action, TRUE);
  games_clock_start (GAMES_CLOCK (clk));

  gtk_action_set_visible (pause_action, TRUE);
  gtk_action_set_sensitive(pause_action, TRUE);
  gtk_action_set_visible (resume_action, FALSE);
  gtk_action_set_sensitive(resume_action, FALSE);

  game_paused = FALSE;
}

static void
marks_changed (GtkWidget * widget, gpointer data)
{
  set_flabel (GTK_MINEFIELD (widget));

  games_clock_start (GAMES_CLOCK (clk));
}

static void
lose_game (GtkWidget * widget, gpointer data)
{
  show_face (pm_sad);

  gtk_widget_grab_focus (mbutton);

  games_clock_stop (GAMES_CLOCK (clk));
}

static void
win_game (GtkWidget * widget, gpointer data)
{
  GamesScoreValue score;
  int pos;
  time_t seconds;

  games_clock_stop (GAMES_CLOCK (clk));

  gtk_widget_grab_focus (mbutton);

  show_face (pm_win);

  seconds = games_clock_get_seconds (GAMES_CLOCK (clk));
  score.time_double = (gfloat) (seconds / 60) + (gfloat) (seconds % 60) / 100;

  pos = games_scores_add_score (highscores, score);

  if (show_scores (pos, TRUE) == GTK_RESPONSE_REJECT)
    quit_game ();
  else
    new_game ();
}

static void
look_cell (GtkWidget * widget, gpointer data)
{
  show_face (pm_worried);

  games_clock_start (GAMES_CLOCK (clk));
}

static void
unlook_cell (GtkWidget * widget, gpointer data)
{
  show_face (pm_cool);
}

static void
verify_ranges (void)
{
  xsize = CLAMP (xsize, XSIZE_MIN, XSIZE_MAX);
  ysize = CLAMP (ysize, YSIZE_MIN, YSIZE_MAX);
  nmines = CLAMP (nmines, 1, xsize * ysize - 9);
  fsize = CLAMP (fsize, 0, 3);
}

static void
about_callback (void)
{
  const gchar *authors[] = {
    _("Main game:"),
    "Szekeres Istvan",
    "",
    _("Score:"),
    "Horacio J. Pe\xc3\xb1" "a",
    "",
    _("Resizing and SVG support:"),
    "Steve Chaplin",
    "Callum McKenzie",
    NULL
  };

  const gchar *artists[] = {
    _("Faces:"),
    "Lapo Calamandrei and Ulisse Perusin",
    "",
    _("Graphics:"),
    "Richard Hoelscher",
    NULL
  };

  const gchar *documenters[] = {
    "Callum McKenzie",
    NULL
  };

  gchar *license = games_get_license (APP_NAME_LONG);

  gtk_show_about_dialog (GTK_WINDOW (window),
			 "name", APP_NAME_LONG,
			 "version", VERSION,
			 "comments",
			 _("The popular logic puzzle minesweeper. "
			   "Clear mines from a board using hints from "
			   "squares you have already uncovered.\n\n"
			   "Mines is a part of MATE Games."),
			 "copyright",
			 "Copyright \xc2\xa9 1997-2008 Free Software Foundation, Inc.",
			 "license", license, "authors", authors, "artists",
			 artists, "documenters", documenters,
			 "translator-credits", _("translator-credits"),
			 "logo-icon-name", "mate-mines", "website",
			 "http://mate-desktop.org/",
			 "website-label", _("MATE Desktop web site"),
			 "wrap-license", TRUE, NULL);
  g_free (license);
}

static void
conf_value_changed_cb (GamesConf *conf, const char *group, const char *key)
{
  if (group == NULL) {
    if (strcmp (key, KEY_USE_QUESTION_MARKS) == 0) {
      use_question_marks = games_conf_get_boolean_with_default (group, key, TRUE);
      gtk_minefield_set_use_question_marks (GTK_MINEFIELD (mfield),
  					  use_question_marks);
    }
    if (strcmp (key, KEY_USE_OVERMINE_WARNING) == 0) {
      use_overmine_warning = games_conf_get_boolean_with_default (group, key, TRUE);
      gtk_minefield_set_use_overmine_warning (GTK_MINEFIELD (mfield),
  					    use_overmine_warning);
    }
    if (strcmp (key, KEY_USE_AUTOFLAG) == 0) {
      use_autoflag = games_conf_get_boolean_with_default (group, key, TRUE);
      gtk_minefield_set_use_autoflag (GTK_MINEFIELD (mfield),
  				    use_autoflag);
    }
  } else if (strcmp (group, KEY_GEOMETRY_GROUP) == 0) {
    if (strcmp (key, KEY_XSIZE) == 0) {
      int i;
      i = games_conf_get_integer_with_default (group, key, 16);
      if (i != xsize) {
        xsize = CLAMP (i, XSIZE_MIN, XSIZE_MAX);
        new_game ();
      }
    }
    if (strcmp (key, KEY_YSIZE) == 0) {
      int i;
      i = games_conf_get_integer_with_default (group, key, 16);
      if (i != ysize) {
        ysize = CLAMP (i, YSIZE_MIN, YSIZE_MAX);
        new_game ();
      }
    }
    if (strcmp (key, KEY_NMINES) == 0) {
      int i;
      i = games_conf_get_integer_with_default (group, key, 40);
      if (nmines != i) {
        nmines = CLAMP (i, 1, xsize * ysize - 2);
        new_game ();
      }
    }
    if (strcmp (key, KEY_MODE) == 0) {
      int i;
      i = games_conf_get_integer_with_default (group, key, 0);
      if (i != fsize) {
        fsize = CLAMP (i, 0, 3);
        new_game ();
      }
    }
  }
}

static void
size_radio_callback (GtkWidget * widget, gpointer data)
{
  int fsc;
  if (!pref_dialog)
    return;

  fsc = GPOINTER_TO_INT (data);

  games_conf_set_integer (KEY_GEOMETRY_GROUP, KEY_MODE, fsc);

  gtk_widget_set_sensitive (cframe, fsc == 3);
}

static void
fix_nmines (int xsize, int ysize)
{
  int maxmines;

  /* Fix up the maximum number of mines so that there is always at least
   * ten free spaces. Nine are so we can clear at least the immediate
   * eight neighbours at the start and one more so the game isn't over 
   * immediately. */
  maxmines = xsize * ysize - 10;
  if (nmines > maxmines) {
    games_conf_set_integer (KEY_GEOMETRY_GROUP, KEY_NMINES, maxmines);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (mentry), maxmines);
  }
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (mentry), 1, maxmines);
}

static void
xsize_spin_cb (GtkSpinButton * spin, gpointer data)
{
  int size = gtk_spin_button_get_value_as_int (spin);
  games_conf_set_integer (KEY_GEOMETRY_GROUP, KEY_XSIZE, size);
  fix_nmines (size, ysize);
}

static void
ysize_spin_cb (GtkSpinButton * spin, gpointer data)
{
  int size = gtk_spin_button_get_value_as_int (spin);
  games_conf_set_integer (KEY_GEOMETRY_GROUP, KEY_YSIZE, size);
  fix_nmines (xsize, size);
}

static void
nmines_spin_cb (GtkSpinButton * spin, gpointer data)
{
  int size = gtk_spin_button_get_value_as_int (spin);
  games_conf_set_integer (KEY_GEOMETRY_GROUP, KEY_NMINES, size);

}

static void
use_question_toggle_cb (GtkCheckButton * check, gpointer data)
{
  gboolean use_marks =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  games_conf_set_boolean (NULL, KEY_USE_QUESTION_MARKS, use_marks);
}

static void
use_overmine_toggle_cb (GtkCheckButton * check, gpointer data)
{
  gboolean use_overmine =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  games_conf_set_boolean (NULL, KEY_USE_OVERMINE_WARNING, use_overmine);
}

static void
set_fullscreen_actions (gboolean is_fullscreen)
{
  gtk_action_set_sensitive (leavefullscreen_action, is_fullscreen);
  gtk_action_set_visible (leavefullscreen_action, is_fullscreen);

  gtk_action_set_sensitive (fullscreen_action, !is_fullscreen);
  gtk_action_set_visible (fullscreen_action, !is_fullscreen);
}

static void
fullscreen_callback (GtkAction * action)
{
  if (action == fullscreen_action)
    gtk_window_fullscreen (GTK_WINDOW (window));
  else
    gtk_window_unfullscreen (GTK_WINDOW (window));
}

static gboolean
window_state_callback (GtkWidget * widget, GdkEventWindowState * event)
{
  if (!(event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN))
    return FALSE;

  set_fullscreen_actions (event->
			  new_window_state & GDK_WINDOW_STATE_FULLSCREEN);
    
  return FALSE;
}


static gboolean
pause_key_callback(GtkWidget *widget, GdkEventKey *event,
gpointer data )
{

  if(event->keyval == GDK_Pause)
  {
    if(game_paused)
      resume_game_cb(NULL, NULL); // Resume the game
    else
      pause_callback(NULL, NULL, NULL); // Pause the game

    return TRUE;
  }

  return FALSE;
}


static void
create_preferences (void)
{
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *table2;
  GtkWidget *label2;
  GtkWidget *question_toggle;
  GtkWidget *overmine_toggle;
  GtkWidget *xentry;
  GtkWidget *yentry;
  GSList *group;
  int i;

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 18);
  gtk_table_set_col_spacings (GTK_TABLE (table), 18);

  frame = games_frame_new (_("Field Size"));

  vbox = gtk_vbox_new (FALSE, 6);

  group = NULL;
  for (i = 0; i < G_N_ELEMENTS (scorecats); ++i) {
    button = gtk_radio_button_new_with_label (group, g_dpgettext2 (NULL, "board size", scorecats[i].name));
    if (fsize == i) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
    }

    g_signal_connect (GTK_OBJECT (button), "clicked",
                      G_CALLBACK (size_radio_callback),
                      GINT_TO_POINTER (i));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  }

  gtk_container_add (GTK_CONTAINER (frame), vbox);

  gtk_table_attach_defaults (GTK_TABLE (table), frame, 0, 1, 0, 1);

  cframe = games_frame_new (_("Custom Size"));
  gtk_widget_set_sensitive (cframe, fsize == 3);

  table2 = gtk_table_new (3, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table2), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 12);

  label2 = gtk_label_new_with_mnemonic (_("_Number of mines:"));
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table2), label2, 0, 1, 2, 3,
		    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  mentry = gtk_spin_button_new_with_range (1, XSIZE_MAX * YSIZE_MAX, 1);
  g_signal_connect (GTK_OBJECT (mentry), "value-changed",
		    G_CALLBACK (nmines_spin_cb), NULL);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (mentry), nmines);
  gtk_table_attach (GTK_TABLE (table2), mentry, 1, 2, 2, 3, 0, 0, 0, 0);
  fix_nmines (xsize, ysize);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label2), mentry);

  label2 = gtk_label_new_with_mnemonic (_("_Horizontal:"));
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table2), label2, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  xentry = gtk_spin_button_new_with_range (XSIZE_MIN, XSIZE_MAX, 1);
  g_signal_connect (GTK_OBJECT (xentry), "value-changed",
		    G_CALLBACK (xsize_spin_cb), NULL);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (xentry), xsize);
  gtk_table_attach (GTK_TABLE (table2), xentry, 1, 2, 0, 1, 0, 0, 0, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label2), xentry);

  label2 = gtk_label_new_with_mnemonic (_("_Vertical:"));
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table2), label2, 0, 1, 1, 2,
		    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  yentry = gtk_spin_button_new_with_range (YSIZE_MIN, YSIZE_MAX, 1);
  g_signal_connect (GTK_OBJECT (yentry), "value-changed",
		    G_CALLBACK (ysize_spin_cb), NULL);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (yentry), ysize);
  gtk_table_attach (GTK_TABLE (table2), yentry, 1, 2, 1, 2, 0, 0, 0, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label2), yentry);

  gtk_container_add (GTK_CONTAINER (cframe), table2);

  gtk_table_attach (GTK_TABLE (table), cframe, 1, 2, 0, 1, GTK_FILL, GTK_FILL,
		    0, 0);

  frame = games_frame_new(_("Flags"));

  question_toggle =
    gtk_check_button_new_with_mnemonic (_("_Use \"I'm not sure\" flags"));
  g_signal_connect (GTK_OBJECT (question_toggle), "toggled",
		    G_CALLBACK (use_question_toggle_cb), NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (question_toggle),
				use_question_marks);
  gtk_widget_show (question_toggle);

  gtk_container_add (GTK_CONTAINER (frame), question_toggle);

  gtk_table_attach_defaults (GTK_TABLE (table), frame, 0, 2, 1, 2);

  frame = games_frame_new(_("Warnings"));

  overmine_toggle =
    gtk_check_button_new_with_mnemonic (_("_Use \"Too many flags\" warning"));
  g_signal_connect (GTK_OBJECT (overmine_toggle), "toggled",
		    G_CALLBACK (use_overmine_toggle_cb), NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (overmine_toggle),
				use_overmine_warning);
  gtk_widget_show (overmine_toggle);

  gtk_container_add (GTK_CONTAINER (frame), overmine_toggle);

  gtk_table_attach_defaults (GTK_TABLE (table), frame, 0, 2, 2, 3);

  pref_dialog = gtk_dialog_new_with_buttons (_("Mines Preferences"),
					     GTK_WINDOW (window),
					     0,
					     GTK_STOCK_CLOSE,
					     GTK_RESPONSE_CLOSE, NULL);

  gtk_dialog_set_has_separator (GTK_DIALOG (pref_dialog), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (pref_dialog), 5);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (pref_dialog))), 2);
  gtk_window_set_resizable (GTK_WINDOW (pref_dialog), FALSE);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (pref_dialog))),
                      table, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (pref_dialog), "destroy",
		    G_CALLBACK (gtk_widget_destroyed), &pref_dialog);
  g_signal_connect (G_OBJECT (pref_dialog), "response",
		    G_CALLBACK (gtk_widget_hide), NULL);

  /* show all child widgets, but do not display the dialog (yet) */
  gtk_widget_show_all (GTK_WIDGET (table));
}


static void
preferences_callback (void)
{
  if (pref_dialog == NULL)
    create_preferences ();
  gtk_window_present (GTK_WINDOW (pref_dialog));
}

static void
help_callback (void)
{
  games_help_display (window, "gnomine", NULL);
}

const GtkActionEntry actions[] = {
  {"GameMenu", NULL, N_("_Game")},
  {"SettingsMenu", NULL, N_("_Settings")},
  {"HelpMenu", NULL, N_("_Help")},
  {"NewGame", GAMES_STOCK_NEW_GAME, NULL, NULL, NULL, G_CALLBACK (new_game)},
  {"PauseGame", GAMES_STOCK_PAUSE_GAME, NULL, NULL, NULL,
   G_CALLBACK (pause_callback)},
  {"ResumeGame", GAMES_STOCK_RESUME_GAME, NULL, NULL, NULL,
   G_CALLBACK (resume_game_cb)},
  {"Hint", GAMES_STOCK_HINT, NULL, NULL, NULL, G_CALLBACK (hint_callback)},
  {"Scores", GAMES_STOCK_SCORES, NULL, NULL, NULL,
   G_CALLBACK (scores_callback)},
  {"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (quit_game)},
  {"Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL,
   G_CALLBACK (preferences_callback)},
  {"Contents", GAMES_STOCK_CONTENTS, NULL, NULL, NULL,
   G_CALLBACK (help_callback)},
  {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (about_callback)},
  {"Fullscreen", GAMES_STOCK_FULLSCREEN, NULL, NULL, NULL,
   G_CALLBACK (fullscreen_callback)},
  {"LeaveFullscreen", GAMES_STOCK_LEAVE_FULLSCREEN, NULL, NULL, NULL,
   G_CALLBACK (fullscreen_callback)}
};

const char ui_description[] =
  "<ui>"
  "  <menubar name='MainMenu'>"
  "    <menu action='GameMenu'>"
  "      <menuitem action='NewGame'/>"
  "      <menuitem action='Hint'/>"
  "      <menuitem action='PauseGame'/>"
  "      <menuitem action='ResumeGame'/>"
  "      <separator/>"
  "      <menuitem action='Scores'/>"
  "      <separator/>"
  "      <menuitem action='Quit'/>"
  "    </menu>"
  "    <menu action='SettingsMenu'>"
  "      <menuitem action='Fullscreen'/>"
  "      <menuitem action='LeaveFullscreen'/>"
  "      <menuitem action='Preferences'/>"
  "    </menu>"
  "    <menu action='HelpMenu'>"
  "      <menuitem action='Contents'/>"
  "      <menuitem action='About'/>"
  "    </menu>"
  "  </menubar>"
  "</ui>";

static GtkUIManager *
create_ui_manager (const gchar * group)
{
  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;

  action_group = gtk_action_group_new ("group");
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, actions, G_N_ELEMENTS (actions),
				window);

  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);
  hint_action = gtk_action_group_get_action (action_group, "Hint");

  fullscreen_action =
    gtk_action_group_get_action (action_group, "Fullscreen");
  leavefullscreen_action =
    gtk_action_group_get_action (action_group, "LeaveFullscreen");
  set_fullscreen_actions (FALSE);

  pause_action = gtk_action_group_get_action (action_group, "PauseGame");
  resume_action = gtk_action_group_get_action (action_group, "ResumeGame");
  gtk_action_set_visible (resume_action, FALSE);

  return ui_manager;
}

#ifdef WITH_SMCLIENT
static int
save_state_cb (EggSMClient *client,
	    GKeyFile* keyfile,
	    gpointer client_data)
{
  char *argv[20];
  int argc = 0, j;
  gint xpos, ypos;

  gdk_window_get_origin (gtk_widget_get_window (window), &xpos, &ypos);

  argv[argc++] = g_get_prgname ();
  argv[argc++] = "-x";
  argv[argc++] = g_strdup_printf ("%d", xsize);
  argv[argc++] = "-y";
  argv[argc++] = g_strdup_printf ("%d", ysize);
  argv[argc++] = "-n";
  argv[argc++] = g_strdup_printf ("%d", nmines);
  argv[argc++] = "-f";
  argv[argc++] = g_strdup_printf ("%d", fsize);
  argv[argc++] = "-a";
  argv[argc++] = g_strdup_printf ("%d", xpos);
  argv[argc++] = "-b";
  argv[argc++] = g_strdup_printf ("%d", ypos);

  egg_sm_client_set_restart_command (client, argc, (const char **) argv);

  for (j = 2; j < argc; j += 2)
    g_free (argv[j]);

  return TRUE;
}

static gint
quit_cb (EggSMClient *client,
         gpointer client_data)
{
  gtk_main_quit ();

  return FALSE;
}

#endif /* WITH_SMCLIENT */

static int xpos = -1, ypos = -1;

#if 0
static void
sound_eos (GstElement * sound_player, gpointer data)
{
  /* Reset the sound. */
  gst_element_set_state (sound_player, GST_STATE_NULL);
}

static void
sound_init (int *argcp, char **argvp[])
{

  gst_init (argcp, argvp);
  sound_player = gst_element_factory_make ("playbin", "play");
  g_object_set (G_OBJECT (sound_player), "uri",
		"file:///home/callum/Desktop/Development/CVS/mate/mate-games/matetris/sounds/land.wav",
		NULL);

  /* Attach the signal for end-of-stream. */
  g_signal_connect (G_OBJECT (sound_player), "eos", G_CALLBACK (sound_eos),
		    NULL);

}
#endif

int
main (int argc, char *argv[])
{
  GOptionContext *context;

  GtkWidget *all_boxes;
  GtkWidget *status_box;
  GtkWidget *button_table;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *face_box;
  GtkUIManager *ui_manager;
  GtkAccelGroup *accel_group;
  gboolean retval;
  GError *error = NULL;
#ifdef WITH_SMCLIENT
  EggSMClient *sm_client;
#endif /* WITH_SMCLIENT */

  static const GOptionEntry options[] = {
    {"width", 'x', 0, G_OPTION_ARG_INT, &xsize, N_("Width of grid"), N_("X")},
    {"height", 'y', 0, G_OPTION_ARG_INT, &ysize, N_("Height of grid"),
     N_("Y")},
    {"mines", 'n', 0, G_OPTION_ARG_INT, &nmines, N_("Number of mines"),
     N_("NUMBER")},
    {"size", 'f', 0, G_OPTION_ARG_INT, &fsize,
     N_("Size of the board (0-2 = small-large, 3=custom)"), NULL},
    {"a", 'a', 0, G_OPTION_ARG_INT, &xpos, N_("X location of window"),
     N_("X")},
    {"b", 'b', 0, G_OPTION_ARG_INT, &ypos, N_("Y location of window"),
     N_("Y")},
    {NULL}
  };

  if (!games_runtime_init ("gnomine"))
    return 1;

#ifdef ENABLE_SETGID
  setgid_io_init ();
#endif

  context = g_option_context_new (NULL);
#if GLIB_CHECK_VERSION (2, 12, 0)
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
#endif
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
#ifdef WITH_SMCLIENT
  g_option_context_add_group (context, egg_sm_client_get_option_group ());
#endif /* WITH_SMCLIENT */
  g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);
  if (!retval) {
    g_print ("%s", error->message);
    g_error_free (error);
    exit (1);
  }

  g_set_application_name (_(APP_NAME_LONG));
    
  games_conf_initialise (APP_NAME);

  highscores = games_scores_new (APP_NAME,
                                 scorecats, G_N_ELEMENTS (scorecats),
                                 "board size", NULL,
                                 0 /* default category */,
                                 GAMES_SCORES_STYLE_TIME_ASCENDING);

  g_signal_connect (games_conf_get_default (), "value-changed",
                    G_CALLBACK (conf_value_changed_cb), NULL);

  gtk_window_set_default_icon_name ("mate-mines");

#ifdef WITH_SMCLIENT
  sm_client = egg_sm_client_get ();
  g_signal_connect (sm_client, "save-state",
		    G_CALLBACK (save_state_cb), NULL);
  g_signal_connect (sm_client, "quit",
                    G_CALLBACK (quit_cb), NULL);
#endif /* WITH_SMCLIENT */


  if (xsize == -1)
    xsize = games_conf_get_integer (KEY_GEOMETRY_GROUP, KEY_XSIZE, NULL);
  if (ysize == -1)
    ysize = games_conf_get_integer (KEY_GEOMETRY_GROUP, KEY_YSIZE, NULL);
  if (nmines == -1)
    nmines = games_conf_get_integer (KEY_GEOMETRY_GROUP, KEY_NMINES, NULL);
  if (fsize == -1)
    fsize = games_conf_get_integer (KEY_GEOMETRY_GROUP, KEY_MODE, NULL);
  use_question_marks =
        games_conf_get_boolean (NULL, KEY_USE_QUESTION_MARKS, NULL);
  use_overmine_warning =
        games_conf_get_boolean (NULL, KEY_USE_OVERMINE_WARNING, NULL);
  use_autoflag = games_conf_get_boolean (NULL, KEY_USE_AUTOFLAG, NULL);

  verify_ranges ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), _(APP_NAME_LONG));
    
  games_conf_add_window (GTK_WINDOW (window), NULL);

  games_stock_init ();

  gtk_window_set_resizable (GTK_WINDOW (window), TRUE);

  g_signal_connect (G_OBJECT (window), "delete_event",
		    G_CALLBACK (quit_game), NULL);
  g_signal_connect (G_OBJECT (window), "focus_out_event",
		    G_CALLBACK (pause_callback), NULL);
  g_signal_connect (G_OBJECT (window), "window_state_event",
		    G_CALLBACK (window_state_callback), NULL);

  // For pause/resume with the keyboard pause key
  g_signal_connect (G_OBJECT (window), "key_press_event", 
                    G_CALLBACK (pause_key_callback), NULL); 

  all_boxes = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (all_boxes);

  gtk_container_add (GTK_CONTAINER (window), all_boxes);

  ui_manager = create_ui_manager ("GnomineActions");
  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  box = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (all_boxes), box, FALSE, FALSE, 0);

  button_table = gtk_table_new (1, 3, FALSE);
  gtk_box_pack_end (GTK_BOX (all_boxes), button_table, TRUE, TRUE, 0);

  pm_current = NULL;

  mbutton = gtk_button_new ();
  g_signal_connect (G_OBJECT (mbutton), "clicked",
		    G_CALLBACK (new_game), NULL);
  gtk_table_attach (GTK_TABLE (button_table), mbutton, 1, 2, 0, 1,
		    0, 0, 5, 5);

  face_box = gtk_vbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (mbutton), face_box);

  pm_win = image_widget_setup ("face-win.svg");
  pm_sad = image_widget_setup ("face-sad.svg");
  pm_smile = image_widget_setup ("face-smile.svg");
  pm_cool = image_widget_setup ("face-cool.svg");
  pm_worried = image_widget_setup ("face-worried.svg");

  gtk_box_pack_start (GTK_BOX (face_box), pm_win, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (face_box), pm_sad, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (face_box), pm_smile, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (face_box), pm_cool, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (face_box), pm_worried, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 0);
  gtk_table_attach_defaults (GTK_TABLE (button_table), box, 1, 2, 1, 2);

  gtk_box_pack_start (GTK_BOX (box), gtk_hseparator_new (), FALSE, FALSE, 0);

  mfield = gtk_minefield_new ();

  /* These next two widgets are created as alignments, but it's more
   * important to know that these are containers, hence the names. */
  mfield_container = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_container_add (GTK_CONTAINER (mfield_container), mfield);
  gtk_box_pack_start (GTK_BOX (box), mfield_container, TRUE, TRUE, 0);

  resume_container = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (box), resume_container, TRUE, TRUE, 0);

  resume_button = gtk_button_new_with_label (_("Press to Resume"));
  g_signal_connect (G_OBJECT (resume_button), "clicked",
		    G_CALLBACK (resume_game_cb), NULL);
  gtk_container_add (GTK_CONTAINER (resume_container), resume_button);

  gtk_minefield_set_use_question_marks (GTK_MINEFIELD (mfield),
					use_question_marks);

  gtk_minefield_set_use_overmine_warning (GTK_MINEFIELD (mfield),
					  use_overmine_warning);
   
  gtk_minefield_set_use_autoflag (GTK_MINEFIELD (mfield),
				  use_autoflag);

  g_signal_connect (G_OBJECT (mfield), "marks_changed",
		    G_CALLBACK (marks_changed), NULL);
  g_signal_connect (G_OBJECT (mfield), "explode",
		    G_CALLBACK (lose_game), NULL);
  g_signal_connect (G_OBJECT (mfield), "win", G_CALLBACK (win_game), NULL);
  g_signal_connect (G_OBJECT (mfield), "look", G_CALLBACK (look_cell), NULL);
  g_signal_connect (G_OBJECT (mfield), "unlook",
		    G_CALLBACK (unlook_cell), NULL);
  g_signal_connect (G_OBJECT (mfield), "hint_used",
		    G_CALLBACK (hint_used), NULL);

  gtk_box_pack_start (GTK_BOX (box), gtk_hseparator_new (), FALSE, FALSE, 0);

  status_box = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), status_box, FALSE, FALSE, 8);

  flabel = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (status_box), flabel, FALSE, FALSE, 0);

  box = gtk_hbox_new (FALSE, 0);
  label = gtk_label_new (_("Time: "));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

  clk = games_clock_new ();
  gtk_box_pack_start (GTK_BOX (box), clk, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (status_box), box, FALSE, FALSE, 0);

  new_game ();

  gtk_widget_show_all (button_table);
  gtk_widget_show (window);

  /* Must be after the window has been created. */
  if (xpos >= 0 && ypos >= 0)
    gdk_window_move (gtk_widget_get_window (GTK_WIDGET (window)), xpos, ypos);

  /* All this hiding is a bit ugly, but it's better than a
   * ton of gtk_widget_show calls. */
  gtk_widget_hide (resume_container);
  gtk_widget_hide (pm_win);
  gtk_widget_hide (pm_sad);
  gtk_widget_hide (pm_cool);
  gtk_widget_hide (pm_worried);

  show_face (pm_smile);

  gtk_main ();
    
  games_conf_shutdown ();

  games_runtime_shutdown ();

  return 0;
}
