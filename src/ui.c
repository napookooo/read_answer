#include <gtk/gtk.h>
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
  const char *checker;
} AppWidgets;

typedef struct {
  char *username;
  char *password;
  char *subject;
  char *ip;
} DB;

typedef struct {
  GtkWidget **entries;
  AppWidgets *widgets;
} DBDialogData;

static DB *db_creds = NULL;


char *fix_space(const char *input) {
  if (!input) return NULL;

  size_t space_count = 0;
  for (const char *p = input; *p; p++) {
    if (*p == ' ') space_count++;
  }

  size_t len = strlen(input);
  size_t new_len = len + space_count; 
  char *result = malloc(new_len + 1);
  if (!result) return NULL;

  char *dst = result;
  for (const char *p = input; *p; p++) {
    if (*p == ' ') {
      *dst++ = '\\';
      *dst++ = ' ';
    } else {
      *dst++ = *p;
    }
  }
  *dst = '\0';

  return result;
}

static void populate_file_list(AppWidgets *widgets);
char *escape_spaces(const char *path) {
  GString *result = g_string_new(NULL);
  for (const char *p = path; *p; p++) {
    if (*p == ' ') {
      g_string_append(result, "\\ ");
    } else {
      g_string_append_c(result, *p);
    }
  }
  return g_string_free(result, FALSE);
}

static void db_dialog_response(GtkDialog *dialog, int response, gpointer user_data) {
  DBDialogData *data = (DBDialogData *)user_data;
  GtkWidget **entries = data->entries;
  AppWidgets *widgets = data->widgets;

  if (response == GTK_RESPONSE_OK) {
    const char *username = gtk_editable_get_text(GTK_EDITABLE(entries[0]));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(entries[1]));
    const char *subject  = gtk_editable_get_text(GTK_EDITABLE(entries[2]));
    const char *ip       = gtk_editable_get_text(GTK_EDITABLE(entries[3]));

    if (!db_creds) {
      db_creds = g_malloc0(sizeof(DB));
    }

    g_free(db_creds->username);
    g_free(db_creds->password);
    g_free(db_creds->subject);
    g_free(db_creds->ip);

    db_creds->username = g_strdup(username);
    db_creds->password = g_strdup(password);
    db_creds->subject  = g_strdup(subject);
    db_creds->ip       = g_strdup(ip);

    // g_print("Stored DB creds: user=%s subject=%s ip=%s\n", db_creds->username, db_creds->subject, db_creds->ip);

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "mongoimport --uri 'mongodb://%s:%s@%s' --db=Answer_database --collection=%s --type=csv --headerline --file='%s/results.csv'",
             db_creds->username,
             db_creds->password,
             db_creds->ip,
             db_creds->subject,
             widgets->out_path);

    g_print("%s\n", cmd);
    system(cmd);
  }

  g_free(entries);
  g_free(data);
  gtk_window_destroy(GTK_WINDOW(dialog));
}

static void show_db_dialog(GtkButton *button, gpointer user_data) {
  AppWidgets *widgets = (AppWidgets *)user_data;
  GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(button)));

  GtkWidget *dialog = gtk_dialog_new_with_buttons(
    "Database Credentials",
    parent,
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    "_Cancel", GTK_RESPONSE_CANCEL,
    "_OK", GTK_RESPONSE_OK,
    NULL
  );

  GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
  gtk_widget_set_margin_top(grid, 10);
  gtk_widget_set_margin_bottom(grid, 10);
  gtk_widget_set_margin_start(grid, 10);
  gtk_widget_set_margin_end(grid, 10);
  gtk_box_append(GTK_BOX(content), grid);

  GtkWidget *entry_user = gtk_entry_new();
  GtkWidget *entry_pass = gtk_entry_new();
  GtkWidget *entry_subj = gtk_entry_new();
  GtkWidget *entry_ip = gtk_entry_new();

  gtk_entry_set_placeholder_text(GTK_ENTRY(entry_user), "Username");
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry_pass), "Password");
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry_subj), "Subject");
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry_ip), "IP address");

  gtk_entry_set_visibility(GTK_ENTRY(entry_pass), FALSE);

  // Add labels and entries
  gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Username:"), 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), entry_user, 1, 0, 1, 1);

  gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Password:"), 0, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), entry_pass, 1, 1, 1, 1);

  gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Subject:"), 0, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), entry_subj, 1, 2, 1, 1);

  gtk_grid_attach(GTK_GRID(grid), gtk_label_new("IP Address:"), 0, 3, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), entry_ip, 1, 3, 1, 1);

  // Store entries
  GtkWidget **entries = g_new(GtkWidget *, 4);
  entries[0] = entry_user;
  entries[1] = entry_pass;
  entries[2] = entry_subj;
  entries[3] = entry_ip;

  // Wrap entries with widgets for callback
  DBDialogData *dialog_data = g_malloc(sizeof(DBDialogData));
  dialog_data->entries = entries;
  dialog_data->widgets = widgets;

  g_signal_connect(dialog, "response", G_CALLBACK(db_dialog_response), dialog_data);

  gtk_widget_show(dialog);
}

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

void check_sel_smth(GObject *source_object, GAsyncResult *res, gpointer user_data) {
  GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
  AppWidgets *widgets = (AppWidgets *)user_data;

  GFile *file = gtk_file_dialog_open_finish(dialog, res, NULL);
  if (!file) {
    g_print("No file selected or dialog was cancelled.\n");
    return;
  }

  gchar *file_path = g_file_get_path(file);
  g_print("Selected output file: %s\n", file_path);

  // Store selected file path
  // g_free(widgets->checker);
  widgets->checker = g_strdup(file_path);

  g_free(file_path);
  g_object_unref(file);
}

void check_sel(GtkButton *button, gpointer user_data) {
  GtkFileDialog *dialog = gtk_file_dialog_new();
  GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(button)));

  gtk_file_dialog_open(dialog, parent, NULL, check_sel_smth, user_data);
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
  g_print("Selected folder again: %s\n", widgets->folder_path);
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
  AppWidgets *widgets = (AppWidgets *)user_data;
  if (!widgets->file_path || !widgets->checker || !widgets->out_path) {
    g_print("Error: Missing paths or parameters.\n");
    return;
  }
  char cmd[256];
  char cmd2[256];
  char *fin_path = fix_space(widgets->file_path);
  snprintf(cmd, sizeof(cmd), "./read_ans %s -f %s -o %s", fin_path, widgets->checker, widgets->out_path);
  snprintf(cmd2, sizeof(cmd2), "rm -rf %s/*", widgets->out_path);
  g_print("%s\n", cmd);
  g_print("%s\n", cmd2);
  system(cmd2);
  system(cmd);
  widgets->folder_path = widgets->out_path;
  populate_file_list(widgets);
}

static void func_write_dir(GtkButton *buttont, gpointer user_data) {
  AppWidgets *widgets = (AppWidgets *)user_data;
  if (!widgets->folder_path || !widgets->checker || !widgets->out_path) {
    g_print("Error: Missing paths or parameters.\n");
    return;
  }
  char cmd[256];
  char cmd2[256];
  char *fin_path = fix_space(widgets->folder_path);
  snprintf(cmd, sizeof(cmd), "./read_ans -d %s -f %s -o %s", fin_path, widgets->checker, widgets->out_path);
  snprintf(cmd2, sizeof(cmd2), "rm -rf %s/*", widgets->out_path);
  g_print("%s\n", cmd);
  g_print("%s\n", cmd2);
  system(cmd2);
  system(cmd);
  widgets->folder_path = widgets->out_path;
  populate_file_list(widgets);
}

// static void func_database(GtkButton *buttont, gpointer user_data) {
//   system("echo \"call database\"");
// }

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

  GtkWidget *check_btn = gtk_button_new_with_label("Pick the right answer");
  gtk_box_append(GTK_BOX(left_pane), check_btn);
  gtk_widget_set_margin_bottom(check_btn, 10);

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
  g_signal_connect(check_btn, "clicked", G_CALLBACK(check_sel), widgets);
  g_signal_connect(write, "clicked", G_CALLBACK(func_write), widgets);
  g_signal_connect(write_dir, "clicked", G_CALLBACK(func_write_dir), widgets);
  g_signal_connect(database, "clicked", G_CALLBACK(show_db_dialog), widgets);
  g_signal_connect_swapped(quit, "clicked", G_CALLBACK(gtk_window_destroy), GTK_WINDOW(window));

  gtk_widget_show(window);
  GtkWidget *prev_window = gtk_widget_get_ancestor(GTK_WIDGET(buttont), GTK_TYPE_WINDOW);
  if (prev_window) {
    gtk_window_destroy(GTK_WINDOW(prev_window));
  }
}

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
