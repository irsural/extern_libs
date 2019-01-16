#ifndef _USBH_VIDEO_STREAM_PARSING_H
#define _USBH_VIDEO_STREAM_PARSING_H


#ifdef __cplusplus
 extern "C" {
#endif 

#include "usbh_video.h"

void video_stream_process_packet(uint16_t size);
void video_stream_init_buffers(uint8_t* buffer0, uint8_t* buffer1);
void uvc_video_stream_ready_update(void);

uint32_t uvc_get_frame_buffer();
uint32_t uvc_get_frame_length();

uint8_t uvc_is_frame_ready();
void uvc_reset_frame_ready_flag();




#ifdef __cplusplus
}
#endif


#endif

