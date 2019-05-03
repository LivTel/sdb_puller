/*
** Module Name:
**    CluVersion.c
**
** Purpose:
**    Code in command-line argument handling to submit a version into the SDB.
**
** Description:
**    Contains function of Command Line Utilities to submit an application's 
**    version into the SDB and perform a CIL setup. This function is located in
**    a separate source file to become a separate object in the library, so 
**    an application not using CIL will not have to link in the CIL library if
**    they are not using this functionality.
**
**    This code has been written to conform to "TTL Coding Standard" v1.0.
**
** Authors:
**    mjf : Martyn J. Ford (TTL)
**
** Version:
**    $Id: CluVersion.c,v 0.2 2000/12/14 12:05:01 mjf Exp $
**
** History:
**    $Log: CluVersion.c,v $
**    Revision 0.2  2000/12/14 12:05:01  mjf
**    Improvements when CIL setup has already been performed.
**
**    Revision 0.1  2000/12/12 18:47:32  mjf
**    Initial revision.
**
**
*/


/*
** Compiler include files
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
** System include files
*/

#include "TtlSystem.h"
#include "Clu.h"
#include "Log.h"
#include "Cil.h"
#include "Sdb.h"
#include "Tim.h"


/*
** Local include files
*/

#include "CluPrivate.h"


/*******************************************************************************
**
** Function Name:
**    eCluSubmitVersion
**
** Type:
**    Status_t
**       Returns the completion status of this function.
**
** Purpose:
**    Function to submit version information for the application into the SDB.
**
** Description:
**    If a CIL setup has not already been performed, then this will be done 
**    using the application's CIL name and any errors reported. If the CIL 
**    setup is successful or one has already been performed, then the 
**    application's version will then be submitted into the SDB, using the 
**    generic version datum. It is normal for an eCluSignOn to have been 
**    performed by this point to ensure that LOG has been setup, although this 
**    is not mandatory.
**
** Arguments:
**    None
**
** Authors:
**    mjf : Martyn J. Ford (TTL)
**
*******************************************************************************/

Status_t eCluSubmitVersion( void )
{
   int      Count;                     /* count for repeating SDB submissions */
   Int32_t  CilId;                     /* CIL ID of current application */
   eCilMsg_t SdbMessage;               /* CIL message for SDB submission */
   iCluSdbData_t SdbSubmission;        /* SDB submission data */
   Status_t Status;                    /* local status in this function */

   /* if a CIL setup has not already been performed */
   if ( eCilAlreadySetup() == FALSE )
   {
      /* attempt to determine the CIL ID */
      Status = eCilLookup( eCluCommon.CilMap, eCluCommon.CilName, &CilId );

      /* if unable to determine the CIL ID */
      if ( Status != SYS_NOMINAL )
      {
         /* log an error */
         eLogReport( E_LOG_ERR, Status, 
                     "Unable to determine CIL ID using name '%s', mapfile '%s'",
                     eCluCommon.CilName, eCluCommon.CilMap );

         /* return the failure generated by the CIL lookup */
         return Status;
      }

      /* attempt to perform CIL setup now CIL ID has been determined */
      Status = eCilSetup( eCluCommon.CilMap, CilId );

      /* check status returned - report any errors */
      switch ( Status )
      {
         case SYS_NOMINAL :
         /* not an error - successful completion */
         break;

         case E_CIL_ALREADY_SETUP :
            /* should not occur - we have checked that CIL is not set up */
            break;

         case E_CIL_NO_MAP_GIVEN :
            eLogReport( E_LOG_ERR, Status, 
                        "CIL setup failed - no CIL map filename supplied" );
            break;

         case E_CIL_ID_INVALID :
            eLogReport( E_LOG_ERR, Status, 
                        "CIL setup failed - CIL ID %d invalid", CilId );
            break;

         case E_CIL_MAP_NOT_FOUND : 
            eLogReport( E_LOG_ERR, Status, 
                        "CIL setup failed - unable to locate CIL map '%s'", 
                        eCluCommon.CilMap );
            break;

         case E_CIL_MAP_ERROR :
            eLogReport( E_LOG_ERR, Status, 
                        "CIL setup failed - error in CIL map file '%s'", 
                        eCluCommon.CilMap );
            break;

         case E_CIL_NEW_DEST_FAIL :
            eLogReport( E_LOG_ERR, Status, 
                        "CIL setup failed - unable to create new destination" );
            break;

         case E_CIL_UNKNOWN_NAME :
            eLogReport( E_LOG_ERR, Status, 
                      "CIL setup failed - local name not found in CIL map '%s'", 
                        eCluCommon.CilMap );
            break;

         case E_CIL_MAP_TOO_SMALL :
            eLogReport( E_LOG_ERR, Status, 
                      "CIL setup failed - insufficient entries in CIL map '%s'", 
                        eCluCommon.CilMap );
            break;

         case E_CIL_SOCKET_FAIL :
            eLogReport( E_LOG_ERR, Status, 
                        "CIL setup failed - unable to create UDP socket" );
            break;

         case E_CIL_ADDRINUSE :
            eLogReport( E_LOG_ERR, Status, 
                        "CIL setup failed - IP address already in use" );
            break;

         case E_CIL_ADDRNOTAVAIL :
            eLogReport( E_LOG_ERR, Status, 
                        "CIL setup failed - IP address unavailable" );
            break;

         case E_CIL_GEN_ERR :
            eLogReport( E_LOG_ERR, Status, 
                        "CIL setup failed - general CIL error" );
            break;
  
         default :
            eLogReport( E_LOG_ERR, Status, 
                        "CIL setup failed" );
            break;
      }

      /* if CIL setup status not done correctly */
      if ( Status != SYS_NOMINAL )
      {
         /* exit this function and return the CIL failure code */
         return Status;
      }
   }

   /* submit version to SDB several times to increase chances of success */
   for ( Count = 0; Count < M_CLU_SDB_SUBMITS; Count++ )
   {
      /* prepare CIL message for submitting version to SDB */
      SdbMessage.SourceId               = eCilId();
      SdbMessage.DestId                 = M_CLU_CIL_SDB;
      SdbMessage.Class                  = E_CIL_CMD_CLASS;
      SdbMessage.Service                = E_SDB_SUBMIT_1P;
      SdbMessage.SeqNum                 = 0;
      SdbMessage.DataPtr                = &SdbSubmission;
      SdbMessage.DataLen                = sizeof( SdbSubmission );

      /* construct SDB submission message */
      SdbSubmission.NumElts             = 1;
      SdbSubmission.Datum.SourceId      = eCilId();
      SdbSubmission.Datum.DatumId       = D_MCP_APP_VERSION;
      SdbSubmission.Datum.Units         = E_SDB_MVERSION_UNITS;
      SdbSubmission.Datum.Msrment.Value = M_CLU_GET_MVERSION( eCluMajorVer, 
                                                              eCluMinorVer );

      /* time-stamp the submission */
      Status = eTimGetTime( &SdbSubmission.Datum.Msrment.TimeStamp );

      /* convert packet to network byte-order */
      Status = eCilConvert32bitArray( SdbMessage.DataLen, SdbMessage.DataPtr );

      /* send the message to the SDB */
      Status = eCilSend( M_CLU_CIL_SDB, &SdbMessage );

      /* check status and report any errors */
      if ( Status != SYS_NOMINAL )
      {
         eLogReport( E_LOG_ERR, Status,
                     "Error sending version submission CIL message to SDB" );
      }
   }
    
   /* return the status - success or otherwise of CIL messaging */
   return Status;

}


/*
** EOF
*/