/* 
** Typed STRING tests
**
** @file atmisv22.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>
#include <string.h>
#include "test022.h"

long M_subs_to_unsibscribe = -1;

/**
 * Receive some string & reply back with string, ok?
 */
void TEST22_STRING(TPSVCINFO *p_svc)
{
    int ret = SUCCEED;
    char *buf = p_svc->data;
    char type[16+1]={EOS};
    int i;
    
    if (FAIL==tptypes(buf, type, NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: TEST22_STRING cannot "
                "determine buffer type");
        FAIL_OUT(ret);
    }
    
    if (0!=strcmp(type, "STRING"))
    {
        NDRX_LOG(log_error, "TESTERROR: Buffer not STRING!");
        FAIL_OUT(ret);
    }
    
    if (0!=strcmp(buf, "HELLO WORLD"))
    {
        NDRX_LOG(log_error, "TESTERROR: Incoming string not \"HELLO WORLD\"");
        FAIL_OUT(ret);
    }
    
    if (NULL== (buf = tprealloc(buf, TEST_REPLY_SIZE+1)))
    {
        NDRX_LOG(log_error, "TESTERROR: TEST22_STRING failed "
                "to realloc the buffer");
    }
    
    /* fill all the buffer with  */
    for (i=0; i<TEST_REPLY_SIZE; i++)
    {
        buf[i]=i%255+1;
    }
    
    buf[TEST_REPLY_SIZE] = EOS;
    
    NDRX_LOG(log_debug, "Sending buffer: [%s]", buf);
    
out:
    
    tpreturn(SUCCEED==ret?TPSUCCESS:TPFAIL, 0, buf, 0L, 0L);
    
}

/**
 * This should match the regex of caller (extra filter test func)
 */
void TEST22 (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    
out:

    tpreturn(0, 0, NULL, 0L, 0L);

}

/**
 * This shall not match the regex of caller (extra filter test func)
 */
void TEST22_2(TPSVCINFO *p_svc)
{
    long ret;
    
    NDRX_LOG(log_debug, "TEST22_2 - Called");
    
    tpreturn(0, 0, NULL, 0L, 0L);
}

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=SUCCEED;
    
    NDRX_LOG(log_debug, "tpsvrinit called");
    TPEVCTL evctl;

    memset(&evctl, 0, sizeof(evctl));

    if (SUCCEED!=tpadvertise("TEST22_STRING", TEST22_STRING))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST22_STRING!");
        ret=FAIL;
    }
    else if (SUCCEED!=tpadvertise("TEST22", TEST22))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST22 (first)!");
        ret=FAIL;
    }
    else if (SUCCEED!=tpadvertise("TEST22_2", TEST22_2))
    {
        NDRX_LOG(log_error, "Failed to initialize TEST22_2!");
        ret=FAIL;
    }
    
    if (SUCCEED!=ret)
    {
        goto out;
    }

    strcpy(evctl.name1, "TEST22");
    evctl.flags|=TPEVSERVICE;

    /* Subscribe to event server */
    if (FAIL==tpsubscribe("TEST22EV", "Hello (.*) Mars", &evctl, 0L))
    {
        NDRX_LOG(log_error, "Failed to subscribe TEST22 "
                                        "to TEST22EV event failed");
        ret=FAIL;
    }

    strcpy(evctl.name1, "TEST22_2");
    /* Subscribe to event server */
    if (FAIL==(M_subs_to_unsibscribe=tpsubscribe("TEST22EV", "Hello (.*) Pluto", &evctl, 0L)))
    {
        NDRX_LOG(log_error, "Failed to subscribe TEST22 "
                                        "to TEST22EV event failed");
        ret=FAIL;
    }
    
out:
    return ret;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}

