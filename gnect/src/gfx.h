/* gfx.h */



gboolean gfx_load_pixmaps (void);
gboolean gfx_set_grid_style (void);
gboolean gfx_change_theme (void);
void gfx_free (void);
void gfx_resize (GtkWidget * w);
void gfx_expose (GdkRectangle * area);
void gfx_draw_tile (gint r, gint c, gboolean refresh);
void gfx_draw_all (void);
gint gfx_get_column (gint xpos);
void gfx_refresh_pixmaps (void);
