#ifndef LAYER_H
    #define LAYER_H

#include <cairo.h>
#include <gtk/gtk.h>

typedef struct {
    char *name;
    cairo_surface_t *surface;
    gboolean visible;
    double opacity;
} Layer;

Layer *layer_new_blank(const char *name, int w, int h);
Layer *layer_new_from_file(const char *filename);
void layer_free(Layer *l);

#endif
