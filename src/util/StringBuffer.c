/*
 * Copyright (C) 2004-2009 Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "Config.h"

#include <stdio.h>
#include <string.h>

#include "StringBuffer.h"


/**
 * Implementation of the StringBuffer interface.
 *
 * @version \$Id: StringBuffer.c,v 1.24 2008/03/20 11:28:54 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */


#define T StringBuffer_T
struct T {
        int used;
        int length;
	uchar_t *buffer;
};


/* ------------------------------------------------------- Private methods */


static inline void doAppend(T S, const char *s, va_list ap) {
        va_list ap_copy;
        while (true) {
                va_copy(ap_copy, ap);
                int n = vsnprintf(S->buffer + S->used, S->length - S->used, s, ap_copy);
                va_end(ap_copy);
                if (n > -1 && (S->used + n) < S->length) {
                        S->used += n;
                        break;
                }
                if (n > -1)
                        S->length += STRLEN + n;
                else
                        S->length *= 2;
                RESIZE(S->buffer, S->length + 1);
        }
}


/* ----------------------------------------------------- Protected methods */


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif


T StringBuffer_new(const char *s) {
        T S;
        NEW(S);
        S->used = 0;
        S->length = STRLEN;
        S->buffer = ALLOC(STRLEN + 1);
        return StringBuffer_append(S, s);
}


void StringBuffer_free(T *S) {
        assert(S && *S);
	FREE((*S)->buffer);
        FREE(*S);
}


T StringBuffer_append(T S, const char *s, ...) {
        assert(S);
        if (s && *s) {
                va_list ap;
                va_start(ap, s);
                doAppend(S, s, ap);
                va_end(ap);
        }
        return S;
}


T StringBuffer_vappend(T S, const char *s, va_list ap) {
        assert(S);
        if (s && *s) {
                va_list ap_copy;
                va_copy(ap_copy, ap);
                doAppend(S, s, ap_copy);
                va_end(ap_copy);
        }
        return S;
}


int StringBuffer_prepare2postgres(T S) {
        int n, i;
        assert(S);
        for (n = i = 0; S->buffer[i]; i++) if (S->buffer[i] == '?') n++;
        if (n > 99)
                THROW(SQLException, "Max 99 parameters are allowed in a PostgreSQL prepared statement. Found %d parameters in statement", n);
        else if (n) {
                int j, xl;
                char x[3] = {'$'};
                int required = (n * 2) + S->used;
                if (required >= S->length) {
                        S->length = required;
                        RESIZE(S->buffer, S->length);
                }
                for (i = 0, j = 1; (j<=n); i++) {
                        if (S->buffer[i] == '?') {
                                if(j<10){xl=2;x[1]=j+'0';}else{xl=3;x[1]=(j/10)+'0';x[2]=(j%10)+'0';}
                                memmove(S->buffer + i + xl, S->buffer + i + 1, (S->used - (i + 1)));
                                memmove(S->buffer + i, x, xl);
                                S->used += xl - 1;
                                j++;
                        }
                }
                S->buffer[S->used] = 0;
        }
        return n;
}


int StringBuffer_length(T S) {
        assert(S);
        return S->used;
}


void StringBuffer_clear(T S) {
        assert(S);
        S->used = 0;
        *S->buffer = 0;
}


const char *StringBuffer_toString(T S) {
        assert(S);
        return S->buffer;
}


#ifdef PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

