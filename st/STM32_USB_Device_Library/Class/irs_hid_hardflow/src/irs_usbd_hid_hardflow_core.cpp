/**
  ******************************************************************************
  * @file    usbd_hid_core.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   This file provides the HID core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                HID Class  Description
  *          ===================================================================
  *           This module manages the HID class V1.11 following the "Device Class Definition
  *           for Human Interface Devices (HID) Version 1.11 Jun 27, 2001".
  *           This driver implements the following aspects of the specification:
  *             - The Boot Interface Subclass
  *             - The Mouse protocol
  *             - Usage Page : Generic Desktop
  *             - Collection : Application
  *
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *
  *
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include <irserror.h>
#include <timer.h>
#include <irsstm32f2xx_usb_hid_hardflow.h>

extern "C" {

#include "irs_usbd_hid_hardflow_core.h"
#include "usbd_desc.h"
#include "usbd_req.h"

}

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_HID
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_HID_Private_FunctionPrototypes
  * @{
  */


namespace {

static uint8_t  USBD_HID_Init (void  *pdev,
                               uint8_t cfgidx);

static uint8_t  USBD_HID_DeInit (void  *pdev,
                                 uint8_t cfgidx);

static uint8_t  USBD_HID_Setup (void  *pdev,
                                USB_SETUP_REQ *req);

static uint8_t  *USBD_HID_GetCfgDesc (uint8_t speed, uint16_t *length);

static uint8_t  USBD_HID_DataIn (void  *pdev, uint8_t epnum);

static uint8_t  USBD_HID_DataOut (void  *pdev, uint8_t epnum);

} // unnamed namespace

/**
  * @}
  */

/** @defgroup USBD_HID_Private_Variables
  * @{
  */

USBD_Class_cb_TypeDef  usbd_hid_hardflow_cb =
{
  USBD_HID_Init,
  USBD_HID_DeInit,
  USBD_HID_Setup,
  NULL, /*EP0_TxSent*/
  NULL, /*EP0_RxReady*/
  USBD_HID_DataIn, /*DataIn*/
  USBD_HID_DataOut, /*DataOut*/
  NULL, /*SOF */
  NULL,
  NULL,
  USBD_HID_GetCfgDesc,
#ifdef USB_OTG_HS_CORE
  USBD_HID_GetCfgDesc, /* use same config as per FS */
#endif
};

/**
  * @brief  USBD_HID_SendReport
  *         Send HID Report
  * @param  pdev: device instance
  * @param  buff: pointer to report
  * @retval status
  */

uint8_t usbd_hid_hardflow_send_report(USB_OTG_CORE_HANDLE  *pdev,
  uint8_t *report, uint16_t len)
{
  if (pdev->dev.device_status == USB_OTG_CONFIGURED) {
    DCD_EP_Tx (pdev, HID_IN_EP, report, len);
    return USBD_OK;
  }
  return USBD_FAIL;
}

namespace {

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN static uint32_t  USBD_HID_AltSet  __ALIGN_END = 0;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN static uint32_t  USBD_HID_Protocol  __ALIGN_END = 0;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN static uint32_t  USBD_HID_IdleState __ALIGN_END = 0;


#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN uint8_t USB_HID_Rx_Buffer   [CDC_DATA_MAX_PACKET_SIZE] __ALIGN_END ;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
/* USB HID device Configuration Descriptor */
#ifndef NOP
__ALIGN_BEGIN static uint8_t USBD_HID_CfgDesc[USB_HID_CONFIG_DESC_SIZ] __ALIGN_END =
{
  0x09, /* bLength: Configuration Descriptor size */
  USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType: Configuration */
  USB_HID_CONFIG_DESC_SIZ,
  /* wTotalLength: Bytes returned */
  0x00,
  0x01, /*bNumInterfaces: 1 interface*/
  0x01, /*bConfigurationValue: Configuration value*/
  0x00, /*iConfiguration: Index of string descriptor describing
    the configuration*/
  // 0xC0,         /*bmAttributes: bus powered */
  0xE0,         /*bmAttributes: bus powered and Support Remote Wake-up */
  0x32,         /*MaxPower 100 mA: this current is used for detecting Vbus*/

  /************** Descriptor of Custom HID interface ****************/
  /* 09 */
  0x09,         /* bLength: Interface Descriptor size */
  USB_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: Interface descriptor type */
  0x00,         /* bInterfaceNumber: Number of Interface */
  0x00,         /* bAlternateSetting: Alternate setting */
  0x02,         /* bNumEndpoints */
  0x03,         /* bInterfaceClass: HID */
  0x00,         /* bInterfaceSubClass : 1=BOOT, 0=no boot */
  0x00,         /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
  0,            /* iInterface: Index of string descriptor */
  /******************** Descriptor of Custom HID HID ********************/
  /* 18 */
  0x09,         /* bLength: HID Descriptor size */
  HID_DESCRIPTOR_TYPE, /* bDescriptorType: HID */
  0x10, //0x10        /* bcdHID: HID Class Spec release number ?? 0x11 ??*/
  0x01,
  0x00,         /* bCountryCode: Hardware target country */
  0x01,         /* bNumDescriptors: Number of HID class descriptors to follow */
  0x22,         /* bDescriptorType */
  CUSTOM_HID_SIZ_REPORT_DESC,/* wItemLength: Total length of Report descriptor */
  0x00,
  /******************** Descriptor of Custom HID endpoints ******************/
  /* 27 */
  0x07,          /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType: */

  HID_IN_EP,     /* bEndpointAddress: Endpoint Address (IN) */
  0x03,          /* bmAttributes: Interrupt endpoint */
  HID_IN_PACKET, /* wMaxPacketSize */
  0x00,
  0x01,//0x64,//0x0A,          /* bInterval: Polling Interval (10 ms) */
  /* 34 */

  0x07,  	/* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,	/* bDescriptorType: */

  HID_OUT_EP,	/* bEndpointAddress: Endpoint Address (OUT) */
  0x03,			/* bmAttributes: Interrupt endpoint */
  HID_OUT_PACKET,	/* wMaxPacketSize */
  0x00,
  0x01,//0x64,//0x0A,			/* bInterval: Polling Interval (10 ms) */
  /* 41 */
}
; /* CustomHID_ConfigDescriptor */
#endif // N0P


#define MANUFACTER_NAME_SIZ 8

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
/*__ALIGN_BEGIN static uint8_t ManufacterName[MANUFACTER_NAME_SIZ] __ALIGN_END = {
  'R',
  0,
  'E',
  0,
  'S',
  0
};*/

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

__ALIGN_BEGIN static uint8_t CustomHID_ReportDescriptor[CUSTOM_HID_SIZ_REPORT_DESC_3] __ALIGN_END = { // USB report descriptor         // ?????????? ????????? ????????? ?????? ?????? ??? ??????
    0x06, 0xff, 0x00,                       /* USAGE_PAGE (Vendor Page: 0xFF00) */

    0x09, 0x01,                             // USAGE (Vendor Usage 1)
    0xa1, 0x01,                             // COLLECTION (Application)    	
    0x15, 0x00,                             //    LOGICAL_MINIMUM (0)        // min.
    0x26, 0xff, 0x00,                       //    LOGICAL_MAXIMUM (255)      // max.
    0x75, 0x08,                             //    REPORT_SIZE (8)
    0x95, 0x40,                             //    REPORT_COUNT
    0x09, 0x00,                             //    USAGE (Undefined)
    0x81, 0x02,                             //    INPUT (Data,Var,Abs,Buf)
    0x09, 0x00,                             //    USAGE (Undefined)
    0x91, 0x82,                             //    OUTPUT (Data,Var,Abs,Vol)
    0xc0                                    // END_COLLECTION
};

/**
  * @}
  */

/** @defgroup USBD_HID_Private_Functions
  * @{
  */

/**
  * @brief  USBD_HID_Init
  *         Initialize the HID interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_HID_Init (void  *pdev,
                               uint8_t cfgidx)
{

  /* Open EP IN */
  DCD_EP_Open(static_cast<USB_OTG_CORE_HANDLE*>(pdev),
              HID_IN_EP,
              HID_IN_PACKET,
              USB_OTG_EP_INT);

  /* Open EP OUT */
  DCD_EP_Open(static_cast<USB_OTG_CORE_HANDLE*>(pdev),
              HID_OUT_EP,
              HID_OUT_PACKET,
              USB_OTG_EP_INT);
  /* Prepare Out endpoint to receive next packet */
  DCD_EP_PrepareRx(static_cast<USB_OTG_CORE_HANDLE*>(pdev),
                   HID_OUT_EP,
                   (uint8_t*)(USB_HID_Rx_Buffer),
                   HID_OUT_PACKET);

  return USBD_OK;
}

/**
  * @brief  USBD_HID_Init
  *         DeInitialize the HID layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_HID_DeInit (void  *pdev,
                                 uint8_t cfgidx)
{
  /* Close HID EPs */
  DCD_EP_Close (static_cast<USB_OTG_CORE_HANDLE*>(pdev), HID_IN_EP);
  DCD_EP_Close (static_cast<USB_OTG_CORE_HANDLE*>(pdev), HID_OUT_EP);
  return USBD_OK;
}

/**
  * @brief  USBD_HID_Setup
  *         Handle the HID specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_HID_Setup (void  *pdev,
                                USB_SETUP_REQ *req)
{
  uint16_t len = 0;
  uint8_t  *pbuf = NULL;
  USB_OTG_CORE_HANDLE* pdev_handle = static_cast<USB_OTG_CORE_HANDLE*>(pdev);
  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    switch (req->bRequest) {
      case HID_REQ_SET_PROTOCOL:
        USBD_HID_Protocol = (uint8_t)(req->wValue);
        break;
      case HID_REQ_GET_PROTOCOL:
        USBD_CtlSendData (pdev_handle, (uint8_t *)&USBD_HID_Protocol, 1);
        break;
      case HID_REQ_SET_IDLE:
        USBD_HID_IdleState = (uint8_t)(req->wValue >> 8);
        break;
      case HID_REQ_GET_IDLE:
        USBD_CtlSendData (pdev_handle, (uint8_t *)&USBD_HID_IdleState, 1);
        break;
      default:
        USBD_CtlError (pdev_handle, req);
        return USBD_FAIL;
    }
    break;
  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest) {
    case USB_REQ_GET_DESCRIPTOR:
      //uint16_t descriptor_type = req->wValue >> 8;
      if( req->wValue >> 8 == HID_REPORT_DESC) {
        len = MIN(CUSTOM_HID_SIZ_REPORT_DESC , req->wLength);
        pbuf = CustomHID_ReportDescriptor;
      } else if( req->wValue >> 8 == HID_DESCRIPTOR_TYPE) {
        #ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
        //pbuf = USBD_HID_Desc;
        #else
        pbuf = USBD_HID_CfgDesc + 0x12;
        #endif
        len = MIN(USB_HID_DESC_SIZ , req->wLength);
      }
      USBD_CtlSendData (pdev_handle, pbuf, len);
      break;
    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev_handle, (uint8_t *)&USBD_HID_AltSet,  1);
      break;
    case USB_REQ_SET_INTERFACE :
      USBD_HID_AltSet = (uint8_t)(req->wValue);
      break;
    }
  }
  return USBD_OK;
}

/**
  * @brief  USBD_HID_GetCfgDesc
  *         return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_HID_GetCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = sizeof (USBD_HID_CfgDesc);
  return USBD_HID_CfgDesc;
}

/**
  * @brief  USBD_HID_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_HID_DataIn (void  *pdev,
                              uint8_t epnum)
{

  /* Ensure that the FIFO is empty before a new transfer, this condition could
  be caused by  a new transfer before the end of the previous transfer */
  DCD_EP_Flush(static_cast<USB_OTG_CORE_HANDLE*>(pdev), HID_IN_EP);
  irs::hardflow::arm::usb_hid_t::tx_buffer_is_empty_event();
  return USBD_OK;
}

/**
  * @brief  USBD_HID_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */

int data_out_read_all_count_test = 0;
int data_out_read_count_test = 0;

static uint8_t  USBD_HID_DataOut (void *pdev, uint8_t epnum)
{
  if (epnum == HID_OUT_EP) {
    /* Get the received data buffer and update the counter */
    uint16_t size = ((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[epnum].xfer_count;
    //bool prepare_rx = false;
    /* USB data will be immediately processed, this allow next USB traffic being
     NAKed till the end of the application Xfer */
    if (((USB_OTG_CORE_HANDLE*)pdev)->dev.device_status == USB_OTG_CONFIGURED) {
      if (size == CDC_DATA_MAX_PACKET_SIZE) {
        irs::hardflow::arm::usb_hid_t::rx_buffer_is_empty_event(
          static_cast<USB_OTG_CORE_HANDLE*>(pdev),
          USB_HID_Rx_Buffer, size);
      } else {
        DCD_EP_PrepareRx(static_cast<USB_OTG_CORE_HANDLE*>(pdev),
          HID_OUT_EP,
          (uint8_t*)(USB_HID_Rx_Buffer),
          HID_OUT_PACKET);
      }
    }
  }
  return USBD_OK;
}

} // unnamed namespace
/**
  * @}
  */


/**
  * @}
  */


/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
