/*
 * File: capi.c
 *
 * Copyright 2002-2007 Jorge Arellano Cid <jcid@dillo.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

/*
 * Cache API
 * This is the module that manages the cache and starts the CCC chains
 * to get the requests served. Kind of a broker.
 */

#include <string.h>

#include "msg.h"
#include "capi.h"
#include "IO/IO.h"    /* for IORead &friends */
#include "IO/Url.h"
#include "chain.h"
#include "history.h"
#include "nav.h"
#include "uicmd.hh"

#ifdef ENABLE_DOWNLOADS
#  include "download.hh"
#endif /* ENABLE_DOWNLOADS */

typedef struct {
   DilloUrl *url;           /* local copy of web->url */
   void *bw;
   char *server;
   char *datastr;
   int SockFD;
   int Flags;
   ChainLink *InfoSend;
   ChainLink *InfoRecv;

   int Ref;
} capi_conn_t;

/* Flags for conn */
enum {
   PENDING = 1,
   TIMEOUT = 2,  /* unused */
   ABORTED = 4
};

/*
 * Local data
 */
static Dlist *CapiConns;      /* Data list for active connections; it holds
                               * pointers to capi_conn_t structures. */

/*
 * Forward declarations
 */
void a_Capi_ccc(int Op, int Branch, int Dir, ChainLink *Info,
                void *Data1, void *Data2);


/* ------------------------------------------------------------------------- */

/*
 * Initialize capi&cache data
 */
void a_Capi_init(void)
{
   /* create an empty list */
   CapiConns = dList_new(32);
   /* init cache */
   a_Cache_init();
}

/* ------------------------------------------------------------------------- */

/*
 * Create a new connection data structure
 */
static capi_conn_t *
 Capi_conn_new(const DilloUrl *url, void *bw, char *server, char *datastr)
{
   capi_conn_t *conn;

   conn = dNew(capi_conn_t, 1);
   conn->url = url ? a_Url_dup(url) : NULL;
   conn->bw = bw;
   conn->server = dStrdup(server);
   conn->datastr = dStrdup(datastr);
   conn->SockFD = -1;
   conn->Flags = (strcmp(server, "http") != 0) ? PENDING : 0;
   conn->InfoSend = NULL;
   conn->InfoRecv = NULL;
   conn->Ref = 0;           /* Reference count */
   return conn;
}

/*
 * Validate a capi_conn_t pointer.
 * Return value: NULL if not valid, conn otherwise.
 */
static capi_conn_t *Capi_conn_valid(capi_conn_t *conn)
{
   return dList_find(CapiConns, conn);
}

/*
 * Increment the reference count and add to the list if not present
 */
static void Capi_conn_ref(capi_conn_t *conn)
{
   if (++conn->Ref == 1) {
      /* add the connection data to list */
      dList_append(CapiConns, (void *)conn);
   }
   _MSG(" Capi_conn_ref #%d %p\n", conn->Ref, conn);
}

/*
 * Decrement the reference count (and remove from list when zero)
 */
static void Capi_conn_unref(capi_conn_t *conn)
{
   _MSG(" Capi_conn_unref #%d %p\n", conn->Ref - 1, conn);

   /* We may validate conn here, but it doesn't *seem* necessary */
   if (--conn->Ref == 0) {
      /* remove conn preserving the list order */
      dList_remove(CapiConns, (void *)conn);
      /* free dynamic memory */
      a_Url_free(conn->url);
      dFree(conn->server);
      dFree(conn->datastr);
      dFree(conn);
   }
   _MSG(" Capi_conn_unref CapiConns=%d\n", dList_length(CapiConns));
}

/*
 * Abort the connection for a given url, using its CCC.
 * (OpAbort 2,BCK removes the cache entry)
 * TODO: when conn is already done, the cache entry isn't removed.
 *       This may be wrong and needs a revision.
 */
void a_Capi_conn_abort_by_url(const DilloUrl *url)
{
   int i;
   capi_conn_t *conn;

   for (i = 0; i < dList_length(CapiConns); ++i) {
      conn = dList_nth_data (CapiConns, i);
      if (a_Url_cmp(conn->url, url) == 0) {
         if (conn->InfoSend) {
            a_Capi_ccc(OpAbort, 1, BCK, conn->InfoSend, NULL, NULL);
         }
         if (conn->InfoRecv) {
            a_Capi_ccc(OpAbort, 2, BCK, conn->InfoRecv, NULL, NULL);
         }
         break;
      }
   }
}

/* ------------------------------------------------------------------------- */

/*
 * When dillo wants to open an URL, this can be either due to user action
 * (e.g., typing in an URL, clicking a link), or automatic (HTTP header
 * indicates redirection, META HTML tag with refresh attribute and 0 delay,
 * and images and stylesheets on an HTML page when autoloading is enabled).
 *
 * For a user request, the action will be permitted.
 * For an automatic request, permission to load depends on the filter set
 * by the user.
 */
static bool_t Capi_filters_test(const DilloUrl *wanted,
                                const DilloUrl *requester)
{
   bool_t ret;

   if (requester == NULL) {
      /* request made by user */
      ret = TRUE;
   } else {
      switch (prefs.filter_auto_requests) {
         case PREFS_FILTER_SAME_DOMAIN:
         {
            const char *req_host = URL_HOST(requester),
                       *want_host = URL_HOST(wanted),
                       *req_suffix,
                       *want_suffix;
            if (want_host[0] == '\0') {
               ret = (req_host[0] == '\0' ||
                      !dStrcasecmp(URL_SCHEME(wanted), "data")) ? TRUE : FALSE;
            } else {
               /* This will regard "www.dillo.org" and "www.dillo.org." as
                * different, but it doesn't seem worth caring about.
                */
               req_suffix = a_Url_host_find_public_suffix(req_host);
               want_suffix = a_Url_host_find_public_suffix(want_host);

               ret = dStrcasecmp(req_suffix, want_suffix) == 0;
            }

            MSG("Capi_filters_test: %s from '%s' to '%s'\n",
                ret ? "ALLOW" : "DENY", req_host, want_host);
            break;
         }
         case PREFS_FILTER_ALLOW_ALL:
         default:
            ret = TRUE;
            break;
      }
   }
   return ret;
}

/*
 * Most used function for requesting a URL.
 * TODO: clean up the ad-hoc bindings with an API that allows dynamic
 *       addition of new plugins.
 *
 * Return value: A primary key for identifying the client,
 *               0 if the client is aborted in the process.
 */
int a_Capi_open_url(DilloWeb *web, CA_Callback_t Call, void *CbData)
{
   int reload;
   capi_conn_t *conn = NULL;
   const char *scheme = URL_SCHEME(web->url);
   int ret = 0, use_cache = 0;

   dReturn_val_if_fail((a_Capi_get_flags(web->url) & CAPI_IsCached) ||
                       Capi_filters_test(web->url, web->requester), 0);

   /* reload test */
   reload = (!(a_Capi_get_flags(web->url) & CAPI_IsCached) ||
             (URL_FLAGS(web->url) & URL_E2EQuery));

   if (web->flags & WEB_Download) {
     /* download request: if cached save from cache, else
      * for http, ftp or https, call a_Download_start() */
     if (a_Capi_get_flags_with_redirection(web->url) & CAPI_IsCached) {
        if (web->filename) {
           if ((web->stream = fopen(web->filename, "wb"))) {
              use_cache = 1;
           } else {
              MSG_WARN("Cannot open \"%s\" for writing.\n", web->filename);
           }
        }
     } else if (a_Cache_download_enabled(web->url)) {
#ifdef ENABLE_DOWNLOADS
        a_Download_start(URL_STR(web->url), web->filename);
#endif /* ENABLE_DOWNLOADS */
     }

   } else if (!dStrcasecmp(scheme, "http")) {
      /* http request */
      if (reload) {
         a_Capi_conn_abort_by_url(web->url);
         /* create a new connection and start the CCC operations */
         conn = Capi_conn_new(web->url, web->bw, "http", "none");
         /* start the reception branch before the query one because the DNS
          * may callback immediately. This may avoid a race condition. */
         a_Capi_ccc(OpStart, 2, BCK, a_Chain_new(), conn, "http");
         a_Capi_ccc(OpStart, 1, BCK, a_Chain_new(), conn, web);
      }
      use_cache = 1;

#ifdef ENABLE_SSL
   } else if (!dStrcasecmp(scheme, "https")) {
      /* https request */
      if (reload) {
         a_Capi_conn_abort_by_url(web->url);
         /* create a new connection and start the CCC operations */
         conn = Capi_conn_new(web->url, web->bw, "http", "none");
         /* start the reception branch before the query one because the DNS
          * may callback immediately. This may avoid a race condition. */
         a_Capi_ccc(OpStart, 2, BCK, a_Chain_new(), conn, "http");
         a_Capi_ccc(OpStart, 1, BCK, a_Chain_new(), conn, web);
      }
      use_cache = 1;
#endif /* ENABLE_SSL */

   } else if (!dStrcasecmp(scheme, "file")) {
      /* file request */
      use_cache = 1;

   } else if (!dStrcasecmp(scheme, "about")) {
      /* internal request */
      use_cache = 1;

   } else {
      /* unsupported protocol */
      a_UIcmd_set_msg(web->bw, "ERROR: %s protocol is not supported", scheme);
   }

   if (use_cache) {
      if (!conn || (conn && Capi_conn_valid(conn))) {
         /* not aborted, let's continue... */
         ret = a_Cache_open_url(web, Call, CbData);
      }
   } else {
      a_Web_free(web);
   }
   return ret;
}

/*
 * Convert cache-defined flags to Capi ones.
 */
static int Capi_map_cache_flags(uint_t flags)
{
   int status = 0;

   if (flags) {
      status |= CAPI_IsCached;
      if (flags & CA_IsEmpty)
         status |= CAPI_IsEmpty;
      if (flags & CA_GotData)
         status |= CAPI_Completed;
      else
         status |= CAPI_InProgress;

      /* CAPI_Aborted is not yet used/defined */
   }
   return status;
}

/*
 * Return status information of an URL's content-transfer process.
 */
int a_Capi_get_flags(const DilloUrl *Url)
{
   uint_t flags = a_Cache_get_flags(Url);
   int status = flags ? Capi_map_cache_flags(flags) : 0;
   return status;
}

/*
 * Same as a_Capi_get_flags() but following redirections.
 */
int a_Capi_get_flags_with_redirection(const DilloUrl *Url)
{
   uint_t flags = a_Cache_get_flags_with_redirection(Url);
   int status = flags ? Capi_map_cache_flags(flags) : 0;
   return status;
}

/*
 * Get the cache's buffer for the URL, and its size.
 * Return: 1 cached, 0 not cached.
 */
int a_Capi_get_buf(const DilloUrl *Url, char **PBuf, int *BufSize)
{
   return a_Cache_get_buf(Url, PBuf, BufSize);
}

/*
 * Unref the cache's buffer when no longer using it.
 */
void a_Capi_unref_buf(const DilloUrl *Url)
{
   a_Cache_unref_buf(Url);
}

/*
 * Get the Content-Type associated with the URL
 */
const char *a_Capi_get_content_type(const DilloUrl *url)
{
   return a_Cache_get_content_type(url);
}

/*
 * Set the Content-Type for the URL.
 */
const char *a_Capi_set_content_type(const DilloUrl *url, const char *ctype,
                                    const char *from)
{
   return a_Cache_set_content_type(url, ctype, from);
}

/*
 * Remove a client from the cache client queue.
 * force = also abort the CCC if this is the last client.
 */
void a_Capi_stop_client(int Key, int force)
{
   CacheClient_t *Client;

   _MSG("a_Capi_stop_client:  force=%d\n", force);

   Client = a_Cache_client_get_if_unique(Key);
   if (Client && (force || Client->BufSize == 0)) {
      /* remove empty entries too */
      a_Capi_conn_abort_by_url(Client->Url);
   }
   a_Cache_stop_client(Key);
}

/*
 * CCC function for the CAPI module
 */
void a_Capi_ccc(int Op, int Branch, int Dir, ChainLink *Info,
                void *Data1, void *Data2)
{
   capi_conn_t *conn;

   dReturn_if_fail( a_Chain_check("a_Capi_ccc", Op, Branch, Dir, Info) );

   if (Branch == 1) {
      if (Dir == BCK) {
         /* Command sending branch */
         switch (Op) {
         case OpStart:
            /* Data1 = conn; Data2 = {Web | server} */
            conn = Data1;
            Capi_conn_ref(conn);
            Info->LocalKey = conn;
            conn->InfoSend = Info;
            if (strcmp(conn->server, "http") == 0) {
               a_Chain_link_new(Info, a_Capi_ccc, BCK, a_Http_ccc, 1, 1);
               a_Chain_bcb(OpStart, Info, Data2, NULL);
#ifdef ENABLE_SSL
            } else if (strcmp(conn->server, "https") == 0) {
               a_Chain_link_new(Info, a_Capi_ccc, BCK, a_Http_ccc, 1, 1);
               a_Chain_bcb(OpStart, Info, Data2, NULL);
#endif /* ENABLE_SSL */
            }
            break;
         case OpSend:
            /* Data1 = dbuf */
            a_Chain_bcb(OpSend, Info, Data1, NULL);
            break;
         case OpEnd:
            conn = Info->LocalKey;
            conn->InfoSend = NULL;
            a_Chain_bcb(OpEnd, Info, NULL, NULL);
            Capi_conn_unref(conn);
            dFree(Info);
            break;
         case OpAbort:
            conn = Info->LocalKey;
            conn->InfoSend = NULL;
            a_Chain_bcb(OpAbort, Info, NULL, NULL);
            Capi_conn_unref(conn);
            dFree(Info);
            break;
         default:
            MSG_WARN("Unused CCC\n");
            break;
         }
      } else {  /* 1 FWD */
         /* Command sending branch (status) */
         switch (Op) {
         case OpSend:
            if (!Data2) {
               MSG_WARN("Capi.c: Opsend [1F] Data2 = NULL\n");
            } else if (strcmp(Data2, "FD") == 0) {
               conn = Info->LocalKey;
               conn->SockFD = *(int*)Data1;
               /* communicate the FD through the answer branch */
               a_Capi_ccc(OpSend, 2, BCK, conn->InfoRecv, &conn->SockFD, "FD");
            }
            break;
         case OpAbort:
            conn = Info->LocalKey;
            conn->InfoSend = NULL;
            if (Data2) {
               if (!strcmp(Data2, "Both") && conn->InfoRecv) {
                  /* abort the other branch too */
                  a_Capi_ccc(OpAbort, 2, BCK, conn->InfoRecv, NULL, NULL);
               }
            }
            /* if URL == expect-url */
            a_Nav_cancel_expect_if_eq(conn->bw, conn->url);
            /* finish conn */
            Capi_conn_unref(conn);
            dFree(Info);
            break;
         default:
            MSG_WARN("Unused CCC\n");
            break;
         }
      }

   } else if (Branch == 2) {
      if (Dir == BCK) {
         /* Answer branch */
         switch (Op) {
         case OpStart:
            /* Data1 = conn; Data2 = {"http" | "<dpi server name>"} */
            conn = Data1;
            Capi_conn_ref(conn);
            Info->LocalKey = conn;
            conn->InfoRecv = Info;
            a_Chain_link_new(Info, a_Capi_ccc, BCK, a_Dpi_ccc, 2, 2);
            a_Chain_bcb(OpStart, Info, NULL, Data2);
            break;
         case OpSend:
            /* Data1 = FD */
            if (Data2 && strcmp(Data2, "FD") == 0) {
               a_Chain_bcb(OpSend, Info, Data1, Data2);
            }
            break;
         case OpAbort:
            conn = Info->LocalKey;
            conn->InfoRecv = NULL;
            a_Chain_bcb(OpAbort, Info, NULL, NULL);
            /* remove the cache entry for this URL */
            a_Cache_entry_remove_by_url(conn->url);
            Capi_conn_unref(conn);
            dFree(Info);
            break;
         default:
            MSG_WARN("Unused CCC\n");
            break;
         }
      } else {  /* 2 FWD */
         /* Server listening branch */
         switch (Op) {
         case OpSend:
            conn = Info->LocalKey;
            if (strcmp(Data2, "send_page_2eof") == 0) {
               /* Data1 = dbuf */
               DataBuf *dbuf = Data1;
               a_Cache_process_dbuf(IORead, dbuf->Buf, dbuf->Size, conn->url);
            } else if (strcmp(Data2, "send_status_message") == 0) {
               a_UIcmd_set_msg(conn->bw, "%s", Data1);
            } else if (strcmp(Data2, "reload_request") == 0) {
               a_Nav_reload(conn->bw);
            } else if (strcmp(Data2, "start_send_page") == 0) {
               /* prepare the cache to receive the data stream for this URL
                *
                * a_Capi_open_url() already added a new cache entry,
                * and a client for it.
                */
            }
            break;
         case OpEnd:
            conn = Info->LocalKey;
            conn->InfoRecv = NULL;

            a_Cache_process_dbuf(IOClose, NULL, 0, conn->url);

            if (conn->InfoSend) {
               /* Propagate OpEnd to the sending branch too */
               a_Capi_ccc(OpEnd, 1, BCK, conn->InfoSend, NULL, NULL);
            }
            Capi_conn_unref(conn);
            dFree(Info);
            break;
         default:
            MSG_WARN("Unused CCC\n");
            break;
         }
      }
   }
}
