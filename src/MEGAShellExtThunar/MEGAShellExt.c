
#include "MEGAShellExt.h"
#include "mega_ext_client.h"
#include <string.h>

G_MODULE_EXPORT void thunar_extension_initialize(ThunarxProviderPlugin *plugin);
G_MODULE_EXPORT void thunar_extension_shutdown(void);
G_MODULE_EXPORT void thunar_extension_list_types(const GType **types, gint *n_types);

static void mega_ext_menu_provider_init(ThunarxMenuProviderIface *iface);
static void mega_ext_finalize(GObject *object);
static GList* mega_ext_get_file_actions(ThunarxMenuProvider *provider, GtkWidget *window, GList *files);
static GList* mega_ext_get_folder_actions(ThunarxMenuProvider *provider, GtkWidget *window, ThunarxFileInfo *folder);
static gboolean mega_ext_path_in_sync(MEGAExt *mega_ext, const gchar *path);

static GType type_list[1];

#ifdef USING_THUNAR3
    #define THUNARITEM ThunarxMenuItem
#else
    #define THUNARITEM GtkAction
#endif


void thunar_extension_initialize(ThunarxProviderPlugin *plugin)
{
    const gchar *mismatch;
    mismatch = thunarx_check_version(THUNARX_MAJOR_VERSION, THUNARX_MINOR_VERSION, THUNARX_MICRO_VERSION);
    if (G_UNLIKELY(mismatch != NULL)) {
        g_warning("Version mismatch: %s", mismatch);
        return;
    }
    g_message("Initializing MEGAsync extension");
    mega_ext_register_type(plugin);
    type_list[0] = MEGA_TYPE_EXT;
}

void thunar_extension_shutdown(void)
{
    g_message("Shutting down MEGAsync extension");
}

void thunar_extension_list_types(const GType **types, gint *n_types)
{
    *types = type_list;
    *n_types = G_N_ELEMENTS(type_list);
}


THUNARX_DEFINE_TYPE_WITH_CODE(MEGAExt,
    mega_ext,
    G_TYPE_OBJECT,
    THUNARX_IMPLEMENT_INTERFACE(THUNARX_TYPE_MENU_PROVIDER, mega_ext_menu_provider_init)
);

static void mega_ext_class_init(MEGAExtClass *klass)
{
    GObjectClass *gobject_class;
    gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = mega_ext_finalize;
}

static void mega_ext_finalize(GObject *object)
{
    (*G_OBJECT_CLASS (mega_ext_parent_class)->finalize)(object);
}

static void mega_ext_init(MEGAExt *mega_ext)
{
    mega_ext->srv_sock = -1;
    mega_ext->chan = NULL;
    mega_ext->num_retries = 2;
    mega_ext->h_syncs = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    mega_ext->string_getlink = NULL;
    mega_ext->string_viewonmega = NULL;
    mega_ext->string_viewprevious = NULL;
    mega_ext->string_upload = NULL;
    mega_ext->syncs_received = FALSE;

    // ignore SIGPIPE as we most likely will write to a closed socket in mega_notify_client_read()
    signal(SIGPIPE, SIG_IGN);
}

static void mega_ext_menu_provider_init(ThunarxMenuProviderIface *iface)
{
#ifdef USING_THUNAR3
    iface->get_file_menu_items = mega_ext_get_file_actions;
    iface->get_folder_menu_items = mega_ext_get_folder_actions;
#else
    iface->get_file_actions = mega_ext_get_file_actions;
    iface->get_folder_actions = mega_ext_get_folder_actions;
#endif
}

static const gchar *file_state_to_str(FileState state)
{
    switch(state) {
        case RESPONSE_SYNCED:
            return "synced";
        case RESPONSE_PENDING:
            return "pending";
        case RESPONSE_SYNCING:
            return "syncing";
        case RESPONSE_IGNORED:
            return "ignored";
        case RESPONSE_PAUSED:
            return "paused";
        case RESPONSE_ERROR:
            return "error";
        case RESPONSE_DEFAULT:
        default:
            return "notfound";
    }
}

// user clicked on "Upload to MEGA" menu item
static void mega_ext_on_upload_selected(THUNARITEM *item, gpointer user_data)
{
    MEGAExt *mega_ext = MEGA_EXT(user_data);
    GList *l;
    GList *files;
    gboolean flag = FALSE;

    files = g_object_get_data(G_OBJECT(item), "MEGAExtension::files");
    for (l = files; l != NULL; l = l->next) {
        ThunarxFileInfo *file = THUNARX_FILE_INFO(l->data);
        FileState state;
        gchar *path;
        GFile *fp;

        fp = thunarx_file_info_get_location(file);
        if (!fp)
            continue;

        path = g_file_get_path(fp);
        if (!path)
            continue;

        state = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(file), "MEGAExtension::state"));

        if (state != RESPONSE_SYNCED && state != RESPONSE_PENDING && state != RESPONSE_SYNCING)
        {
            if (mega_ext_client_upload(mega_ext, path))
                flag = TRUE;
        }
        g_free(path);
    }

    if (flag)
        mega_ext_client_end_request(mega_ext);
}

void expanselocalpath(const char *path, char *absolutepath)
{
    if (strlen(path) && path[0] == '/')
    {
        strcpy(absolutepath, path);

        char canonical[PATH_MAX];
        if (realpath(absolutepath,canonical) != NULL)
        {
            strcpy(absolutepath, canonical);
        }
        return;
    }
}

// user clicked on "Get MEGA link" menu item
static void mega_ext_on_get_link_selected(THUNARITEM *item, gpointer user_data)
{
    MEGAExt *mega_ext = MEGA_EXT(user_data);
    GList *l;
    GList *files;
    gboolean flag = FALSE;

    files = g_object_get_data(G_OBJECT(item), "MEGAExtension::files");
    for (l = files; l != NULL; l = l->next) {
        ThunarxFileInfo *file = THUNARX_FILE_INFO(l->data);
        FileState state;
        gchar *path;
        GFile *fp;

        fp = thunarx_file_info_get_location(file);
        if (!fp)
            continue;

        path = g_file_get_path(fp);
        if (!path)
            continue;

        state = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(file), "MEGAExtension::state"));

        if (state == RESPONSE_SYNCED) {
            if (mega_ext_client_paste_link(mega_ext, path))
                flag = TRUE;
        }
        g_free(path);
    }

    if (flag)
        mega_ext_client_end_request(mega_ext);
}


// user clicked on "View on MEGA" menu item
static void mega_ext_on_view_on_mega_selected(THUNARITEM *item, gpointer user_data)
{
    MEGAExt *mega_ext = MEGA_EXT(user_data);
    GList *l;
    GList *files;
    gboolean flag = FALSE;

    files = g_object_get_data(G_OBJECT(item), "MEGAExtension::files");
    for (l = files; l != NULL; l = l->next) {
        ThunarxFileInfo *file = THUNARX_FILE_INFO(l->data);
        FileState state;
        gchar *path;
        GFile *fp;

        fp = thunarx_file_info_get_location(file);
        if (!fp)
            continue;

        path = g_file_get_path(fp);
        if (!path)
            continue;

        state = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(file), "MEGAExtension::state"));

        if (state == RESPONSE_SYNCED) {
            if (mega_ext_client_open_link(mega_ext, path))
                flag = TRUE;
        }
        g_free(path);
    }

    if (flag)
        mega_ext_client_end_request(mega_ext);
}

// user clicked on "View previous versions" menu item
static void mega_ext_on_open_previous_selected(THUNARITEM *item, gpointer user_data)
{
    MEGAExt *mega_ext = MEGA_EXT(user_data);
    GList *l;
    GList *files;
    gboolean flag = FALSE;

    files = g_object_get_data(G_OBJECT(item), "MEGAExtension::files");
    for (l = files; l != NULL; l = l->next) {
        ThunarxFileInfo *file = THUNARX_FILE_INFO(l->data);
        FileState state;
        gchar *path;
        GFile *fp;

        fp = thunarx_file_info_get_location(file);
        if (!fp)
            continue;

        path = g_file_get_path(fp);
        if (!path)
            continue;

        state = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(file), "MEGAExtension::state"));

        if (state == RESPONSE_SYNCED) {
            if (mega_ext_client_open_previous(mega_ext, path))
                flag = TRUE;
        }
        g_free(path);
    }

    if (flag)
        mega_ext_client_end_request(mega_ext);
}



static GList* mega_ext_get_file_actions(ThunarxMenuProvider *provider, G_GNUC_UNUSED GtkWidget *window, GList *files)
{
    MEGAExt *mega_ext = MEGA_EXT(provider);

    GList *l, *l_out = NULL;
    int syncedFiles, syncedFolders, unsyncedFiles, unsyncedFolders;
    gchar *out = NULL;

    g_debug("mega_ext_get_file_items: %u", g_list_length(files));

    syncedFiles = syncedFolders = unsyncedFiles = unsyncedFolders = 0;

    // get list of selected objects
    for (l = files; l != NULL; l = l->next)
    {
        ThunarxFileInfo *file = THUNARX_FILE_INFO(l->data);
        gchar *path;
        GFile *fp;
        FileState state = RESPONSE_ERROR;

        fp = thunarx_file_info_get_location(file);
        if (!fp)
        {
            continue;
        }

        path = g_file_get_path(fp);
        if (!path)
        {
            continue;
        }

        // avoid sending requests for files which are not in synced folders
        // but make sure we received the list of synced folders first
        if (mega_ext->syncs_received && !mega_ext_path_in_sync(mega_ext, path))
        {
            state = RESPONSE_DEFAULT;
        }
        else
        {
            state = mega_ext_client_get_path_state(mega_ext, path, 1);
            if (state == RESPONSE_DEFAULT)
            {
                char canonical[PATH_MAX];
                expanselocalpath(path,canonical);
                state = mega_ext_client_get_path_state(mega_ext, canonical, 1);
            }
        }
        g_free(path);

        if (state == RESPONSE_ERROR)
        {
            continue;
        }

        g_debug("State: %s", file_state_to_str(state));

        g_object_set_data_full((GObject*)file, "MEGAExtension::state", GINT_TO_POINTER(state), NULL);

        // count the number of synced / unsynced files and folders
        if (state == RESPONSE_SYNCED || state == RESPONSE_SYNCING || state == RESPONSE_PENDING)
        {
            if (thunarx_file_info_is_directory(file)) 
            {
                syncedFolders++;
            }
            else
            {
                syncedFiles++;
            }
        }
        else
        {
            if (thunarx_file_info_is_directory(file)) 
            {
                unsyncedFolders++;
            }
            else
            {
                unsyncedFiles++;
            }
        }
    }
    // if there any unsynced files / folders selected
    if (unsyncedFiles || unsyncedFolders)
    {
        THUNARITEM *item = NULL;

        out = mega_ext_client_get_string(mega_ext, STRING_UPLOAD, unsyncedFiles, unsyncedFolders);
        if(out)
        {
            g_free(mega_ext->string_upload);
            mega_ext->string_upload = g_strdup(out);
            g_free(out);
#ifdef USING_THUNAR3
            item = thunarx_menu_item_new("MEGAExtension::upload_to_mega",
                                         mega_ext->string_upload,
                                         NULL,
                                         "mega");
#else
            item = g_object_new (GTK_TYPE_ACTION,
                                 "name", "MEGAExtension::upload_to_mega",
                                 "icon-name", "mega",
                                 "label", mega_ext->string_upload,
                                 NULL
                                 );
#endif

            g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(mega_ext_on_upload_selected), provider);
            g_object_set_data_full(G_OBJECT(item), "MEGAExtension::files", thunarx_file_info_list_copy(files), (GDestroyNotify)thunarx_file_info_list_free);
            l_out = g_list_append(l_out, item);
        }
    }

    // if there any synced files / folders selected
    if (syncedFiles || syncedFolders)
    {
        THUNARITEM *item = NULL;

        out = mega_ext_client_get_string(mega_ext, STRING_GETLINK, syncedFiles, syncedFolders);
        if(out)
        {
            g_free(mega_ext->string_getlink);
            mega_ext->string_getlink = g_strdup(out);
            g_free(out);
#ifdef USING_THUNAR3
            item = thunarx_menu_item_new("MEGAExtension::get_mega_link",
                                         mega_ext->string_getlink,
                                         NULL,
                                         "mega");
#else
            item = g_object_new (GTK_TYPE_ACTION,"name", "MEGAExtension::get_mega_link","icon-name", "mega","label", mega_ext->string_getlink,NULL);
#endif
            g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(mega_ext_on_get_link_selected), provider);
            g_object_set_data_full(G_OBJECT(item), "MEGAExtension::files", thunarx_file_info_list_copy(files), (GDestroyNotify)thunarx_file_info_list_free);
            l_out = g_list_append(l_out, item);
        }


        if ( ((syncedFiles + syncedFolders) == 1 ) && ( (unsyncedFiles+unsyncedFolders) == 0  ) )
        {
            if (syncedFolders)
            {
                out = mega_ext_client_get_string(mega_ext, STRING_VIEW_ON_MEGA, 0, 0);
                if(out)
                {
                    THUNARITEM *item = NULL;
                    g_free(mega_ext->string_viewonmega);
                    mega_ext->string_viewonmega = g_strdup(out);
                    g_free(out);
#ifdef USING_THUNAR3
                    item = thunarx_menu_item_new("MEGAExtension::view_on_mega",
                                                 mega_ext->string_viewonmega,
                                                 NULL,
                                                 "mega");
#else
                    item = g_object_new (GTK_TYPE_ACTION,"name", "MEGAExtension::view_on_mega","icon-name", "mega","label", mega_ext->string_viewonmega,NULL);
#endif

                    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(mega_ext_on_view_on_mega_selected), provider);
                    g_object_set_data_full(G_OBJECT(item), "MEGAExtension::files", thunarx_file_info_list_copy(files), (GDestroyNotify)thunarx_file_info_list_free);
                    l_out = g_list_append(l_out, item);
                }
            }
            else
            {
                out = mega_ext_client_get_string(mega_ext, STRING_VIEW_VERSIONS, 0, 0);
                if(out)
                {
                    THUNARITEM *item = NULL;
                    g_free(mega_ext->string_viewprevious);
                    mega_ext->string_viewprevious = g_strdup(out);
                    g_free(out);
#ifdef USING_THUNAR3
                    item = thunarx_menu_item_new("MEGAExtension::view_previous_versions",
                                                 mega_ext->string_viewprevious,
                                                 NULL,
                                                 "mega");
#else
                    item = g_object_new (GTK_TYPE_ACTION,"name", "MEGAExtension::view_previous_versions","icon-name", "mega","label", mega_ext->string_viewprevious,NULL);
#endif
                    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(mega_ext_on_open_previous_selected), provider);
                    g_object_set_data_full(G_OBJECT(item), "MEGAExtension::files", thunarx_file_info_list_copy(files), (GDestroyNotify)thunarx_file_info_list_free);
                    l_out = g_list_append(l_out, item);
                }
            }
        }
    }

    return l_out;
}

static GList* mega_ext_get_folder_actions(ThunarxMenuProvider *provider, G_GNUC_UNUSED GtkWidget *window, ThunarxFileInfo *folder)
{
    MEGAExt *mega_ext = MEGA_EXT(provider);
    mega_ext->string_upload = NULL;

    GList *l_out = NULL;
    int syncedFolders, unsyncedFolders;
    gchar *out = NULL;

    // get list of selected objects
    gchar *path;
    GFile *fp;
    FileState state;

    syncedFolders = unsyncedFolders = 0;

    fp = thunarx_file_info_get_location(folder);
    if (!fp)
    {
        return NULL;
    }

    path = g_file_get_path(fp);
    if (!path)
    {
        return NULL;
    }

    // avoid sending requests for files which are not in synced folders
    // but make sure we received the list of synced folders first
    if (mega_ext->syncs_received && !mega_ext_path_in_sync(mega_ext, path)) 
    {
        state = RESPONSE_DEFAULT;
    } 
    else
    {
        state = mega_ext_client_get_path_state(mega_ext, path, 0);
        if (state == RESPONSE_DEFAULT)
        {
            char canonical[PATH_MAX];
            expanselocalpath(path,canonical);
            state = mega_ext_client_get_path_state(mega_ext, canonical, 0);
        }
    }
    g_free(path);

    if (state == RESPONSE_ERROR)
    {
        return NULL;
    }

    g_debug("State: %s", file_state_to_str(state));

    g_object_set_data_full((GObject*)folder, "MEGAExtension::state", GINT_TO_POINTER(state), NULL);

    // count the number of synced / unsynced files and folders
    if (state == RESPONSE_SYNCED || state == RESPONSE_SYNCING || state == RESPONSE_PENDING)
    {
        syncedFolders++;
    } else
    {
        unsyncedFolders++;
    }

    // if there any unsynced files / folders selected
    if (unsyncedFolders)
    {
        THUNARITEM *item = NULL;
        GList *tmp;

        out = mega_ext_client_get_string(mega_ext, STRING_UPLOAD, 0, unsyncedFolders);
        g_free(mega_ext->string_upload);
        mega_ext->string_upload = g_strdup(out);
        g_free(out);
#ifdef USING_THUNAR3
        item = thunarx_menu_item_new("MEGAExtension::upload_to_mega",
			             mega_ext->string_upload,
			             NULL,
			             "mega");
#else
        item = g_object_new (GTK_TYPE_ACTION,"name", "MEGAExtension::upload_to_mega","icon-name", "mega","label", mega_ext->string_upload,NULL);
#endif
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(mega_ext_on_upload_selected), provider);
        tmp = g_list_append(NULL, folder);
        g_object_set_data_full(G_OBJECT(item), "MEGAExtension::files", thunarx_file_info_list_copy(tmp), (GDestroyNotify)thunarx_file_info_list_free);
        g_list_free(tmp);
        l_out = g_list_append(l_out, item);
    }

    // if there any synced files / folders selected
    if (syncedFolders)
    {
        THUNARITEM *item = NULL;
        GList *tmp;

        out = mega_ext_client_get_string(mega_ext, STRING_GETLINK, 0, syncedFolders);
        g_free(mega_ext->string_getlink);
        mega_ext->string_getlink = g_strdup(out);
        g_free(out);
#ifdef USING_THUNAR3
        item = thunarx_menu_item_new("MEGAExtension::get_mega_link",
			             mega_ext->string_getlink,
			             NULL,
			             "mega");
#else
        item = g_object_new (GTK_TYPE_ACTION,"name", "MEGAExtension::get_mega_link","icon-name", "mega","label", mega_ext->string_getlink,NULL);
#endif
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(mega_ext_on_get_link_selected), provider);
        tmp = g_list_append(NULL, folder);
        g_object_set_data_full(G_OBJECT(item), "MEGAExtension::files", thunarx_file_info_list_copy(tmp), (GDestroyNotify)thunarx_file_info_list_free);
        g_list_free(tmp);
        l_out = g_list_append(l_out, item);
    }

    return l_out;
}

// path: a full path to filesystem object
// return TRUE if path located in one of the sync folders
static gboolean mega_ext_path_in_sync(MEGAExt *mega_ext, const gchar *path)
{
    GList *l, *p;
    gboolean found = FALSE;

    l = g_hash_table_get_keys(mega_ext->h_syncs);
    for (p = g_list_first(l); p; p = g_list_next(p)) {
        const gchar *sync = p->data;
        // sync must be a prefix of path
        if (strlen(sync) <= strlen(path)) {
            if (!strncmp(sync, path, strlen(sync))) {
                found = TRUE;
                break; 
            }
        }

        char canonical[PATH_MAX];
        expanselocalpath(path,canonical);
        if (strlen(sync) <= strlen(canonical)) {
            if (!strncmp(sync, canonical, strlen(sync))) {
                found = TRUE;
                break;
            }
        }
    }

    g_list_free(l);

    return found;
}
