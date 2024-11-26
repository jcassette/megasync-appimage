#ifdef __has_include                           // Check if __has_include is present
#if __has_include(<nautilus-extension.h>)
#include <nautilus-extension.h>
#define __NAUTILUS_MAIN_HEADER_INCLUDED
#endif
#endif

#ifndef __NAUTILUS_MAIN_HEADER_INCLUDED
#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-file-info.h>
#include <libnautilus-extension/nautilus-menu-provider.h>
#include <libnautilus-extension/nautilus-info-provider.h>
#endif

#include "MEGAShellExt.h"
#include "mega_ext_client.h"
#include "mega_notify_client.h"
#include <string.h>

static GObjectClass *parent_class;

static void mega_ext_class_init(MEGAExtClass *class, G_GNUC_UNUSED gpointer class_data)
{
    parent_class = g_type_class_peek_parent(class);
}

static void mega_ext_instance_init(MEGAExt *mega_ext, G_GNUC_UNUSED gpointer g_class)
{
    mega_ext->srv_sock = -1;
    mega_ext->notify_sock = -1;
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

    // start notification client
    mega_notify_client_timer_start(mega_ext);
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

// received path from notify server with the path to item which state was changed
void mega_ext_on_item_changed(MEGAExt *mega_ext, const gchar *path)
{
    GFile *f;
    f = g_file_new_for_path(path);
    if (!f) {
        g_debug("No file found for %s!", path);
        return;
    }

    NautilusFileInfo *file = nautilus_file_info_lookup(f);
    if (!file) {
        g_debug("No NautilusFileInfo found for %s!", path);
        return;
    }
    g_debug("Item changed: %s", path);
    nautilus_info_provider_update_file_info((NautilusInfoProvider*)mega_ext, file, (void*)1, (void*)1);
}

// user clicked on "Upload to MEGA" menu item
static void mega_ext_on_upload_selected(NautilusMenuItem *item, gpointer user_data)
{
    MEGAExt *mega_ext = MEGA_EXT(user_data);
    GList *l;
    GList *files;
    gboolean flag = FALSE;

    files = g_object_get_data(G_OBJECT(item), "MEGAExtension::files");
    for (l = files; l != NULL; l = l->next) {
        NautilusFileInfo *file = NAUTILUS_FILE_INFO(l->data);
        FileState state;
        gchar *path;
        GFile *fp;

        fp = nautilus_file_info_get_location(file);
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

void mega_ext_on_sync_add(MEGAExt *mega_ext, const gchar *path)
{
    // ignore empty sync
    if (!strcmp(path, "."))
        return;
    g_debug("New sync path: %s", path);
    g_hash_table_insert(mega_ext->h_syncs, g_strdup(path), GINT_TO_POINTER(1));
}

void mega_ext_on_sync_del(MEGAExt *mega_ext, const gchar *path)
{
    g_debug("Deleted sync path: %s", path);
    g_hash_table_remove(mega_ext->h_syncs, path);
}

void expanselocalpath(const char *path, char *absolutepath)
{
    if (strlen(path) && path[0] == '/')
    {
        //*absolutepath = *path;
        strcpy(absolutepath, path);

        char canonical[PATH_MAX];
        if (realpath(absolutepath,canonical) != NULL)
        {
            strcpy(absolutepath, canonical);
        }
        return;
    }
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

// user clicked on "Get MEGA link" menu item
static void mega_ext_on_get_link_selected(NautilusMenuItem *item, gpointer user_data)
{
    MEGAExt *mega_ext = MEGA_EXT(user_data);
    GList *l;
    GList *files;
    gboolean flag = FALSE;

    files = g_object_get_data(G_OBJECT(item), "MEGAExtension::files");
    for (l = files; l != NULL; l = l->next) {
        NautilusFileInfo *file = NAUTILUS_FILE_INFO(l->data);
        FileState state;
        gchar *path;
        GFile *fp;

        fp = nautilus_file_info_get_location(file);
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
static void mega_ext_on_view_on_mega_selected(NautilusMenuItem *item, gpointer user_data)
{
    MEGAExt *mega_ext = MEGA_EXT(user_data);
    GList *l;
    GList *files;
    gboolean flag = FALSE;

    files = g_object_get_data(G_OBJECT(item), "MEGAExtension::files");
    for (l = files; l != NULL; l = l->next) {
        NautilusFileInfo *file = NAUTILUS_FILE_INFO(l->data);
        FileState state;
        gchar *path;
        GFile *fp;

        fp = nautilus_file_info_get_location(file);
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
static void mega_ext_on_open_previous_selected(NautilusMenuItem *item, gpointer user_data)
{
    MEGAExt *mega_ext = MEGA_EXT(user_data);
    GList *l;
    GList *files;
    gboolean flag = FALSE;

    files = g_object_get_data(G_OBJECT(item), "MEGAExtension::files");
    for (l = files; l != NULL; l = l->next) {
        NautilusFileInfo *file = NAUTILUS_FILE_INFO(l->data);
        FileState state;
        gchar *path;
        GFile *fp;

        fp = nautilus_file_info_get_location(file);
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



// Executed on context menu for selected object(-s)
// Check the state of the selected files
// to show the menu item "Upload to MEGA", "Get MEGA link" or both
// Return: list of NautilusMenuItem
static GList *mega_ext_get_file_items(NautilusMenuProvider *provider,
                                      #if (NAUTILUS_EXT_API_VERSION < 4)
                                      G_GNUC_UNUSED GtkWidget *window,
                                      #endif
                                      GList *files)
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
        NautilusFileInfo *file = NAUTILUS_FILE_INFO(l->data);
        gchar *path;
        GFile *fp;
        FileState state;

        fp = nautilus_file_info_get_location(file);
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
            if (nautilus_file_info_get_file_type(file) == G_FILE_TYPE_DIRECTORY)
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
            if (nautilus_file_info_get_file_type(file) == G_FILE_TYPE_DIRECTORY)
            {
                unsyncedFolders++;
            }
            else
            {
                unsyncedFiles++;
            }
        }
    }


    NautilusMenuItem *root_menu_item = nautilus_menu_item_new("NautilusObj::root_menu_item",
                                                "MEGA",
                                                "Select MEGA action",
                                                "mega");
    NautilusMenu *subMenu = nautilus_menu_new();

    nautilus_menu_item_set_submenu(root_menu_item, subMenu); //Connect submenu to root menu item

    // if there are any unsynced files / folders selected
    if (unsyncedFiles || unsyncedFolders)
    {
        NautilusMenuItem *item;

        out = mega_ext_client_get_string(mega_ext, STRING_UPLOAD, unsyncedFiles, unsyncedFolders);

        if(out)
        {
            item = nautilus_menu_item_new("MEGAExtension::upload_to_mega", out, "Upload files to your MEGA account", "mega");
            g_free(mega_ext->string_upload);
            mega_ext->string_upload = g_strdup(out);
            g_free(out);

            g_signal_connect(item, "activate", G_CALLBACK(mega_ext_on_upload_selected), provider);
            g_object_set_data_full((GObject*)item, "MEGAExtension::files", nautilus_file_info_list_copy(files), (GDestroyNotify)nautilus_file_info_list_free);
            nautilus_menu_append_item(subMenu, item);
            g_object_unref(item);
        }
     }

    // if there any synced files / folders selected
    if (syncedFiles || syncedFolders)
    {
        NautilusMenuItem *item;

        out = mega_ext_client_get_string(mega_ext, STRING_GETLINK, syncedFiles, syncedFolders);
        if(out)
        {
            item = nautilus_menu_item_new("MEGAExtension::get_mega_link", out, "Get MEGA link", "mega");
            g_free(mega_ext->string_getlink);
            mega_ext->string_getlink = g_strdup(out);
            g_free(out);

            g_signal_connect(item, "activate", G_CALLBACK(mega_ext_on_get_link_selected), provider);
            g_object_set_data_full((GObject*)item, "MEGAExtension::files", nautilus_file_info_list_copy(files), (GDestroyNotify)nautilus_file_info_list_free);
            nautilus_menu_append_item(subMenu, item);
            g_object_unref(item);
        }

        if ( ((syncedFiles + syncedFolders) == 1 ) && ( (unsyncedFiles+unsyncedFolders) == 0  ) )
        {
            if (syncedFolders)
            {
                out = mega_ext_client_get_string(mega_ext, STRING_VIEW_ON_MEGA, 0, 0);

                if(out)
                {
                    item = nautilus_menu_item_new("MEGAExtension::view_on_mega", out, "View on MEGA", "mega");
                    g_free(mega_ext->string_viewonmega);
                    mega_ext->string_viewonmega = g_strdup(out);
                    g_free(out);

                    g_signal_connect(item, "activate", G_CALLBACK(mega_ext_on_view_on_mega_selected), provider);
                    g_object_set_data_full((GObject*)item, "MEGAExtension::files", nautilus_file_info_list_copy(files), (GDestroyNotify)nautilus_file_info_list_free);
                    nautilus_menu_append_item(subMenu, item);
                    g_object_unref(item);
                }
            }
            else
            {
                out = mega_ext_client_get_string(mega_ext, STRING_VIEW_VERSIONS, 0, 0);
                if(out)
                {
                    item = nautilus_menu_item_new("MEGAExtension::view_previous_versions", out, "View previous versions", "mega");
                    g_free(mega_ext->string_viewprevious);
                    mega_ext->string_viewprevious = g_strdup(out);
                    g_free(out);

                    g_signal_connect(item, "activate", G_CALLBACK(mega_ext_on_open_previous_selected), provider);
                    g_object_set_data_full((GObject*)item, "MEGAExtension::files", nautilus_file_info_list_copy(files), (GDestroyNotify)nautilus_file_info_list_free);
                    nautilus_menu_append_item(subMenu, item);
                    g_object_unref(item);
                }
            }
        }
    }

    GList *submenus = nautilus_menu_get_items(subMenu);
    if (submenus)
    {
        if (g_list_length(submenus))
        {
            l_out = g_list_append(l_out, root_menu_item);
        }
        nautilus_menu_item_list_free(submenus);
    }
    g_object_unref(subMenu);
    return l_out;
}

static NautilusOperationResult mega_ext_update_file_info(NautilusInfoProvider *provider,
    NautilusFileInfo *file, G_GNUC_UNUSED GClosure *update_complete, G_GNUC_UNUSED NautilusOperationHandle **handle)
{
    MEGAExt *mega_ext = MEGA_EXT(provider);
    gchar *path;
    GFile *fp;
    FileState state;

    // invalidate current emblems.
    nautilus_file_info_invalidate_extension_info(file);

    fp = nautilus_file_info_get_location(file);
    if (!fp)
    {
        return NAUTILUS_OPERATION_COMPLETE;
    }

    path = g_file_get_path(fp);
    if (!path)
    {
        return NAUTILUS_OPERATION_COMPLETE;
    }

    gboolean has_mega_icon = FALSE;
    GFileInfo* file_info = g_file_query_info(fp, "metadata::custom-icon", G_FILE_QUERY_INFO_NONE, NULL, NULL);
    if (file_info != NULL)
    {
        char* icon_path = g_file_info_get_attribute_as_string (file_info, "metadata::custom-icon");
        if (icon_path != NULL)
        {
            if (strstr(icon_path, "/usr/share/icons") && strstr(icon_path, "apps/mega.png"))
            {
                has_mega_icon = TRUE;
            }

            g_free(icon_path);
        }

        g_object_unref(file_info);
    }

    state = mega_ext_client_get_path_state(mega_ext, path, 0);
    if (state == RESPONSE_DEFAULT)
    {
        char canonical[PATH_MAX];
        expanselocalpath(path,canonical);
        state = mega_ext_client_get_path_state(mega_ext, canonical, 0);
    }

    g_debug("mega_ext_update_file_info. File: %s  State: %s", path, file_state_to_str(state));

    // process items located in sync folders
    if (state == RESPONSE_DEFAULT || state == RESPONSE_IGNORED)
    {
        if (has_mega_icon)
        {
            g_file_set_attribute(fp, "metadata::custom-icon", G_FILE_ATTRIBUTE_TYPE_INVALID, NULL, G_FILE_QUERY_INFO_NONE, NULL, NULL);
            g_debug("mega_ext_update_file_info. removed mega-icon on %s", path);
        }

        g_free(path);
        return NAUTILUS_OPERATION_COMPLETE;
    }

    g_free(path);

    if (state == RESPONSE_ERROR)
    {
        return NAUTILUS_OPERATION_COMPLETE;
    }

    switch (state)
    {
        case RESPONSE_SYNCED:
            nautilus_file_info_add_emblem(file, "mega-synced");
            break;
        case RESPONSE_PENDING:
            nautilus_file_info_add_emblem(file, "mega-pending");
            break;
        case RESPONSE_SYNCING:
            nautilus_file_info_add_emblem(file, "mega-syncing");
            break;
        default:
            break;
    }

    return NAUTILUS_OPERATION_COMPLETE;
}

static void mega_ext_menu_provider_iface_init(
        #if (NAUTILUS_EXT_API_VERSION < 4)
        NautilusMenuProviderIface *iface,
        #else
        NautilusMenuProviderInterface *iface,
        #endif
        G_GNUC_UNUSED gpointer iface_data)
{
    iface->get_file_items = mega_ext_get_file_items;
}

static void mega_ext_info_provider_iface_init(
        #if (NAUTILUS_EXT_API_VERSION < 4)
        NautilusInfoProviderIface *iface,
        #else
        NautilusInfoProviderInterface *iface,
        #endif
        G_GNUC_UNUSED gpointer iface_data)
{
    iface->update_file_info = mega_ext_update_file_info;
}

static GType mega_ext_type = 0;

GType mega_ext_get_type (void)
{
    return mega_ext_type;
}

void mega_ext_register_type(GTypeModule *module)
{
    static const GTypeInfo mega_type_info = {
        sizeof(MEGAExtClass),
        NULL,
        NULL,
        (GClassInitFunc)mega_ext_class_init,
        NULL,
        NULL,
        sizeof (MEGAExt),
        0,
        (GInstanceInitFunc)mega_ext_instance_init,
        NULL
    };

    static const GInterfaceInfo menu_provider_iface_info = {
        (GInterfaceInitFunc) mega_ext_menu_provider_iface_init,
        NULL,
        NULL
    };

    static const GInterfaceInfo info_provider_iface_info = {
        (GInterfaceInitFunc) mega_ext_info_provider_iface_init,
        NULL,
        NULL
    };

    mega_ext_type = g_type_module_register_type(module,
        G_TYPE_OBJECT, "MEGAExtension", &mega_type_info, 0);

    g_type_module_add_interface(module,
        mega_ext_type,
        NAUTILUS_TYPE_MENU_PROVIDER,
        &menu_provider_iface_info);

    g_type_module_add_interface(module,
        mega_ext_type,
        NAUTILUS_TYPE_INFO_PROVIDER,
        &info_provider_iface_info);
}
