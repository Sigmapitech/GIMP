#include <cairo.h>
#include <gtk/gtk.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "app_state.h"
#include "layer.h"

#define DEFAULT_CANVAS_W 512
#define DEFAULT_CANVAS_H 512

static
void layer_fill_checkerboard(Layer *base, int cell_size)
{
    if (!base || !base->surface) return;

    int w = cairo_image_surface_get_width(base->surface);
    int h = cairo_image_surface_get_height(base->surface);
    cairo_t *cr = cairo_create(base->surface);

    for (int y = 0; y < h; y += cell_size) {
        for (int x = 0; x < w; x += cell_size) {
            if (((x / cell_size) + (y / cell_size)) % 2 == 0)
                cairo_set_source_rgb(cr, 0.8, 0.8, 0.8); // light gray
            else
                cairo_set_source_rgb(cr, 0.6, 0.6, 0.6); // dark gray
            cairo_rectangle(cr, x, y, cell_size, cell_size);
            cairo_fill(cr);
        }
    }

    cairo_destroy(cr);
}

static
gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    AppState *app = user_data;

    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    int vw = alloc.width;
    int vh = alloc.height;

    cairo_surface_t *tmp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, vw, vh);
    cairo_t *tmp_cr = cairo_create(tmp_surface);

    GdkRGBA bg;
    GtkStyleContext *ctx = gtk_widget_get_style_context(widget);
    gtk_render_background(ctx, cr, 0, 0, alloc.width, alloc.height);
    cairo_set_source_rgba(tmp_cr, bg.red, bg.green, bg.blue, bg.alpha);

    cairo_scale(tmp_cr, app->zoom, app->zoom);
    cairo_paint(tmp_cr);

    int canvas_x0 = (int)floor(app->pan_x);
    int canvas_y0 = (int)floor(app->pan_y);

    for (GList *it = app->layers; it != NULL; it = it->next) {
        Layer *l = it->data;
        if (!l->visible || !l->surface) continue;

        cairo_save(tmp_cr);

        cairo_set_source_surface(tmp_cr, l->surface, -canvas_x0, -canvas_y0);
        cairo_pattern_t *pattern = cairo_get_source(tmp_cr);
        cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
        cairo_paint_with_alpha(tmp_cr, l->opacity);

        cairo_restore(tmp_cr);
    }

    cairo_set_source_surface(cr, tmp_surface, 0, 0);
    cairo_pattern_t *pattern = cairo_get_source(cr);
    cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
    cairo_paint(cr);
    cairo_destroy(tmp_cr);
    cairo_surface_destroy(tmp_surface);
    return FALSE;
}

static
void on_brush_button(GtkButton *btn, gpointer user_data)
{
    AppState *app = user_data;
    app->current_tool = &TOOL_BRUSH;
}

static
void on_eraser_button(GtkButton *btn, gpointer user_data)
{
    AppState *app = user_data;
    app->current_tool = &TOOL_ERASER;
}

static
void on_bucket_button(GtkButton *btn, gpointer user_data)
{
    AppState *app = user_data;
    app->current_tool = &TOOL_BUCKET;
}

static void on_brush_radius_changed(GtkRange *range, gpointer user_data)
{
    AppState *app = user_data;
    app->brush_radius = gtk_range_get_value(range);
}

static
void destroy_widget_cb(GtkWidget *widget, gpointer user_data)
{
    gtk_widget_destroy(widget);
}

gboolean on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
    AppState *app = user_data;

    if (!(event->state & GDK_CONTROL_MASK))
        return FALSE;

    double old_zoom = app->zoom;
    double mx = event->x;
    double my = event->y;

    if (event->direction == GDK_SCROLL_UP) app->zoom *= 1.1;
    else if (event->direction == GDK_SCROLL_DOWN) app->zoom /= 1.1;

    app->pan_x += mx / old_zoom - mx / app->zoom;
    app->pan_y += my / old_zoom - my / app->zoom;

    gtk_widget_queue_draw(app->drawing_area);
    return TRUE;
}

static
void widget_to_canvas(AppState *app, double wx, double wy, double *cx, double *cy)
{
    *cx = wx / app->zoom + app->pan_x;
    *cy = wy / app->zoom + app->pan_y;
}

static
void on_layer_visibility_toggled(GtkToggleButton *toggle, gpointer user_data)
{
    Layer *l = user_data;
    l->visible = gtk_toggle_button_get_active(toggle);
    AppState *app = g_object_get_data(G_OBJECT(toggle), "appstate");
    gtk_widget_queue_draw(app->drawing_area);
}

static
GtkWidget *create_layer_row(AppState *app, Layer *l)
{
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *visible = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(visible), l->visible);
    g_object_set_data(G_OBJECT(visible), "appstate", app);
    g_signal_connect(visible, "toggled", G_CALLBACK(on_layer_visibility_toggled), l);

    GtkWidget *label = gtk_label_new(l->name);
    gtk_widget_set_halign(label, GTK_ALIGN_START);

    gtk_box_pack_start(GTK_BOX(hbox), visible, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 2);

    return hbox;
}

static
void refresh_layer_list(AppState *app)
{
    gtk_list_box_prepend(GTK_LIST_BOX(app->layer_list_box), gtk_label_new(""));

    GList *children = g_list_copy(app->layers);
    gtk_container_foreach(GTK_CONTAINER(app->layer_list_box), (GtkCallback)destroy_widget_cb, NULL);
    for (GList *it = g_list_last(children); it != NULL; it = it->prev) {
        Layer *l = it->data;
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *row_content = create_layer_row(app, l);
        gtk_container_add(GTK_CONTAINER(row), row_content);
        g_object_set_data(G_OBJECT(row), "layer_ptr", l);
        gtk_list_box_insert(GTK_LIST_BOX(app->layer_list_box), row, -1);
        if (l == app->active_layer)
            gtk_list_box_select_row(GTK_LIST_BOX(app->layer_list_box), GTK_LIST_BOX_ROW(row));
        gtk_widget_show_all(row);
    }
    g_list_free(children);
}


void on_add_layer(GtkButton *btn, gpointer user_data)
{
    AppState *app = user_data;
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Open Image as Layer",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Image files");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_filter_add_mime_type(filter, "image/gif");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        Layer *l = layer_new_from_file(filename);
        if (l) {
            app->layers = g_list_append(app->layers, l);
            app->active_layer = l;
            refresh_layer_list(app);
            gtk_widget_queue_draw(app->drawing_area);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static
void on_new_blank_layer(GtkButton *btn, gpointer user_data)
{
    AppState *app = user_data;
    Layer *l = layer_new_blank("Layer", DEFAULT_CANVAS_W, DEFAULT_CANVAS_H);

    app->layers = g_list_append(app->layers, l);
    app->active_layer = l;
    refresh_layer_list(app);
    gtk_widget_queue_draw(app->drawing_area);
}

static
gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    AppState *app = user_data;
    double cx, cy;
    widget_to_canvas(app, event->x, event->y, &cx, &cy);

    if (event->button == GDK_BUTTON_PRIMARY && app->current_tool && app->current_tool->on_button_press)
        app->current_tool->on_button_press(app, cx, cy);

    app->last_mouse_x = event->x;
    app->last_mouse_y = event->y;
    gtk_widget_queue_draw(app->drawing_area);
    return TRUE;
}

static
gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
    AppState *app = user_data;
    double cx, cy;
    widget_to_canvas(app, event->x, event->y, &cx, &cy);

    if (app->current_tool && app->current_tool->on_motion)
        app->current_tool->on_motion(app, cx, cy);

    gtk_widget_queue_draw(app->drawing_area);
    return TRUE;
}

static
gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    AppState *app = user_data;
    double cx, cy;
    widget_to_canvas(app, event->x, event->y, &cx, &cy);

    if (event->button == GDK_BUTTON_PRIMARY && app->current_tool && app->current_tool->on_button_release)
        app->current_tool->on_button_release(app, cx, cy);

    gtk_widget_queue_draw(app->drawing_area);
    return TRUE;
}

static gboolean is_dark = TRUE;

static void on_toggle_theme(GtkButton *button, gpointer user_data)
{
    is_dark = !is_dark;

    GtkSettings *settings = gtk_settings_get_default();
    g_object_set(settings, "gtk-application-prefer-dark-theme", is_dark, NULL);

    gtk_button_set_label(button, is_dark ? "Switch to Light" : "Switch to Dark");
}

static
void on_layer_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    AppState *app = user_data;
    if (!row) return;

    Layer *l = g_object_get_data(G_OBJECT(row), "layer_ptr");
    if (l) {
        app->active_layer = l;
        gtk_widget_queue_draw(app->drawing_area);
    }
}

void build_ui(AppState *app)
{
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "GIMP - Layers Demo");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1400, 800);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_add(GTK_CONTAINER(app->window), main_hbox);

    GtkWidget *tools_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(main_hbox), tools_vbox, FALSE, FALSE, 4);

    GtkWidget *brush_btn = gtk_button_new_with_label("Brush");
    g_signal_connect(brush_btn, "clicked", G_CALLBACK(on_brush_button), app);
    gtk_box_pack_start(GTK_BOX(tools_vbox), brush_btn, FALSE, FALSE, 2);

    GtkWidget *eraser_btn = gtk_button_new_with_label("Eraser");
    g_signal_connect(eraser_btn, "clicked", G_CALLBACK(on_eraser_button), app);
    gtk_box_pack_start(GTK_BOX(tools_vbox), eraser_btn, FALSE, FALSE, 2);

    GtkWidget *bucket_btn = gtk_button_new_with_label("Bucket");
    g_signal_connect(bucket_btn, "clicked", G_CALLBACK(on_bucket_button), app);
    gtk_box_pack_start(GTK_BOX(tools_vbox), bucket_btn, FALSE, FALSE, 2);

    GtkWidget *radius_label = gtk_label_new("Brush Radius");
    gtk_box_pack_start(GTK_BOX(tools_vbox), radius_label, FALSE, FALSE, 2);
    GtkWidget *radius_slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, 50, 1);
    gtk_range_set_value(GTK_RANGE(radius_slider), app->brush_radius);
    g_signal_connect(radius_slider, "value-changed", G_CALLBACK(on_brush_radius_changed), app);
    gtk_box_pack_start(GTK_BOX(tools_vbox), radius_slider, FALSE, FALSE, 2);

    GtkWidget *theme_btn = gtk_button_new_with_label("Switch Theme");
    g_signal_connect(theme_btn, "clicked", G_CALLBACK(on_toggle_theme), app);
    gtk_box_pack_start(GTK_BOX(tools_vbox), theme_btn, FALSE, FALSE, 2);

    GtkWidget *center_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(main_hbox), center_vbox, TRUE, TRUE, 4);

    app->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(app->drawing_area, TRUE);
    gtk_widget_set_vexpand(app->drawing_area, TRUE);
    gtk_widget_add_events(app->drawing_area,
        GDK_BUTTON_PRESS_MASK |
        GDK_BUTTON_RELEASE_MASK |
        GDK_POINTER_MOTION_MASK |
        GDK_SCROLL_MASK |
        GDK_BUTTON_MOTION_MASK);
    g_signal_connect(app->drawing_area, "draw", G_CALLBACK(on_draw_event), app);
    g_signal_connect(app->drawing_area, "button-press-event", G_CALLBACK(on_button_press), app);
    g_signal_connect(app->drawing_area, "button-release-event", G_CALLBACK(on_button_release), app);
    g_signal_connect(app->drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify), app);
    g_signal_connect(app->drawing_area, "scroll-event", G_CALLBACK(on_scroll), app);

    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), app->drawing_area);
    gtk_box_pack_start(GTK_BOX(center_vbox), frame, TRUE, TRUE, 2);

    GtkWidget *layers_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(main_hbox), layers_vbox, FALSE, FALSE, 4);

    GtkWidget *add_btn = gtk_button_new_with_label("Add Image Layer");
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_layer), app);
    gtk_box_pack_start(GTK_BOX(layers_vbox), add_btn, FALSE, FALSE, 2);

    GtkWidget *new_btn = gtk_button_new_with_label("New Blank Layer");
    g_signal_connect(new_btn, "clicked", G_CALLBACK(on_new_blank_layer), app);
    gtk_box_pack_start(GTK_BOX(layers_vbox), new_btn, FALSE, FALSE, 2);

    GtkWidget *layer_label = gtk_label_new("Layers");
    gtk_box_pack_start(GTK_BOX(layers_vbox), layer_label, FALSE, FALSE, 2);

    app->layer_list_box = gtk_list_box_new();
    gtk_widget_set_size_request(app->layer_list_box, 200, -1);
    g_signal_connect(app->layer_list_box, "row-selected",
                     G_CALLBACK(on_layer_row_selected), app);
    gtk_box_pack_start(GTK_BOX(layers_vbox), app->layer_list_box, TRUE, TRUE, 2);
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    AppState *app = g_new0(AppState, 1);

    app->pan_x = 0.0;
    app->pan_y = 0.0;
    app->zoom = 1.0;
    app->current_tool = &TOOL_BRUSH;
    app->brush_radius = 10.0;
    gdk_rgba_parse(&app->brush_color, "#000000");

    GtkSettings *settings = gtk_settings_get_default();
    g_object_set(settings, "gtk-application-prefer-dark-theme", is_dark, NULL);

    build_ui(app);

    Layer *base = layer_new_blank("Base", DEFAULT_CANVAS_W, DEFAULT_CANVAS_H);
    layer_fill_checkerboard(base, 10);
    app->layers = g_list_append(app->layers, base);
    app->active_layer = base;
    refresh_layer_list(app);

    gtk_widget_show_all(app->window);
    gtk_main();

    for (GList *it = app->layers; it != NULL; it = it->next) {
        layer_free(it->data);
    }
    g_list_free(app->layers);
    g_object_unref(app->css_provider);
    g_free(app);
    return 0;
}
