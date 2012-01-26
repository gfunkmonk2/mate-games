/* 
 * Copyright © 1998, 2001, 2003, 2006 Jonathan Blandford <jrb@alum.mit.edu>
 * Copyright © 2007 Christian Persch
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

#include <string.h>

#include <glib/gi18n.h>

#include <gtk/gtk.h>

#include "games-string-utils.h"

/**
 * games_filename_to_display_name:
 * @filename:
 *
 * Transforms @filename from filename encoding into a
 * translated string in UTF-8 that can be shown to the
 * user.
 *
 * Returns: a newly allocated UTF-8 string
 */
char *
games_filename_to_display_name (const char *filename)
{
  char *base_name, *display_name, *translated, *p;
  GString *prettified_name;
  gboolean start_of_word, free_segment;
  gunichar c;
  char utf8[7];
  gsize len;

  g_return_val_if_fail (filename != NULL, NULL);

  base_name = g_path_get_basename (filename);
  g_return_val_if_fail (base_name != NULL, NULL);

  /* Hide extension */
  g_strdelimit (base_name, ".", '\0');
  /* Hide undesirable characters */
  g_strdelimit (base_name, NULL, ' ');

  g_strstrip (base_name);

  display_name = g_filename_display_name (base_name);
  g_free (base_name);

  g_return_val_if_fail (display_name != NULL, NULL);

  /* Now turn the first character in each word to uppercase */

  prettified_name = g_string_sized_new (strlen (display_name) + 8);
  start_of_word = TRUE;
  for (p = display_name; p && *p; p = g_utf8_next_char (p)) {
    if (start_of_word) {
      c = g_unichar_toupper (g_utf8_get_char (p));
    } else {
      c = g_utf8_get_char (p);
    }

    len = g_unichar_to_utf8 (c, utf8);
    g_string_append_len (prettified_name, utf8, len);

    start_of_word = g_unichar_isspace (c);
  }
  g_free (display_name);

  translated = gettext (prettified_name->str);
  if (translated != prettified_name->str) {
    display_name = g_strdup (translated);
    free_segment = TRUE;
  } else {
    display_name = prettified_name->str;
    free_segment = FALSE;
  }

  g_string_free (prettified_name, free_segment);

  return display_name;
}
