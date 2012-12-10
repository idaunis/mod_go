/*
**  MOD_GO.C - Version 0.1 - Apache module for deploying web applications in Go
**
**  To install this module compile it and install it into Apache's modules
**  directory by running:
**
**    $ make
**    $ sudo make install
**
**  Then activate it in Apache's httpd.conf file as follows:
**
**    LoadModule go_module modules/mod_go.so
**    AddHandler golang .go
**
**  Then restart Apache via
**
**    $ apachectl restart
**
**  This module was developed by Use Labs, LLC.
**  Home page: http://www.uselabs.com
**
**  Author: Ivan Daunis, CTO, Use Labs, LLC.
**  Email: ivan@uselabs.com
**
**  Copyright (C) 2012 Use Labs, LLC and/or its subsidiary(-ies).
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**  http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "ap_config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

#define APP_MAX_ARGC	64

void ag_setenv( const char *key, const char * value ) {
    if( value ) setenv(key, value, 1);
    else setenv(key, "", 1);
}

char * get_name( char * filename) {
    char name[1024];
    int i = 0;
    while( i<strlen(filename) ) {
        if( filename[i]=='/' || filename[i]=='\\' ) name[i] = '.';
        else name[i] = filename[i];
        i++;
    }
    name[i] = 0;
    return strdup(name);
}

void parse_headers(request_rec *r, char *str) {
    char * value;
    str = strtok(str, " :\r\n");
    while( str != NULL ) {
        value = strtok(NULL, "\r\n");
        if (value != NULL) {
            if (strcmp(str, "Content-Type") == 0) {
                r->content_type = strdup( value );
            }
            if (strcmp(str, "Cookie") == 0) {
            }
            if (strcmp(str, "Referer") == 0) {
            }
            if (strcmp(str, "User-Agent") == 0) {
            }
        }
        str = strtok(NULL, " :\r\n");
    }
}

static int exec_go_app(request_rec *r, char * command)
{
    apr_pool_t *server_pool;
    apr_status_t rv;

    apr_procattr_t *pattr;
    apr_proc_t proc;
    int argc = 0;
    const char* argv[APP_MAX_ARGC];

	char argsbuffer[HUGE_STRING_LEN];
	apr_size_t 	nbytes;
	apr_off_t   len_read;

    apr_pool_create(&server_pool,NULL);
    argv[argc++] = command;
    argv[argc++] = NULL; /* argvs should be null-terminated */

    /* set the process properties */
    rv = apr_procattr_create(&pattr, server_pool);
    rv = apr_procattr_io_set(pattr, APR_CHILD_BLOCK, APR_CHILD_BLOCK, APR_CHILD_BLOCK);
    //rv = apr_procattr_io_set(pattr, APR_FULL_BLOCK, APR_FULL_BLOCK, APR_NO_PIPE);
    rv = apr_procattr_cmdtype_set(pattr, APR_PROGRAM_PATH);

    /* run the process */
    apr_threadattr_detach_set( (apr_threadattr_t *) pattr, 1);
    rv = ap_os_create_privileged_process(r,&proc,command,argv, NULL, (apr_procattr_t*)pattr, server_pool);
    if (rv != APR_SUCCESS) {
        // Error
        return rv;
    } else {
        apr_file_pipe_timeout_set(proc.out,r->server->timeout);
        apr_file_pipe_timeout_set(proc.in,r->server->timeout);
        apr_file_pipe_timeout_set(proc.err,r->server->timeout);
    }

    /* write client data to Go app input pipe */
    if((rv = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR))) {
        return (rv);
    }
    if(ap_should_client_block(r)) {
        while((len_read = ap_get_client_block(r, argsbuffer, sizeof(argsbuffer))) > 0) {
            nbytes = len_read;
            apr_file_write(proc.in, argsbuffer, &nbytes);
        }
    }
    apr_file_close(proc.in);

    /* read from Go error pipe and send it to apache */
    while( (rv = apr_file_read(proc.err, argsbuffer, &nbytes )) == APR_SUCCESS ) {
        ap_rwrite( argsbuffer, nbytes, r);
    }

    /* read from Go app out pipe and send it to apache */
    char *rbuf = (char *) malloc(HUGE_STRING_LEN);
    apr_size_t rpos = 0;
    nbytes = HUGE_STRING_LEN;
    while( (rv = apr_file_read(proc.out, argsbuffer, &nbytes )) == APR_SUCCESS ) {
        if( rpos + nbytes > HUGE_STRING_LEN ) {
            rbuf = realloc( rbuf, rpos + nbytes);
        }
        memcpy( rbuf + rpos, argsbuffer, (size_t) nbytes);
        rpos += nbytes;
    }
    apr_file_close(proc.out);

    int i, end = 0;
    for( i=0; i<rpos; i++) {
        if( rbuf[i] == '\r' && rbuf[i+1] == '\n' && rbuf[i+2] == '\r' && rbuf[i+3] == '\n' ) {
            end = i;
            break;
        }
    }

    if( end != 0 ) {
        parse_headers( r, strndup(rbuf, end) );
        ap_rwrite( rbuf+end+4, rpos-end-4, r);
    } else {
        /* there are no headers */
        ap_rwrite( rbuf, rpos, r);
    }

    free(rbuf);

    {
        int st;
        apr_exit_why_e why;
        rv = apr_proc_wait(&proc, &st, &why, APR_WAIT);
    }
    return OK;
}

/* The go handler */
static int go_handler(request_rec *r)
{
    size_t len, i;
    bool error = false;
    char *path, *name, exec[1024];

    if ( strcmp(r->handler, "golang") ) {
        return DECLINED;
    }
    //r->content_type = "text/html";

    i = strlen(r->filename);
    while( i>0 && (r->filename[i]!='/' && r->filename[i]!='\\') ) i--;

    path = strndup( r->filename, i );
    name = get_name( r->filename );
    sprintf( exec, "/etc/modgo/.bin/%s", name);
   
    struct stat st1, st2;
    stat(path, &st1);

    if( stat(exec, &st2) == -1 || st1.st_mtime > st2.st_mtime ) {
        FILE * fp = NULL;
        char * temp, *command = NULL;
	    char * gd = "/etc/modgo/gd";

	    command = malloc( 1024 );
	    temp = malloc( 1025 );

	    sprintf( temp, "/etc/modgo/.temp/%s", get_name(path) );
    	sprintf( command, "%s -o %s -L %s %s", gd, exec, temp, path );
    	mkdir( temp, 0777 );

    	if( NULL == (fp = popen( command, "r")) ) {
    		ap_rputs("Error!\n", r);
		    error = true;
    	}
        char *out = NULL;
        const char *br = "<br />";
        len = 0;
        int total = 0;
        while(fgets(temp, 1024, fp) != NULL) {
	        strcat( temp, br );
		    len = strlen( temp );
		    if( out == NULL ) {
			    out = malloc( total + len + 1 );
		    } else {
    			out = realloc( out, total + len + 1 );
    		}
    		memcpy( out+total, temp, len );
    		total += len;
    	}
        if( pclose(fp) != 0 ) {
            error = true;
            ap_rwrite(out, total, r);
        }
    	free( command );
    	free( temp );
	    if( out != NULL ) free( out );
    }

    if( !error ) {
        ag_setenv("QUERY_STRING", r->args );
        ag_setenv("HTTP_ACCEPT", apr_table_get( r->headers_in, "Accept") );
        ag_setenv("HTTP_ACCEPT_ENCODING", apr_table_get( r->headers_in, "Accept-Encoding") );
        ag_setenv("HTTP_ACCEPT_LANGUAGE", apr_table_get( r->headers_in, "Accept-Language") );
        ag_setenv("HTTP_CONNECTION", apr_table_get( r->headers_in, "Connection") );
        ag_setenv("HTTP_COOKIE", apr_table_get( r->headers_in, "Cookie") );
        ag_setenv("HTTP_HOST", apr_table_get( r->headers_in, "Host") );
        ag_setenv("HTTP_REFERER", apr_table_get( r->headers_in, "Referer") );
        ag_setenv("HTTP_USER_AGENT", apr_table_get( r->headers_in, "User-Agent") );

        ag_setenv("CONTENT_TYPE", apr_table_get( r->headers_in, "Content-Type") );
        ag_setenv("CONTENT_LENGTH", apr_table_get( r->headers_in, "Content-Length") );
        ag_setenv("REQUEST_METHOD", r->method );

	    exec_go_app(r, exec);
    }

    return OK;
}

static void mod_go_register_hooks(apr_pool_t *p)
{
    ap_hook_handler(go_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/* Dispatch list for API hooks */
module AP_MODULE_DECLARE_DATA go_module = {
    STANDARD20_MODULE_STUFF, 
    NULL,                  /* create per-dir    config structures */
    NULL,                  /* merge  per-dir    config structures */
    NULL,                  /* create per-server config structures */
    NULL,                  /* merge  per-server config structures */
    NULL,                  /* table of config file commands       */
    mod_go_register_hooks  /* register hooks                      */
};
