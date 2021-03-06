/* 
** Basic test client
**
** @file atmiclt1.c
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
#include <fcntl.h>
#include <unistd.h>

#include <nstdutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define _BENCHONLY

#define SYNC_FILE "sync.log"
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Sync test start...
 */
int do_sync (void)
{
    while (FAIL == access( SYNC_FILE, F_OK ) )
    {
        /* sleep for 0.01 sec */
        usleep(10000);
    }
    
    return SUCCEED;
}

/*
 * Do the test call to the server
 */
int main(int argc, char** argv) {

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 156000);
    long rsplen;
    int i, j;
    int ret=SUCCEED;
    double d;
    double cps;
    double dv = 55.66;
    char bench_mode[16]=":0:1:8:";
    char buf[1024];
    char test_buf_carray[56*1024];
    char test_buf_small[1024];
    ndrx_timer_t timer;
    int call_num = MAX_ASYNC_CALLS *10;
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 1", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 2", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 3", 0);

    /* Do synchronization if needed...  */
    if (argc>1 && (argv[1][0]=='s' || argv[1][0]=='S'))
    {
        printf("syncing...\n");
        do_sync();
    }
    
    
    printf("syncing ok...\n");
    fflush(stdout);
    
    if (argc>2)
    {
        strcpy(bench_mode, argv[2]);
    }
    
    
    printf("test mode [%s]\n", bench_mode);
    fflush(stdout);
    
    for (i=0; i<50; i++)
    {
        char tmp[128];
        sprintf(tmp, "HELLO FROM CLIENT %d abc", i);
        if (FAIL==CBchg(p_ub, T_CARRAY_FLD, i, tmp, 0, BFLD_STRING))
            {
                NDRX_LOG(log_debug, "Failed to get T_CARRAY_FLD[%d]", i);
                ret=FAIL;
                goto out;
            }
    }
    
    tplogprintubf(log_error, "Carray buffer", p_ub);
        
    if (strstr(bench_mode, ":0:"))
    {
        for (i=0; i<50; i++)
        {
            d=i;
            if (FAIL==Bchg(p_ub, T_DOUBLE_FLD, i, (char *)&d, 0))
            {
                NDRX_LOG(log_debug, "Failed to get T_DOUBLE_FLD[%d]", i);
                ret=FAIL;
                goto out;
            }

            if (FAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
            {
                NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
                ret=FAIL;
                goto out;
            }

            /* Verify the data */
            if (FAIL==Bget(p_ub, T_DOUBLE_FLD, i, (char *)&d, 0))
            {
                NDRX_LOG(log_debug, "Failed to get T_DOUBLE_FLD[%d]", i);
                ret=FAIL;
                goto out;
            }

            if (fabs(i - d) > 0.00001)
            {
                NDRX_LOG(log_debug, "%lf!=%lf =>  FAIL", dv, d);
                ret=FAIL;
                goto out;
            }

            /* print the output */
            Bfprint(p_ub, stderr);
        }

        NDRX_LOG(log_debug, "Do second call to timeout-server");
        if (FAIL == tpcall("TIMEOUTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TIMEOUTSV failed: %s", tpstrerror(tperrno));
            if (TPETIME!=tperrno)
            {
                NDRX_LOG(log_debug, "No timeout err from TIMEOUTSV!!!");
                ret=FAIL;
                goto out;
            }
        }
        else
        {
            NDRX_LOG(log_debug, "No timeout from TIMEOUTSV!!!");
            ret=FAIL;
            goto out;
        }

        /* Do Another test, but now we will wait! */
        NDRX_LOG(log_debug, "Do third call to timeout-server, TPNOTIME");
        if (FAIL == tpcall("TIMEOUTSV", (char *)p_ub, 0L, (char **)&p_ub, 
                &rsplen, TPNOTIME))
        {
            NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }

        /*
         * We should be able to call serices with out passing buffer to call.
         */
        if (FAIL==tpcall("NULLSV", NULL, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
        {
            NDRX_LOG(log_error, "TESTERROR: NULLSV failed: %s", tpstrerror(tperrno));
            ret=FAIL;
            goto out;
        }


        for (i=0; i<MAX_ASYNC_CALLS+2000; i++) /* Test the cd loop */
        {
            /*
            * Test the case when some data should be returned
            */
            if (FAIL==tpcall("RETSOMEDATA", NULL, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
            {
                NDRX_LOG(log_error, "TESTERROR: RETSOMEDATA failed: %s", 
                        tpstrerror(tperrno));
                ret=FAIL;
                goto out;
            }

            if (FAIL==Bget(p_ub, T_STRING_2_FLD, 0, buf, NULL))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_2_FLD: %s", 
                        Bstrerror(Berror));
                ret=FAIL;
                goto out;
            }
        }

        if (0!=strcmp(buf, "RESPONSE DATA 1"))
        {
            NDRX_LOG(log_error, "TESTERROR: Invalid response data in "
                    "T_STRING_2_FLD, got [%s]", buf);
            ret=FAIL;
            goto out;
        }
    }
    /**************************************************************************/
    /* 1KB benchmark*/
    if (strstr(bench_mode, ":1:"))
    {
        /* p_ub = (UBFH *)tprealloc ((char *)p_ub, 1128); */

        if (SUCCEED!=Bchg(p_ub, T_CARRAY_FLD, 0, test_buf_small, sizeof(test_buf_small)))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_CARRAY_FLD");
            ret=FAIL;
            goto out;
        }

        ndrx_timer_reset(&timer);

        /* Do the loop call! */
        for (i=0; i<call_num; i++) /* Test the cd loop */
        {
            /*
            * Test the case when some data should be returned
            */
            if (FAIL==tpcall("ECHO", (char *)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
            {
                NDRX_LOG(log_error, "TESTERROR: ECHO failed: %s", tpstrerror(tperrno));
                ret=FAIL;
                goto out;
            }
        }

        d = (double)(sizeof(test_buf_small)*(call_num))/((double)ndrx_timer_get_delta(&timer)/1000.0f);

        cps = (double)(call_num)/((double)ndrx_timer_get_delta(&timer)/1000.0f);

        printf("1KB Performance: %d bytes in %ld (sec) = %lf bytes/sec = %lf bytes/MB sec, "
                "calls/sec = %lf\n", 
                (int)(sizeof(test_buf_small)*(call_num)), 
                (long)ndrx_timer_get_delta_sec(&timer),  
                d,
                (d/1024)/1024, 
                cps);
        fflush(stdout);
    }
    /**************************************************************************/
    /* 8KB benchmark*/
    
    if (strstr(bench_mode, ":8:"))
    {
        /* p_ub = (UBFH *)tprealloc ((char *)p_ub, 9216); */
        tplogprintubf(log_error, "Buffer before 8K test", p_ub);

        if (SUCCEED!=Bchg(p_ub, T_CARRAY_FLD, 0, test_buf_carray, 1024*8))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to set T_CARRAY_FLD: %s",
                Bstrerror(Berror));
            ret=FAIL;
            goto out;
        }

        ndrx_timer_reset(&timer);

        /* Do the loop call! */
        for (i=0; i<call_num; i++) /* Test the cd loop */
        {
            /*
            * Test the case when some data should be returned
            */
            if (FAIL==tpcall("ECHO", (char *)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
            {
                NDRX_LOG(log_error, "TESTERROR: ECHO failed: %s", tpstrerror(tperrno));
                ret=FAIL;
                goto out;
            }
        }

        d = (double)(sizeof(test_buf_carray)*(call_num))/((double)ndrx_timer_get_delta(&timer)/1000.0f);

        cps = (double)(call_num)/((double)ndrx_timer_get_delta(&timer)/1000.0f);

        printf("8KB Performance: %d bytes in %ld (sec) = %lf bytes/sec = %lf bytes/MB sec, calls/sec = %lf\n", 
                (int)(sizeof(test_buf_carray)*(call_num)), 
                (long)ndrx_timer_get_delta_sec(&timer),  
                d,
                (d/1024)/1024, 
                cps);
        fflush(stdout);
    }
    
    /* XKB benchmark*/
    
    if (strstr(bench_mode, ":B:"))
    {
        int bench_call_num;
        int first = TRUE;
        
        /*start with 1 byte, then with 1 kb, then +4 kb up till 56... */
        for (j=0; j<56; j=(j==0?j=1:j+4))
        {
            int callsz = j*1024;
            if (0==j)
            {
                callsz = 1; /* send 1 byte.. */
            }
            
            
            /* p_ub = (UBFH *)tprealloc ((char *)p_ub, callsz+500); */

            if (SUCCEED!=Bchg(p_ub, T_CARRAY_FLD, 0, test_buf_carray, callsz))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to set T_CARRAY_FLD to %d: %s",
                        callsz, Bstrerror(Berror));
                ret=FAIL;
                goto out;
            }
            
            if (j<10)
            {
                bench_call_num = call_num*2;
            }
            else
            {
                bench_call_num = call_num;
            }
            
B_warmed_up:
            ndrx_timer_reset(&timer);
            
            /* Do the loop call! */
            for (i=0; i<bench_call_num; i++) /* Test the cd loop */
            {
                /*
                * Test the case when some data should be returned
                */
                if (FAIL==tpcall("ECHO", (char *)p_ub, 0L, (char **)&p_ub, &rsplen, TPNOTIME))
                {
                    NDRX_LOG(log_error, "TESTERROR: ECHO failed: %s", tpstrerror(tperrno));
                    ret=FAIL;
                    goto out;
                }
            }

            /*do the warmup... */
            if (first)
            {
                first = FALSE;
                goto B_warmed_up;
            }

            d = (double)(sizeof(test_buf_carray)*(call_num))/(double)((double)ndrx_timer_get_delta(&timer)/1000.0f);

            cps = (double)(bench_call_num)/((double)ndrx_timer_get_delta(&timer)/1000.0f);

            printf("%dKB Performance: %d bytes in %ld (sec) = %lf bytes/sec = %lf bytes/MB sec, calls/sec = %lf\n", 
                    callsz,
                    (int)(sizeof(test_buf_carray)*(call_num)), 
                    (long)ndrx_timer_get_delta_sec(&timer),  
                    d,
                    (d/1024)/1024, 
                    cps);
            
            fflush(stdout);
            
            if (SUCCEED!=ndrx_bench_write_stats((double)j, cps))
            {
                NDRX_LOG(log_always, "Failed to write stats!");
                FAIL_OUT(ret);
            }
        }
    }
    
    /* Benchmark with non-rely/async calls */
    if (strstr(bench_mode, ":b:"))
    {
        int bench_call_num;
        int first = TRUE;
        for (j=0; j<56; j=(j==0?j=1:j+4))
        {
            int callsz = j*1024;
            if (0==j)
            {
                callsz = 1; /* send 1 byte.. */
            }

            /* p_ub = (UBFH *)tprealloc ((char *)p_ub, callsz+500); */

            if (SUCCEED!=Bchg(p_ub, T_CARRAY_FLD, 0, test_buf_carray, callsz))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to set T_CARRAY_FLD to %d", callsz);
                ret=FAIL;
                goto out;
            }
            
            if (j<20)
            {
                bench_call_num = call_num*5;
            }
            else
            {
                bench_call_num = call_num;
            }

            ndrx_timer_reset(&timer);
            
            
/* Repeat the test if we did the warump for first time. */
b_warmed_up:
            /* Do the loop call! */
            for (i=0; i<bench_call_num; i++) /* Test the cd loop */
            {
                /*
                * Test the case when some data should be returned
                */
                if (FAIL==tpacall("ECHO", (char *)p_ub, 0L, TPNOTIME|TPNOREPLY))
                {
                    NDRX_LOG(log_error, "TESTERROR: ECHO failed: %s", tpstrerror(tperrno));
                    ret=FAIL;
                    goto out;
                }
            }            
            
            /* +1 call to ensure that target is processed all the stuff.. */
            if (FAIL==tpcall("ECHO", (char *)p_ub, 0L, (char **)&p_ub, &rsplen, 
                        TPNOTIME))
                {
                    NDRX_LOG(log_error, "TESTERROR: ECHO failed: %s", tpstrerror(tperrno));
                    ret=FAIL;
                    goto out;
            }

            /*do the warmup... */
            if (first)
            {
                first = FALSE;
                goto b_warmed_up;
            }


            d = (double)(sizeof(test_buf_carray)*(call_num))/(double)((double)ndrx_timer_get_delta(&timer)/1000.0f);

            cps = (double)(bench_call_num)/((double)ndrx_timer_get_delta(&timer)/1000.0f);

            printf("%dKB Performance: %d bytes in %ld (sec) = %lf bytes/sec = %lf bytes/MB sec, calls/sec = %lf\n", 
                    callsz,
                    (int)(sizeof(test_buf_carray)*(call_num)), 
                    (long)ndrx_timer_get_delta_sec(&timer),  
                    d,
                    (d/1024)/1024, 
                    cps);
            
            fflush(stdout);
            
            if (SUCCEED!=ndrx_bench_write_stats((double)j, cps))
            {
                NDRX_LOG(log_always, "Failed to write stats!");
                FAIL_OUT(ret);
            }
        }
    }
    
out:
    tpterm();

    if (argc>1 && argv[1][0]=='S')
    {
        printf("Test finished waiting for kill..");
        fflush(stdout);
        sleep(9999999);
    }
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

