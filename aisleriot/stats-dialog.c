/*
 * Copyright © 2003 Callum McKenzie <callum@physics.otago.ac.nz>
 * Copyright © 2007 Christian Persch
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <glib/gi18n.h>

#include <gtk/gtk.h>

#include <libgames-support/games-gtk-compat.h>
#include <libgames-support/games-stock.h>

#ifndef HAVE_HILDON
#include <libgames-support/games-atk-utils.h>
#endif

#include "conf.h"
#include "util.h"

#include "stats-dialog.h"

#define AISLERIOT_STATS_DIALOG_GET_PRIVATE(stats_dialog)(G_TYPE_INSTANCE_GET_PRIVATE ((stats_dialog), AISLERIOT_TYPE_STATS_DIALOG, AisleriotStatsDialogPrivate))

struct _AisleriotStatsDialogPrivate
{
  GtkLabel *game_label;
  GtkLabel *wins_label;
  GtkLabel *total_label;
  GtkLabel *percentage_label;
  GtkLabel *best_label;
  GtkLabel *worst_label;
};

G_DEFINE_TYPE (AisleriotStatsDialog, aisleriot_stats_dialog, GTK_TYPE_DIALOG);

/* helper functions */

static void
pack_in_frame (GtkWidget *box,
               GtkWidget *content,
               const char *text)
{
  GtkWidget *frame, *alignment, *label;
  char *markup;

  markup = g_markup_printf_escaped ("<b>%s</b>", text);
  label = gtk_label_new (markup);
  g_free (markup);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  frame = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (frame), label, FALSE, FALSE, 0);
  
  alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment),
                             0, 0, 12, 0);
  gtk_box_pack_start (GTK_BOX (frame), alignment, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (alignment), content);

  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, FALSE, 0);
  gtk_widget_show_all (frame);

#ifndef HAVE_HILDON
  games_atk_util_add_atk_relation (label, frame, ATK_RELATION_LABEL_FOR);
  games_atk_util_add_atk_relation (frame, label, ATK_RELATION_LABELLED_BY);
#endif /* !HAVE_HILDON */
}

static GtkLabel *
add_row (GtkTable *table,
         int row,
         const char *text)
{
  GtkWidget *label, *data_label;

  label = gtk_label_new (text);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (table, label,
                    0, 1, row, row + 1,
                    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  data_label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (data_label), 0.0, 0.5);
  gtk_label_set_selectable (GTK_LABEL (data_label), TRUE);
  gtk_table_attach_defaults (table, data_label,
                             1, 2, row, row + 1);

#ifndef HAVE_HILDON
  games_atk_util_add_atk_relation (label, data_label, ATK_RELATION_LABEL_FOR);
  games_atk_util_add_atk_relation (data_label, label, ATK_RELATION_LABELLED_BY);
#endif /* !HAVE_HILDON */

  return GTK_LABEL (data_label);
}

/* Class implementation */

static void
aisleriot_stats_dialog_init (AisleriotStatsDialog *stats_dialog)
{
  AisleriotStatsDialogPrivate *priv;
  GtkDialog *dialog = GTK_DIALOG (stats_dialog);
  GtkWidget *vbox, *hbox, *content_area;
  GtkTable *table;

  priv = stats_dialog->priv = AISLERIOT_STATS_DIALOG_GET_PRIVATE (stats_dialog);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  content_area = gtk_dialog_get_content_area (dialog);

  gtk_dialog_set_has_separator (dialog, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_box_set_spacing (GTK_BOX (content_area), 2);

  vbox = gtk_vbox_new (FALSE, 18);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  gtk_box_pack_start (GTK_BOX (content_area), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  priv->game_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_label_set_use_markup (priv->game_label, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (priv->game_label),
                      FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (priv->game_label));

  hbox = gtk_hbox_new (TRUE, 18);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  table = GTK_TABLE (gtk_table_new (3, 2, FALSE));
  gtk_table_set_row_spacings (table, 6);
  gtk_table_set_col_spacings (table, 12);

  /* Translators: this is the total number of won games */
  priv->wins_label = add_row (table, 0, _("Wins:"));
  /* Translators: this is the number of games played */
  priv->total_label = add_row (table, 1, _("Total:"));
  /* Translators: this is the percentage of games won out of all games played */
  priv->percentage_label = add_row (table, 2, _("Percentage:"));
  /* Translators: this is the section title of a section which contains the n
   * number of games played, number of games won, and the ratio of these 2 numbers.
   */
  pack_in_frame (hbox, GTK_WIDGET (table), _("Wins"));

  table = GTK_TABLE (gtk_table_new (2, 2, FALSE));
  gtk_table_set_row_spacings (table, 6);
  gtk_table_set_col_spacings (table, 12);

  /* Translators: this is the best time of all wins */
  priv->best_label = add_row (table, 0, _("Best:"));
  /* Translators: this is the worst time of all wins */
  priv->worst_label = add_row (table, 1, _("Worst:"));
  /* Translators: this is the section title of a section containing the
   * best and worst time taken to win a game.
   */
  pack_in_frame (hbox, GTK_WIDGET (table), _("Time"));

  gtk_dialog_add_buttons (dialog,
                          GAMES_STOCK_RESET, GTK_RESPONSE_REJECT,
                          GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                          NULL);
  gtk_dialog_set_alternative_button_order (dialog,
                                           GTK_RESPONSE_CLOSE,
                                           GTK_RESPONSE_REJECT,
                                           -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

#ifndef HAVE_MAEMO
  /* Empty title shows up as "<unnamed>" on maemo */
  gtk_window_set_title (GTK_WINDOW (dialog), "");
#endif
}

static void
aisleriot_stats_dialog_class_init (AisleriotStatsDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (gobject_class, sizeof (AisleriotStatsDialogPrivate));
}

/* public API */

AisleriotStatsDialog *
aisleriot_stats_dialog_new (void)
{
  return g_object_new (AISLERIOT_TYPE_STATS_DIALOG,
                       "title", _("Statistics"),
                       NULL);
}

void
aisleriot_stats_dialog_update (AisleriotStatsDialog *dialog,
                               AisleriotStatistic *current_stats)
{
  AisleriotStatsDialogPrivate *priv = dialog->priv;
  char text[128];

  /* Translators: Translate this to "%Id" if you want to use localised digits,
   * and to "%d" otherwise. Do not translate it to anything else!
   */
  g_snprintf (text, sizeof (text), _("%d"), current_stats->wins);
  gtk_label_set_text (priv->wins_label, text);

  /* Translators: Translate this to "%Id" if you want to use localised digits,
   * and to "%d" otherwise. Do not translate it to anything else!
   */
  g_snprintf (text, sizeof (text), _("%d"), current_stats->total);
  gtk_label_set_text (priv->total_label, text);

  if (current_stats->total != 0) {
    /* Translators: Translate the "%d" in this string this to "%Id" if you
     * want to use localised digits, and to "%d" otherwise.
     * Do not translate the "%d" part to anything else!
     * You may translate the "%%" part to use any other percent character(s)
     * instead, or leave it as "%%". If you chose a character other than
     * "%" (U+0025 PERCENT SIGN) you do NOT need to escape it with another "%"!
     */
    g_snprintf (text, sizeof (text), _("%d%%"),
                (int) (100.0 * ((double) current_stats->wins) / ((double) current_stats->total) + 0.5));
    gtk_label_set_text (priv->percentage_label, text);
  } else
    /* For translators: N/A means "Not Applicable", use whatever
     * abbreviation you have for a value that has no meaning. */
    gtk_label_set_text (priv->percentage_label, _("N/A"));

  if (current_stats->best != 0) {
    /* Translators: this represents minutes:seconds. */
    g_snprintf (text, sizeof (text), _("%d:%02d"),
                current_stats->best / 60,
                current_stats->best % 60);
    gtk_label_set_text (priv->best_label, text);
  } else
    gtk_label_set_text (priv->best_label, _("N/A"));

  if (current_stats->worst != 0) {
    g_snprintf (text, sizeof (text), _("%d:%02d"),
                current_stats->worst / 60,
                current_stats->worst % 60);
    gtk_label_set_text (priv->worst_label, text);
  } else
    gtk_label_set_text (priv->worst_label, _("N/A"));
}

void
aisleriot_stats_dialog_set_name (AisleriotStatsDialog *dialog,
                                 const char *game_name)
{
  AisleriotStatsDialogPrivate *priv = dialog->priv;
  char *markup;

  markup = g_markup_printf_escaped ("<b>%s</b>", game_name);
  gtk_label_set_markup (priv->game_label, markup);
  g_free (markup);
}
