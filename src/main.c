#include <cairo.h>
#include <math.h>
#include <gtk/gtk.h>
#include <string.h>

#include "layer.h"

#define DEFAULT_CANVAS_W 255
#define DEFAULT_CANVAS_H 255

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
    double zoom;

    GtkCssProvider *css_provider;
} AppState;


static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    AppState *app = user_data;

    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    int vw = alloc.width;
    int vh = alloc.height;

    cairo_surface_t *tmp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, vw, vh);
    cairo_t *tmp_cr = cairo_create(tmp_surface);

    cairo_set_source_rgb(tmp_cr, 1, 1, 1);
    cairo_paint(tmp_cr);

    int canvas_x0 = (int)floor(app->pan_x);
    int canvas_y0 = (int)floor(app->pan_y);

    cairo_scale(tmp_cr, app->zoom, app->zoom);

    for (GList *it = app->layers; it != NULL; it = it->next) {
        Layer *l = it->data;
        if (!l->visible || !l->surface) continue;

        cairo_save(tmp_cr);

        cairo_set_source_surface(tmp_cr, l->surface,
                                 -canvas_x0, -canvas_y0);
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

static
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
void build_ui(AppState *app)
{
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "GIMP - Layers Demo");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1200, 800);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_container_add(GTK_CONTAINER(app->window), hpaned);

    GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_paned_pack1(GTK_PANED(hpaned), left_vbox, FALSE, FALSE);

    GtkWidget *add_btn = gtk_button_new_with_label("Add Image Layer");
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_layer), app);
    gtk_box_pack_start(GTK_BOX(left_vbox), add_btn, FALSE, FALSE, 2);

    GtkWidget *new_btn = gtk_button_new_with_label("New Blank Layer");
    g_signal_connect(new_btn, "clicked", G_CALLBACK(on_new_blank_layer), app);
    gtk_box_pack_start(GTK_BOX(left_vbox), new_btn, FALSE, FALSE, 2);

    GtkWidget *layer_label = gtk_label_new("Layers");
    gtk_box_pack_start(GTK_BOX(left_vbox), layer_label, FALSE, FALSE, 2);

    app->layer_list_box = gtk_list_box_new();
    gtk_box_pack_start(GTK_BOX(left_vbox), app->layer_list_box, TRUE, TRUE, 2);

    GtkWidget *right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_paned_pack2(GTK_PANED(hpaned), right_vbox, TRUE, FALSE);

    app->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(app->drawing_area, TRUE);
    gtk_widget_set_vexpand(app->drawing_area, TRUE);

    gtk_widget_add_events(app->drawing_area,
        GDK_BUTTON_PRESS_MASK
        | GDK_BUTTON_RELEASE_MASK
        | GDK_POINTER_MOTION_MASK
        | GDK_SCROLL_MASK
        | GDK_BUTTON_MOTION_MASK);

    g_signal_connect(app->drawing_area, "draw", G_CALLBACK(on_draw_event), app);
    g_signal_connect(app->drawing_area, "scroll-event", G_CALLBACK(on_scroll), app);

    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), app->drawing_area);
    gtk_box_pack_start(GTK_BOX(right_vbox), frame, TRUE, TRUE, 2);
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    AppState *app = g_new0(AppState, 1);

    app->pan_x = 0.0;
    app->pan_y = 0.0;
    app->zoom = 1.0;
    app->current_tool = TOOL_BRUSH;
    app->brush_radius = 10.0;
    gdk_rgba_parse(&app->brush_color, "#000000");

    build_ui(app);

    Layer *base = layer_new_blank("Base", DEFAULT_CANVAS_W, DEFAULT_CANVAS_H);
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
