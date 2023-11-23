/*
 * Emulator initialisation code
 *
 * Copyright 2000 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "main.h"
#include "md5.h"

extern char **environ;

/* the preloader will set this variable */
const struct wine_preload_info *wine_main_preload_info = NULL;

/* canonicalize path and return its directory name */
static char *realpath_dirname( const char *name )
{
    char *p, *fullpath = realpath( name, NULL );

    if (fullpath)
    {
        p = strrchr( fullpath, '/' );
        if (p == fullpath) p++;
        if (p) *p = 0;
    }
    return fullpath;
}

/* if string ends with tail, remove it */
static char *remove_tail( const char *str, const char *tail )
{
    size_t len = strlen( str );
    size_t tail_len = strlen( tail );
    char *ret;

    if (len < tail_len) return NULL;
    if (strcmp( str + len - tail_len, tail )) return NULL;
    ret = malloc( len - tail_len + 1 );
    memcpy( ret, str, len - tail_len );
    ret[len - tail_len] = 0;
    return ret;
}

/* build a path from the specified dir and name */
static char *build_path( const char *dir, const char *name )
{
    size_t len = strlen( dir );
    char *ret = malloc( len + strlen( name ) + 2 );

    memcpy( ret, dir, len );
    if (len && ret[len - 1] != '/') ret[len++] = '/';
    strcpy( ret + len, name );
    return ret;
}

static const char *get_self_exe( char *argv0 )
{
#if defined(__linux__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
    return "/proc/self/exe";
#elif defined (__FreeBSD__) || defined(__DragonFly__)
    return "/proc/curproc/file";
#else
    if (!strchr( argv0, '/' )) /* search in PATH */
    {
        char *p, *path = getenv( "PATH" );

        if (!path || !(path = strdup(path))) return NULL;
        for (p = strtok( path, ":" ); p; p = strtok( NULL, ":" ))
        {
            char *name = build_path( p, argv0 );
            if (!access( name, X_OK ))
            {
                free( path );
                return name;
            }
            free( name );
        }
        free( path );
        return NULL;
    }
    return argv0;
#endif
}

static void *try_dlopen( const char *dir, const char *name )
{
    char *path = build_path( dir, name );
    void *handle = dlopen( path, RTLD_NOW );
    free( path );
    return handle;
}

static void *load_ntdll( char *argv0 )
{
    const char *self = get_self_exe( argv0 );
    char *path, *p;
    void *handle = NULL;

    if (self && ((path = realpath_dirname( self ))))
    {
        if ((p = remove_tail( path, "/loader" )))
        {
            handle = try_dlopen( p, "dlls/ntdll/ntdll.so" );
            free( p );
        }
        else handle = try_dlopen( path, BIN_TO_DLLDIR "/ntdll.so" );
        free( path );
    }

    if (!handle && (path = getenv( "WINEDLLPATH" )))
    {
        path = strdup( path );
        for (p = strtok( path, ":" ); p; p = strtok( NULL, ":" ))
        {
            handle = try_dlopen( p, "ntdll.so" );
            if (handle) break;
        }
        free( path );
    }

    if (!handle && !self) handle = try_dlopen( DLLDIR, "ntdll.so" );

    return handle;
}

char *digest_file (const char *path, int md5_len)
{
    FILE *fp = fopen (path, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];
    unsigned char decrypt[32] = "";
    char *file_md5;
    int i;
 
    if (fp == NULL) {
        fprintf (stderr, "fopen %s failed\n", path);
        return NULL;
    }
 
    MD5Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, fp)) != 0)
    {
        MD5Update (&mdContext, data, bytes);
    }
    MD5Final (&mdContext, decrypt);
    
    file_md5 = (char *)malloc((md5_len + 1) * sizeof(char));
    if(file_md5 == NULL)
    {
        fprintf(stderr, "malloc failed.\n");
        return NULL;
    }
    memset(file_md5, 0, (md5_len + 1));
    
    if(md5_len == 16)
    {
        for(i=4; i<12; i++)
        {
            sprintf(&file_md5[(i-4)*2], "%02x", decrypt[i]);
        }
    }
    else if(md5_len == 32)
    {
        for(i=0; i<16; i++)
        {
            sprintf(&file_md5[i*2], "%02x", decrypt[i]);
        }
    }
    else
    {
        fclose(fp);
        free(file_md5);
        return NULL;
    }
    
    fclose (fp);
    return file_md5;
}

#ifdef __HOTPOT_PROFESSIONAL__
bool check_reg()
{
    static char regstatus[256] = "";
    FILE *fp = popen("/usr/local/bin/reg_yuanyanghotpot.py 'yuanyanghotpot' 'check'","r");
    if( fp != NULL)
    {
        fread(regstatus, 1, sizeof(regstatus), fp);
        printf("regstatus:%s\n", regstatus);
        pclose(fp);
        if (!strncmp(regstatus, "registed",8)) {
            printf("Please regist first\n");
            return true;
        }
    } else {
        printf("exec regstatus check failed\n");
        return false;
    }
    return false;
}
#endif


/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
#ifndef __HOTPOT_DISABLE_REGISTER__  /* 20210601 wanghg remove for reg --> */
    static char regstatus[256] = "";
    char* digest = digest_file("/usr/local/bin/reg_yuanyanghotpot.py", 32);
    if (strcmp(digest, "9bc852b05f1f316a1c1f06933d49cbff")) {
        printf("digest_file not match\n");
        exit(1);
    } else {
        printf("digest_file matched\n");
    }
    printf("digest:%s\n", digest);
#ifdef __HOTPOT_PROFESSIONAL__
    FILE *fp = popen("/usr/local/bin/reg_yuanyanghotpot.py 'yuanyanghotpot'","r");
#else
    FILE *fp = popen("/usr/local/bin/reg_yuanyanghotpot.py 'os'","r");
#endif
    if( fp != NULL)
    {
        fread(regstatus, 1, sizeof(regstatus), fp);
        printf("regstatus:%s\n", regstatus);
        pclose(fp);
#ifdef __HOTPOT_PROFESSIONAL__
        if (strncmp(regstatus, "registed",8) && !check_reg()) {
#else
	if (strncmp(regstatus, "registed",8)) {
#endif
            printf("Please regist first\n");
            exit(1);
        }
    } else {
        printf("exec regstatus check failed\n");
        exit(1);
    }
#endif /* 20210601 wanghg remove for reg <-- */
    void *handle;

    if ((handle = load_ntdll( argv[0] )))
    {
        void (*init_func)(int, char **, char **) = dlsym( handle, "__wine_main" );
        if (init_func) init_func( argc, argv, environ );
        fprintf( stderr, "wine: __wine_main function not found in ntdll.so\n" );
        exit(1);
    }

    fprintf( stderr, "wine: could not load ntdll.so: %s\n", dlerror() );
    pthread_detach( pthread_self() );  /* force importing libpthread for OpenGL */
    exit(1);
}
