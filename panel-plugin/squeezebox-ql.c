/***************************************************************************
 *            squeezebox-ql.c
 *
 *  Fri Aug 25 17:20:09 2006
 *  Copyright  2006  Hakan Erduman
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

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>

#include "squeezebox.h"

DEFINE_BACKEND(QL, "QuodLibet (pipe)")

#define QL_MAP(a) player->a = ql##a;

typedef struct {
	SPlayer		*parent;
	void		*player;
}qlData;

#define MKTHIS qlData *this = (qlData *)thsPtr;

void *QL_attach(SPlayer *player);

gboolean qlAssure(gpointer thsPtr)
{
	MKTHIS;
	return (this->player != NULL);
}

gboolean qlNext(gpointer thsPtr)
{
	MKTHIS;
	gboolean bRet = FALSE;
	LOG("Enter qlNext\n");
	if( !qlAssure(this) )
		return FALSE;
	else
		bRet = FALSE;
	LOG("Leave qlNext\n");
	return bRet;
}

gboolean qlPrevious(gpointer thsPtr)
{
	MKTHIS;
	LOG("Enter qlPrevious\n");
	LOG("Leave qlPrevious\n");
	return FALSE;
}

gboolean qlPlayPause(gpointer thsPtr, gboolean newState)
{
	MKTHIS;
	LOG("Enter qlPlayPause\n");
	LOG("LEAVE qlPlayPause\n");
	return FALSE;
}

gboolean qlIsPlaying(gpointer thsPtr)
{
	MKTHIS;
	LOG("Enter qlIsPlaying\n");
	LOG("Leave qlIsPlaying\n");
	return FALSE;
}

gboolean qlToggle(gpointer thsPtr, gboolean *newState)
{
	MKTHIS;
	LOG("Enter qlToggle\n");
	LOG("Leave qlToggle\n");
	return FALSE;
}

gboolean qlDetach(gpointer thsPtr)
{
	MKTHIS;
	LOG("Enter qlDetach\n");
	LOG("Leave qlDetach\n");
	return FALSE;
}

void qlPersist(gpointer thsPtr, XfceRc *rc, gboolean bIsStoring)
{
	MKTHIS;
	LOG("Enter qlPersist\n");
	LOG("Leave qlPersist\n");
}

gboolean qlGetRepeat(gpointer thsPtr, gboolean *oldRepeat)
{
    MKTHIS;
    return FALSE;
}

gboolean qlSetRepeat(gpointer thsPtr, gboolean newShuffle)
{
    MKTHIS;
    return FALSE;
}

gboolean qlGetShuffle(gpointer thsPtr, gboolean *oldShuffle)
{
    MKTHIS;
    return FALSE;
}

gboolean qlSetShuffle(gpointer thsPtr, gboolean newRandom)
{
    MKTHIS;
    return FALSE;
}

void *QL_attach(SPlayer *player)
{
	qlData *this = g_new0(qlData, 1);
	LOG("Enter QL_attach\n");
	
	if( player->Detach )
		player->Detach(player->db);
	QL_MAP(Assure);
	QL_MAP(Next);
	QL_MAP(Previous);
	QL_MAP(PlayPause);
	QL_MAP(IsPlaying);
	QL_MAP(Toggle);
	QL_MAP(Detach);
	QL_MAP(Persist);
	//QL_MAP(Configure);
    //QL_MAP(IsVisible);
    //QL_MAP(Show);
    QL_MAP(GetRepeat);
    QL_MAP(SetRepeat);
    QL_MAP(GetShuffle);
    QL_MAP(SetShuffle);
	
	// we init default values 
	this->parent = player;
	
	LOG("Leave QL_attach\n");
	return this;
}
