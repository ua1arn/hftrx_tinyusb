//

#include "tusb_option.h"

#if (CFG_TUH_ENABLED && CFG_TUH_BTH)

#include "host/usbh.h"
#include "host/usbh_pvt.h"

#include "bth_host.h"

// Level where CFG_TUSB_DEBUG must be at least for this driver is logged
#ifndef CFG_TUH_BTH_LOG_LEVEL
  #define CFG_TUH_BTH_LOG_LEVEL   CFG_TUH_LOG_LEVEL
#endif

#define TU_LOG_DRV(...)   TU_LOG(CFG_TUH_BTH_LOG_LEVEL, __VA_ARGS__)

#define HCI_ACL_PAYLOAD_SIZE (1691 + 4)

 //--------------------------------------------------------------------+
 // Host BTH Interface
 //--------------------------------------------------------------------+
#define CFG_TUH_BTH_TX_BUFSIZE HCI_ACL_PAYLOAD_SIZE
#define CFG_TUH_BTH_TX_EPSIZE 64
#define CFG_TUH_BTH_RX_BUFSIZE HCI_ACL_PAYLOAD_SIZE
#define CFG_TUH_BTH_RX_EPSIZE 64

typedef struct {
	uint8_t daddr;
	uint8_t bInterfaceNumber;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;

	//uint8_t serial_drid; // Serial Driver ID
	//bth_acm_capability_t acm_capability;
	//uint8_t ep_notif;

	//uint8_t line_state;                               // DTR (bit0), RTS (bit1)
//	TU_ATTR_ALIGNED(4) bth_line_coding_t line_coding; // Baudrate, stop bits, parity, data width

	uint8_t itf_num;
	//tuh_xfer_cb_t user_control_cb;
	uint8_t ep_notif;
	uint8_t hci_event[258];
	unsigned hci_event_offset;
	unsigned event_in_len;
	tuh_xfer_t ep_notif_xfer;

	struct {
	 tu_edpt_stream_t acl_in;
	 tu_edpt_stream_t acl_out;

	 uint8_t acl_out_ff_buf[CFG_TUH_BTH_TX_BUFSIZE];
	 CFG_TUH_MEM_ALIGN uint8_t acl_out_ep_buf[CFG_TUH_BTH_TX_EPSIZE];

	 uint8_t acl_in_ff_buf[CFG_TUH_BTH_TX_BUFSIZE];
	 CFG_TUH_MEM_ALIGN uint8_t acl_in_ep_buf[CFG_TUH_BTH_TX_EPSIZE];
	} stream;

} bthh_interface_t;

CFG_TUH_MEM_SECTION
static bthh_interface_t bthh_data[CFG_TUH_BTH];

static bthh_interface_t* make_new_itf(uint8_t daddr, tusb_desc_interface_t const *itf_desc)
{
  for(uint8_t i=0; i<CFG_TUH_BTH; i++)
  {
    if (bthh_data[i].daddr == 0) {
      bthh_interface_t* const p_bth = &bthh_data[i];

      p_bth->daddr              = daddr;
      p_bth->bInterfaceNumber   = itf_desc->bInterfaceNumber;
      p_bth->bInterfaceSubClass = itf_desc->bInterfaceSubClass;
      p_bth->bInterfaceProtocol = itf_desc->bInterfaceProtocol;
     // p_bth->line_state         = 0;
      return p_bth;
    }
  }

  return NULL;
}


static inline bthh_interface_t* get_itf(uint8_t idx) {
  TU_ASSERT(idx < CFG_TUH_BTH, NULL);
  bthh_interface_t* const p_bth = &bthh_data[idx];

  return (p_bth->daddr != 0) ? p_bth : NULL;
}

static inline uint8_t get_idx_by_ep_addr(uint8_t daddr, uint8_t ep_addr) {
  for(uint8_t i=0; i<CFG_TUH_BTH; i++)
  {
    bthh_interface_t* const p_bth = &bthh_data[i];
    if ( (p_bth->daddr == daddr) &&
         (ep_addr == p_bth->ep_notif || ep_addr == p_bth->stream.acl_in.ep_addr || ep_addr == p_bth->stream.acl_out.ep_addr))
    {
      return i;
    }
  }

  return TUSB_INDEX_INVALID_8;
}

uint8_t tuh_bth_itf_get_index(uint8_t daddr, uint8_t itf_num) {
  for(uint8_t i=0; i<CFG_TUH_BTH; i++)
  {
    const bthh_interface_t* const p_bth = &bthh_data[i];

    if (p_bth->daddr == daddr && p_bth->bInterfaceNumber == itf_num) return i;
  }

  return TUSB_INDEX_INVALID_8;
}

static bool bth_send_command(bthh_interface_t* p_bth, const uint8_t * packet, uint16_t len) {

  tusb_control_request_t const request = {
    .bmRequestType_bit = {
      .recipient = TUSB_REQ_RCPT_INTERFACE,
      .type      = TUSB_REQ_TYPE_CLASS,
      .direction = TUSB_DIR_OUT
    },
    .bRequest = 0x00,
    .wValue   = tu_htole16(0),
    .wIndex   = tu_htole16((uint16_t) p_bth->bInterfaceNumber),
    .wLength  = tu_htole16((uint16_t) len)
  };

  //p_bth->user_control_cb = complete_cb;

  tuh_xfer_t xfer = {
    .daddr       = p_bth->daddr,
    .ep_addr     = 0,
    .setup       = &request,
    .buffer      = (uint8_t*) packet,
    .complete_cb = NULL,//complete_cb ? bthh_internal_control_complete : NULL, // complete_cb is NULL for sync call
    .user_data   = 0//user_data
  };

  TU_ASSERT(tuh_control_xfer(&xfer), false);
  return true;
}

bool tuh_bth_send_cmd(uint8_t idx, const uint8_t * packet, uint16_t len)
{
	bthh_interface_t * const p_bth = get_itf(idx);
	TU_VERIFY(p_bth);
	TU_ASSERT(bth_send_command(p_bth, packet, len), false);
	return true;
}

bool bthh_set_config(uint8_t dev_addr, uint8_t itf_num)
{
	uint8_t const idx = tuh_bth_itf_get_index(dev_addr, itf_num);
	//TU_LOG_DRV("bthh_set_config: dev_addr=%u, itf_num=%u, idx=%u\n", dev_addr, itf_num, idx);
	bthh_interface_t * const p_bth = get_itf(idx);
	//TU_LOG_DRV("bthh_set_config: idx=%u\n", idx);
	TU_ASSERT(bth_send_command(p_bth, NULL, 0), false);		// RESET command

	// Prepare for incoming data
	tu_edpt_stream_read_xfer(&p_bth->stream.acl_in);

    p_bth->hci_event_offset = 0;
    tuh_xfer_t * const xfer = &p_bth->ep_notif_xfer;
    {
      xfer->daddr       = dev_addr;
      xfer->ep_addr     = p_bth->ep_notif;
	  xfer->buflen      = TU_MIN(sizeof p_bth->hci_event - p_bth->hci_event_offset, p_bth->event_in_len);
      xfer->buffer      = p_bth->hci_event + p_bth->hci_event_offset;
      xfer->complete_cb = NULL;
      xfer->user_data   = (uintptr_t) p_bth->hci_event; // since buffer is not available in callback; use user data to store the buffer
    };
    // submit transfer for this EP
    tuh_edpt_xfer(xfer);

	// notify usbh that driver enumeration is complete
	usbh_driver_set_config_complete(p_bth->daddr, itf_num);

	if (tuh_bth_mount_cb) tuh_bth_mount_cb(idx);
	return true;
}

bool bthh_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t event, uint32_t xferred_bytes) {
  // TODO handle stall response, retry failed transfer ...
  //TU_LOG_DRV("bthh_xfer_cb: event=%d, dev_addr=%u, ep_addr=0x%02X, xferred_bytes=%u\n", (int) event, (unsigned) dev_addr, (unsigned) ep_addr, (unsigned) xferred_bytes);
  if (event != XFER_RESULT_SUCCESS)
	  return true;
  TU_ASSERT(event == XFER_RESULT_SUCCESS);

  uint8_t const idx = get_idx_by_ep_addr(dev_addr, ep_addr);
  bthh_interface_t * p_bth = get_itf(idx);
  TU_ASSERT(p_bth);

  if ( ep_addr == p_bth->stream.acl_out.ep_addr ) {
	// invoke tx complete callback to possibly refill tx fifo
	if (tuh_bth_tx_complete_cb) tuh_bth_tx_complete_cb(idx);

	if ( 0 == tu_edpt_stream_write_xfer(&p_bth->stream.acl_out) ) {
	  // If there is no data left, a ZLP should be sent if:
	  // - xferred_bytes is multiple of EP Packet size and not zero
	  tu_edpt_stream_write_zlp_if_needed(&p_bth->stream.acl_out, xferred_bytes);
	}
  }
  else if ( ep_addr == p_bth->stream.acl_in.ep_addr ) {
	  tu_edpt_stream_read_xfer_complete(&p_bth->stream.acl_in, xferred_bytes);

	// invoke receive callback
	if (tuh_bth_rx_cb) tuh_bth_rx_cb(idx);

	// prepare for next transfer if needed
	tu_edpt_stream_read_xfer(&p_bth->stream.acl_in);
  }else if ( ep_addr == p_bth->ep_notif ) {
	  // done trough event callback

	  if (xferred_bytes < p_bth->event_in_len) {
		  // last ZLP or last packet
		  p_bth->hci_event_offset += xferred_bytes;
		  if (p_bth->hci_event_offset >= 2 && p_bth->hci_event_offset >= (p_bth->hci_event [1] + 2)) {
			  // Packet format validation pass
			  if (tuh_bth_event_cb) tuh_bth_event_cb(idx, p_bth->hci_event, p_bth->hci_event [1] + 2);
		  }
		  // next packet prepare
		  p_bth->hci_event_offset = 0;
		  p_bth->ep_notif_xfer.buflen = TU_MIN(sizeof p_bth->hci_event - p_bth->hci_event_offset, p_bth->event_in_len);
		  p_bth->ep_notif_xfer.buffer = p_bth->hci_event + p_bth->hci_event_offset;
	  }
	  else if (xferred_bytes == p_bth->event_in_len &&
			  (p_bth->hci_event_offset + p_bth->event_in_len) <= sizeof p_bth->hci_event) {
		  // continue read
		  p_bth->hci_event_offset += xferred_bytes;
		  p_bth->ep_notif_xfer.buflen = TU_MIN(sizeof p_bth->hci_event - p_bth->hci_event_offset, p_bth->event_in_len);
		  p_bth->ep_notif_xfer.buffer = p_bth->hci_event + p_bth->hci_event_offset;
	  }
	  else {
		  // Some kind of errors
	      TU_LOG_DRV("Strange input bthh_xfer_cb addr = %u index = %u: kind of error\r\n", dev_addr, idx);
		  p_bth->hci_event_offset += xferred_bytes;
		  if (p_bth->hci_event_offset >= 2 && p_bth->hci_event_offset >= (p_bth->hci_event [1] + 2)) {
			  // Packet format validation pass
			  if (tuh_bth_event_cb) tuh_bth_event_cb(idx, p_bth->hci_event, p_bth->hci_event [1] + 2);
		  }
		  p_bth->hci_event_offset = 0;
		  p_bth->ep_notif_xfer.buflen = TU_MIN(sizeof p_bth->hci_event - p_bth->hci_event_offset, p_bth->event_in_len);
		  p_bth->ep_notif_xfer.buffer = p_bth->hci_event + p_bth->hci_event_offset;
	  }

	  tuh_edpt_xfer(& p_bth->ep_notif_xfer);

  }else {
	TU_ASSERT(false);
  }

  return true;
}

void bthh_init(void)
{
	tu_memclr(bthh_data, sizeof(bthh_data));

	for (size_t i=0; i<CFG_TUH_BTH; i++)
	{
		bthh_interface_t* const p_bth = &bthh_data[i];

		tu_edpt_stream_init(&p_bth->stream.acl_out, true, true, false,
							  p_bth->stream.acl_out_ff_buf, CFG_TUH_BTH_TX_BUFSIZE,
							  p_bth->stream.acl_out_ep_buf, CFG_TUH_BTH_TX_EPSIZE);

		tu_edpt_stream_init(&p_bth->stream.acl_in, true, false, false,
							  p_bth->stream.acl_in_ff_buf, CFG_TUH_BTH_RX_BUFSIZE,
							  p_bth->stream.acl_in_ep_buf, CFG_TUH_BTH_RX_EPSIZE);
	}

}

bool bthh_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *itf_desc, uint16_t max_len)
{
	  (void) rhport;
	  uint8_t const * p_desc_end = ((uint8_t const*) itf_desc) + max_len;


	  if ( TUSB_CLASS_WIRELESS_CONTROLLER == itf_desc->bInterfaceClass &&
		  TUH_BT_APP_SUBCLASS == itf_desc->bInterfaceSubClass &&
		  TUH_BT_PROTOCOL_PRIMARY_CONTROLLER == itf_desc->bInterfaceProtocol &&
		  0 == itf_desc->bInterfaceNumber &&
		  3 == itf_desc->bNumEndpoints
		  )
	  {
		  bthh_interface_t * p_bth = make_new_itf(dev_addr, itf_desc);
		  TU_VERIFY(p_bth);
		  if (p_bth == NULL)
			  return false;

		  tusb_desc_endpoint_t const* ep_desc = (tusb_desc_endpoint_t const *) tu_desc_next(itf_desc);

		  for (uint32_t i = 0; i < 3; i++) {
			if (TUSB_DESC_ENDPOINT == ep_desc->bDescriptorType && TUSB_XFER_INTERRUPT == ep_desc->bmAttributes.xfer)
			{
				p_bth->ep_notif = ep_desc->bEndpointAddress;
				p_bth->event_in_len = tu_edpt_packet_size(ep_desc);
				TU_ASSERT(tuh_edpt_open(dev_addr, ep_desc));
			}
			else if (TUSB_DIR_IN == tu_edpt_dir(ep_desc->bEndpointAddress)) {
				p_bth->stream.acl_in.ep_addr = ep_desc->bEndpointAddress;
				TU_ASSERT(tuh_edpt_open(dev_addr, ep_desc));
			    tu_edpt_stream_open(&p_bth->stream.acl_in, p_bth->daddr, ep_desc);
			} else {
				p_bth->stream.acl_out.ep_addr = ep_desc->bEndpointAddress;
				TU_ASSERT(tuh_edpt_open(dev_addr, ep_desc));
			    tu_edpt_stream_open(&p_bth->stream.acl_out, p_bth->daddr, ep_desc);
			}

		    ep_desc = (tusb_desc_endpoint_t const*) tu_desc_next(ep_desc);
		  }

		  p_bth->itf_num = itf_desc->bInterfaceNumber;

		  return true;
	  }

	return false;
}

void bthh_close(uint8_t dev_addr)
{
	  for(uint8_t idx=0; idx<CFG_TUH_BTH; idx++)
	  {
	    bthh_interface_t* const p_bth = &bthh_data[idx];
	    if (p_bth->daddr == dev_addr)
	    {
	      TU_LOG_DRV("  BTHh close addr = %u index = %u\r\n", dev_addr, idx);

	      // Invoke application callback
	      ////if (tuh_bth_umount_cb) tuh_bth_umount_cb(idx);

	      tuh_edpt_abort_xfer(dev_addr, p_bth->ep_notif);
	      tu_edpt_stream_close(&p_bth->stream.acl_in);
	      tu_edpt_stream_close(&p_bth->stream.acl_out);

	      //tu_memclr(p_bth, sizeof(bthh_interface_t));
	      p_bth->daddr = 0;
	      p_bth->bInterfaceNumber = 0;
	      p_bth->ep_notif = 0;
	    }
	  }

}

//--------------------------------------------------------------------+
// Write
//--------------------------------------------------------------------+

uint32_t tuh_bth_send_acl(uint8_t idx, void const* buffer, uint32_t bufsize)
{
  bthh_interface_t* p_bth = get_itf(idx);
  TU_VERIFY(p_bth);

  return tu_edpt_stream_write(&p_bth->stream.acl_out, buffer, bufsize);
}

uint32_t tuh_bth_write_flush(uint8_t idx)
{
  bthh_interface_t* p_bth = get_itf(idx);
  TU_VERIFY(p_bth);

  return tu_edpt_stream_write_xfer(&p_bth->stream.acl_out);
}

bool tuh_bth_write_clear(uint8_t idx)
{
  bthh_interface_t* p_bth = get_itf(idx);
  TU_VERIFY(p_bth);

  return tu_edpt_stream_clear(&p_bth->stream.acl_out);
}

uint32_t tuh_bth_write_available(uint8_t idx)
{
  bthh_interface_t* p_bth = get_itf(idx);
  TU_VERIFY(p_bth);

  return tu_edpt_stream_write_available(&p_bth->stream.acl_out);
}

//--------------------------------------------------------------------+
// Read
//--------------------------------------------------------------------+

uint32_t tuh_bth_read (uint8_t idx, void* buffer, uint32_t bufsize)
{
  bthh_interface_t* p_bth = get_itf(idx);
  TU_VERIFY(p_bth);

  return tu_edpt_stream_read(&p_bth->stream.acl_in, buffer, bufsize);
}

uint32_t tuh_bth_read_available(uint8_t idx)
{
  bthh_interface_t* p_bth = get_itf(idx);
  TU_VERIFY(p_bth);

  return tu_edpt_stream_read_available(&p_bth->stream.acl_in);
}

bool tuh_bth_peek(uint8_t idx, uint8_t* ch)
{
  bthh_interface_t* p_bth = get_itf(idx);
  TU_VERIFY(p_bth);

  return tu_edpt_stream_peek(&p_bth->stream.acl_in, ch);
}

bool tuh_bth_read_clear (uint8_t idx)
{
  bthh_interface_t* p_bth = get_itf(idx);
  TU_VERIFY(p_bth);

  bool ret = tu_edpt_stream_clear(&p_bth->stream.acl_in);
  tu_edpt_stream_read_xfer(&p_bth->stream.acl_in);
  return ret;
}

#endif