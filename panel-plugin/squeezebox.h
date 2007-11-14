/***************************************************************************
 *            squeezebox.h - frontend of xfce4-squeezebox-plugin
 *
 *  Wed Aug 30 17:37:58 2006
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

#ifndef XFCE4_SQUEEZEBOX_PLUGIN_MAIN_HEADER
#define XFCE4_SQUEEZEBOX_PLUGIN_MAIN_HEADER

  
 
typedef enum {
	estPlay = 0,
	estPause = 1,
	estStop = 2,
	estErr = 10
}eSynoptics;
 
typedef struct {
	// comon stuff
	gint updateRateMS;
	
	// backend implementations
	gboolean(* Assure)(gpointer thsPtr);
	gboolean(* Next)(gpointer thsPtr);
	gboolean(* Previous)(gpointer thsPtr);
	gboolean(* PlayPause)(gpointer thsPtr, gboolean newState);
	gboolean(* IsPlaying)(gpointer thsPtr);
	gboolean(* Toggle)(gpointer thsPtr, gboolean *newState);
	gboolean(* Detach)(gpointer thsPtr);
    gboolean(* GetRepeat)(gpointer thsPtr, gboolean *oldShuffle);
    gboolean(* SetRepeat)(gpointer thsPtr, gboolean newShuffle);
    gboolean(* GetShuffle)(gpointer thsPtr, gboolean *oldShuffle);
    gboolean(* SetShuffle)(gpointer thsPtr, gboolean newShuffle);
	gboolean(* IsVisible)(gpointer thsPtr);
	gboolean(* Show)(gpointer thsPtr, gboolean newState);
	void(* Persist)(gpointer thsPtr, XfceRc *rc, gboolean bIsStoring);
	void(* Configure)(gpointer thsPtr, GtkWidget *parent);
	
	// data provided by backend
	GString *artist;
	GString *album;
	GString *title;
	GString *albumArt;  // path to image file
    gint    secPos;     // Current track position in seconds
    gint    secTot;     // Current track length in seconds
	
	// backend "this" pointer, first param of above functions
	gpointer db;
	
	
    // frontend "this" pointer
	gpointer sd;
	
	// frontend callbacks
	void(* Update)(gpointer thsPlayer, gboolean SongChanged, eSynoptics State, const gchar* playerMessage);
    void(* UpdateRepeat)(gpointer thsPlayer, gboolean newRepeat);
    void(* UpdateShuffle)(gpointer thsPlayer, gboolean newShuffle);
    void(* UpdateTimePosition)(gpointer thsPlayer);
}SPlayer;

typedef struct {
    void*( *BACKEND_attach)(SPlayer *player);
    gchar*( *BACKEND_name)();
}Backend;
 
#define IMPORT_BACKEND(t) \
 	extern void * t##_attach(SPlayer *player); \
 	extern gchar * t##_name();
 
extern const Backend* squeezebox_get_backends();

#define BEGIN_BACKEND_MAP() const Backend* squeezebox_get_backends() \
{ \
    static const Backend ret[] = { 
#define BACKEND(t) {t##_attach, t##_name},
#define END_BACKEND_MAP() \
        {NULL, NULL} \
    }; \
    return &ret[0]; \
}

#define DEFINE_BACKEND(t,n) gchar* t##_name(){ return _(n);}
    
		
#define LOG(t) printf(t);fflush(stdout)
#define LOGERR(t) fprintf(stderr, t);fflush(stderr)
#define LOGERRF g_warning
#define LOGF g_message
	
#define xfce_screen_position_is_right_ex(position) \
    (position >= XFCE_SCREEN_POSITION_NE_V && \
     position <= XFCE_SCREEN_POSITION_SE_V) || \
	(position == XFCE_SCREEN_POSITION_NE_H) || \
	(position == XFCE_SCREEN_POSITION_SE_H) || \
	(position == XFCE_SCREEN_POSITION_E)

    
#endif //defined XFCE4_SQUEEZEBOX_PLUGIN_MAIN_HEADER