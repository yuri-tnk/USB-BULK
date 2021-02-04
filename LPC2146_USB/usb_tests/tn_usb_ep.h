#ifndef _TN_USB_EP_H_
#define _TN_USB_EP_H_

typedef struct _EP_INFO
{
   TN_DQUE * queue;
   TN_FMP * mem_pool;
   int ep_num_phys;
}EP_INFO;


void tn_usb_ep_rx_int(EP_INFO * ei); //-- OUT
void tn_usb_ep_tx_int(EP_INFO * ei); //-- OUT


#define tn_usb_ep_rx_enable(ep_num_phys)  \
        tn_usb_lpc_cmd(CMD_EP_SELECT |(ep_num_phys)); \
        tn_usb_lpc_cmd(CMD_EP_CLEAR_BUFFER);


#endif
