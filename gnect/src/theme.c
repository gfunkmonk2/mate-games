/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */

/* theme.c
 *
 * Four-in-a-row for MATE
 * (C) 2000 - 2004
 * Authors: Timothy Musson <trmusson@ihug.co.nz>
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "main.h"
#include "theme.h"
#include "prefs.h"


#define DEFAULT_GRID_STYLE     GDK_LINE_ON_OFF_DASH


extern Prefs p;


Theme theme[] = {
  {
   N_("Classic"),
   "tileset_50x50_sunspot.svg",
   NULL,
   "#9999CC",
   DEFAULT_GRID_STYLE,
   N_("Red"), N_("Yellow")
   },
  {
   N_("High Contrast"),
   "tileset_50x50_hcontrast.svg",
   NULL,
   "#000000",
   DEFAULT_GRID_STYLE,
   N_("Light"), N_("Dark")
   },
  {
   N_("High Contrast Inverse"),
   "tileset_50x50_hcinverse.svg",
   NULL,
   "#FFFFFF",
   DEFAULT_GRID_STYLE,
   N_("Light"), N_("Dark")
   },
  {
   N_("Cream Marbles"),
   "tileset_50x50_catseyes.png",
   "bg_toplight.png",
   "#727F8C",
   DEFAULT_GRID_STYLE,
   N_("Red"), N_("Blue")
   },
  {
   N_("Glass Marbles"),
   "tileset_50x50_glassy.png",
   "bg_grotty.jpg",
   "#727F8C",
   DEFAULT_GRID_STYLE,
   N_("Red"), N_("Blue")
   },
  {
   N_("Nightfall"),
   "tileset_50x50_glassy.png",
   "bg_nightfall.jpg",
   "#727F8C",
   DEFAULT_GRID_STYLE,
   N_("Red"), N_("Yellow")
   },
  {
   N_("Blocks"),
   "tileset_50x50_square.png",
   NULL,
   "#666666",
   DEFAULT_GRID_STYLE,
   N_("Orange"), N_("Blue")
   }
};

gint n_themes = G_N_ELEMENTS (theme);



const gchar *
theme_get_player (PlayerID who)
{
  if (who == PLAYER1)
    return theme[p.theme_id].player1;
  return theme[p.theme_id].player2;
}



const gchar *
theme_get_title (gint id)
{
  return theme[id].title;
}
