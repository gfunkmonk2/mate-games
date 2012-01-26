/* drawing.c : Code to draw the tiles.
 *
 * Copyright (C) 2003 by Callum McKenzie
 *
 * Created: <2003-09-07 05:02:22 callum>
 * Time-stamp: <2006-04-29 21:53:02 callum>
 *
 */

/* We store two large pixmaps in the X server. One has a copy of all the
 * tile foregrounds composited against the normal background. The other
 * is composited against the "selected tile" background. Any drawing is
 * then done from these onto a buffer pixmap and finally onto the
 * actual window. */
/* This is a step back from the old way which used mate-canvas, but
 * the overhead was quite noticeable. */

#include <config.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libgames-support/games-gtk-compat.h>
#include <libgames-support/games-preimage.h>
#include <libgames-support/games-runtime.h>

#include "mahjongg.h"
#include "drawing.h"

typedef struct _view_geom_record {
  gint x;
  gint y;
  guint noverlaps;
  guchar overlaps[MAX_TILES - 1];
} view_geom_record;

view_geom_record view_geometry[MAX_TILES];

/* The number of different tile patterns plus a blank tile at the end. */
#define NUM_PATTERNS 43

/* Aspect ratio of individual tiles (i.e., height/width).
 * 1.375 = 64/88 */
#define ASPECT 1.375

GtkWidget *board = NULL;
GdkPixmap *buffer = NULL;
GdkPixmap *tileimages = NULL;
GdkPixmap *tilebuffer = NULL;
GdkBitmap *tilemask = NULL;

GdkGC *gc = NULL;

gboolean update_tileimages = TRUE;
gboolean nowindow = TRUE;

GdkColor bgcolour;

GamesPreimage *tilepreimage = NULL;

static gint windowwidth;
static gint windowheight;
gint prior_tilebasewidth = 0;
gint tilebasewidth = 0;
gint tilebaseheight = 0;
gint tileoffsetx;
gint tileoffsety;
gint tilewidth;
gint tileheight;
gint xoffset;
gint yoffset;

/* This is the minimum size of the widget containing the tiles. These
 * numbers are completely arbitrary and any resemblance to the resolution
 * of the basic VGA 256 colour mode is a sign of a mis-spent youth. */
#define MINWIDTH 320
#define MINHEIGHT 200

/* These two are in units of tiles and do not include a half tile border. */
static gint gridwidth = 0, gridheight = 0;

static gchar *warning_message = NULL;

static void
recalculate_sizes (gint width, gint height)
{
  gdouble scale;

  /* This calculates four things: the size of the complete tile pixmap,
   * the offsets from the edge of the window, the offset for the 3-D effect
   * (i.e. the sides of the tile) and the size of the face of the tile. */
  scale = MIN (width / gridwidth, height / (gridheight * ASPECT));
  tilebasewidth = scale;
  tilebaseheight = scale * ASPECT;
  xoffset = (width - (gridwidth - 1) * tilebasewidth) / 2;
  yoffset = (height - (gridheight - 1) * tilebaseheight) / 2;
  tileoffsetx = tilebasewidth / 7;
  tileoffsety = tilebaseheight / 10;
  tilewidth = tilebasewidth + tileoffsetx;
  tileheight = tilebaseheight + tileoffsety;
}

static void
calculate_tile_positions (void)
{
  int i;
  view_geom_record *v;

  v = view_geometry;
  for (i = 0; i < MAX_TILES; i++) {
    v->x =
      pos[i].x * tilebasewidth / 2 + pos[i].layer * tileoffsetx + xoffset;
    v->y =
      pos[i].y * tilebaseheight / 2 - pos[i].layer * tileoffsety + yoffset;
    v++;
  }
}

void
calculate_view_geometry (void)
{
  gint i, j;
  view_geom_record *v, *v2;

  gridwidth = 0;
  gridheight = 0;

  if (tilebasewidth == 0) {
    /* We may not yet have a valid window geometry, so supply some dummy
     * data. */
    tilewidth = 64;
    tileheight = 88;
    tileoffsetx = tileoffsety = 8;
    tilebasewidth = tilewidth - tileoffsetx;
    tilebaseheight = tileheight - tileoffsety;
    xoffset = yoffset = 0;
  }

  calculate_tile_positions ();

  v = view_geometry;
  for (i = 0; i < MAX_TILES; i++) {
    if (pos[i].x > gridwidth)
      gridwidth = pos[i].x;
    if (pos[i].y > gridheight)
      gridheight = pos[i].y;
    v->noverlaps = 0;
    v2 = view_geometry;
    for (j = 0; j < MAX_TILES; j++) {
      /* We include the tile as an overlap with itself. This simplifies the
       * drawing routines later. */
      if ((((v2->x >= v->x) && (v2->x < v->x + tilewidth)) ||
	   ((v2->x < v->x) && (v2->x + tilewidth > v->x))) &&
	  (((v2->y >= v->y) && (v2->y < v->y + tileheight)) ||
	   ((v2->y < v->y) && (v2->y + tileheight > v->y)))) {
	v->overlaps[v->noverlaps] = j;
	v->noverlaps++;
      }
      v2++;
    }
    v++;
  }

  /* The +2 allows for both a half-tile border and the fact that the
   * position information is for the upper left corner. */
  gridwidth = gridwidth / 2 + 2;
  gridheight = gridheight / 2 + 2;
}

static gint
find_tile (guint x, guint y)
{
  gint i;
  gint tx, ty;

  /* FIXME: this is a really naive way to do things. */
  /* Because of the ordering of things, this gets the top tile first. */
  for (i = 0; i < MAX_TILES; i++) {
    if (tiles[i].visible) {
      tx = view_geometry[i].x;
      ty = view_geometry[i].y;
      if ((x >= tx) && (x < (tx + tilewidth)) &&
	  (y >= ty) && (y < (ty + tileheight))) {
	return i;
      }
    }
  }

  return -1;
}

void
set_background (gchar * colour)
{
  if (!colour || !gdk_color_parse (colour, &bgcolour)) {
    bgcolour.red = bgcolour.green = bgcolour.blue = 0;
  }

  if (gc) {
    gdk_colormap_alloc_color (gdk_colormap_get_system (), &bgcolour, FALSE,
			      TRUE);
    gdk_gc_set_foreground (gc, &bgcolour);
  }
}

void
draw_tile (gint tileno)
{
  guint ox, oy;
  guint dx, dy;
  guint sx, sy;
  gint i, j;
   
  if (nowindow)
    return;

  ox = view_geometry[tileno].x;
  oy = view_geometry[tileno].y;
  gdk_gc_set_clip_mask (gc, tilemask);
  gdk_gc_set_clip_origin (gc, 0, 0);

  gdk_draw_rectangle (tilebuffer, gc, TRUE, 0, 0, tilewidth, tileheight);

  for (i = view_geometry[tileno].noverlaps - 1; i >= 0; i--) {
    j = view_geometry[tileno].overlaps[i];
    if (tiles[j].visible) {
      dx = view_geometry[j].x - ox;
      dy = view_geometry[j].y - oy;
      sy = tiles[j].selected ? tileheight : 0;
      sx = tiles[j].image * tilewidth;
      gdk_gc_set_clip_origin (gc, dx, dy);
      gdk_draw_drawable (tilebuffer, gc, tileimages,
			 sx, sy, dx, dy, tilewidth, tileheight);
    }
  }

  gdk_gc_set_clip_origin (gc, ox, oy);
  gdk_draw_drawable (buffer, gc, tilebuffer, 0, 0, ox, oy,
		     tilewidth, tileheight);

  /* We could queue this draw, but given that this function is at worst case
   * called twice in a short time span it doesn't seem worth the code. */
  gdk_draw_drawable (gtk_widget_get_window (board),
                     gtk_widget_get_style (board)->black_gc, buffer, ox, oy,
		     ox, oy, tilewidth, tileheight);
}

void
draw_all_tiles (void)
{
  gint i;
  guint sx, sy;
  guint dx, dy;
   
  if (nowindow)
    return;

  gdk_gc_set_clip_mask (gc, NULL);
  gdk_draw_rectangle (buffer, gc, TRUE, 0, 0, windowwidth, windowheight);

  /* This works because of the way the tiles are sorted. We could
   * reverse them to make this look a little nicer, but when searching
   * for a tile we want it the other way around. */

  gdk_gc_set_clip_mask (gc, tilemask);
  for (i = MAX_TILES - 1; i >= 0; i--) {
    if (!tiles[i].visible)
      continue;

    dx = view_geometry[i].x;
    dy = view_geometry[i].y;

    if (paused) {
      sx = tilewidth * (NUM_PATTERNS - 1);
      sy = 0;
    } else {
      sx = tiles[i].image * tilewidth;
      sy = tiles[i].selected ? tileheight : 0;
    }

    gdk_gc_set_clip_origin (gc, dx, dy);

    gdk_draw_drawable (buffer, gc, tileimages,
		       sx, sy, dx, dy, tilewidth, tileheight);
  }

  gtk_widget_queue_draw (board);
}

static void
recreate_tile_images (void)
{
  GdkPixbuf *fg = NULL;

  /* Now composite the tiles across it. */

  if (tilepreimage) {
    fg = games_preimage_render (tilepreimage, tilewidth * NUM_PATTERNS,
				tileheight * 2);

    /* Handle corrupt images that were not caught during preimage creation. */
    if (fg == NULL) {
      g_object_unref (tilepreimage);
      tilepreimage = NULL;

      g_free (warning_message);
      warning_message = g_strdup (_("The selected theme failed to render.\n\n"
				    "Please check that Mahjongg is installed correctly."));
    }

  }

  /* If there's a warning, display it. */
  if (warning_message) {
    mahjongg_theme_warning (warning_message);
    g_free (warning_message);
    warning_message = NULL;
  }

  /* If there's no theme pixbuf, provide a blank one. */
  if (fg == NULL) {
    fg = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
			 tilewidth * NUM_PATTERNS, tileheight * 2);
    gdk_pixbuf_fill (fg, 0x00000000);
  }

  gdk_pixbuf_render_threshold_alpha (fg, tilemask, 0, 0, 0, 0,
				     tilewidth, tileheight, 128);
  gdk_draw_pixbuf (tileimages, NULL, fg, 0, 0, 0, 0,
		   tilewidth * NUM_PATTERNS, tileheight * 2,
		   GDK_RGB_DITHER_MAX, 0, 0);

  g_object_unref (fg);

}

/* This is for when the geometry changes. It is called both from the
 * normal configure event handler and from code which detects when
 * the "internal" geometry (i.e. the layout) has changed. */
void
configure_pixmaps (void)
{
  if (nowindow)
    return;

  prior_tilebasewidth = tilebasewidth;

  recalculate_sizes (windowwidth, windowheight);
  calculate_tile_positions ();

  if (buffer != NULL)
    g_object_unref (buffer);
  buffer = gdk_pixmap_new (gtk_widget_get_window (board), windowwidth, 
                           windowheight, -1);

  /* Recreate the tile images only if the theme or tile size changed. */
  if ((prior_tilebasewidth != tilebasewidth) || (update_tileimages)) {

    if (tileimages != NULL)
      g_object_unref (tileimages);
    if (tilemask != NULL)
      g_object_unref (tilemask);
    if (tilebuffer != NULL)
      g_object_unref (tilebuffer);

    tileimages = gdk_pixmap_new (gtk_widget_get_window (board),
                                 NUM_PATTERNS * tilewidth, 2 * tileheight, -1);
    tilemask = gdk_pixmap_new (NULL, tilewidth, tileheight, 1);
    tilebuffer = gdk_pixmap_new (gtk_widget_get_window (board), tilewidth,
                                 tileheight, -1);

    recreate_tile_images ();
    update_tileimages = FALSE;
  }
}

/* Here is where we create the backing pixmap and set up the tile pixmaps. */
static gboolean
configure_board (GtkWidget * w, GdkEventConfigure * e, gpointer data)
{
  nowindow = FALSE;

  if (gc == NULL) {
    gc = gdk_gc_new (gtk_widget_get_window (w));
    gdk_gc_copy (gc, gtk_widget_get_style (w)->black_gc);
    gdk_colormap_alloc_color (gdk_colormap_get_system (), &bgcolour, FALSE,
			      TRUE);
    gdk_gc_set_foreground (gc, &bgcolour);
  }

  windowwidth = e->width;
  windowheight = e->height;

  configure_pixmaps ();

  draw_all_tiles ();

  return FALSE;
}

/* Handle exposes by dumping out the backing pixmap. */
static gboolean
expose_board (GtkWidget * w, GdkEventExpose * e, gpointer data)
{
  gdk_draw_drawable (gtk_widget_get_window (w),
                     gtk_widget_get_style (w)->black_gc, buffer, e->area.x,
		     e->area.y, e->area.x, e->area.y, e->area.width,
                     e->area.height);
  return TRUE;
}

static void
board_click (GtkWidget * w, GdkEventButton * e, gpointer data)
{
  gint tileno;

  /* Ignore the 2BUTTON and 3BUTTON events. */
  if (e->type != GDK_BUTTON_PRESS)
    return;

  tileno = find_tile (e->x, e->y);

  if (tileno < 0)
    return;

  tile_event (tileno, e->button);
}

/* Create the widget. */
/* This is a public routine. */
GtkWidget *
create_mahjongg_board (void)
{
  board = gtk_drawing_area_new ();
  gtk_widget_set_can_focus (board, TRUE);

  gtk_widget_set_size_request (board, MINWIDTH, MINHEIGHT);

  gtk_widget_add_events (board, GDK_BUTTON_PRESS_MASK);

  g_signal_connect (G_OBJECT (board), "expose_event",
		    G_CALLBACK (expose_board), NULL);
  g_signal_connect (G_OBJECT (board), "configure_event",
		    G_CALLBACK (configure_board), NULL);
  g_signal_connect (G_OBJECT (board), "button_press_event",
		    G_CALLBACK (board_click), NULL);
  /* We do our own double-buffering. */
  gtk_widget_set_double_buffered (board, FALSE);

  return board;
}

/* Load the selected images. */
void
load_images (gchar * file)
{
  char *filename;
  const char *dname;
  GError *error = NULL;

  if (tilepreimage)
    g_object_unref (tilepreimage);

  dname =  games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);

  filename = g_build_filename (dname, file, NULL);
  tilepreimage = games_preimage_new_from_file (filename, &error);
  g_free (filename);

  if (!tilepreimage) {
    warning_message = g_strdup_printf (_("Unable to render file:\n'%s'\n\n"
                                         "Please check that Mahjongg is installed correctly."),
                                       error->message);
    g_clear_error (&error);

    /* Try the default tileset */
    filename = g_build_filename (dname, "postmodern.svg", NULL);
    tilepreimage = games_preimage_new_from_file (filename, &error);
    g_free (filename);

    if (!tilepreimage) {
      /* This usually happens with bad images or a missing image loader. */
      g_free (warning_message);
      warning_message = g_strdup_printf (_("Unable to render file:\n'%s'\n\n"
					   "Please check that Mahjongg is installed correctly."),
					 error->message);
      g_error_free (error);
    }
  }

  update_tileimages = TRUE;

  g_free (tileset);
  tileset = g_strdup (file);

  /* We may be called before the window is created, in which case we let
   * the configure callback handle this. But if this is a change of tileset
   * we need to do this. */
  if (buffer)
    recreate_tile_images ();
}

/* EOF */
