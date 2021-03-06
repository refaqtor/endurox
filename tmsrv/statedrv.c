/* 
** Transaction state driver (uses libatmi/xastates.c for driving)
**
** @file statedrv.c
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
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>

#include <exnet.h>
#include <ndrxdcmn.h>

#include "tmsrv.h"
#include "../libatmisrv/srv_int.h"
#include "tperror.h"
#include <xa_cmn.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Do one try for transaciton processing using state machine defined in atmilib
 * @param p_xai - xa info structure
 * @param p_tl - transaction log
 * @return TPreturn code.
 */
public int tm_drive(atmi_xa_tx_info_t *p_xai, atmi_xa_log_t *p_tl, int master_op,
                        short rmid)
{
    int ret = SUCCEED;
    int i;
    int again;
    rmstatus_driver_t* vote_txstage;
    txstage_descriptor_t* descr;
    char stagearr[NDRX_MAX_RMS];
    int min_in_group;
    int min_in_overall;
    NDRX_LOG(log_info, "tm_drive() enter from xid=[%s]", p_xai->tmxid);
    do
    {
        short new_txstage = 0;
        int op_code = 0;
        int op_ret = 0;
        int op_reason = 0;
        int op_tperrno = 0;
        
        again = FALSE;
        
        if (NULL==(descr = xa_stage_get_descr(p_tl->txstage)))
        {
            NDRX_LOG(log_error, "Invalid stage %hd", p_tl->txstage);
            ret=TPESYSTEM;
            goto out;
        }
        
        NDRX_LOG(log_info, "Entered in stage: %s", descr->descr);
        memset(stagearr, 0, sizeof(stagearr));
        
        for (i=0; i<NDRX_MAX_RMS; i++)
        {
            /* Skipt not joined... */
            if (!p_tl->rmstatus[i].rmstatus || (FAIL!=rmid && i+1!=rmid))
                continue;

            NDRX_LOG(log_info, "RMID: %hd status %c", 
					i+1, p_tl->rmstatus[i].rmstatus);
            
            op_reason = XA_OK;
            op_tperrno = 0;
            op_code = xa_status_get_op(p_tl->txstage, p_tl->rmstatus[i].rmstatus);
            switch (op_code)
            {
                case XA_OP_NOP:
                    NDRX_LOG(log_error, "OP_NOP");
                    break;
                case XA_OP_PREPARE:
                    NDRX_LOG(log_error, "Prepare RMID %d", i+1);
                    if (SUCCEED!=(op_ret = tm_prepare_combined(p_xai, i+1)))
                    {
                        op_reason = atmi_xa_get_reason();
                        op_tperrno = tperrno;
                    }
                    break;
                case XA_OP_COMMIT:
                    NDRX_LOG(log_error, "Commit RMID %d", i+1);
                    if (SUCCEED!=(op_ret = tm_commit_combined(p_xai, i+1)))
                    {
                        op_reason = atmi_xa_get_reason();
                        op_tperrno = tperrno;
                    }
                    break;
                case XA_OP_ROLLBACK:
                    NDRX_LOG(log_error, "Rollback RMID %d", i+1);
                    if (SUCCEED!=(op_ret = tm_rollback_combined(p_xai, i+1)))
                    {
                        op_reason = atmi_xa_get_reason();
                        op_tperrno = tperrno;
                    }
                    break;
                default:
                    NDRX_LOG(log_error, "Invalid opcode %d", op_code);
                    ret=TPESYSTEM;
                    goto out;
                    break;
            }
            NDRX_LOG(log_info, "Operation tperrno: %d, xa return code: %d",
                                     op_tperrno, op_reason);
            
            /* Now get the transition of the state/vote */
            if (XA_OP_NOP == op_code)
            {
                if (NULL==(vote_txstage = xa_status_get_next_by_new_status(p_tl->txstage, 
                        p_tl->rmstatus[i].rmstatus)))
                {
                    NDRX_LOG(log_error, "No stage info for %hd/%c - ignore", p_tl->txstage, 
                        p_tl->rmstatus[i].rmstatus);
                    /*
                    ret=TPESYSTEM;
                    goto out;
                    */
                    continue;
                }
            }
            else
            {
                if (NULL==(vote_txstage = xa_status_get_next_by_op(p_tl->txstage, 
                        p_tl->rmstatus[i].rmstatus, op_code, op_reason)))
                {
                    NDRX_LOG(log_error, "Invalid stage for %hd/%c/%d/%d", 
                            p_tl->txstage, p_tl->rmstatus[i].rmstatus, op_code, op_reason);
                    ret=TPESYSTEM;
                    goto out;
                }
                
                /* Log RM status change... */
                tms_log_rmstatus(p_tl, i+1, vote_txstage->next_rmstatus, tperrno, 
                        op_reason);
            }
            /* Stage switching... */
            
            stagearr[i] = vote_txstage->next_txstage;
            
            if ((descr->txs_stage_min>vote_txstage->next_txstage ||
                    descr->txs_max_complete<vote_txstage->next_txstage) && descr->allow_jump)
            {
                NDRX_LOG(log_warn, "Stage group left!");
                
                new_txstage = vote_txstage->next_txstage;
                /* switch the stage */
                again = TRUE;
                break;
            }
                    
            /* Maybe we need some kind of arrays to put return stages in? 
             We need to put all states in one array.
             1. If there is any stage in the min & max ranges => Stick with the lowest from range
             2. If there is nothing in range, but have stuff outside, then take lowest from outside
             */
        }
        
        if (!new_txstage)
        {
            min_in_group = XA_TX_STAGE_MAX_NEVER;
            min_in_overall = XA_TX_STAGE_MAX_NEVER;
            /* Calculate from array */
            for (i=0; i<NDRX_MAX_RMS; i++)
            {
                if (stagearr[i])
                {
                    NDRX_LOG(log_info, "RM %d votes for stage: %d", i+1, stagearr[i]);
                    if (stagearr[i] < min_in_group)
                    {
                        min_in_overall = stagearr[i];
                    }
                    
                    if (descr->txs_stage_min<vote_txstage->next_txstage &&
                        descr->txs_max_complete>vote_txstage->next_txstage)
                    {
                        min_in_group = stagearr[i];
                    }
                }
            }/* for */
            
            if (min_in_group!=XA_TX_STAGE_MAX_NEVER)
                new_txstage=min_in_group;
            else
                new_txstage=min_in_overall;
            
            if (XA_TX_STAGE_MAX_NEVER==new_txstage)
            {
                NDRX_LOG(log_error, "Stage not switched - assume MAX COMPLETED!");
                new_txstage=descr->txs_max_complete;
                /*
                ret=TPESYSTEM;
                goto out;
                */
            }
            
        } /* calc stage */
        
        /* Finally switch the stage & run again! */
        if (new_txstage!=descr->txstage && new_txstage!=XA_TX_STAGE_MAX_NEVER)
        {
            tms_log_stage(p_tl, new_txstage);
            again = TRUE;
        }
        
    } while (again);
    
    /* TODO: Check are we complete */
    if (descr->txstage >=descr->txs_min_complete &&
            descr->txstage <=descr->txs_max_complete)
    {
        NDRX_LOG(log_info, "Transaction completed - remove logs");
        
        /* p_tl becomes invalid! */
        tms_remove_logfile(p_tl);
    }
    else
    {
        /* move transaction to background */
        NDRX_LOG(log_info, "Transaction not completed - leave "
                "to background");
        p_tl->is_background = TRUE;
        /* TODO: Unlock the transaction */
        tms_unlock_entry(p_tl);
    }
    
    /* map stage to return code */
    ret = xa_txstage2tperrno(descr->txstage, master_op);
    
out:          
    NDRX_LOG(log_info, "tm_drive() returns %d", ret);
    return ret;
}
