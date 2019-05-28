// UVC HOST video capture for STM32 by ILIASAM
// Search for "GRXFSIZ" to change RX FIFO size
// See mor info at "usbh_video_stram_parsing.c" file

/* Includes ------------------------------------------------------------------*/
#include "usbh_video.h"
#include "usbh_video_desc_parsing.h"
#include "usbh_video_stream_parsing.h"


static USBH_StatusTypeDef USBH_VIDEO_InterfaceInit  (USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_VIDEO_InterfaceDeInit  (USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_VIDEO_SOFProcess(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_VIDEO_ClassRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_VIDEO_CSRequest(USBH_HandleTypeDef *phost, 
                                               uint8_t feature, 
                                               uint8_t channel);
static USBH_StatusTypeDef USBH_VIDEO_ProcessDummy(USBH_HandleTypeDef *phost);

static USBH_StatusTypeDef USBH_VIDEO_HandleCSRequest(USBH_HandleTypeDef *phost);

static USBH_StatusTypeDef USBH_VIDEO_InputStream(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_VIDEO_StartStream (USBH_HandleTypeDef *phost);

USBH_ClassTypeDef  VIDEO_Class = 
{
  "VIDEO",
  CC_VIDEO,
  USBH_VIDEO_InterfaceInit,
  USBH_VIDEO_InterfaceDeInit,
  USBH_VIDEO_ClassRequest,
  //USBH_VIDEO_Process, //BgndProcess
  USBH_VIDEO_ProcessDummy,
  USBH_VIDEO_SOFProcess,
  NULL,
};

// This struct is used for PROBE control request ( Setup Packet )
VIDEO_ProbeTypedef ProbeParams;
//Buffer to store received UVC data packet
volatile uint8_t tmp_packet_framebuffer[UVC_RX_FIFO_SIZE_LIMIT];


/** @defgroup Private_Functions
  * @{
  */ 


/**
  * @brief  USBH_VIDEO_InterfaceInit 
  *         The function init the Video class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_VIDEO_InterfaceInit (USBH_HandleTypeDef *phost)
{
  USBH_StatusTypeDef status = USBH_FAIL ;
  USBH_StatusTypeDef out_status ;    
  VIDEO_HandleTypeDef *VIDEO_Handle;
  uint8_t  interface, index;
  uint8_t choosen_ep = 0;
  
  interface = USBH_FindInterface(phost, CC_VIDEO, USB_SUBCLASS_VIDEOCONTROL, 0x00);
  
  if(interface == 0xFF) /* Not Valid Interface */
  {
    USBH_ErrLog("Cannot Find the interface for %s class.", phost->pActiveClass->Name);
    status = USBH_FAIL;      
  }
  else
  {
    phost->pActiveClass->pData = (VIDEO_HandleTypeDef *)USBH_malloc (sizeof(VIDEO_HandleTypeDef));
    VIDEO_Handle =  (VIDEO_HandleTypeDef *) phost->pActiveClass->pData; 
    USBH_memset(VIDEO_Handle, 0, sizeof(VIDEO_HandleTypeDef));
    
    //Сохраняем параметры интерфейса video control
    VIDEO_Handle->camera.video_control_interface = 
      (uint8_t)phost->device.CfgDesc.Itf_Desc[interface].bInterfaceNumber;
    VIDEO_Handle->camera.video_control_altinterface = 
      (uint8_t)phost->device.CfgDesc.Itf_Desc[interface].bAlternateSetting;
  
    USBH_UsrLog("VC interface: %d, VC alt.interface: %d",
      VIDEO_Handle->camera.video_control_interface, 
      VIDEO_Handle->camera.video_control_altinterface);

    /* 1st Step:  Find IN Video Interfaces */
    out_status = USBH_VIDEO_FindStreamingIN(phost);
    
    if(out_status == USBH_FAIL)
    {
      USBH_UsrLog("%s class configuration not supported.", phost->pActiveClass->Name);
      status = USBH_FAIL;
      return USBH_FAIL;
    }
    
    int ep_cnt = 0;
    /* 2nd Step:  Select Video Streaming Interfaces with best endpoint size*/ 
    for (index = 0; index < VIDEO_MAX_VIDEO_STD_INTERFACE; index++)
    {      
      if( VIDEO_Handle->stream_in[index].valid == 1)
      {
        ep_cnt++;
        uint16_t ep_size = VIDEO_Handle->stream_in[index].EpSize;
        
        //Читаем первые 10 бит
        uint16_t real_ep_size;
        real_ep_size = ep_size << 5;
        real_ep_size = real_ep_size >> 5;
        
        
        //Случай с дополнительными транзакциями
        if (ep_size & EXTRA_TX_FID_BIT0 || ep_size & EXTRA_TX_FID_BIT1) {
          //Одна доп. транзакция
          if (ep_size & EXTRA_TX_FID_BIT0) {
            /*
            VIDEO_Handle->camera.interface = VIDEO_Handle->stream_in[index].interface;
            VIDEO_Handle->camera.AltSettings = VIDEO_Handle->stream_in[index].AltSettings;                
            VIDEO_Handle->camera.Ep = VIDEO_Handle->stream_in[index].Ep;
            VIDEO_Handle->camera.EpSize = real_ep_size;
            VIDEO_Handle->camera.Poll = VIDEO_Handle->stream_in[index].Poll;         
            VIDEO_Handle->camera.supported = 1;    
            
            choosen_ep = index;
            */
            
            USBH_DbgLog("(%d) EP size: %d bytes", index, real_ep_size);
            USBH_DbgLog("^^^^Need 1 additional transaction per microframe\n");
          }
          //2 доп. транзакции
          else if (ep_size & EXTRA_TX_FID_BIT1) {
              
            /*
            VIDEO_Handle->camera.interface = VIDEO_Handle->stream_in[index].interface;
            VIDEO_Handle->camera.AltSettings = VIDEO_Handle->stream_in[index].AltSettings;                
            VIDEO_Handle->camera.Ep = VIDEO_Handle->stream_in[index].Ep;
            VIDEO_Handle->camera.EpSize = real_ep_size;
            VIDEO_Handle->camera.Poll = VIDEO_Handle->stream_in[index].Poll;         
            VIDEO_Handle->camera.supported = 1;    
            
            choosen_ep = index;
            */
              
              

            USBH_DbgLog("(%d) EP size: %d bytes", index, real_ep_size);
            USBH_DbgLog("^^^^Need 2 additional transaction per microframe\n");
          }
          //Этот случай обозначает ошибку
          else if (ep_size & EXTRA_TX_FID_BIT0 && ep_size & EXTRA_TX_FID_BIT1) {
            USBH_DbgLog("wMaxPacketSize field error");
          }
        } 
        //Доп. транзакций нет
        else {
          VIDEO_Handle->camera.interface = VIDEO_Handle->stream_in[index].interface;
          VIDEO_Handle->camera.AltSettings = VIDEO_Handle->stream_in[index].AltSettings;                
          VIDEO_Handle->camera.Ep = VIDEO_Handle->stream_in[index].Ep;
          VIDEO_Handle->camera.EpSize = real_ep_size;
          VIDEO_Handle->camera.Poll = VIDEO_Handle->stream_in[index].Poll;         
          VIDEO_Handle->camera.supported = 1;    
          
          choosen_ep = index;
          
          USBH_DbgLog("(%d)EP size: %d bytes\n", index, VIDEO_Handle->stream_in[index].EpSize);
        }
      }
    }
    USBH_UsrLog("Endpoints detected: %d", ep_cnt);
    USBH_UsrLog("Selected EP number: %d, size: %d bytes", choosen_ep, VIDEO_Handle->camera.EpSize);
    USBH_UsrLog("Interface/altinterface: %d/%d", VIDEO_Handle->camera.interface, VIDEO_Handle->camera.AltSettings);
    UNUSED(choosen_ep);
    
    /* 3rd Step:  Find and Parse Video interfaces */ 
    USBH_VIDEO_ParseCSDescriptors(phost);
    
    /* 4rd Step:  Find desrcroptors for target settings */
    USBH_VIDEO_AnalyseFormatDescriptors(&VIDEO_Handle->class_desc);
    if (USBH_VIDEO_Best_bFormatIndex == -1)
    {
      status = USBH_FAIL;
      return USBH_FAIL;
    }
    
    USBH_VIDEO_AnalyseFrameDescriptors(&VIDEO_Handle->class_desc);
    if (USBH_VIDEO_Best_bFrameIndex == -1)
    {
      status = USBH_FAIL;
      return USBH_FAIL;
    }
    
    if(VIDEO_Handle->camera.supported == 1)
    {       
      VIDEO_Handle->camera.Pipe = USBH_AllocPipe(phost, VIDEO_Handle->camera.Ep);
      
      /* Open pipe for IN endpoint */
      USBH_OpenPipe(phost,
                    VIDEO_Handle->camera.Pipe,
                    VIDEO_Handle->camera.Ep,
                    phost->device.address,
                    phost->device.speed,
                    USB_EP_TYPE_ISOC,
                    VIDEO_Handle->camera.EpSize);//working ok 
      
      USBH_LL_SetToggle(phost, VIDEO_Handle->camera.Pipe, 0);  
    }
    
    VIDEO_Handle->req_state     = VIDEO_REQ_INIT;
    VIDEO_Handle->control_state = VIDEO_CONTROL_INIT;
    
    status = USBH_OK;
  }
  return status;
}



/**
  * @brief  USBH_VIDEO_InterfaceDeInit 
  *         The function DeInit the Pipes used for the Video class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_VIDEO_InterfaceDeInit(USBH_HandleTypeDef *phost)
{
  VIDEO_HandleTypeDef *VIDEO_Handle = (VIDEO_HandleTypeDef *) phost->pActiveClass->pData; 
  
  if(VIDEO_Handle->camera.Pipe != 0x00)
  {
    USBH_ClosePipe  (phost, VIDEO_Handle->camera.Pipe);
    USBH_FreePipe  (phost, VIDEO_Handle->camera.Pipe);
    VIDEO_Handle->camera.Pipe = 0;     /* Reset the pipe as Free */  
  }
  
  if(phost->pActiveClass->pData)
  {
    USBH_free (phost->pActiveClass->pData);
    phost->pActiveClass->pData = 0;
  }  
  return USBH_OK ;
}

/**
  * @brief  USBH_VIDEO_ClassRequest 
  *         The function is responsible for handling Standard requests
  *         for Video class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_VIDEO_ClassRequest(USBH_HandleTypeDef *phost)
{   
  VIDEO_HandleTypeDef *VIDEO_Handle = (VIDEO_HandleTypeDef *)phost->pActiveClass->pData;  
  USBH_StatusTypeDef status = USBH_BUSY;
  USBH_StatusTypeDef req_status = USBH_BUSY;
  
  if (VIDEO_Handle->control_state == VIDEO_CONTROL_INIT) {
    
    req_status =  USBH_SetInterface(phost, 
      VIDEO_Handle->camera.video_control_interface, 
      VIDEO_Handle->camera.video_control_altinterface);
    
    if (req_status == USBH_OK) {
      phost->pUser(phost, HOST_USER_CLASS_SELECTED);
      VIDEO_Handle->control_state = VIDEO_CONTROL_IDLE;
    } else 
    {
      //USBH_UsrLog("VIDEO_CONTROL_INIT error");
    }
    return status;
  }
  
  /* Switch VIDEO REQ state machine */
  switch (VIDEO_Handle->req_state)
  {
  case VIDEO_REQ_INIT:
  case VIDEO_REQ_SET_DEFAULT_IN_INTERFACE:
    if(VIDEO_Handle->camera.supported == 1)
    {
      req_status = USBH_SetInterface(phost, 
                                     VIDEO_Handle->camera.interface, 
                                     0);
      
      if(req_status == USBH_OK)
      {
        VIDEO_Handle->req_state = VIDEO_REQ_SET_IN_INTERFACE;
      }
    }
    else
    {
      VIDEO_Handle->req_state = VIDEO_REQ_SET_DEFAULT_IN_INTERFACE;//???
#if (USBH_USE_OS == 1)
      osMessagePut ( phost->os_event, USBH_URB_EVENT, 0);
#endif       
    }
    break;
    
  case VIDEO_REQ_CS_REQUESTS:
    if(USBH_VIDEO_HandleCSRequest(phost) == USBH_OK)
    {
      VIDEO_Handle->req_state = VIDEO_REQ_SET_IN_INTERFACE;
    }
    break;
   
  case VIDEO_REQ_SET_IN_INTERFACE:
    if(VIDEO_Handle->camera.supported == 1)
    {
      req_status = USBH_SetInterface(phost, 
                                     VIDEO_Handle->camera.interface, 
                                     VIDEO_Handle->camera.AltSettings);
      
      if(req_status == USBH_OK)
      {
        VIDEO_Handle->req_state = VIDEO_REQ_IDLE;
        VIDEO_Handle->steam_in_state = VIDEO_STATE_START_IN;
      }
    }
    else
    {
      VIDEO_Handle->req_state = VIDEO_REQ_SET_IN_INTERFACE;
#if (USBH_USE_OS == 1)
      osMessagePut ( phost->os_event, USBH_URB_EVENT, 0);
#endif       
    }
    break;

  case VIDEO_REQ_IDLE:
    phost->pUser(phost, HOST_USER_CLASS_ACTIVE); 
    status  = USBH_OK;    
#if (USBH_USE_OS == 1)
      osMessagePut ( phost->os_event, USBH_CLASS_EVENT, 0);
#endif
  default:
    break;
  }
  return status; 
}

/**
  * @brief  USBH_VIDEO_CSRequest 
  *         The function is responsible for handling AC Specific requests for a specific feature and channel
  *         for Video class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_VIDEO_CSRequest(USBH_HandleTypeDef *phost, uint8_t feature, uint8_t channel)
{   
  USBH_StatusTypeDef status = USBH_BUSY;

  return status; 
}

/**
  * @brief  USBH_VIDEO_HandleCSRequest 
  *         The function is responsible for handling VC Specific requests for a all features 
  *         and associated channels for Video class.
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_VIDEO_HandleCSRequest(USBH_HandleTypeDef *phost)
{ 

  USBH_StatusTypeDef status = USBH_BUSY;
  USBH_StatusTypeDef cs_status = USBH_BUSY;
  VIDEO_HandleTypeDef *VIDEO_Handle =  (VIDEO_HandleTypeDef *) phost->pActiveClass->pData;  
        
  cs_status = USBH_VIDEO_CSRequest(phost, 
                                   VIDEO_Handle->temp_feature, 
                                   0);
  
  if(cs_status != USBH_BUSY)
  {        
  }
  
  return status;
}

//Used because of USN HOST HAL need it (BgndProcess)
static USBH_StatusTypeDef USBH_VIDEO_ProcessDummy(USBH_HandleTypeDef *phost)
{
  return USBH_OK ;
}

/**
  * @brief  USBH_VIDEO_Process 
  *         The function is for managing state machine for Video data transfers 
            MUST be called frequently!
  * @param  phost: Host handle
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_VIDEO_Process(USBH_HandleTypeDef *phost, stream_status_t a_status)
{   
  USBH_StatusTypeDef status = USBH_BUSY;
  VIDEO_HandleTypeDef *VIDEO_Handle =  (VIDEO_HandleTypeDef *)  phost->pActiveClass->pData;
  
  if(VIDEO_Handle->camera.supported == 1)
  {
    switch(a_status)
    {
      case ss_start: {
        USBH_VIDEO_StartStream(phost);
      } break;
      case ss_input: {
        USBH_VIDEO_InputStream(phost);
      } break;
    }
  }
 
  return status;
}

/**
  * @brief  Handle Input stream process
  * @param  phost: Host handle
  * @retval USBH Status
  */
#if 0
static USBH_StatusTypeDef USBH_VIDEO_InputStream (USBH_HandleTypeDef *phost)
{
  // Called from USB_HOST_fast_class_call
  USBH_StatusTypeDef status = USBH_BUSY;
  VIDEO_HandleTypeDef *VIDEO_Handle =  (VIDEO_HandleTypeDef*) phost->pActiveClass->pData;
  USBH_URBStateTypeDef result;
  
  switch(VIDEO_Handle->steam_in_state)
  {
    case VIDEO_STATE_START_IN:
      USBH_IsocReceiveData(phost,
                            (uint8_t*)tmp_packet_framebuffer,
                            VIDEO_Handle->camera.EpSize,
                            VIDEO_Handle->camera.Pipe);
      VIDEO_Handle->steam_in_state = VIDEO_STATE_DATA_IN;
    break;
    
  
  
    case VIDEO_STATE_DATA_IN:
      
      result = USBH_LL_GetURBState(phost, VIDEO_Handle->camera.Pipe);
      if((result == USBH_URB_DONE) && ((phost->Timer - VIDEO_Handle->camera.timer) >= VIDEO_Handle->camera.Poll))
      {
        VIDEO_Handle->camera.timer = phost->Timer;
        volatile uint32_t rxlen = USBH_LL_GetLastXferSize(phost, VIDEO_Handle->camera.Pipe);//Return the last transfered packet size.
        video_stream_process_packet((uint16_t)rxlen);
          
        USBH_IsocReceiveData(phost,
                            (uint8_t*)tmp_packet_framebuffer,
                            VIDEO_Handle->camera.EpSize,
                            VIDEO_Handle->camera.Pipe);
      }
    break;
    
    default:
      break;
  }
  return status;  
}
#else
static USBH_StatusTypeDef USBH_VIDEO_StartStream (USBH_HandleTypeDef *phost)
{
  VIDEO_HandleTypeDef *VIDEO_Handle =  (VIDEO_HandleTypeDef*) phost->pActiveClass->pData;
  //USBH_URBStateTypeDef result;
  
  if (VIDEO_Handle->steam_in_state == VIDEO_STATE_START_IN) {
    
      USBH_IsocReceiveData(phost, (uint8_t*)tmp_packet_framebuffer,
        VIDEO_Handle->camera.EpSize, VIDEO_Handle->camera.Pipe);
      VIDEO_Handle->steam_in_state = VIDEO_STATE_DATA_IN;
  }
  return USBH_BUSY;
}

static USBH_StatusTypeDef USBH_VIDEO_InputStream (USBH_HandleTypeDef *phost)
{
  VIDEO_HandleTypeDef *VIDEO_Handle =  (VIDEO_HandleTypeDef*) phost->pActiveClass->pData;
  USBH_URBStateTypeDef result;
  
  if (VIDEO_Handle->steam_in_state == VIDEO_STATE_DATA_IN)
  {
      result = USBH_LL_GetURBState(phost, VIDEO_Handle->camera.Pipe);
      if((result == USBH_URB_DONE) /*&& ((phost->Timer - VIDEO_Handle->camera.timer) >= VIDEO_Handle->camera.Poll)*/)
      {
        VIDEO_Handle->camera.timer = phost->Timer;
        volatile uint32_t rxlen = USBH_LL_GetLastXferSize(phost, VIDEO_Handle->camera.Pipe);//Return the last transfered packet size.
        
        video_stream_process_packet((uint16_t)rxlen);
          
        USBH_IsocReceiveData(phost, (uint8_t*)tmp_packet_framebuffer,
          VIDEO_Handle->camera.EpSize, VIDEO_Handle->camera.Pipe);
      }
  }
  return USBH_BUSY;
}

#endif

/**
  * @brief  USBH_VIDEO_SOFProcess 
  *         The function is for managing the SOF callback
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_VIDEO_SOFProcess (USBH_HandleTypeDef *phost)
{    
  return USBH_OK;
}

//*****************************************************************************
//*****************************************************************************

USBH_StatusTypeDef USBH_VS_SetCur(USBH_HandleTypeDef *phost, uint16_t request_type)
{
  uint16_t wLength = 26;
  
  if (request_type == (VS_PROBE_CONTROL << 8))
  {
    memset(&ProbeParams, 0, sizeof(ProbeParams));

    ProbeParams.bmHint = 0x0001;
    ProbeParams.bFormatIndex = USBH_VIDEO_Best_bFormatIndex;
    ProbeParams.bFrameIndex = USBH_VIDEO_Best_bFrameIndex;     
    ProbeParams.dwFrameInterval = 333333;        //in 100ns units
    ProbeParams.wKeyFrameRate = 0;               //unused
    ProbeParams.wPFrameRate = 0;                 //unused
    ProbeParams.wCompQuality = 0;                //unused
    ProbeParams.wCompWindowSize = 0;            //unused
    ProbeParams.wDelay = 0;                      //set by device
    ProbeParams.dwMaxVideoFrameSize = 0;         //set by device
    ProbeParams.dwMaxPayloadTransferSize = 0;    //set by device  
  }
  
  //H2D - host to device
  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE | \
  USB_REQ_TYPE_CLASS;
  
  VIDEO_HandleTypeDef *VIDEO_Handle;
  VIDEO_Handle =  (VIDEO_HandleTypeDef *) phost->pActiveClass->pData;
  
  phost->Control.setup.b.bRequest = UVC_SET_CUR;
  phost->Control.setup.b.wValue.w = request_type;
  phost->Control.setup.b.wIndex.w = VIDEO_Handle->camera.interface; //Video Streaming interface number
  phost->Control.setup.b.wLength.w = wLength;
  
  USBH_StatusTypeDef status;
  
  uint32_t timeout = HAL_GetTick();
  do 
  {
    status = USBH_CtlReq(phost, (uint8_t *)&ProbeParams, wLength);
  } while (status == USBH_BUSY && ((HAL_GetTick() - timeout) < 200));
  
  return USBH_OK;
}

USBH_StatusTypeDef USBH_VS_GetCur(USBH_HandleTypeDef *phost, uint16_t request_type)
{
  memset(&ProbeParams, 0, sizeof(ProbeParams));
  uint16_t wLength = 48;

  //H2D - host to devicenot 
  phost->Control.setup.b.bmRequestType = USB_D2H | USB_REQ_RECIPIENT_INTERFACE |
    USB_REQ_TYPE_CLASS;
  
  VIDEO_HandleTypeDef *VIDEO_Handle;
  VIDEO_Handle =  (VIDEO_HandleTypeDef *) phost->pActiveClass->pData;
  
  phost->Control.setup.b.bRequest = UVC_GET_CUR;
  phost->Control.setup.b.wValue.w = request_type;
  phost->Control.setup.b.wIndex.w = VIDEO_Handle->camera.interface; //Video Streaming interface number
  phost->Control.setup.b.wLength.w = wLength;
  
  USBH_StatusTypeDef status;
  const uint32_t wait_response_ms = 100;
  uint32_t stop_tick = HAL_GetTick() + wait_response_ms;
  do 
  {
    status = USBH_CtlReq(phost, (uint8_t *)&ProbeParams, wLength);
    if (HAL_GetTick() > stop_tick) {
      status = USBH_FAIL;
      break;
    }
  } while (status == USBH_BUSY);
  
  if (status == USBH_OK)
  {
    if (ProbeParams.dwMaxVideoFrameSize > 0)
    {
      return USBH_OK;
    }
    else
      return USBH_FAIL;
  }
  
  return status;
}

VIDEO_ProbeTypedef get_probe_params()
{
  return ProbeParams;
}

