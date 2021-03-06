/* 
** `shm_psrv' command implementation - SHM - print servers
**
** @file cmd_shm_psrv.c
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
#include <sys/param.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>

#include "ntimer.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Print header
 * @return
 */
private void print_hdr(void)
{
    fprintf(stderr, "SLOT  SRVID LAST FLGS STATUS LAST CMD LAST CALLER   \n");
    fprintf(stderr, "----- ----- --------- ------ -------- --------------\n");
}

/**
 * Process response back.
 * @param reply
 * @param reply_len
 * @return
 */
public int shm_psrv_rsp_process(command_reply_t *reply, size_t reply_len)
{
    int i;
    char binary[9+1];

    if (NDRXD_CALL_TYPE_PM_SHM_PSRV==reply->msg_type)
    {
        command_reply_shm_psrv_t * shm_psrv_info = (command_reply_shm_psrv_t*)reply;
        fprintf(stdout, "%-5d %-5d %-9d %-6hd %-8hd %s\n", 
                shm_psrv_info->slot, shm_psrv_info->srvid, shm_psrv_info->last_call_flags, 
                shm_psrv_info->status, shm_psrv_info->last_command_id, 
                shm_psrv_info->last_reply_q
                );
    }
    
    return SUCCEED;
}

/**
 * Get service listings
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_shm_psrv(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    command_call_t call;
    memset(&call, 0, sizeof(call));

    /* Print header at first step! */
    print_hdr();
    /* Then get listing... */
    return cmd_generic_listcall(p_cmd_map->ndrxd_cmd, NDRXD_SRC_ADMIN,
                        NDRXD_CALL_TYPE_GENERIC,
                        &call, sizeof(call),
                        G_config.reply_queue_str,
                        G_config.reply_queue,
                        G_config.ndrxd_q,
                        G_config.ndrxd_q_str,
                        argc, argv,
                        p_have_next,
                        G_call_args,
                        FALSE);
}

