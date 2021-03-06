/* 
** XA transaction processing, cli tests (listing & commit & abort)
**
** @file atmiclt21-cli.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <ntimer.h>
#include <oatmi.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/*
 * Just open the transaction, and let do shell testing later
 */
int main(int argc, char** argv) {

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 9216);
    long rsplen;
    int i=0;
    int ret=SUCCEED;
    
    for (i=0; i<100; i++)
    {
        TPCONTEXT_T ctx1 = tpnewctxt(0,0);

        if (NULL==ctx1)
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to get new context (%p): %s",
                    ctx1, tpstrerror(tperrno));
            FAIL_OUT(ret);
        }

        if (SUCCEED!=Otpopen(&ctx1))
        {
            NDRX_LOG(log_error, "TESTERROR: tpopen() fail: %d:[%s]", 
                                                tperrno, tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }

        if (SUCCEED!=Otpbegin(&ctx1, 60, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: Otpbegin() fail: %d:[%s]", 
                                                tperrno, tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }

        if (SUCCEED!=Otpabort(&ctx1, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: Otpabort() fail: %d:[%s]", 
                                                tperrno, tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }


        /* So for some reason we do not  */
        if (SUCCEED!=Otpclose(&ctx1))
        {
            NDRX_LOG(log_error, "TESTERROR: Otpclose() fail: %d:[%s]", 
                                                tperrno, tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }

        Otpterm(&ctx1);

        tpfreectxt(ctx1);

    }
    
out:
    NDRX_LOG(log_error, "Existing with %d", ret);
            
    return ret;
}

