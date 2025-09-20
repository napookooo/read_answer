#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  GtkApplication *app;
  GtkEntry *entry;
} AppEntry;

typedef struct {
  GtkWidget *image;
  GtkWidget *file_list;
  const char *folder_path;
  const char *out_path;
  const char *file_path;
} AppWidgets;

static void populate_file_list(AppWidgets *widgets);

void out_select_smth(GObject *source_object, GAsyncResult *res, gpointer user_data) {
  GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
  AppWidgets *widgets = (AppWidgets *)user_data;

  GFile *folder = gtk_file_dialog_select_folder_finish(dialog, res, NULL);
  if (!folder) {
    g_print("No folder selected or dialog was cancelled.\n");
    return;
  }

  gchar *folder_path = g_file_get_path(folder);
  g_print("Selected output folder: %s\n", folder_path);

  // Update folder path and populate list
  widgets->out_path = g_strdup(folder_path);
  populate_file_list(widgets);

  g_free(folder_path);
  g_object_unref(folder);
}

void out_select(GtkButton *button, gpointer user_data) {
  GtkFileDialog *dialog = gtk_file_dialog_new();
  GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(button)));

  gtk_file_dialog_select_folder(dialog, parent, NULL, out_select_smth, user_data);
}

void folder_select_smth(GObject *source_object, GAsyncResult *res, gpointer user_data) {
  GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
  AppWidgets *widgets = (AppWidgets *)user_data;

  GFile *folder = gtk_file_dialog_select_folder_finish(dialog, res, NULL);
  if (!folder) {
    g_print("No folder selected or dialog was cancelled.\n");
    return;
  }

  gchar *folder_path = g_file_get_path(folder);
  g_print("Selected folder: %s\n", folder_path);

  // Update folder path and populate list
  widgets->folder_path = g_strdup(folder_path);
  populate_file_list(widgets);

  g_free(folder_path);
  g_object_unref(folder);
}

void folder_select(GtkButton *button, gpointer user_data) {
  GtkFileDialog *dialog = gtk_file_dialog_new();
  GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(button)));

  gtk_file_dialog_select_folder(dialog, parent, NULL, folder_select_smth, user_data);
}

static void on_file_button_clicked(GtkButton *buttont, gpointer user_data) {
  AppWidgets *widgets = (AppWidgets *)user_data;
  const gchar *filename = gtk_button_get_label(buttont);

  gchar *filepath = g_build_filename(widgets->folder_path, filename, NULL);
  GFile *file = g_file_new_for_path(filepath);
  char *file_path = g_file_get_path(file);
  g_print("file path %s\n", file_path);
  widgets->file_path = g_strdup(file_path);
  gtk_picture_set_file(GTK_PICTURE(widgets->image), file);
  g_object_unref(file);
  g_free(filepath);
}

static void clear_box_children(GtkWidget *box) {
  GtkWidget *child = gtk_widget_get_first_child(box);
  while (child != NULL) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_widget_unparent(child);
    child = next;
  }
}

static void populate_file_list(AppWidgets *widgets) {
  clear_box_children(widgets->file_list);

  GFile *folder = g_file_new_for_path(widgets->folder_path);
  GFileEnumerator *enumerator = g_file_enumerate_children(
    folder,
    "standard::name,standard::type",
    G_FILE_QUERY_INFO_NONE,
    NULL,
    NULL
  );

  if (enumerator) {
    GFileInfo *info;
    while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL)) != NULL) {
      if (g_file_info_get_file_type(info) == G_FILE_TYPE_REGULAR) {
        const char *filename = g_file_info_get_name(info);
        GtkWidget *btn = gtk_button_new_with_label(filename);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_file_button_clicked), widgets);
        gtk_box_append(GTK_BOX(widgets->file_list), btn);
      }
      g_object_unref(info);
    }
    g_file_enumerator_close(enumerator, NULL, NULL);
    g_object_unref(enumerator);
  }

  g_object_unref(folder);
}

static void on_reload_clicked(GtkButton *buttont, gpointer user_data) {
  AppWidgets *widgets = (AppWidgets *)user_data;
  g_print("Folder : %s\n", widgets->folder_path);
  g_print("Out : %s\n", widgets->out_path);
  g_print("File : %s\n", widgets->file_path);
  populate_file_list(widgets);
}

static void func_write(GtkButton *buttont, gpointer user_data) {
  system("echo \"call read_answer\"");
}

static void func_write_dir(GtkButton *buttont, gpointer user_data) {
  system("echo \"call read_answer in directory mode\"");
}

static void func_database(GtkButton *buttont, gpointer user_data) {
  system("echo \"call database\"");
}

static void app_window(GtkButton *buttont, gpointer user_data) {
  GtkApplication *app = GTK_APPLICATION(user_data);
  GtkWidget *window;
  window = gtk_application_window_new (app);
  gtk_window_set_title(GTK_WINDOW(window), "Image Viewer");
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

  GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_window_set_child(GTK_WINDOW(window), main_box);

  // --- LEFT PANE ---
  GtkWidget *left_pane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_widget_set_size_request(left_pane, 220, -1);
  gtk_widget_set_margin_top(left_pane, 10);
  gtk_widget_set_margin_start(left_pane, 10);

  GtkWidget *reload_btn = gtk_button_new_with_label("Reload");
  gtk_box_append(GTK_BOX(left_pane), reload_btn);

  GtkWidget *folder_btn = gtk_button_new_with_label("Folders");
  gtk_box_append(GTK_BOX(left_pane), folder_btn);

  GtkWidget *out_btn = gtk_button_new_with_label("Output folder");
  gtk_box_append(GTK_BOX(left_pane), out_btn);
  gtk_widget_set_margin_bottom(out_btn, 10);

  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  GtkWidget *file_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), file_list);
  gtk_box_append(GTK_BOX(left_pane), scroll);

  gtk_box_append(GTK_BOX(main_box), left_pane);

  // --- RIGHT PANE ---
  GtkWidget *image_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_hexpand(image_box, TRUE);
  gtk_widget_set_vexpand(image_box, TRUE);

  GtkWidget *image = gtk_picture_new();
  gtk_picture_set_content_fit(GTK_PICTURE(image), GTK_CONTENT_FIT_CONTAIN);
  gtk_widget_set_hexpand(image, TRUE);
  gtk_widget_set_vexpand(image, TRUE);
  gtk_box_append(GTK_BOX(image_box), image);

  GtkWidget *write = gtk_button_new_with_label("Check 1 file");
  gtk_widget_set_margin_top(write, 10);
  gtk_box_append(GTK_BOX(image_box), write);
  GtkWidget *write_dir = gtk_button_new_with_label("Check entire directory");
  gtk_widget_set_margin_top(write_dir, 10);
  gtk_box_append(GTK_BOX(image_box), write_dir);
  GtkWidget *database = gtk_button_new_with_label("Send output to database");
  gtk_widget_set_margin_top(database, 10);
  gtk_box_append(GTK_BOX(image_box), database);
  GtkWidget *quit = gtk_button_new_with_label("Quit");
  gtk_widget_set_margin_top(quit, 10);
  gtk_box_append(GTK_BOX(image_box), quit);
  gtk_widget_set_margin_bottom(quit, 10);

  gtk_box_append(GTK_BOX(main_box), image_box);

  AppWidgets *widgets = g_malloc(sizeof(AppWidgets));
  const char *folder_path = "";
  const char *file_path= "";
  widgets->image = image;
  widgets->file_list = file_list;
  widgets->folder_path = folder_path;
  widgets->file_path = file_path;

  populate_file_list(widgets);

  g_signal_connect(reload_btn, "clicked", G_CALLBACK(on_reload_clicked), widgets);
  g_signal_connect(folder_btn, "clicked", G_CALLBACK(folder_select), widgets);
  g_signal_connect(out_btn, "clicked", G_CALLBACK(out_select), widgets);
  g_signal_connect(write, "clicked", G_CALLBACK(func_write), window);
  g_signal_connect(write_dir, "clicked", G_CALLBACK(func_write_dir), window);
  g_signal_connect(database, "clicked", G_CALLBACK(func_database), window);
  g_signal_connect_swapped(quit, "clicked", G_CALLBACK(gtk_window_destroy), GTK_WINDOW(window));

  gtk_widget_show(window);
  GtkWidget *prev_window = gtk_widget_get_ancestor(GTK_WIDGET(buttont), GTK_TYPE_WINDOW);
  if (prev_window) {
    gtk_window_destroy(GTK_WINDOW(prev_window));
  }
}

// static void app_window(GtkButton *buttont, gpointer user_data) {
//   GtkApplication *app = GTK_APPLICATION(user_data);
//   GtkWidget *window;
//   GtkWidget *grid;
//   GtkFileDialog *dialog;
//   GtkWidget *button;
//
//   window = gtk_application_window_new (app);
//   gtk_window_set_title (GTK_WINDOW (window), "Lovely Application");
//   grid = gtk_grid_new ();
//   gtk_window_set_child (GTK_WINDOW (window), grid);
//   gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
//
//   dialog = gtk_file_dialog_new();
//
//   button = gtk_button_new_with_label ("Choose folder");
//   g_signal_connect(button, "clicked", G_CALLBACK (folder_select), dialog);
//   gtk_grid_attach (GTK_GRID (grid), button, 0, 0, 1, 1);
//   gtk_widget_set_size_request(button, 200, 100);
//   gtk_widget_set_hexpand(button, TRUE);
//
//   button = gtk_button_new_with_label ("Quit");
//   g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_window_destroy), window);
//   gtk_grid_attach (GTK_GRID (grid), button, 0, 1, 2, 1);
//   gtk_widget_set_size_request(button, 200, 100);
//   gtk_widget_set_hexpand(button, TRUE);
//
//   gtk_window_present (GTK_WINDOW (window));
//   GtkWidget *prev_window = gtk_widget_get_ancestor(GTK_WIDGET(buttont), GTK_TYPE_WINDOW);
//   if (prev_window) {
//     gtk_window_destroy(GTK_WINDOW(prev_window));
//   }
// }

static void check_entry(GtkButton *buttont, gpointer user_data) {
  AppEntry *data = (AppEntry *)user_data;
  GtkApplication *app = data->app;
  GtkEntry *entry = data->entry;
  char *text = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
  if (!strcmp("right", text)){
    g_signal_connect(buttont, "clicked", G_CALLBACK(app_window), app);
  }
}

static void login(GtkButton *buttont, gpointer user_data) {
  GtkApplication *app = GTK_APPLICATION(user_data);
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *entry;
  GtkWidget *button;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Login");
  grid = gtk_grid_new ();
  gtk_window_set_child (GTK_WINDOW (window), grid);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 10);

  entry = gtk_entry_new();
  gtk_grid_attach (GTK_GRID (grid), entry, 0, 0, 1, 1);
  gtk_widget_set_size_request(entry, 200, 100);
  gtk_widget_set_hexpand(entry, TRUE);

  button = gtk_button_new_with_label ("Check");
  AppEntry *data = g_malloc(sizeof(AppEntry));
  data->app = app;
  data->entry = GTK_ENTRY(entry);
  g_signal_connect(button, "clicked", G_CALLBACK(check_entry), data);
  gtk_grid_attach (GTK_GRID (grid), button, 0, 1, 1, 1);
  gtk_widget_set_size_request(button, 200, 100);
  gtk_widget_set_hexpand(button, TRUE);

  button = gtk_button_new_with_label ("Quit");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_window_destroy), window);
  gtk_grid_attach (GTK_GRID (grid), button, 0, 2, 1, 1);
  gtk_widget_set_size_request(button, 200, 100);
  gtk_widget_set_hexpand(button, TRUE);

  gtk_window_present (GTK_WINDOW (window));

  GtkWidget *prev_window = gtk_widget_get_ancestor(GTK_WIDGET(buttont), GTK_TYPE_WINDOW);
  if (prev_window) {
    gtk_window_destroy(GTK_WINDOW(prev_window));
  }
}

static void activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window;
  GtkWidget *grid;
  GtkWidget *button;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Welcome");
  grid = gtk_grid_new ();
  gtk_window_set_child (GTK_WINDOW (window), grid);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 10);

  button = gtk_button_new_with_label ("Enter the app");
  g_signal_connect(button, "clicked", G_CALLBACK(login), app);
  gtk_grid_attach (GTK_GRID (grid), button, 0, 0, 1, 1);
  gtk_widget_set_size_request(button, 200, 100);
  gtk_widget_set_hexpand(button, TRUE);

  button = gtk_button_new_with_label ("Quit");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_window_destroy), window);
  gtk_grid_attach (GTK_GRID (grid), button, 0, 1, 2, 1);
  gtk_widget_set_size_request(button, 200, 100);
  gtk_widget_set_hexpand(button, TRUE);

  gtk_window_present (GTK_WINDOW (window));
}

int main(int argc, char **argv) {

  (void) argc;
  (void) argv;

  GtkApplication *app;
  int status;

  app = gtk_application_new("org.gtk.ReadImageUI", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}
