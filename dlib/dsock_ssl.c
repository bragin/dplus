/*
 * File: dsock_ssl.c
 *
 * Copyright (C) 2011 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * As a special exception, permission is granted to link the code of
 * the https dillo plugin with the OpenSSL project's "OpenSSL"
 * library, and distribute the linked executables, without including
 * the source code for OpenSSL in the source distribution. You must
 * obey the GNU General Public License, version 3, in all respects
 * for all of the code used other than "OpenSSL".
 */

/*
 * TODO: Work on problem handler logic
 */

#ifdef ENABLE_SSL

#include <stdio.h>
#include <errno.h>

#include "dlib.h"
#include "dsock.h"

/* This is to avoid a pack of compiler warnings when we're building with
 * CyaSSL, as its config.h conflicts with our own (included through dlib.h). */
#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#define MSG(...) fprintf(stderr, __VA_ARGS__)

SSL_CTX *ssl_context;
SslCertProblemCb_t problem_handler = NULL;

/* buffer for error string */
char sslerr[120];


/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */


/*
 * Data type for SSL connection information
 */
struct SConn_t
{
   int fd;
   SSL *ssl;

   struct SConn_t *next;
};

/* List of active SSL connections */
struct SConn_t *conn_list = NULL;


/*
 * Return SSL connection information for a given file
 * descriptor, or NULL if no SSL connection was found.
 */
void *Sock_ssl_connection(int fd)
{
   struct SConn_t *curr = conn_list;
   while (curr) {
      if (curr->fd == fd)
	 return (void*)curr;
      curr = curr->next;
   }

   return NULL;
}

/*
 * Add a new SSL connection information node
 */
static void Sock_ssl_conn_new(int fd, SSL *ssl)
{
   struct SConn_t *conn = dNew0(struct SConn_t, 1);
   conn->fd = fd;
   conn->ssl = ssl;
   conn->next = NULL;

   if (conn_list) {
      struct SConn_t *curr = conn_list;
      while (curr && curr->next)
	 curr = curr->next;

      curr->next = conn;
   } else
      conn_list = conn;
}

/*
 * Remove an SSL connection information node
 */
static void Sock_ssl_conn_free(int fd)
{
   struct SConn_t *prev = NULL;
   struct SConn_t *curr = conn_list;

   while (curr) {
      if (curr->fd == fd) {
	 if (prev)
	    prev->next = curr->next;
	 else
	    conn_list = curr->next;

	 dFree(curr);
	 break;
      }

      prev = curr;
      curr = curr->next;
   }
}


/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */


/*
 * Create a new SSL connection
 */
int dConnect_ssl(int s, const struct sockaddr *name, int namelen)
{
   int retval = dConnect(s, name, namelen);
   if (retval) {
      if (retval == EINPROGRESS) {
         /* Wait until a non-blocking connection is established */
         fd_set fds;
         FD_ZERO(&fds);
         FD_SET(s, &fds);
         select(1, NULL, &fds, NULL, NULL);
      } else
	 return retval;
   }

   return a_Sock_ssl_handshake(s);
}

/*
 * Perform the SSL handshake on an open socket.
 */
int a_Sock_ssl_handshake(int s)
{
   SSL *ssl = SSL_new(ssl_context);
   if (ssl == NULL) {
      MSG("\nSSL ERROR[1]: %s\n", ERR_error_string(ERR_get_error(), sslerr));
      return -1;
   }

   /* assign SSL connection to this file descriptor */
   SSL_set_fd(ssl, s);
   Sock_ssl_conn_new(s, ssl);

   /* initiate the SSL handshake */
   int retval = SSL_connect(ssl);
   if (retval < 1) {
      if (problem_handler != NULL) {
	 if ((*problem_handler)((void*)ssl) < 0) {
	    MSG("Certificate verification error\n");
	    dClose(s);
	 }
      } else
	 MSG("SSL ERROR[2]: %s\n", ERR_error_string(retval, sslerr));
   }

   return retval;
}


/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */


/*
 * Set certificate problem handler function
 */
void a_Sock_ssl_error_handler(SslCertProblemCb_t handler)
{
   problem_handler = handler;
}


/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */


/*
 * Initialize the OpenSSL library
 */
void Sock_ssl_init(void)
{
   SSL_library_init();
   SSL_load_error_strings();
   if (RAND_status() != 1) {
      /* Insufficient entropy. Deal with it? */
      MSG("OpenSSL: Insufficient random entropy\n");
   }

   /* create SSL context */
   ssl_context = SSL_CTX_new(SSLv23_client_method());
   if (ssl_context == NULL) {
      MSG("OpenSSL: Error creating SSL context\n");
      return;
   }

   /* enable all possible ciphers */
   SSL_CTX_set_cipher_list(ssl_context, "ALL");

   /* don't allow the obsolete SSLv2 protocol */
   SSL_CTX_set_options(ssl_context, SSL_OP_NO_SSLv2);

   /* this lets us deal with self-signed certificates */
   SSL_CTX_set_verify(ssl_context, SSL_VERIFY_NONE, NULL);

   /* set directory to load certificates from */
   if (SSL_CTX_load_verify_locations(ssl_context, NULL,
                                     "/etc/ssl/certs/") == 0) {
      MSG("OpenSSL: Error opening system x509 certificate location\n");
      /* return; */
   }

   char buf[4096];
   snprintf(buf, sizeof(buf) - 1, "%s/certs/", dGetprofdir());
   if (SSL_CTX_load_verify_locations(ssl_context, NULL, buf) == 0) {
      MSG("OpenSSL: Error opening user x509 certificate location\n");
      /* return; */
   }
}

/*
 * Clean up the OpenSSL library
 */
void Sock_ssl_freeall(void)
{
   SSL_CTX_free(ssl_context);
}

/*
 * Close an open SSL connection
 */
int Sock_ssl_close(void *conn)
{
   int retval;
   struct SConn_t *c = (struct SConn_t*)conn;

   retval = SSL_shutdown(c->ssl);
   Sock_ssl_conn_free(c->fd);

   return retval;
}

/*
 * Read data from an open SSL connection
 */
int Sock_ssl_read(void *conn, void *buf, size_t len)
{
   struct SConn_t *c = (struct SConn_t*)conn;
   return SSL_read(c->ssl, buf, len);
}

/*
 * Write data to an open SSL connection
 */
int Sock_ssl_write(void *conn, const void *buf, size_t len)
{
   struct SConn_t *c = (struct SConn_t*)conn;
   return SSL_write(c->ssl, buf, len);
}


#endif /* ENABLE_SSL */
