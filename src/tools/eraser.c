#include "app_state.h"
#include "layer.h"
#include <cairo.h>
#include <math.h>

#include "tools.h"

static gboolean is_erasing = FALSE;
static double last_x, last_y;

static void erase_point(AppState *app, double cx, double cy)
{
    if (!app->active_layer || !app->active_layer->surface)
        return;

    cairo_t *cr = cairo_create(app->active_layer->surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_arc(cr, cx, cy, app->brush_radius, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_destroy(cr);
}

static void erase_line(AppState *app, double x0, double y0, double x1, double y1)
{
    double dx = x1 - x0;
    double dy = y1 - y0;
    double dist = sqrt(dx*dx + dy*dy);
    int steps = fmax(1, dist / (app->brush_radius * 0.5));

    for (int i = 0; i <= steps; i++) {
        double t = (double)i / steps;
        double x = x0 + dx * t;
        double y = y0 + dy * t;
        erase_point(app, x, y);
    }
}

static void on_button_press(AppState *app, double x, double y)
{
    is_erasing = TRUE;
    erase_point(app, x, y);
}

static void on_motion(AppState *app, double x, double y)
{
    if (is_erasing)
        erase_line(app, last_x, last_y, x, y);

    last_x = x;
    last_y = y;
}

static void on_button_release(AppState *app, double x, double y)
{
    is_erasing = FALSE;
}

Tool TOOL_ERASER = {
    .name = "Eraser",
    .on_button_press = on_button_press,
    .on_motion = on_motion,
    .on_button_release = on_button_release
};
