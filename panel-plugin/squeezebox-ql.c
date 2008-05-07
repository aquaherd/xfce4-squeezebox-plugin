/***************************************************************************
 *            squeezebox-ql.c
 *
 *  Fri Aug 25 17:08:15 2007
 *  Copyright  2007  Hakan Erduman
 *  Email Hakan.Erduman@web.de
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_BACKEND_QL

// default
#include "squeezebox.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <glib/gstdio.h>

#include <thunar-vfs/thunar-vfs.h>

// pixmap
#include "squeezebox-ql.png.h"

DEFINE_BACKEND(QL, _("QuodLibet (pipe)"))
/* --- */
#define QL_MAP(a) player->a = ql##a;
#define MKTHIS qlData *this = (qlData *)thsPtr;
#define QL_FIFO_PATH "/.quodlibet/control"
#define QL_STAT_PATH "/.quodlibet/current"
#define QL_ALBUM_ART_PATH "/.quodlibet/current.cover"
typedef struct {
    SPlayer *parent;
    void *player;
    FILE *fp;
    char fifo[256];
    char stat[256];
    char cover[256];
    ThunarVfsPath *statPath;
    ThunarVfsMonitorHandle *statMon;
    GHashTable *current;
    gboolean isPlaying;
    gboolean isVisible;
    gboolean isShuffle;
    gboolean isRepeat;
    NetkScreen *netkScreen;
    int connections[2];
} qlData;

void *QL_attach(SPlayer * player);
void qlStatus(gpointer thsPtr);

void qlCurrentChanged(ThunarVfsMonitor * monitor,
		      ThunarVfsMonitorHandle * handle,
		      ThunarVfsMonitorEvent event,
		      ThunarVfsPath * handle_path,
		      ThunarVfsPath * event_path, gpointer thsPtr)
{
    
    char *fc = NULL;
    MKTHIS;
    switch (event) {
        case THUNAR_VFS_MONITOR_EVENT_CREATED:
        case THUNAR_VFS_MONITOR_EVENT_CHANGED:
            LOG("CHANGEDETECT...");
            LOG(this->stat);
            if (g_file_get_contents(this->stat, &fc, NULL, NULL)) {
                if (!this->current)
                    this->current = g_hash_table_new(g_str_hash, g_str_equal);
                gchar **set = g_strsplit_set(fc, "\n", -1);
                g_free(fc);
                
                LOG("...OK\n");
                
                gchar **ptr = set;
                do {
                    gchar **line = g_strsplit_set(*ptr, "=", 2);
                    gchar *key = g_strdup(line[0]);
                    gchar *value = g_strdup(line[1]);
                    //LOG(key); LOG("="); LOG(value); LOG("\n");
                    
                    g_hash_table_replace(this->current, key, value);
                    ptr++;
                    g_strfreev(line);
                }
                while (**ptr);
                g_strfreev(set);
                
                gchar *tmpArtist =
                    g_hash_table_lookup(this->current, "artist");
                gchar *tmpAlbum = g_hash_table_lookup(this->current, "album");
                gchar *tmpTitle = g_hash_table_lookup(this->current, "title");
                
                /* -- this ised to be available in pre 1.0
                 gchar *hasPic = g_hash_table_lookup(this->current, "~picture");
                 
                 if(hasPic && *hasPic == 'y') {
                     g_string_assign(this->parent->albumArt, this->cover);
            } -- */
                
                if (g_file_test(this->cover, G_FILE_TEST_EXISTS))
                    g_string_assign(this->parent->albumArt, this->cover);
                else
                    g_string_assign(this->parent->albumArt, "");
                
                g_string_assign(this->parent->artist, tmpArtist);
                g_string_assign(this->parent->album, tmpAlbum);
                g_string_assign(this->parent->title, tmpTitle);
                
                this->parent->Update(this->parent->sd, TRUE,
                                     (this->isPlaying) ? estPlay : estPause,
                                     NULL);
            } else {
                LOG("...KO\n");
                g_string_assign(this->parent->artist, "");
                g_string_assign(this->parent->album, "");
                g_string_assign(this->parent->title, "");
                g_string_assign(this->parent->albumArt, "");
                this->parent->Update(this->parent->sd, FALSE, estStop, NULL);
            }
            break;
        case THUNAR_VFS_MONITOR_EVENT_DELETED:
            LOG("...fifo has died\n");
            g_string_assign(this->parent->artist, "");
            g_string_assign(this->parent->album, "");
            g_string_assign(this->parent->title, "");
            g_string_assign(this->parent->albumArt, "");
            this->parent->Update(this->parent->sd, FALSE, estStop, NULL);
            break;
    }
}

gboolean qlAssure(gpointer thsPtr)
{
    MKTHIS;
    LOG("Enter qlAssure\n");
    if (g_file_test(this->fifo, G_FILE_TEST_EXISTS)) {
        if (!this->fp) {
            LOG("Opening ");
            LOG(this->fifo);
            this->fp = g_fopen(this->fifo, "w");
            LOG((this->fp) ? " OK" : " KO");
            LOG("\n");

            if (this->fp) {
                GError *err = NULL;
                LOG("enter VFS...");
                this->statPath = thunar_vfs_path_new(this->stat, &err);
                LOG("...OK\n");
                if (!this->statPath) {
                    LOG("VFS Fail '");
                    LOG(err->message);
                    LOG("'\n");
                } else {
                    ThunarVfsMonitor *monitor =
                    thunar_vfs_monitor_get_default();

                    this->statMon = thunar_vfs_monitor_add_file(monitor,
                                        this->statPath,
                                        qlCurrentChanged,
                                        thsPtr);
                    if (!this->statMon) {
                        LOG("VFS Fail2");
                    } else {
                        /* --- this is too late for this->noUpdate
                           thunar_vfs_monitor_feed(
                           monitor,
                           THUNAR_VFS_MONITOR_EVENT_CHANGED,
                           this->statPath);
                           --- we fake it instead.
                        */
                        qlCurrentChanged(monitor,
                                 NULL,
                                 THUNAR_VFS_MONITOR_EVENT_CHANGED,
                                 this->statPath,
                                 this->statPath, thsPtr);
                    }
                }
            }
        }
        LOG("Leave qlAssure OK\n");
        return TRUE;
    } else if (this->fp) {
        LOG("Anomaly in qlAssure: FIFO disappeared!\n");
        fclose(this->fp);
        this->fp = NULL;
        this->parent->Update(this->parent->sd, FALSE, estStop, NULL);
    }
    LOG("Leave qlAssure KO\n");
    return FALSE;
}

gboolean qlPrintFlush(FILE * fp, const char *str) {
    int iLen = strlen(str);
    int iRet = fprintf(fp, str);
    fflush(fp);
    return (iLen == iRet);
}

gboolean qlNext(gpointer thsPtr) {
    MKTHIS;
    gboolean bRet = FALSE;
    LOG("Enter qlNext\n");
    if (qlAssure(this))
        bRet = qlPrintFlush(this->fp, "next\n");
    else
        bRet = FALSE;
    LOG("Leave qlNext\n");
    return bRet;
}

gboolean qlPrevious(gpointer thsPtr)
{
    MKTHIS;
    gboolean bRet = FALSE;
    LOG("Enter qlPrevious\n");
    if (qlAssure(this))
        bRet = qlPrintFlush(this->fp, "previous\n");
    else
        bRet = FALSE;
    LOG("Leave qlPrevious\n");
    return bRet;
}

gboolean qlPlayPause(gpointer thsPtr, gboolean newState)
{
    MKTHIS;
    LOG("Enter qlPlayPause\n");
    gboolean bRet = FALSE;
    if (qlAssure(this))
        bRet = qlPrintFlush(this->fp, "play-pause\n");
    else {
        LOG("Running...");
        xfce_exec("quodlibet", FALSE, TRUE, NULL);
        bRet = FALSE;
    }
    LOG("LEAVE qlPlayPause\n");
    return bRet;
}

gboolean qlIsPlaying(gpointer thsPtr)
{
    MKTHIS;
    LOG("Enter qlIsPlaying\n");
    qlStatus(thsPtr);
    LOG("Leave qlIsPlaying\n");
    return this->isPlaying;
}

void
qlWindowOpened(NetkScreen * screen, NetkWindow * window, gpointer thsPtr)
{
    MKTHIS;
    const gchar *windowName = netk_window_get_name(window);
    if (windowName && windowName[0] == '[')	// minimized but visible
        windowName++;
    if (g_str_has_prefix(windowName, "Quod Libet")) {
        this->isVisible = TRUE;
        LOG("QL:Appeared\n");
        this->parent->UpdateVisibility(this->parent->sd, this->isVisible);
    }
}

void
qlWindowClosed(NetkScreen * screen, NetkWindow * window, gpointer thsPtr)
{
    MKTHIS;
    const gchar *windowName = netk_window_get_name(window);
    if (windowName && windowName[0] == '[')	// minimized but visible
        windowName++;
    if (g_str_has_prefix(windowName, "Quod Libet")) {
        this->isVisible = FALSE;
        LOG("QL:Disappeared\n");
        this->parent->UpdateVisibility(this->parent->sd, this->isVisible);
    }
}


void qlStatus(gpointer thsPtr)
{
    MKTHIS;
    gchar *outText = NULL;
    const gchar *argv[] = {
	"/usr/bin/quodlibet",
	"--status",
	NULL
    };
    gint exit_status = 0;
    this->isPlaying = TRUE;
    if (g_spawn_sync(NULL, (gchar **) argv, NULL,
		     G_SPAWN_STDERR_TO_DEV_NULL,
		     NULL, NULL, &outText, NULL, &exit_status, NULL)) {

        LOG("QL says: '");
        LOG(outText);
        LOG("'\n");

        // QL generally says things like "paused AlbumList 1.000 inorder on"

        if (strlen(outText)) {
            gchar *ptr = strtok(outText, " ");
            this->isPlaying = !g_ascii_strncasecmp(ptr, "playing", 7);
            this->parent->Update(this->parent->sd, FALSE,
                     (this->isPlaying) ? estPlay : estPause,
                     NULL);
            if ((ptr = strtok(NULL, " "))) {
                //current view
                if ((ptr = strtok(NULL, " "))) {
                    //1.000 whatever
                    if ((ptr = strtok(NULL, " "))) {
                        //shuffle
                        this->isShuffle =
                            g_ascii_strncasecmp(ptr, "inorder", 7);
                        this->parent->UpdateShuffle(this->parent->sd,
                                        this->isShuffle);
                        if ((ptr = strtok(NULL, " "))) {
                            //repeat
                            this->isRepeat =
                            g_ascii_strncasecmp(ptr, "on", 2);
                            this->parent->UpdateRepeat(this->parent->sd,
                                           this->isRepeat);
                        }
                    }
                }
            }
        }
        g_free(outText);
    }
}

gboolean qlToggle(gpointer thsPtr, gboolean * newState)
{
    MKTHIS;
    gboolean bRet = FALSE;
    LOG("Enter qlToggle\n");
    if (qlAssure(this)) {
        bRet = qlPrintFlush(this->fp, "play-pause\n");
        qlStatus(thsPtr);
    } else {
        LOG("Running...");
        xfce_exec("quodlibet", FALSE, TRUE, NULL);
        bRet = FALSE;
    }
    LOG("Leave qlToggle\n");
    return bRet;
}

gboolean qlDetach(gpointer thsPtr)
{
    MKTHIS;
    gboolean bRet = FALSE;
    LOG("Enter qlDetach\n");
    if (this->fp) {
        fclose(this->fp);
        this->fp = NULL;
    }
    if (this->statPath) {
        thunar_vfs_path_unref(this->statPath);
        this->statPath = NULL;
    }
    if (this->statMon) {
        thunar_vfs_monitor_remove(thunar_vfs_monitor_get_default(),
				  this->statMon);
        this->statMon = NULL;
    }
    if (this->current)
        g_hash_table_destroy(this->current);
    
    thunar_vfs_shutdown();

    if (this->connections[0])
        g_signal_handler_disconnect(this->netkScreen, this->connections[0]);

    if (this->connections[1])
	    g_signal_handler_disconnect(this->netkScreen, this->connections[1]);

    LOG("Leave qlDetach\n");
    return bRet;
}

void qlPersist(gpointer thsPtr, XfceRc * rc, gboolean bIsStoring)
{
    //MKTHIS;
    LOG("Enter qlPersist\n");
    LOG("Leave qlPersist\n");
}

gboolean qlGetRepeat(gpointer thsPtr)
{
    MKTHIS;
    qlStatus(thsPtr);
    return this->isShuffle;
}

gboolean qlSetRepeat(gpointer thsPtr, gboolean newRepeat)
{
    MKTHIS;
    LOG("Enter qlSetRepeat\n");
    gboolean bRet = FALSE;
    this->isShuffle = newRepeat;
    if (qlAssure(this))
        bRet = qlPrintFlush(this->fp,(newRepeat) ? 
                            "repeat=1\n" : "repeat=0\n");
    else {
        bRet = FALSE;
    }
    LOG("LEAVE qlSetRepeat\n");
    return bRet;
}

gboolean qlGetShuffle(gpointer thsPtr)
{
    MKTHIS;
    qlStatus(thsPtr);
    return this->isShuffle;
}

gboolean qlSetShuffle(gpointer thsPtr, gboolean newRepeat)
{
    MKTHIS;
    LOG("Enter qlSetShuffle\n");
    gboolean bRet = FALSE;
    this->isRepeat = newRepeat;
    if (qlAssure(this))
        bRet = qlPrintFlush(this->fp, (newRepeat) ? 
                            "order=shuffle\n" : "order=inorder\n");
    else {
        bRet = FALSE;
    }
    LOG("LEAVE qlSetShuffle\n");
    return bRet;
}

gboolean qlIsVisible(gpointer thsPtr)
{
    MKTHIS;
    qlStatus(thsPtr);
    return this->isVisible;
}

gboolean qlShow(gpointer thsPtr, gboolean bShow)
{
    MKTHIS;
    LOG("Enter qlShow\n");
    gboolean bRet = FALSE;
    if (qlAssure(this))
        bRet = qlPrintFlush(this->fp, "toggle-window\n");
    else
        bRet = FALSE;
    LOG("Leave qlShow\n");
    return bRet;
}

void *QL_attach(SPlayer * player)
{
    qlData *this = g_new0(qlData, 1);
    LOG("Enter QL_attach\n");

    QL_MAP(Assure);
    QL_MAP(Next);
    QL_MAP(Previous);
    QL_MAP(PlayPause);
    QL_MAP(IsPlaying);
    QL_MAP(Toggle);
    QL_MAP(Detach);
    QL_MAP(Persist);
    NOMAP(Configure);
    QL_MAP(IsVisible);
    QL_MAP(Show);
    QL_MAP(GetRepeat);
    QL_MAP(SetRepeat);
    QL_MAP(GetShuffle);
    QL_MAP(SetShuffle);

    // we init default values 
    this->parent = player;
    this->fp = NULL;
    strcpy(this->fifo, g_get_home_dir());
    strcat(this->fifo, QL_FIFO_PATH);

    strcpy(this->stat, g_get_home_dir());
    strcat(this->stat, QL_STAT_PATH);

    strcpy(this->cover, g_get_home_dir());
    strcat(this->cover, QL_ALBUM_ART_PATH);
    thunar_vfs_init();

    // connect to notification events
    int i = 0;
    GList *windows, *l;

    netk_screen_force_update(this->netkScreen);

    this->netkScreen =
	netk_screen_get(gdk_screen_get_number(gdk_screen_get_default()));

    this->connections[i++] =
	g_signal_connect(this->netkScreen, "window_opened",
			 G_CALLBACK(qlWindowOpened), this);

    this->connections[i++] =
	g_signal_connect(this->netkScreen, "window_closed",
			 G_CALLBACK(qlWindowClosed), this);

    // update data
    if( qlAssure(this) ) {
        qlStatus (this);
    } else {
        this->parent->Update(this->parent->sd, FALSE, 
            estStop, NULL);        
    }

    windows = netk_screen_get_windows(this->netkScreen);

    for (l = windows; l != NULL; l = l->next) {
        NetkWindow *w = l->data;
        qlWindowOpened(this->netkScreen, w, this);
    }

    LOG("Leave QL_attach\n");
    return this;
}

#endif
