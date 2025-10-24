#ifndef APP_STATE_H
    #define APP_STATE_H

    #include <gtk/gtk.h>
    #include <stdbool.h>

    #include "layer.h"

typedef enum { TOOL_PAN, TOOL_BRUSH } ToolType;

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    GtkWidget *layer_list_box;
    GList *layers;
    Layer *active_layer;

    ToolType current_tool;
    double brush_radius;
    GdkRGBA brush_color;

    double pan_x;
    double pan_y;
    double last_mouse_x, last_mouse_y;
    double zoom;

    bool is_drawing;

    GtkCssProvider *css_provider;
} AppState;

#endif
