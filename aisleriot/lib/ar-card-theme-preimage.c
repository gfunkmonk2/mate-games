/*
  Copyright © 2004 Callum McKenzie
  Copyright © 2007, 2008 Christian Persch

  This library is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Authors:   Callum McKenzie <callum@physics.otago.ac.nz> */

#include <config.h>

#include <string.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#ifdef HAVE_RSVG
#include <librsvg/librsvg-features.h>
#endif

#include <libgames-support/games-preimage.h>
#include <libgames-support/games-runtime.h>
#include <libgames-support/games-string-utils.h>

#include "ar-card-theme.h"
#include "ar-card-theme-private.h"

#define N_ROWS ((double) 5.0)
#define N_COLS ((double) 13.0)

#define DELTA (0.0f)

/* Class implementation */

G_DEFINE_ABSTRACT_TYPE (ArCardThemePreimage, ar_card_theme_preimage, AR_TYPE_CARD_THEME);

void
_ar_card_theme_preimage_clear_sized_theme_data (ArCardThemePreimage *theme)
{
  void (* clear_sized_theme_data) (ArCardThemePreimage *) =
    AR_CARD_THEME_PREIMAGE_GET_CLASS (theme)->clear_sized_theme_data;

  if (clear_sized_theme_data)
    clear_sized_theme_data (theme);
}

static gboolean
ar_card_theme_preimage_load (ArCardTheme *card_theme,
                                GError **error)
{
  ArCardThemePreimage *theme = (ArCardThemePreimage *) card_theme;
  ArCardThemeInfo *theme_info = card_theme->theme_info;
  const char *slot_dir;
  char *path;

  /* First the slot image */
  /* FIXMEchpe: use uninstalled data dir for rendering the card theme! */
  slot_dir = games_runtime_get_directory (GAMES_RUNTIME_PIXMAP_DIRECTORY);
  path = g_build_filename (slot_dir, "slot.svg", NULL);
  theme->slot_preimage = games_preimage_new_from_file (path, error);
  g_free (path);
  if (!theme->slot_preimage)
    return FALSE;


  /* Now the main course */
  path = g_build_filename (theme_info->path, theme_info->filename, NULL);
  theme->cards_preimage = games_preimage_new_from_file (path, error);
  g_free (path);
  if (!theme->cards_preimage)
    return FALSE;

  if (AR_CARD_THEME_PREIMAGE_GET_CLASS (theme)->needs_scalable_cards &&
      !games_preimage_is_scalable (theme->cards_preimage)) {
    g_set_error (error, AR_CARD_THEME_ERROR, AR_CARD_THEME_ERROR_NOT_SCALABLE,
                 "Theme is not scalable");
    return FALSE;
  }

  if (theme->font_options) {
    games_preimage_set_font_options (theme->slot_preimage, theme->font_options);
    games_preimage_set_font_options (theme->cards_preimage, theme->font_options);
  }

  return TRUE;
}

static void
ar_card_theme_preimage_init (ArCardThemePreimage *theme)
{
  theme->cards_preimage = NULL;
  theme->slot_preimage = NULL;

  theme->subsize.width = -1;
  theme->subsize.height = -1;

  theme->card_size.width = theme->card_size.height = theme->slot_size.width =
    theme->slot_size.width = -1;
}

static void
ar_card_theme_preimage_finalize (GObject * object)
{
  ArCardThemePreimage *theme = AR_CARD_THEME_PREIMAGE (object);

  _ar_card_theme_preimage_clear_sized_theme_data (theme);

  if (theme->cards_preimage != NULL) {
    g_object_unref (theme->cards_preimage);
  }
  if (theme->slot_preimage != NULL) {
    g_object_unref (theme->slot_preimage);
  }

  if (theme->font_options) {
    cairo_font_options_destroy (theme->font_options);
  }

  G_OBJECT_CLASS (ar_card_theme_preimage_parent_class)->finalize (object);
}

static void
ar_card_theme_preimage_set_font_options (ArCardTheme *card_theme,
                                            const cairo_font_options_t *font_options)
{
  ArCardThemePreimage *theme = (ArCardThemePreimage *) card_theme;

  if (font_options &&
      theme->font_options &&
      cairo_font_options_equal (font_options, theme->font_options))
    return;

  if (theme->font_options) {
    cairo_font_options_destroy (theme->font_options);
  }

  if (font_options) {
    theme->font_options = cairo_font_options_copy (font_options);
  } else {
    theme->font_options = NULL;
  }

  _ar_card_theme_preimage_clear_sized_theme_data (theme);
  _ar_card_theme_emit_changed (card_theme);
}

static gboolean
ar_card_theme_preimage_set_card_size (ArCardTheme *card_theme,
                                         int width,
                                         int height,
                                         double proportion)
{
  ArCardThemePreimage *theme = (ArCardThemePreimage *) card_theme;
  double aspect_ratio, twidth, theight;

  if ((width == theme->slot_size.width) &&
      (height == theme->slot_size.height))
    return FALSE;

  theme->slot_size.width = width;
  theme->slot_size.height = height;

  /* Now calculate the card size: find the maximum size that fits
   * into the given area, preserving the card's aspect ratio.
   */
  aspect_ratio = ar_card_theme_get_aspect (card_theme);

  twidth = proportion * width;
  theight = proportion * height;
  if (twidth / theight < aspect_ratio) {
    theight = twidth / aspect_ratio;
  } else {
    twidth = theight * aspect_ratio;
  }

  if (theme->card_size.width == (int) twidth &&
      theme->card_size.height == (int) theight)
    return FALSE;

  theme->card_size.width = twidth;
  theme->card_size.height = theight;

  _ar_card_theme_preimage_clear_sized_theme_data (theme);
  _ar_card_theme_emit_changed (card_theme);

  return TRUE;
}

static void
ar_card_theme_preimage_get_card_size (ArCardTheme *card_theme,
                                         CardSize *size)
{
  ArCardThemePreimage *theme = (ArCardThemePreimage *) card_theme;

  *size = theme->card_size;
}

static double
ar_card_theme_preimage_get_card_aspect (ArCardTheme* card_theme)
{
  ArCardThemePreimage *theme = (ArCardThemePreimage *) card_theme;
  double aspect;
aspect =
      (((double) games_preimage_get_width (theme->cards_preimage))
       * N_ROWS) /
      (((double) games_preimage_get_height (theme->cards_preimage))
       * N_COLS);

  return aspect;
}

static ArCardThemeInfo *
ar_card_theme_preimage_class_get_theme_info (ArCardThemeClass *klass,
                                                const char *path,
                                                const char *filename)
{
#ifdef HAVE_RSVG
  ArCardThemeInfo *info;
  char *display_name;

  if (!g_str_has_suffix (filename, ".svg")
#if defined(LIBRSVG_HAVE_SVGZ) && LIBRSVG_HAVE_SVGZ
      && !g_str_has_suffix (filename, ".svgz")
#endif
     )
    return NULL;

  display_name = games_filename_to_display_name (filename);
  info = _ar_card_theme_info_new (G_OBJECT_CLASS_TYPE (klass),
                                     path,
                                     filename,
                                     display_name /* adopts */,
                                     NULL /* filled in by the derived classes */,
                                     NULL, NULL);

  return info;
#else
  return NULL;
#endif
}

static void
ar_card_theme_preimage_class_init (ArCardThemePreimageClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ArCardThemeClass *theme_class = AR_CARD_THEME_CLASS (klass);

  gobject_class->finalize = ar_card_theme_preimage_finalize;

  theme_class->get_theme_info = ar_card_theme_preimage_class_get_theme_info;

  theme_class->load = ar_card_theme_preimage_load;
  theme_class->set_card_size = ar_card_theme_preimage_set_card_size;
  theme_class->get_card_size = ar_card_theme_preimage_get_card_size;
  theme_class->get_card_aspect = ar_card_theme_preimage_get_card_aspect;
  theme_class->get_card_pixbuf = NULL;
  theme_class->set_font_options = ar_card_theme_preimage_set_font_options;
}
