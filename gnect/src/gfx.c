/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */

/* gfx.c
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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libgames-support/games-runtime.h>

#include "main.h"
#include "theme.h"
#include "prefs.h"
#include "gfx.h"


extern Prefs p;
extern Theme theme[];
extern gint gboard[7][7];
extern GtkWidget *app;
extern GtkWidget *drawarea;

static gint boardsize = 0;
static gint tilesize = 0;
static gint offset[6];

/* unscaled pixbufs */
static GdkPixbuf *pb_tileset_raw = NULL;
static GdkPixbuf *pb_bground_raw = NULL;

/* scaled pixbufs */
static GdkPixbuf *pb_tileset = NULL;
static GdkPixbuf *pb_bground = NULL;

/* background pixmap, we draw the grid on it */
static GdkPixmap *pm_bground = NULL;

/* the buffer we draw in */
static GdkPixmap *pm_display = NULL;

static GdkGC *gc = NULL;
static GdkGC *solidgc = NULL;


void
gfx_free (void)
{
  if (pb_tileset_raw != NULL) {
    g_object_unref (pb_tileset_raw);
    pb_tileset = NULL;
  }
  if (pb_bground_raw != NULL) {
    g_object_unref (pb_bground_raw);
    pb_bground = NULL;
  }
  if (pb_tileset != NULL) {
    g_object_unref (pb_tileset);
    pb_tileset = NULL;
  }
  if (pb_bground != NULL) {
    g_object_unref (pb_bground);
    pb_bground = NULL;
  }
  if (pm_bground != NULL) {
    g_object_unref (pm_bground);
    pm_bground = NULL;
  }
  if (pm_display != NULL) {
    g_object_unref (pm_display);
    pm_display = NULL;
  }
}



gint
gfx_get_column (gint xpos)
{
  /* Derive column from pixel position */
  gint c = xpos / tilesize;
  if (c > 6)
    c = 6;
  if (c < 0)
    c = 0;

  return c;
}



void
gfx_draw_tile (gint r, gint c, gboolean refresh)
{
  gint x = c * tilesize;
  gint y = r * tilesize;
  gint tile = gboard[r][c];
  gint os = 0;

  g_return_if_fail (gc != NULL);

  switch (tile) {
  case TILE_PLAYER1:
    if (r == 0)
      os = offset[TILE_PLAYER1_CURSOR];
    else
      os = offset[TILE_PLAYER1];
    break;
  case TILE_PLAYER2:
    if (r == 0)
      os = offset[TILE_PLAYER2_CURSOR];
    else
      os = offset[TILE_PLAYER2];
    break;
  default:
    break;
  }

  gdk_draw_drawable (pm_display, gc, pm_bground, x, y, x, y, tilesize,
		     tilesize);

  if (tile != TILE_CLEAR) {
    gdk_draw_pixbuf (pm_display, NULL, pb_tileset,
                     os, 0, x, y, tilesize, tilesize,
                     GDK_RGB_DITHER_NORMAL, 0, 0);
  }

  if (refresh) {
    gtk_widget_queue_draw_area (drawarea, x, y, tilesize, tilesize);
  }
}



void
gfx_draw_all (void)
{
  gint r, c;

  for (r = 0; r < 7; r++) {
    for (c = 0; c < 7; c++) {
      gfx_draw_tile (r, c, FALSE);
    }
  }

  gtk_widget_queue_draw_area (drawarea, 0, 0, boardsize, boardsize);
}



void
gfx_refresh_pixmaps (void)
{
  gint i;

  g_return_if_fail (gc != NULL);

  /* scale the pixbufs */
  if (pb_tileset)
    g_object_unref (pb_tileset);
  if (pb_bground)
    g_object_unref (pb_bground);

  pb_tileset = gdk_pixbuf_scale_simple (pb_tileset_raw,
					tilesize * 6, tilesize,
					GDK_INTERP_BILINEAR);
  pb_bground = gdk_pixbuf_scale_simple (pb_bground_raw,
					boardsize, boardsize,
					GDK_INTERP_BILINEAR);

  /* draw the background */
  gdk_draw_pixbuf (pm_bground, gc, pb_bground,
		   0, 0, 0, 0, -1, -1, GDK_RGB_DITHER_NORMAL, 0, 0);

  /* draw the grid on the background pixmap */
  for (i = 1; i < 7; i++) {
    gdk_draw_line (pm_bground, gc, i * tilesize, 0, i * tilesize, boardsize);
    gdk_draw_line (pm_bground, gc, 0, i * tilesize, boardsize, i * tilesize);
  }
  /* Mark the top off with a solid line. */
  gdk_draw_line (pm_bground, solidgc, 0, tilesize, boardsize, tilesize);
}



void
gfx_resize (GtkWidget * w)
{
  GtkAllocation allocation;

  gtk_widget_get_allocation (w, &allocation);

  boardsize = MIN (allocation.width, allocation.height);
  tilesize = boardsize / 7;

  offset[TILE_PLAYER1] = 0;
  offset[TILE_PLAYER2] = tilesize;
  offset[TILE_CLEAR] = tilesize * 2;
  offset[TILE_CLEAR_CURSOR] = tilesize * 3;
  offset[TILE_PLAYER1_CURSOR] = tilesize * 4;
  offset[TILE_PLAYER2_CURSOR] = tilesize * 5;

  /* create the buffer and background pixmap of the proper size */
  if (pm_display != NULL)
    g_object_unref (pm_display);
  if (pm_bground != NULL)
    g_object_unref (pm_bground);

  pm_display = gdk_pixmap_new (gtk_widget_get_window (w), boardsize, boardsize, -1);
  pm_bground = gdk_pixmap_new (gtk_widget_get_window (w), boardsize, boardsize, -1);

  /* the first time the configure signal is emitted, the drawarea
   * is not shown yet so we do not have the gc.
   */
  if (gc) {
    gfx_refresh_pixmaps ();
    gfx_draw_all ();
  }
}



void
gfx_expose (GdkRectangle * area)
{
  gdk_draw_drawable (GDK_DRAWABLE (gtk_widget_get_window (drawarea)),
                     gc,
                     pm_display,
                     area->x, area->y,
                     area->x, area->y, area->width, area->height);
}



static void
gfx_load_error (const gchar * fname)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (GTK_WINDOW (app),
				   GTK_DIALOG_MODAL,
				   GTK_MESSAGE_WARNING,
				   GTK_BUTTONS_CLOSE,
				   _("Unable to load image:\n%s"), fname);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}


gboolean
gfx_load_pixmaps (void)
{
  const char *dname;
  gchar *fname;
  GdkPixbuf *pb_tileset_tmp;
  GdkPixbuf *pb_bground_tmp = NULL;

  dname = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);
  /* Try the theme pixmaps, fallback to the default and then give up */
  while (TRUE) {
    fname = g_build_filename (dname, theme[p.theme_id].fname_tileset, NULL);
    pb_tileset_tmp = gdk_pixbuf_new_from_file (fname, NULL);
    if (pb_tileset_tmp == NULL) {
      if (p.theme_id != 0) {
	p.theme_id = 0;
	g_free (fname);
	continue;
      } else {
	gfx_load_error (fname);
	g_free (fname);
	return FALSE;
      }
    }

    g_free (fname);
    break;
  }

  if (pb_tileset_raw)
    g_object_unref (pb_tileset_raw);

  pb_tileset_raw = pb_tileset_tmp;

  if (theme[p.theme_id].fname_bground != NULL) {
    fname = g_build_filename (dname, theme[p.theme_id].fname_bground, NULL);
    pb_bground_tmp = gdk_pixbuf_new_from_file (fname, NULL);
    if (pb_bground_tmp == NULL) {
      gfx_load_error (fname);
      g_object_unref (pb_tileset_tmp);
      g_free (fname);
      return FALSE;
    }
    g_free (fname);
  }

  if (pb_bground_raw)
    g_object_unref (pb_bground_raw);

  /* If a separate background image wasn't supplied,
   * derive the background image from the tile set
   */
  if (pb_bground_tmp != NULL) {
    pb_bground_raw = pb_bground_tmp;
  } else {
    gint tilesize_raw;
    gint i, j;

    tilesize_raw = gdk_pixbuf_get_height (pb_tileset_raw);

    pb_bground_raw = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
				     tilesize_raw * 7, tilesize_raw * 7);
    for (i = 0; i < 7; i++) {
      gdk_pixbuf_copy_area (pb_tileset_raw,
			    tilesize_raw * 3, 0,
			    tilesize_raw, tilesize_raw,
			    pb_bground_raw, i * tilesize_raw, 0);
      for (j = 1; j < 7; j++) {
	gdk_pixbuf_copy_area (pb_tileset_raw,
			      tilesize_raw * 2, 0,
			      tilesize_raw, tilesize_raw,
			      pb_bground_raw,
			      i * tilesize_raw, j * tilesize_raw);
      }
    }
  }

  return TRUE;
}



gboolean
gfx_set_grid_style (void)
{
  GdkColormap *cmap;
  GdkColor color;

  g_return_val_if_fail (drawarea != NULL, FALSE);

  if (!gc) {
    gc = gdk_gc_new (gtk_widget_get_window (drawarea));
    solidgc = gdk_gc_new (gtk_widget_get_window (drawarea));
  }

  if (theme[p.theme_id].grid_rgb == NULL)
    return FALSE;

  if (!gdk_color_parse (theme[p.theme_id].grid_rgb, &color))
    gdk_color_parse ("#727F8C", &color);

  cmap = gtk_widget_get_colormap (drawarea);
  gdk_colormap_alloc_color (cmap, &color, FALSE, TRUE);

  gdk_gc_set_foreground (gc, &color);

  gdk_colormap_free_colors (cmap, &color, 1);
  g_object_unref (cmap);

  gdk_gc_set_line_attributes (gc, 0, theme[p.theme_id].grid_style,
			      GDK_CAP_BUTT, GDK_JOIN_MITER);

  gdk_gc_copy (solidgc, gc);
  gdk_gc_set_line_attributes (solidgc, 0, GDK_LINE_SOLID,
			      GDK_CAP_BUTT, GDK_JOIN_MITER);


  return TRUE;
}



gboolean
gfx_change_theme (void)
{
  if (!gfx_load_pixmaps ())
    return FALSE;

  if (!gfx_set_grid_style ())
    return FALSE;

  gfx_refresh_pixmaps ();
  gfx_draw_all ();

  return TRUE;
}
