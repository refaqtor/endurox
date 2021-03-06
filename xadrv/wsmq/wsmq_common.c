/* 
** Load wsmq drivers - static version
**
** @file wsmq_common.c
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

#include <ndrstandard.h>
#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <sys_mqueue.h>

#include "atmi_shm.h"

#include <xa.h>
#define __USE_GNU
#include <dlfcn.h>
#include "wsmq_common.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/*
 * The function is exported and dynamically retrieved after the module was
 * fetched
 */
struct xa_switch_t *ndrx_get_xa_switch_int(char *symbol, char *descr)
{
    struct xa_switch_t * sw = NULL;
    void *handle = NULL;
    int ret = SUCCEED;
    NDRX_LOG(log_debug, "%s", descr);
    
    sw = (struct xa_switch_t * )dlsym( RTLD_DEFAULT, symbol );
    if( sw == NULL )
    {
        NDRX_LOG(log_debug, "%s symbol not found in "
                "process address space - loading .so!", symbol);
        
        /* Loading the symbol... */
        handle = dlopen (G_atmi_env.xa_rmlib, RTLD_NOW);
        if (!handle)
        {
            NDRX_LOG(log_error, "Failed to load XA Resource Manager lib [%s]: %s", 
                G_atmi_env.xa_rmlib, dlerror());
            FAIL_OUT(ret);
        }
        
        /* reslove symbol now... */
        if (NULL==(sw = (struct xa_switch_t * )dlsym( handle, symbol )))
        {
            NDRX_LOG(log_error, "Wsmq XA switch `%s' handler "
                    "not found!", symbol);
            FAIL_OUT(ret);
        }
    }
    
out:
    if (SUCCEED!=ret && NULL!=handle)
    {
        /* close the handle */
        dlclose(handle);
    }

    
    return sw;
}



#undef __USE_GNU
