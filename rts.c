#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parseSaleae.h"

/**
 ****************************************************************************************************
 *
 * \brief
 *
 * \param
 *
 * \returns
 *
 *****************************************************************************************************/
void
   rts_process (frame_t *frame)
{
   static unsigned int last_RTS = 0;

   if ((frame->RTS != last_RTS)) {
      hdr(frame);
      printf("RTS: %s\n", frame->RTS ? "Rise":"Fall");

      last_RTS = frame->RTS;
   }
}
