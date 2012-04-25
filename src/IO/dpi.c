/*
 * File: dpi.c
 *
 * Copyright (C) 2002-2007 Jorge Arellano Cid <jcid@dillo.org>
 * Copyright (C) 2012 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

/*
 * FIXME: Dillo's I/O system is designed so that all requests pass through
 * a_Dpi_ccc, regardless of whether they use DPI or not. Thus, we're stuck
 * with this last bit of DPI cruft until someone can determine how Dillo's
 * I/O system actually works. (It seems the only person who understands it
 * is Jorge himself; several of the other core developers have admitted in
 * private emails they don't know anything about it, which doesn't inspire
 * much confidence in the Dillo Project's organization.)
 */


#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>           /* for errno */
#include <ctype.h>           /* isxdigit */

#include "../msg.h"
#include "../klist.h"
#include "IO.h"
#include "Url.h"


typedef struct {
   int InTag;
   int Send2EOF;

   Dstr *Buf;

   int BufIdx;
   int TokIdx;
   int TokSize;
   int TokIsTag;

   ChainLink *InfoRecv;
   int Key;
} dpi_conn_t;


/*
 * Local data
 */
static Klist_t *ValidConns = NULL; /* Active connections list. It holds
                                    * pointers to dpi_conn_t structures. */

/*
 * Create a new connection data structure
 */
static dpi_conn_t *Dpi_conn_new(ChainLink *Info)
{
   dpi_conn_t *conn = dNew0(dpi_conn_t, 1);

   conn->Buf = dStr_sized_new(8*1024);
   conn->InfoRecv = Info;
   conn->Key = a_Klist_insert(&ValidConns, conn);

   return conn;
}

/*
 * Free a connection data structure
 */
static void Dpi_conn_free(dpi_conn_t *conn)
{
   a_Klist_remove(ValidConns, conn->Key);
   dStr_free(conn->Buf, 1);
   dFree(conn);
}

/*
 * Check whether a conn is still valid.
 * Return: 1 if found, 0 otherwise
 */
static int Dpi_conn_valid(int key)
{
   return (a_Klist_get_data(ValidConns, key)) ? 1 : 0;
}

/*
 * Append the new buffer in 'dbuf' to Buf in 'conn'
 */
static void Dpi_append_dbuf(dpi_conn_t *conn, DataBuf *dbuf)
{
   if (dbuf->Code == 0 && dbuf->Size > 0) {
      dStr_append_l(conn->Buf, dbuf->Buf, dbuf->Size);
   }
}

/*
 * Split the data stream into tokens.
 * Here, a token is either:
 *    a) a dpi tag
 *    b) a raw data chunk
 *
 * Return Value: 0 upon a new token, -1 on not enough data.
 *
 * TODO: define an API and move this function into libDpip.a.
*/
static int Dpi_get_token(dpi_conn_t *conn)
{
   int i, resp = -1;
   char *buf = conn->Buf->str;

   if (conn->BufIdx == conn->Buf->len) {
      dStr_truncate(conn->Buf, 0);
      conn->BufIdx = 0;
      return resp;
   }

   if (conn->Send2EOF) {
      conn->TokIdx = conn->BufIdx;
      conn->TokSize = conn->Buf->len - conn->BufIdx;
      conn->BufIdx = conn->Buf->len;
      return 0;
   }

   _MSG("conn->BufIdx = %d; conn->Buf->len = %d\nbuf: [%s]\n",
        conn->BufIdx,conn->Buf->len, conn->Buf->str + conn->BufIdx);

   if (!conn->InTag) {
      /* search for start of tag */
      while (conn->BufIdx < conn->Buf->len && buf[conn->BufIdx] != '<')
         ++conn->BufIdx;
      if (conn->BufIdx < conn->Buf->len) {
         /* found */
         conn->InTag = 1;
         conn->TokIdx = conn->BufIdx;
      } else {
         MSG_ERR("[Dpi_get_token] Can't find token start\n");
      }
   }

   if (conn->InTag) {
      /* search for end of tag (EOT=" '>") */
      for (i = conn->BufIdx; i < conn->Buf->len; ++i)
         if (buf[i] == '>' && i >= 2 && buf[i-1] == '\'' && buf[i-2] == ' ')
            break;
      conn->BufIdx = i;

      if (conn->BufIdx < conn->Buf->len) {
         /* found EOT */
         conn->TokIsTag = 1;
         conn->TokSize = conn->BufIdx - conn->TokIdx + 1;
         ++conn->BufIdx;
         conn->InTag = 0;
         resp = 0;
      }
   }

   return resp;
}

/*
 * Parse a dpi tag and take the appropriate actions
 */
static void Dpi_parse_token(dpi_conn_t *conn)
{
   char *tag;
   DataBuf *dbuf;
   char *Tok = conn->Buf->str + conn->TokIdx;

   if (conn->Send2EOF) {
      /* we're receiving data chunks from a HTML page */
      dbuf = a_Chain_dbuf_new(Tok, conn->TokSize, 0);
      a_Chain_fcb(OpSend, conn->InfoRecv, dbuf, "send_page_2eof");
      dFree(dbuf);
      return;
   }

   tag = dStrndup(Tok, (size_t)conn->TokSize);
   _MSG("Dpi_parse_token: {%s}\n", tag);

   dFree(tag);
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Get a new data buffer (within a 'dbuf'), save it into local data,
 * split in tokens and parse the contents.
 */
static void Dpi_process_dbuf(int Op, void *Data1, dpi_conn_t *conn)
{
   DataBuf *dbuf = Data1;
   int key = conn->Key;

   /* Very useful for debugging: show the data stream as received. */
   /* fwrite(dbuf->Buf, dbuf->Size, 1, stdout); */

   if (Op == IORead) {
      Dpi_append_dbuf(conn, dbuf);
      /* 'conn' has to be validated because Dpi_parse_token() MAY call abort */
      while (Dpi_conn_valid(key) && Dpi_get_token(conn) != -1) {
         Dpi_parse_token(conn);
      }

   } else if (Op == IOClose) {
      /* unused */
   }
}


/*
 * CCC function for the Dpi module
 */
void a_Dpi_ccc(int Op, int Branch, int Dir, ChainLink *Info,
               void *Data1, void *Data2)
{
   dpi_conn_t *conn;

   dReturn_if_fail( a_Chain_check("a_Dpi_ccc", Op, Branch, Dir, Info) );

   if (Branch == 1) {
      if (Dir == BCK) {
         /* Send commands to dpi-server */
         switch (Op) {
         case OpStart:
            MSG_ERR("dpi.c: can't start dpi daemon\n");
            a_Dpi_ccc(OpAbort, 1, FWD, Info, NULL, "DpidERROR");
            break;
         case OpSend:
            a_Chain_bcb(OpSend, Info, Data1, NULL);
            break;
         case OpEnd:
            a_Chain_bcb(OpEnd, Info, NULL, NULL);
            dFree(Info->LocalKey);
            dFree(Info);
            break;
         case OpAbort:
            a_Chain_bcb(OpAbort, Info, NULL, NULL);
            dFree(Info->LocalKey);
            dFree(Info);
            break;
         default:
            MSG_WARN("Unused CCC\n");
            break;
         }
      } else {  /* 1 FWD */
         /* Send commands to dpi-server (status) */
         switch (Op) {
         case OpAbort:
            a_Chain_fcb(OpAbort, Info, NULL, Data2);
            dFree(Info);
            break;
         default:
            MSG_WARN("Unused CCC\n");
            break;
         }
      }

   } else if (Branch == 2) {
      if (Dir == FWD) {
         /* Receiving from server */
         switch (Op) {
         case OpSend:
            /* Data1 = dbuf */
            Dpi_process_dbuf(IORead, Data1, Info->LocalKey);
            break;
         case OpEnd:
            a_Chain_fcb(OpEnd, Info, NULL, NULL);
            Dpi_conn_free(Info->LocalKey);
            dFree(Info);
            break;
         default:
            MSG_WARN("Unused CCC\n");
            break;
         }
      } else {  /* 2 BCK */
         switch (Op) {
         case OpStart:
            conn = Dpi_conn_new(Info);
            Info->LocalKey = conn;

            /* Hack: for receiving HTTP through the DPI framework */
            if (strcmp(Data2, "http") == 0) {
               conn->Send2EOF = 1;
            }

            a_Chain_link_new(Info, a_Dpi_ccc, BCK, a_IO_ccc, 2, 2);
            a_Chain_bcb(OpStart, Info, NULL, NULL); /* IORead */
            break;
         case OpSend:
            if (Data2 && !strcmp(Data2, "FD")) {
               a_Chain_bcb(OpSend, Info, Data1, Data2);
            }
            break;
         case OpAbort:
            a_Chain_bcb(OpAbort, Info, NULL, NULL);
            Dpi_conn_free(Info->LocalKey);
            dFree(Info);
            break;
         default:
            MSG_WARN("Unused CCC\n");
            break;
         }
      }
   }
}
