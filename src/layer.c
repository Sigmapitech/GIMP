#include "layer.h"

static
cairo_surface_t *create_surface(int w, int h)
{
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = cairo_create(s);

    cairo_set_source_rgba(cr, 0,0,0,0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_destroy(cr);
    return s;
}


Layer *layer_new_blank(const char *name, int w, int h)
{
    Layer *l = g_new0(Layer, 1);
    l->name = g_strdup(name ? name : "Layer");
    l->surface = create_surface(w, h);
    l->visible = TRUE;
    l->opacity = 1.0;
    return l;
}

Layer *layer_new_from_file(const char *filename)
{
    GError *err = NULL;
    GdkPixbuf *pix = gdk_pixbuf_new_from_file(filename, &err);
    if (!pix) {
        g_warning("Failed to load '%s': %s", filename, err->message);
        g_error_free(err);
        return NULL;
    }

    int w = gdk_pixbuf_get_width(pix);
    int h = gdk_pixbuf_get_height(pix);
    Layer *l = g_new0(Layer, 1);
    l->name = g_path_get_basename(filename);
    l->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);

    cairo_t *cr = cairo_create(l->surface);
    gdk_cairo_set_source_pixbuf(cr, pix, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);

    g_object_unref(pix);
    l->visible = TRUE;
    l->opacity = 1.0;
    return l;
}

void layer_free(Layer *l)
{
    if (!l) return;
    if (l->name) g_free(l->name);
    if (l->surface) cairo_surface_destroy(l->surface);
    g_free(l);
}
