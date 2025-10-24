#include <cairo.h>
#include <math.h>

#include "app_state.h"
#include "layer.h"
#include "tools.h"

static gboolean is_drawing = FALSE;
static double last_x, last_y;

static void brush_point(AppState *app, double cx, double cy)
{
    if (!app->active_layer || !app->active_layer->surface)
        return;

    cairo_t *cr = cairo_create(app->active_layer->surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    // Use the current brush color from app state
    cairo_set_source_rgba(cr,
        app->brush_color.red,
        app->brush_color.green,
        app->brush_color.blue,
        app->brush_color.alpha);

    cairo_arc(cr, cx, cy, app->brush_radius, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_destroy(cr);
}

static void brush_line(AppState *app, double x0, double y0, double x1, double y1)
{
    double dx = x1 - x0;
    double dy = y1 - y0;
    double dist = sqrt(dx*dx + dy*dy);
    int steps = fmax(1, dist / (app->brush_radius * 0.5));

    for (int i = 0; i <= steps; i++) {
        double t = (double)i / steps;
        double x = x0 + dx * t;
        double y = y0 + dy * t;
        brush_point(app, x, y);
    }
}

static void on_button_press(AppState *app, double x, double y)
{
    is_drawing = TRUE;
    last_x = x;
    last_y = y;
    brush_point(app, x, y);
}

static void on_motion(AppState *app, double x, double y)
{
    if (is_drawing)
        brush_line(app, last_x, last_y, x, y);

    last_x = x;
    last_y = y;
}

static void on_button_release(AppState *app, double x, double y)
{
    is_drawing = FALSE;
}

Tool TOOL_BRUSH = {
    .name = "Brush",
    .on_button_press = on_button_press,
    .on_motion = on_motion,
    .on_button_release = on_button_release
};

