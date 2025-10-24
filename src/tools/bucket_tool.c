#include "tools.h"
#include "app_state.h"
#include "layer.h"
#include <cairo.h>
#include <math.h>
#include <stdlib.h>

typedef struct {
    unsigned char r, g, b, a;
} Pixel;

static gboolean colors_equal(Pixel a, Pixel b, int tolerance)
{
    return abs(a.r - b.r) <= tolerance &&
           abs(a.g - b.g) <= tolerance &&
           abs(a.b - b.b) <= tolerance &&
           abs(a.a - b.a) <= tolerance;
}

// Get pixel from a Cairo surface
static Pixel get_pixel(cairo_surface_t *surface, int x, int y)
{
    unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    unsigned char *p = data + y * stride + x * 4;
    Pixel px = { p[2], p[1], p[0], p[3] }; // Cairo stores BGRA
    return px;
}

// Set pixel on a Cairo surface
static void set_pixel(cairo_surface_t *surface, int x, int y, Pixel px)
{
    unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    unsigned char *p = data + y * stride + x * 4;
    p[2] = px.r;
    p[1] = px.g;
    p[0] = px.b;
    p[3] = px.a;
}

static void flood_fill(AppState *app, cairo_surface_t *surface, int x, int y)
{
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);

    cairo_surface_flush(surface);

    unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);

    Pixel target = get_pixel(surface, x, y);

    GdkRGBA c = app->brush_color;
    Pixel fill = {
        (unsigned char)(c.red * 255),
        (unsigned char)(c.green * 255),
        (unsigned char)(c.blue * 255),
        (unsigned char)(c.alpha * 255)
    };

    if (colors_equal(target, fill, 0)) return; // No need to fill same color

    bool *visited = calloc(width * height, sizeof(bool));
    if (!visited) return;

    // Simple stack-based flood fill (4-connected)
    typedef struct { int x, y; } Point;
    Point *stack = malloc(width * height * sizeof(Point));
    int sp = 0;
    stack[sp++] = (Point){ x, y };

    while (sp > 0) {
        Point p = stack[--sp];
        if (p.x < 0 || p.y < 0 || p.x >= width || p.y >= height)
            continue;
        int idx = p.y * width + p.x;
        if (visited[idx]) continue;

        Pixel cur = get_pixel(surface, p.x, p.y);
        if (!colors_equal(cur, target, 10)) continue; // tolerance of 10

        visited[idx] = TRUE;
        set_pixel(surface, p.x, p.y, fill);

        stack[sp++] = (Point){ p.x + 1, p.y };
        stack[sp++] = (Point){ p.x - 1, p.y };
        stack[sp++] = (Point){ p.x, p.y + 1 };
        stack[sp++] = (Point){ p.x, p.y - 1 };
    }

    free(stack);
    free(visited);

    cairo_surface_mark_dirty(surface);
}

static void on_button_press(AppState *app, double x, double y)
{
    if (!app->active_layer || !app->active_layer->surface)
        return;

    int px = (int)round(x);
    int py = (int)round(y);

    flood_fill(app, app->active_layer->surface, px, py);
}

static void on_motion(AppState *app, double x, double y) { (void)app; (void)x; (void)y; }
static void on_button_release(AppState *app, double x, double y) { (void)app; (void)x; (void)y; }

Tool TOOL_BUCKET = {
    .name = "Bucket Fill",
    .on_button_press = on_button_press,
    .on_motion = on_motion,
    .on_button_release = on_button_release
};
