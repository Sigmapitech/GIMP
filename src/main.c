#include <gtk/gtk.h>

typedef struct {
    GtkWidget *image;
    GtkWidget *window;
} AppWidgets;

static void on_open_image(GtkButton *button, AppWidgets *app) {
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new(
        "Open Image",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Image files");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_filter_add_mime_type(filter, "image/gif");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        GError *error = NULL;
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
        if (!pixbuf) {
            g_warning("Could not open image: %s", error->message);
            g_error_free(error);
        } else {
            gtk_image_set_from_pixbuf(GTK_IMAGE(app->image), pixbuf);
            g_object_unref(pixbuf);
        }
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    AppWidgets *app = g_slice_new(AppWidgets);

    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Mini GIMP (GTK3)");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 800, 600);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(app->window), vbox);

    GtkWidget *button = gtk_button_new_with_label("Open Image");
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    app->image = gtk_image_new();
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled), app->image);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    g_signal_connect(button, "clicked", G_CALLBACK(on_open_image), app);

    gtk_widget_show_all(app->window);
    gtk_main();

    g_slice_free(AppWidgets, app);
    return 0;
}
