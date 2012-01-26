/*
 *  Copyright © 2007 Christian Persch
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2.1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope conf it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <string.h>
#include <errno.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef HAVE_MATE
#include <mateconf/mateconf-client.h>
#else
#define ACCELMAP_EXT "accels"
#endif

#include "games-debug.h"
#include "games-glib-compat.h"
#include "games-gtk-compat.h"
#include "games-marshal.h"

#include "games-conf.h"

#define GAMES_CONF_GET_PRIVATE(that)(G_TYPE_INSTANCE_GET_PRIVATE ((that), GAMES_TYPE_CONF, GamesConfPrivate))

struct _GamesConfPrivate {
  char *game_name;

#ifdef HAVE_MATE
  MateConfClient *mateconf_client;
  char *base_path;
  gsize base_path_len;
#else
  GKeyFile *key_file;
  char *main_group;
#endif
  guint need_init : 1;
  guint dirty : 1;
};

enum
{
  PROP_0,
  PROP_GAME_NAME
};

enum
{
  VALUE_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static GamesConf *instance;

G_DEFINE_TYPE (GamesConf, games_conf, G_TYPE_OBJECT);

/* helper functions */

#define WINDOW_STATE_TIMEOUT 1 /* s */

enum {
  STATE_KEY_MAXIMISED,
  STATE_KEY_FULLSCREEN,
  STATE_KEY_WIDTH,
  STATE_KEY_HEIGHT,
  LAST_STATE_KEY
};

static const char window_state_key_name[][12] = {
#ifdef HAVE_MATE
  "maximized",
  "fullscreen",
  "width",
  "height"
#else
  "Maximised",
  "Fullscreen",
  "Width",
  "Height"
#endif /* HAVE_MATE */
};

typedef struct {
  GtkWindow *window;
  char *group;
  guint timeout_id;
  int width;
  int height;
  guint is_maximised : 1;
  guint is_fullscreen : 1;
} WindowState;

static gboolean
window_state_timeout_cb (WindowState *state)
{
  games_conf_set_integer (state->group, window_state_key_name[STATE_KEY_WIDTH], state->width);
  games_conf_set_integer (state->group, window_state_key_name[STATE_KEY_HEIGHT], state->height);

  _games_debug_print (GAMES_DEBUG_WINDOW_STATE,
                      "[window %p] timeout: persisting width:%d height:%d\n",
                      state->window,
                      state->width, state->height);

  state->timeout_id = 0;
  return FALSE;
}

static void
free_window_state (WindowState *state)
{
  if (state->timeout_id != 0) {
    g_source_remove (state->timeout_id);

    /* And store now */
    window_state_timeout_cb (state);
  }

  g_free (state->group);

  g_slice_free (WindowState, state);
}

static gboolean
window_configure_event_cb (GtkWidget *widget,
                           GdkEventConfigure *event,
                           WindowState *state)
{
  _games_debug_print (GAMES_DEBUG_WINDOW_STATE,
                      "[window %p] configure event current %dx%d new %dx%d [state: is-maximised:%s is-fullscreen:%s]\n",
                      state->window,
                      state->width, state->height,
                      event->width, event->height,
                      state->is_maximised ? "t" : "f",
                      state->is_fullscreen ? "t" : "f");

  if (!state->is_maximised && !state->is_fullscreen &&
      (state->width != event->width || state->height != event->height)) {
    state->width = event->width;
    state->height = event->height;

  _games_debug_print (GAMES_DEBUG_WINDOW_STATE,
                      "[window %p] scheduling save of new window size\n",
                      state->window);

    if (state->timeout_id == 0) {
      state->timeout_id = g_timeout_add_seconds (WINDOW_STATE_TIMEOUT,
                                                 (GSourceFunc) window_state_timeout_cb,
                                                 state);
    }
  }

  return FALSE;
}

static gboolean
window_state_event_cb (GtkWidget *widget,
                       GdkEventWindowState *event,
                       WindowState *state)
{
  _games_debug_print (GAMES_DEBUG_WINDOW_STATE,
                      "[window %p] state event, mask:%x new-state:%x current state: is-maximised:%s is-fullscreen:%s\n",
                      state->window,
                      event->changed_mask, event->new_window_state,
                      state->is_maximised ? "t" : "f",
                      state->is_fullscreen ? "t" : "f");

  if (event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) {
    state->is_maximised = (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0;
    games_conf_set_boolean (state->group, window_state_key_name[STATE_KEY_MAXIMISED], state->is_maximised);
  }
  if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) {
    state->is_fullscreen = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;
    games_conf_set_boolean (state->group, window_state_key_name[STATE_KEY_FULLSCREEN], state->is_fullscreen);
  }

  _games_debug_print (GAMES_DEBUG_WINDOW_STATE,
                      "  > new state: is-maximised:%s is-fullscreen:%s\n",
                      state->is_maximised ? "t" : "f",
                      state->is_fullscreen ? "t" : "f");


  return FALSE;
}

#ifndef HAVE_HILDON

static char *
games_conf_get_accel_map_path (GamesConf *conf,
                               gboolean ensure_dir_exists)
{
  GamesConfPrivate *priv = conf->priv;
  char *game_name, *conf_dir;
  char *conf_file = NULL;
  const char *override;

  game_name = g_ascii_strdown (priv->game_name, -1);

#ifdef HAVE_MATE
  override = g_getenv ("MATE22_USER_DIR");
  if (override)
    conf_dir = g_build_filename (override, "accels", NULL);
  else
    conf_dir = g_build_filename (g_get_home_dir (), ".mate2", "accels", NULL);
#else
  conf_dir = g_build_filename (g_get_user_config_dir (), "mate-games", NULL);
#endif
  if (!conf_dir)
    goto loser;

  /* Mode 0700 per the XDG basedir spec */
  if (ensure_dir_exists &&
      g_mkdir_with_parents (conf_dir, 0700) < 0) {
    int err = errno;

    if (err != EEXIST) {
      g_warning ("Failed to create config directory \"%s\": %s\n", conf_dir, g_strerror (err));
      goto loser;
    }
  }

#ifdef HAVE_MATE
  conf_file = g_build_filename (conf_dir, game_name, NULL);
#else
{
  char *accelmap_filename;

  accelmap_filename = g_strdup_printf ("%s.%s", game_name, ACCELMAP_EXT);
  conf_file = g_build_filename (conf_dir, accelmap_filename, NULL);
  g_free (accelmap_filename);
}
#endif

loser:
  g_free (conf_dir);
  g_free (game_name);

  return conf_file;
}

static void
games_conf_load_accel_map (GamesConf *conf)
{
  char *conf_file;

  conf_file = games_conf_get_accel_map_path (conf, FALSE);
  if (!conf_file)
    return;

  gtk_accel_map_load (conf_file);
  g_free (conf_file);
}

static void
games_conf_save_accel_map (GamesConf *conf)
{
  char *conf_file;

  conf_file = games_conf_get_accel_map_path (conf, TRUE);
  if (!conf_file)
    return;

  gtk_accel_map_save (conf_file);
  g_free (conf_file);
}

#endif /* !HAVE_HILDON */

#ifdef HAVE_MATE

static void
mateconf_notify_cb (MateConfClient *client,
                 guint cnxn_id,
                 MateConfEntry *gcentry,
                 GamesConf *conf)
{
  GamesConfPrivate *priv = conf->priv;
  char *key;
  char **path;

  if (!g_str_has_prefix (gcentry->key, priv->base_path))
    return;

  key = gcentry->key + priv->base_path_len;
  if (*key != '/')
    return;

  path = g_strsplit (key + 1, "/", 2);
  if (!path)
    return;

  if (path[0] && path[1])
    g_signal_emit (conf, signals[VALUE_CHANGED], 0, path[0], path[1]);
  else if (path[0])
    g_signal_emit (conf, signals[VALUE_CHANGED], 0, NULL, path[0]);

  g_strfreev (path);
}

static char *
get_mateconf_key_name (const char *group, const char *key)
{
  GamesConfPrivate *priv = instance->priv;

  if (!group)
    return g_strdup_printf ("%s/%s", priv->base_path, key);

  return g_strdup_printf ("%s/%s/%s", priv->base_path, group, key);
}

static MateConfValueType
get_mateconf_value_type_from_schema (const char *key_name)
{
  GamesConfPrivate *priv = instance->priv;
  MateConfSchema *schema;
  char *schema_key;
  MateConfValueType type = MATECONF_VALUE_STRING;

  schema_key = g_strconcat ("/schemas", key_name, NULL);
  schema = mateconf_client_get_schema (priv->mateconf_client, schema_key, NULL);

  if (schema) {
    type = mateconf_schema_get_type (schema);
    mateconf_schema_free (schema);
  }

  g_free (schema_key);

  return type;
}

#endif /* HAVE_MATE */

#ifndef HAVE_MATE

static void
mark_dirty_cb (GamesConf *conf)
{
  GamesConfPrivate *priv = conf->priv;

  priv->dirty = TRUE;
}

#endif /* !HAVE_MATE */

/* Class implementation */

static void
games_conf_init (GamesConf *conf)
{
  GamesConfPrivate *priv;

  priv = conf->priv = GAMES_CONF_GET_PRIVATE (conf);

  priv->need_init = FALSE;
  priv->dirty = FALSE;

#ifndef HAVE_MATE
  g_signal_connect (conf, "value-changed", G_CALLBACK (mark_dirty_cb), NULL);
#endif
}

static GObject *
games_conf_constructor (GType type,
			  guint n_construct_properties,
			  GObjectConstructParam *construct_params)
{
  GObject *object;
  GamesConf *conf;
  GamesConfPrivate *priv;
  char *game_name;
#ifndef HAVE_MATE
  char *conf_file;
  GError *error = NULL;
#endif /* HAVE_MATE */

  g_assert (instance == NULL);

  object = G_OBJECT_CLASS (games_conf_parent_class)->constructor
             (type, n_construct_properties, construct_params);

  conf = GAMES_CONF (object);
  priv = conf->priv;

  g_assert (priv->game_name);

  game_name = g_ascii_strdown (priv->game_name, -1);

#ifdef HAVE_MATE
  priv->mateconf_client = mateconf_client_get_default ();

  priv->base_path = g_strdup_printf ("/apps/%s", game_name);
  priv->base_path_len = strlen (priv->base_path);

  mateconf_client_add_dir (priv->mateconf_client, priv->base_path,
                        MATECONF_CLIENT_PRELOAD_NONE, NULL);

  mateconf_client_notify_add (priv->mateconf_client,
                           priv->base_path,
                           (MateConfClientNotifyFunc) mateconf_notify_cb,
                           conf, NULL,
                           NULL);

#else /* !HAVE_MATE */

  priv->main_group = g_strdup_printf ("%s Config", priv->game_name);

  conf_file = g_build_filename (g_get_user_config_dir (), "mate-games", game_name, NULL);

  priv->key_file = g_key_file_new ();
  if (!g_key_file_load_from_file (priv->key_file, conf_file, 0, &error)) {
    /* Don't warn on non-existent file */
    if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
      g_warning ("Failed to read settings from \"%s\": %s",
                  conf_file, error->message);
    }

    g_error_free (error);

    priv->need_init = TRUE;
  }

  g_free (conf_file);

#endif /* HAVE_MATE */

#ifndef HAVE_HILDON
  games_conf_load_accel_map (conf);
#endif /* !HAVE_HILDON */

  g_free (game_name);

  return object;
}

static void
games_conf_finalize (GObject *object)
{
  GamesConf *conf = GAMES_CONF (object);
  GamesConfPrivate *priv = conf->priv;

#ifndef HAVE_HILDON
  /* Save the accel map */
  games_conf_save_accel_map (conf);
#endif /* !HAVE_HILDON */

#ifdef HAVE_MATE
  mateconf_client_remove_dir (priv->mateconf_client, priv->base_path, NULL);

  g_free (priv->base_path);

  g_object_unref (priv->mateconf_client);
  priv->mateconf_client = NULL;

#else /* !HAVE_MATE */

  games_conf_save ();

  g_free (priv->main_group);
  g_key_file_free (priv->key_file);

#endif /* HAVE_MATE */

  g_free (priv->game_name);

  G_OBJECT_CLASS (games_conf_parent_class)->finalize (object);

  instance = NULL;
}

static void
games_conf_set_property (GObject *object,
			   guint prop_id,
			   const GValue *value,
			   GParamSpec *pspec)
{
  GamesConf *conf = GAMES_CONF (object);
  GamesConfPrivate *priv = conf->priv;

  switch (prop_id) {
    case PROP_GAME_NAME:
      priv->game_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
games_conf_class_init (GamesConfClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GType string_types[] = {
    G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
    G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE
  };

  gobject_class->constructor = games_conf_constructor;
  gobject_class->finalize = games_conf_finalize;
  gobject_class->set_property = games_conf_set_property;

  signals[VALUE_CHANGED] =
    g_signal_newv ("value-changed",
                   G_OBJECT_CLASS_TYPE (gobject_class),
                   (GSignalFlags) (G_SIGNAL_RUN_LAST),
                   NULL,
                   NULL, NULL,
                   games_marshal_VOID__STRING_STRING,
                   G_TYPE_NONE,
                   2, string_types);

  g_object_class_install_property
    (gobject_class,
     PROP_GAME_NAME,
     g_param_spec_string ("game-name", NULL, NULL,
                          NULL,
                          G_PARAM_WRITABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB |
                          G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (gobject_class, sizeof (GamesConfPrivate));
}

/* public API */

/**
 * games_conf_initialise:
 * @game_name: the name of the game
 *
 * Initialises the default #GamesConf instance.
 *
 * Returns: %TRUE if @game_name had saved settings; %FALSE otherwise
 */
gboolean
games_conf_initialise (const char *game_name)
{
  instance = g_object_new (GAMES_TYPE_CONF,
                           "game-name", game_name,
                           NULL);

#ifdef HAVE_MATE
  /* MateConf uses MateCORBA2 which needs threads (but it's too late to call
   * g_thread_init() here). See bug #547885.
   */
  g_assert (g_thread_supported ());
#endif

  return !instance->priv->need_init;
}

/**
 * games_conf_shutdown:
 *
 * Shuts down the default #GamesConf instance.
 */
void
games_conf_shutdown (void)
{
  g_assert (instance != NULL);

  g_object_unref (instance);
  instance = NULL;
}

/**
 * games_conf_get_default:
 *
 * Returns the default #GamesConf instance. games_conf_init() must have
 * been called before this!
 *
 * Returns: a #GamesConf (no reference)
 */
GamesConf *
games_conf_get_default (void)
{
  g_assert (instance != NULL);

  return instance;
}

/**
 * games_conf_save:
 *
 * Ensures settings are written to disk.
 */
void
games_conf_save (void)
{
#ifndef HAVE_MATE
  GamesConfPrivate *priv = instance->priv;
  char *game_name, *conf_file, *conf_dir, *data = NULL;
  gsize len = 0;
  GError *error = NULL;

  if (!priv->dirty)
    return;

  game_name = g_ascii_strdown (priv->game_name, -1);
  conf_dir = g_build_filename (g_get_user_config_dir (), "mate-games", NULL);
  conf_file = g_build_filename (conf_dir, game_name, NULL);

  /* Ensure the directory exists; mode 0700 per the XDG basedir spec. */
  if (g_mkdir_with_parents (conf_dir, 0700) < 0) {
    int err = errno;

    if (err != EEXIST) {
      g_warning ("Failed to create config directory \"%s\": %s\n", conf_dir, g_strerror (err));
      goto loser;
    }
  }

  data = g_key_file_to_data (priv->key_file, &len, &error);
  if (!data) {
    g_warning ("Failed to save settings to \"%s\": %s",
               conf_file, error->message);
    g_error_free (error);
    goto loser;
  }

  if (!g_file_set_contents (conf_file, data, len, &error)) {
    g_warning ("Failed to save settings to \"%s\": %s",
              conf_file, error->message);
    g_error_free (error);
    goto loser;
  }

  /* Sucessfully saved */
  priv->dirty = FALSE;

loser:
  g_free (data);
  g_free (conf_file);
  g_free (conf_dir);
  g_free (game_name);
#endif /* !HAVE_MATE */
}

/**
 * games_conf_get_string:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @error: a location for a #GError
 *
 * Returns the string associated with @key in @group, or %NULL if
 * @key is not set, or an error occurred
 *
 * Returns: a newly allocated string, or %NULL
 */
char *
games_conf_get_string (const char *group, const char *key,
                       GError ** error)
{
  GamesConfPrivate *priv = instance->priv;

#ifdef HAVE_MATE
  char *key_name, *value;

  key_name = get_mateconf_key_name (group, key);
  value = mateconf_client_get_string (priv->mateconf_client, key_name, NULL);
  g_free (key_name);

  return value;
#else
  return g_key_file_get_string (priv->key_file, group ? group : priv->main_group, key, error);
#endif /* HAVE_MATE */
}

/**
 * games_conf_get_string_with_default:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @def_value: the default value
 *
 * Returns the string associated with @key in @group, or a copy of
 * @def_value if @key is not set, or an error occurred
 *
 * Returns: a newly allocated string
 */
char *
games_conf_get_string_with_default (const char *group, const char *key,
                                    const char *def_value)
{
  GError *error = NULL;
  char *value;

  value = games_conf_get_string (group, key, &error);
  if (value)
    return value;

  if (error) {
    g_error_free (error);
  }

  return g_strdup (def_value);
}

/**
 * games_conf_set_string:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @value: the value to store
 *
 * Associates @value with the key @key in group @group.
 */
void
games_conf_set_string (const char *group, const char *key,
                       const char *value)
{
  GamesConfPrivate *priv = instance->priv;

#ifdef HAVE_MATE
  char *key_name;

  key_name = get_mateconf_key_name (group, key);
  mateconf_client_set_string (priv->mateconf_client, key_name, value, NULL);
  g_free (key_name);
#else
  g_key_file_set_string (priv->key_file, group ? group : priv->main_group, key, value);
  g_signal_emit (instance, signals[VALUE_CHANGED], 0, group, key);
#endif /* HAVE_MATE */
}

/**
 * games_conf_get_string_list:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @len: a location to store the length of the returned array
 * @error: a location for a #GError
 *
 * Returns the string array associated with @key in @group, or %NULL if
 * @key is not set, or an error occurred
 *
 * Returns: a newly allocated string array, or %NULL
 */
char **
games_conf_get_string_list (const char *group, const char *key,
                            gsize * n_values, GError ** error)
{
  GamesConfPrivate *priv = instance->priv;

#ifdef HAVE_MATE
  char *key_name;
  GSList *list, *l;
  char **values = NULL;
  gsize n = 0;

  key_name = get_mateconf_key_name (group, key);

  list = mateconf_client_get_list (priv->mateconf_client, key_name, MATECONF_VALUE_STRING, NULL);
  if (list != NULL) {
    values = g_new (char *, g_slist_length (list) + 1);

    for (l = list; l != NULL; l = l->next) {
      values[n++] = l->data;
    }

    /* NULL termination */
    values[n] = NULL;

    g_slist_free (list); /* the strings themselves are now owned by the array */
  }

  *n_values = n;
  
  g_free (key_name);
  return values;
#else
  return g_key_file_get_string_list (priv->key_file, group ? group : priv->main_group, key, n_values, error);
#endif /* HAVE_MATE */
}

/**
 * games_conf_set_string_list:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @values: the value to store
 * @n_values: the length of the @values array
 *
 * Associates @value with the key @key in group @group.
 */
void
games_conf_set_string_list (const char *group, const char *key,
                            const char * const *values, gsize n_values)
{
  GamesConfPrivate *priv = instance->priv;

#ifdef HAVE_MATE
  char *key_name;
  GSList *list = NULL;
  gsize i;

  key_name = get_mateconf_key_name (group, key);

  for (i = 0; i < n_values; ++i) {
    list = g_slist_prepend (list, (gpointer) values[i]);
  }
  list = g_slist_reverse (list);

  mateconf_client_set_list (priv->mateconf_client, key_name, MATECONF_VALUE_STRING, list, NULL);

  g_slist_free (list);

  g_free (key_name);
#else
  g_key_file_set_string_list (priv->key_file, group ? group : priv->main_group, key, values, n_values);
  g_signal_emit (instance, signals[VALUE_CHANGED], 0, group, key);
#endif /* HAVE_MATE */
}

/**
 * games_conf_get_integer:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @error: a location for a #GError
 *
 * Returns the integer associated with @key in @group, or 0 if
 * @key is not set, or an error occurred
 *
 * Returns: an integer
 */
int
games_conf_get_integer (const char *group, const char *key,
                        GError ** error)
{
  GamesConfPrivate *priv = instance->priv;

#ifdef HAVE_MATE
  int value;
  char *key_name;

  key_name = get_mateconf_key_name (group, key);
  value = mateconf_client_get_int (priv->mateconf_client, key_name, error);  
  g_free (key_name);

  return value;
#else
  return g_key_file_get_integer (priv->key_file, group ? group : priv->main_group, key, error);
#endif /* HAVE_MATE */
}

/**
 * games_conf_get_integer_with_default:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @def_value: the default value
 *
 * Returns the integer associated with @key in @group, or @def_value if
 * @key is not set, or an error occurred
 *
 * Returns: an integer
 */
int
games_conf_get_integer_with_default (const char *group, const char *key,
                                     int def_value)
{
  GError *error = NULL;
  int value;

  value = games_conf_get_integer (group, key, &error);
  if (error) {
    g_error_free (error);
    value = def_value;
  }

  return value;
}


/**
 * games_conf_set_integer:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @value: the value to store
 *
 * Associates @value with the key @key in group @group.
 */
void
games_conf_set_integer (const char *group, const char *key, int value)
{
  GamesConfPrivate *priv = instance->priv;

#ifdef HAVE_MATE
  char *key_name;

  key_name = get_mateconf_key_name (group, key);
  mateconf_client_set_int (priv->mateconf_client, key_name, value, NULL);
  g_free (key_name);
#else
  g_key_file_set_integer (priv->key_file, group ? group : priv->main_group, key, value);
  g_signal_emit (instance, signals[VALUE_CHANGED], 0, group, key);
#endif /* HAVE_MATE */
}

/**
 * games_conf_get_integer_list:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @len: a location to store the length of the returned array
 * @error: a location for a #GError
 *
 * Returns the integer associated with @key in @group, or 0 if
 * @key is not set, or an error occurred
 *
 * Returns: an integer
 */
int *
games_conf_get_integer_list (const char *group, const char *key,
                             gsize * n_values, GError ** error)
{
  GamesConfPrivate *priv = instance->priv;

#ifdef HAVE_MATE
  char *key_name;
  GSList *list, *l;
  int *values = NULL;
  gsize n = 0;

  key_name = get_mateconf_key_name (group, key);

  list = mateconf_client_get_list (priv->mateconf_client, key_name, MATECONF_VALUE_STRING, NULL);
  if (list != NULL) {
    values = g_new (int, g_slist_length (list));

    for (l = list; l != NULL; l = l->next) {
      values[n++] = GPOINTER_TO_INT (l->data);
    }
  }

  *n_values = n;
  
  g_free (key_name);
  return values;
#else
  return g_key_file_get_integer_list (priv->key_file, group ? group : priv->main_group, key, n_values, error);
#endif /* HAVE_MATE */
}

/**
 * games_conf_set_integer_list:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @values: the value to store
 * @n_values: the length of the @values array
 *
 * Associates @value with the key @key in group @group.
 */
void
games_conf_set_integer_list (const char *group, const char *key,
                             int *values, gsize n_values)
{
  GamesConfPrivate *priv = instance->priv;

#ifdef HAVE_MATE
  char *key_name;
  GSList *list = NULL;
  gsize i;

  key_name = get_mateconf_key_name (group, key);

  for (i = 0; i < n_values; ++i) {
    list = g_slist_prepend (list, GINT_TO_POINTER (values[i]));
  }
  list = g_slist_reverse (list);

  mateconf_client_set_list (priv->mateconf_client, key_name, MATECONF_VALUE_INT, list, NULL);

  g_slist_free (list);

  g_free (key_name);
#else
  g_key_file_set_integer_list (priv->key_file, group ? group : priv->main_group, key, values, n_values);
  g_signal_emit (instance, signals[VALUE_CHANGED], 0, group, key);
#endif /* HAVE_MATE */
}

/**
 * games_conf_get_boolean:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @error: a location for a #GError
 *
 * Returns the boolean associated with @key in @group, or %FALSE if
 * @key is not set, or an error occurred
 *
 * Returns: a boolean
 */
gboolean
games_conf_get_boolean (const char *group, const char *key,
                        GError ** error)
{
  GamesConfPrivate *priv = instance->priv;

#ifdef HAVE_MATE
  gboolean value;
  char *key_name;

  key_name = get_mateconf_key_name (group, key);
  value = mateconf_client_get_bool (priv->mateconf_client, key_name, error);
  g_free (key_name);

  return value;
#else
  return g_key_file_get_boolean (priv->key_file, group ? group : priv->main_group, key, error);
#endif /* HAVE_MATE */
}

/**
 * games_conf_get_boolean_width_default:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @def_value: the default value
 *
 * Returns the boolean associated with @key in @group, or @def_value if
 * @key is not set, or an error occurred
 *
 * Returns: a boolean
 */
gboolean
games_conf_get_boolean_with_default (const char *group, const char *key,
                                     gboolean def_value)
{
  GError *error = NULL;
  gboolean value;

  value = games_conf_get_boolean (group, key, &error);
  if (error) {
    g_error_free (error);
    value = def_value;
  }

  return value;
}

/**
 * games_conf_set_boolean:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @value: the value to store
 *
 * Associates @value with the key @key in group @group.
 */
void
games_conf_set_boolean (const char *group, const char *key,
                        gboolean value)
{
  GamesConfPrivate *priv = instance->priv;

#ifdef HAVE_MATE
  char *key_name;

  key_name = get_mateconf_key_name (group, key);
  mateconf_client_set_bool (priv->mateconf_client, key_name, value, NULL);
  g_free (key_name);
#else
  g_key_file_set_boolean (priv->key_file, group ? group : priv->main_group, key, value);
  g_signal_emit (instance, signals[VALUE_CHANGED], 0, group, key);
#endif /* HAVE_MATE */
}

/**
 * games_conf_get_double:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @error: a location for a #GError
 *
 * Returns the value associated with @key in @group, or 0 if
 * @key is not set, or an error occurred
 *
 * Returns: a double
 */
double
games_conf_get_double (const char *group, const char *key,
                       GError ** error)
{
#if defined(HAVE_MATE)
  GamesConfPrivate *priv = instance->priv;
  double value;
  char *key_name;

  key_name = get_mateconf_key_name (group, key);
  value = mateconf_client_get_float (priv->mateconf_client, key_name, error);
  g_free (key_name);

  return value;
#elif GLIB_CHECK_VERSION (2, 12, 0)
  GamesConfPrivate *priv = instance->priv;

  return g_key_file_get_double (priv->key_file, group ? group : priv->main_group, key, error);
#else
#warning games_conf_get_double not implemented on glib < 2.12!
  /* Not supported */
  return 0.0;
#endif /* HAVE_MATE */
}

/**
 * games_conf_set_double:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @value: the value to store
 *
 * Associates @value with the key @key in group @group.
 */
void
games_conf_set_double (const char *group, const char *key, double value)
{
#if defined(HAVE_MATE)
  GamesConfPrivate *priv = instance->priv;
  char *key_name;

  key_name = get_mateconf_key_name (group, key);
  mateconf_client_set_float (priv->mateconf_client, key_name, value, NULL);
  g_free (key_name);
#elif GLIB_CHECK_VERSION (2, 12, 0)
  GamesConfPrivate *priv = instance->priv;

  g_key_file_set_double (priv->key_file, group ? group : priv->main_group, key, value);
  g_signal_emit (instance, signals[VALUE_CHANGED], 0, group, key);
#else
#warning games_conf_set_double not implemented on glib < 2.12!
#endif /* HAVE_MATE */
}

/**
 * games_conf_get_keyval:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @error: a location for a #GError
 *
 * Returns the keyboard key associated with @key in @group, or 0 if
 * @key is not set, or an error occurred
 *
 * Returns: a keyboard key value
 */
guint
games_conf_get_keyval (const char *group, const char *key,
                       GError ** error)
{
  GamesConfPrivate *priv = instance->priv;

#ifdef HAVE_MATE
  MateConfValueType type;
  char *key_name, *value;
  guint keyval = GDK_VoidSymbol;

  key_name = get_mateconf_key_name (group, key);
  type = get_mateconf_value_type_from_schema (key_name);

  /* The result could be a keycode or a key name. */
  if (type == MATECONF_VALUE_STRING) {
    value = mateconf_client_get_string (priv->mateconf_client, key_name, error);
    if (!value) {
      keyval = GDK_VoidSymbol;
    } else {
      keyval = gdk_keyval_from_name (value);
      g_free (value);
    }
  } else if (type == MATECONF_VALUE_INT) {
    keyval = mateconf_client_get_int (priv->mateconf_client, key_name, error);
    if (*error || keyval == 0)
      keyval = GDK_VoidSymbol;
  } else {
    g_warning ("Unknown value type for key %s\n", key_name);
  }

  g_free (key_name);

  return keyval;
#else
  char *value;
  guint keyval = GDK_VoidSymbol;

  value = g_key_file_get_string (priv->key_file, group, key, error);
  if (value) {
    keyval = gdk_keyval_from_name (value);
    g_free (value);
  }

  return keyval;
#endif /* HAVE_MATE */
}

/**
 * games_conf_get_keyval:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @default_keyval: the default value
 *
 * Returns the keyboard key associated with @key in @group, or @default_keyval
 * if @key is not set, or an error occurred
 *
 * Returns: a keyboard key value
 */
guint
games_conf_get_keyval_with_default (const char *group, const char *key,
                                    guint default_keyval)
{
  GError *error = NULL;
  guint value;

  value = games_conf_get_keyval (group, key, &error);
  if (error) {
    g_error_free (error);
    value = default_keyval;
  }
  if (value == GDK_VoidSymbol) {
    value = default_keyval;
  }

  return value;
}

/**
 * games_conf_set_keyval:
 * @group: the group name, or %NULL to use the default group
 * @key: the key name
 * @value: the value to store
 *
 * Associates @value with the key @key in group @group.
 */
void
games_conf_set_keyval (const char *group, const char *key, guint value)
{
  GamesConfPrivate *priv = instance->priv;

#ifdef HAVE_MATE
  MateConfValueType type;
  char *key_name, *name;

  if (value == GDK_VoidSymbol)
    return;

  key_name = get_mateconf_key_name (group, key);
  type = get_mateconf_value_type_from_schema (key_name);

  /* The result could be a keycode or a key name. */
  if (type == MATECONF_VALUE_STRING) {
    name = gdk_keyval_name (value);
    mateconf_client_set_string (priv->mateconf_client, key_name, name, NULL);
  } else if (type == MATECONF_VALUE_INT) {
    mateconf_client_set_int (priv->mateconf_client, key_name, (int) value, NULL);
  } else {
    g_warning ("Unknown value type for key %s\n", key_name);
  }

  g_free (key_name);
#else
  char *name;

  if (value == GDK_VoidSymbol)
    return;
  
  name = gdk_keyval_name (value);
  g_key_file_set_string (priv->key_file, group, key, name);
  g_signal_emit (instance, signals[VALUE_CHANGED], 0, group, key);
#endif /* HAVE_MATE */
}

/**
 * games_conf_add_window:
 * @window: a #GtkWindow
 * @group: the group to store the state in, or %NULL to use
 * the default group
 * 
 * Restore the window configuration, and persist changes to the window configuration:
 * window width and height, and maximised and fullscreen state.
 * @window must not be realised yet.
 */
void
games_conf_add_window (GtkWindow *window,
                       const char *group)
{
  WindowState *state;
  int width, height;
  gboolean maximised, fullscreen;

  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (!GTK_WIDGET_REALIZED (window));

  state = g_slice_new0 (WindowState);

  state->window = window;
  state->group = g_strdup (group);
  g_object_set_data_full (G_OBJECT (window), "GamesConf::WindowState",
                          state, (GDestroyNotify) free_window_state);

  g_signal_connect (window, "configure-event",
                    G_CALLBACK (window_configure_event_cb), state);
  g_signal_connect (window, "window-state-event",
                    G_CALLBACK (window_state_event_cb), state);

  maximised = games_conf_get_boolean (group, window_state_key_name[STATE_KEY_MAXIMISED], NULL);
  fullscreen = games_conf_get_boolean (group, window_state_key_name[STATE_KEY_FULLSCREEN], NULL);
  width = games_conf_get_integer (group, window_state_key_name[STATE_KEY_WIDTH], NULL);
  height = games_conf_get_integer (group, window_state_key_name[STATE_KEY_HEIGHT], NULL);

  if (width > 0 && height > 0) {
    _games_debug_print (GAMES_DEBUG_WINDOW_STATE,
                        "[window %p] restoring size %dx%d\n",
                        state->window,
                        width, height);
    gtk_window_set_default_size (GTK_WINDOW (window), width, height);
  }
  if (maximised) {
    _games_debug_print (GAMES_DEBUG_WINDOW_STATE,
                        "[window %p] restoring maximised state\n",
                        state->window);
    gtk_window_maximize (GTK_WINDOW (window));
  }
  if (fullscreen) {
    _games_debug_print (GAMES_DEBUG_WINDOW_STATE,
                        "[window %p] restoring fullscreen state\n",
                        state->window);
    gtk_window_fullscreen (GTK_WINDOW (window));
  }
}
