/* Copyright (C) 2001-2004 Bart Massey and Jamey Sharp.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Except as contained in this notice, the names of the authors or their
 * institutions shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the authors.
 */

/* A cache for QueryExtension results. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xcb.h"
#include "xcbext.h"
#include "xcbint.h"

typedef struct lazyreply {
    enum lazy_reply_tag tag;
    union {
        xcb_query_extension_cookie_t cookie;
        xcb_query_extension_reply_t *reply;
    } value;
} lazyreply;

static lazyreply *get_index(xcb_connection_t *c, int idx)
{
    if(idx > c->ext.extensions_size)
    {
        int new_size = idx << 1;
        lazyreply *new_extensions = realloc(c->ext.extensions, sizeof(lazyreply) * new_size);
        if(!new_extensions)
            return 0;
        memset(new_extensions + c->ext.extensions_size, 0, sizeof(lazyreply) * (new_size - c->ext.extensions_size));
        c->ext.extensions = new_extensions;
        c->ext.extensions_size = new_size;
    }
    return c->ext.extensions + idx - 1;
}

xcb_query_extension_cookie_t
xcb_query_extension (xcb_connection_t *c, uint16_t name_len, const char *name)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ 0,
        /* opcode */ XCB_QUERY_EXTENSION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[6];
    xcb_query_extension_cookie_t xcb_ret;
    xcb_query_extension_request_t xcb_out;
    
    xcb_out.pad0 = 0;
    xcb_out.name_len = name_len;
    memset(xcb_out.pad1, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* char name */
    xcb_parts[4].iov_base = (char *) name;
    xcb_parts[4].iov_len = name_len * sizeof(char);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

static lazyreply *get_lazyreply(xcb_connection_t *c, xcb_extension_t *ext)
{
    static pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
    static int next_global_id;

    lazyreply *data;

    pthread_mutex_lock(&global_lock);
    if(!ext->global_id)
        ext->global_id = ++next_global_id;
    pthread_mutex_unlock(&global_lock);

    data = get_index(c, ext->global_id);
    if(data && data->tag == LAZY_NONE)
    {
        /* cache miss: query the server */
        data->tag = LAZY_COOKIE;
        printf("%s, %d\n", __FILE__, __LINE__);
        data->value.cookie = xcb_query_extension(c, strlen(ext->name), ext->name);
    }
    printf("%s, %d\n", __FILE__, __LINE__);
    return data;
}

/* Public interface */

// void *xcb_wait_for_reply(xcb_connection_t *c, unsigned int request, xcb_generic_error_t **e)
// {
//     void *ret;
//     if(e)
//         *e = 0;
//     if(c->has_error)
//         return 0;

//     pthread_mutex_lock(&c->iolock);
//     ret = wait_for_reply(c, widen(c, request), e);
//     pthread_mutex_unlock(&c->iolock);
//     return ret;
// }

xcb_query_extension_reply_t *
xcb_query_extension_reply (xcb_connection_t *c, xcb_query_extension_cookie_t cookie, xcb_generic_error_t **e)
{
    return (xcb_query_extension_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

/* Do not free the returned xcb_query_extension_reply_t - on return, it's aliased
 * from the cache. */
const xcb_query_extension_reply_t *xcb_get_extension_data(xcb_connection_t *c, xcb_extension_t *ext)
{
    lazyreply *data;
    if(c->has_error)
        return 0;

    pthread_mutex_lock(&c->ext.lock);
    data = get_lazyreply(c, ext);
    if(data && data->tag == LAZY_COOKIE)
    {
        data->tag = LAZY_FORCED;
        printf("%s, %d\n", __FILE__, __LINE__);
        data->value.reply = xcb_query_extension_reply(c, data->value.cookie, 0);
    }
    pthread_mutex_unlock(&c->ext.lock);

    return data ? data->value.reply : 0;
}

void xcb_prefetch_extension_data(xcb_connection_t *c, xcb_extension_t *ext)
{
    if(c->has_error)
        return;
    pthread_mutex_lock(&c->ext.lock);
    get_lazyreply(c, ext);
    pthread_mutex_unlock(&c->ext.lock);
}

/* Private interface */

int _xcb_ext_init(xcb_connection_t *c)
{
    if(pthread_mutex_init(&c->ext.lock, 0))
        return 0;
    return 1;
}

void _xcb_ext_destroy(xcb_connection_t *c)
{
    pthread_mutex_destroy(&c->ext.lock);
    while(c->ext.extensions_size-- > 0)
        if(c->ext.extensions[c->ext.extensions_size].tag == LAZY_FORCED)
            free(c->ext.extensions[c->ext.extensions_size].value.reply);
    free(c->ext.extensions);
}
