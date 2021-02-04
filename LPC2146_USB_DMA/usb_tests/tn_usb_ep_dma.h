#ifndef  _TN_USB_EP_DMA_H_
#define  _TN_USB_EP_DMA_H_



typedef struct _EP_DMA_ADDR
{
   int word_1;           //-- EP DMA configuration - as word 1 in DD
   int nbuffers;         //-- total for EP(for tx EP - include no_packet buf)
   unsigned int pDD;     //-- Start addr in USB RAM for DD group of this
   unsigned int p_data_buffers;  //-- Start addr in USB RAM for data buffers
   unsigned int p_iso_packets;   //-- For isochronous exch only
}EP_DMA_ADDR;

typedef struct _EP_DMA_INFO
{
   EP_DMA_ADDR * ep_dma_addr;
   unsigned int * dd_ptr;
   unsigned int * pUDCA_pos;
   TN_DQUE * queue;
   TN_FMP * mem_pool;
   int ep_num_phys;
}EP_DMA_INFO;

//---------------

void tn_usb_EP0_tx_int_func(USB_DEVICE_INFO * udi); //-- IN
void tn_usb_EP0_rx_int_func(USB_DEVICE_INFO * udi); //-- OUT
void ep_rx_end_of_transfer(EP_DMA_INFO * edi);
void ep_rx_DD_request(EP_DMA_INFO * edi);
void ep_tx_int_DD_request(EP_DMA_INFO * edi);
void tn_usb_dma_tx_init(EP_DMA_INFO * edi);
void tn_usb_dma_rx_init(EP_DMA_INFO * edi);

#endif
