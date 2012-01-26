/* theme.h */



typedef struct _Theme Theme;
struct _Theme {
  const gchar *title;
  const gchar *fname_tileset;
  const gchar *fname_bground;
  const gchar *grid_rgb;
  const GdkLineStyle grid_style;
  const gchar *player1;
  const gchar *player2;
};


const gchar *theme_get_player (PlayerID who);
const gchar *theme_get_title (gint id);
