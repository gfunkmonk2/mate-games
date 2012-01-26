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

#include <string.h>
#include <errno.h>

#include <libgames-support/games-string-utils.h>

#include "util.h"
#include "conf.h"

#ifdef HAVE_MATE
#include <mateconf/mateconf-client.h>
#endif

#ifdef HAVE_MATE
static const char key_names[] =
  "card_style\0"
  "game_file\0"
  "recent_games_list\0"
  "show_toolbar\0"
  "click_to_move\0"
  "sound\0"
  "show_statusbar\0"
  "animations";

static const guint8 key_name_offsets[] = {
  0, 11, 21, 39, 52, 66, 72, 87
};

static const char statistics_key[] = "/apps/aisleriot/statistics";

#else

static const char key_names[] =
  "Theme\0"
  "Variation\0"
  "Recent\0"
  "ShowToolbar\0"
  "ClickToMove\0"
  "Sound\0"
  "ShowStatusbar\0"
  "Animations";

static const guint8 key_name_offsets[] = {
  0, 6, 16, 23, 35, 47, 53, 67
};

#endif /* HAVE_MATE */

#ifdef HAVE_MATE

static MateConfClient *mateconf_client;
static GHashTable *stats;

static char *
options_mateconf_key (const char *game_file)
{
  static const char basekey[] = "/apps/aisleriot/rules/";

  return g_strconcat (basekey, game_file, NULL);
}

static void
load_statistics (void)
{
  GSList *raw_list;
  AisleriotStatistic *new_stats;
  guint64 value;

  raw_list = mateconf_client_get_list (mateconf_client, statistics_key,
				    MATECONF_VALUE_STRING, NULL);

  while (raw_list) {
    new_stats = g_hash_table_lookup (stats, raw_list->data);

    if (!new_stats) {
      new_stats = g_new (AisleriotStatistic, 1);
      new_stats->wins = 0;
      new_stats->total = 0;
      new_stats->best = 0;
      new_stats->worst = 0;
      g_hash_table_insert (stats, raw_list->data, new_stats);
    } else {
      g_free (raw_list->data);
    }

    raw_list = g_slist_delete_link (raw_list, raw_list);

    if (!raw_list)
      break;
    new_stats->wins = g_ascii_strtoull (raw_list->data, NULL, 10);
    g_free (raw_list->data);
    raw_list = g_slist_delete_link (raw_list, raw_list);

    if (!raw_list)
      break;
    new_stats->total = g_ascii_strtoull (raw_list->data, NULL, 10);
    g_free (raw_list->data);
    raw_list = g_slist_delete_link (raw_list, raw_list);

    if (!raw_list)
      break;
    value = g_ascii_strtoull (raw_list->data, NULL, 10);
    /* Sanitise value to fix statistics from bug #474615 */
    if (value > 0 && value <= 6000) {
      new_stats->best = value;
    } else {
      new_stats->best = 0;
    }
    g_free (raw_list->data);
    raw_list = g_slist_delete_link (raw_list, raw_list);

    if (!raw_list)
      break;
    value = g_ascii_strtoull (raw_list->data, NULL, 10);
    /* Sanitise value to fix statistics from bug #474615 */
    if (value > 0 && value <= 6000) {
      new_stats->worst = value;
    } else {
      new_stats->worst = 0;
    }
    g_free (raw_list->data);
    raw_list = g_slist_delete_link (raw_list, raw_list);
  }
}

static void
save_single_stat (char *name,
                  AisleriotStatistic *entry,
                  GSList **list)
{
  /* Everything is pushed onto the list in reverse order. */
  *list = g_slist_prepend (*list, g_strdup_printf ("%d", entry->worst));
  *list = g_slist_prepend (*list, g_strdup_printf ("%d", entry->best));
  *list = g_slist_prepend (*list, g_strdup_printf ("%d", entry->total));
  *list = g_slist_prepend (*list, g_strdup_printf ("%d", entry->wins));
  *list = g_slist_prepend (*list, g_strdup (name));
}

static void
save_statistics (void)
{
  GSList *stats_list = NULL;

  g_hash_table_foreach (stats, (GHFunc) save_single_stat, &stats_list);

  mateconf_client_set_list (mateconf_client, statistics_key,
                         MATECONF_VALUE_STRING, stats_list, NULL);

  g_slist_foreach (stats_list, (GFunc) g_free, NULL);
  g_slist_free (stats_list);
}

#endif /* HAVE_MATE */

void
aisleriot_conf_init (void)
{
  if (!games_conf_initialise ("Aisleriot")) {
    /* Set defaults */
    games_conf_set_boolean (NULL, aisleriot_conf_get_key (CONF_SHOW_TOOLBAR), TRUE);
    games_conf_set_boolean (NULL, aisleriot_conf_get_key (CONF_SHOW_STATUSBAR), TRUE);
    games_conf_set_boolean (NULL, aisleriot_conf_get_key (CONF_SOUND), TRUE);
    games_conf_set_boolean (NULL, aisleriot_conf_get_key (CONF_ANIMATIONS), TRUE);

#ifdef HAVE_HILDON
    games_conf_set_boolean (NULL, aisleriot_conf_get_key (CONF_CLICK_TO_MOVE), TRUE);
#endif
  }

#ifdef HAVE_MATE
  
  mateconf_client = mateconf_client_get_default ();

  stats = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  load_statistics ();
  mateconf_client_notify_add (mateconf_client, statistics_key,
			   (MateConfClientNotifyFunc) load_statistics,
			   NULL, NULL, NULL);

#endif /* HAVE_MATE */
}

void
aisleriot_conf_shutdown (void)
{
#ifdef HAVE_MATE
  g_object_unref (mateconf_client);
  mateconf_client = NULL;

  g_hash_table_destroy (stats);
  stats = NULL;
#endif /* HAVE_MATE */

  games_conf_shutdown ();
}

const char *
aisleriot_conf_get_key (AisleriotConfKey key)
{
  return key_names + key_name_offsets[key];
}

gboolean
aisleriot_conf_get_options (const char *game_file,
                            int *options)
{
#ifdef HAVE_MATE
  MateConfEntry *entry;
  MateConfValue *value;
  char *mateconf_key;

  mateconf_key = options_mateconf_key (game_file);
  entry = mateconf_client_get_entry (mateconf_client, mateconf_key, NULL, TRUE, NULL);
  g_free (mateconf_key);
  if (!entry)
    return FALSE;

  value = mateconf_entry_get_value (entry);
  if (!value ||
      value->type != MATECONF_VALUE_INT) {
    mateconf_entry_unref (entry);
    return FALSE;
  }

  *options = mateconf_value_get_int (value);
  mateconf_entry_unref (entry);

  return TRUE;
#else
  GError *error = NULL;

  *options = games_conf_get_integer (game_file, "Options", &error);
  if (error) {
    g_error_free (error);
    return FALSE;
  }

  return TRUE;
#endif /* HAVE_MATE */
}

void
aisleriot_conf_set_options (const char *game_file,
                            int value)
{
#ifdef HAVE_MATE
  MateConfSchema *schema;
  char *mateconf_key, *schemas_key;

  mateconf_key = options_mateconf_key (game_file);

  schemas_key = g_strconcat ("/schemas", mateconf_key, NULL);

  /* Check if we have a schema for this key, and make one if we don't */
  schema = mateconf_client_get_schema (mateconf_client, schemas_key, NULL);
  if (!schema) {
    MateConfValue *def;

    schema = mateconf_schema_new ();
    mateconf_schema_set_type (schema, MATECONF_VALUE_INT);
    mateconf_schema_set_owner (schema, "aisleriot");
    /* FIXME: Translation - how? */
    mateconf_schema_set_short_desc (schema, "A per-game option");
    mateconf_schema_set_long_desc (schema,
                                "An integer encoding a list of boolean values (LSB = first item) for use as options in a solitaire game.");

    def = mateconf_value_new (MATECONF_VALUE_INT);
    /* Not entirely correct, but there's no way to get the default options from
     * the game, and setting this to 0 makes the game options be incorrect.
     */
    mateconf_value_set_int (def, value);
    mateconf_schema_set_default_value_nocopy (schema, def);

    mateconf_client_set_schema (mateconf_client, schemas_key, schema, NULL);
    mateconf_engine_associate_schema (mateconf_engine_get_default (), mateconf_key, schemas_key, NULL);
  }
  mateconf_schema_free (schema);
  g_free (schemas_key);

  mateconf_client_set_int (mateconf_client, mateconf_key, value, NULL);
  g_free (mateconf_key);
#else
  games_conf_set_integer (game_file, "Options", value);
#endif /* HAVE_MATE */
}

void
aisleriot_conf_get_statistic (const char *game_file,
                              AisleriotStatistic *statistic)
{
#ifdef HAVE_MATE
  AisleriotStatistic *game_stat;

  game_stat = g_hash_table_lookup (stats, game_file);
  if (!game_stat) {
    char *display_name;

    /* Previous versions used the localised name as key, so try it as fall-back.
     * See bug #406267 and bug #525177.
     */
    display_name = games_filename_to_display_name (game_file);
    game_stat = g_hash_table_lookup (stats, display_name);
    g_free (display_name);
  }

  if (game_stat) {
    *statistic = *game_stat;
  } else {
    memset (statistic, 0, sizeof (AisleriotStatistic));
  }

#else

  int *values;
  gsize len = 0;
  GError *error = NULL;

  memset (statistic, 0, sizeof (AisleriotStatistic));

  values = games_conf_get_integer_list (game_file, "Statistic", &len, &error);
  if (error) {
    g_error_free (error);
    return;
  }
  if (len != 4) {
    g_free (values);
    return;
  }

  statistic->wins = values[0];
  statistic->total = values[1];
  /* Sanitise values to fix statistics from bug #474615 */
  if (values[2] > 0 && values[2] <= 6000) {
    statistic->best = values[2];
  } else {
    statistic->best = 0;
  }
  if (values[3] > 0 && values[3] <= 6000) {
    statistic->worst = values[3];
  } else {
    statistic->worst = 0;
  }

  g_free (values);
#endif /* HAVE_MATE */
}

void
aisleriot_conf_set_statistic (const char *game_file,
                              AisleriotStatistic *statistic)
{
#ifdef HAVE_MATE
  AisleriotStatistic *game_stat;

  game_stat = g_hash_table_lookup (stats, game_file);
  /* Backward compatibility with buggy old aisleriot versions
   * which stored the localised game name.
   */
  if (!game_stat) {
    char *localised_name;

    localised_name = games_filename_to_display_name (game_file);
    game_stat = g_hash_table_lookup (stats, localised_name);
    g_free (localised_name);
  }

  if (!game_stat) {
    game_stat = g_new0 (AisleriotStatistic, 1);
    g_hash_table_insert (stats, g_strdup (game_file), game_stat);
  }

  *game_stat = *statistic;

  save_statistics ();

#else

  int values[4];

  values[0] = statistic->wins;
  values[1] = statistic->total;
  values[2] = statistic->best;
  values[3] = statistic->worst;

  games_conf_set_integer_list (game_file, "Statistic", values, G_N_ELEMENTS (values));
#endif /* HAVE_MATE */
}
