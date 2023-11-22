/*
 * bth_host.h
 *
 *  Created on: Nov 19, 2023
 *      Author: Gena
 */

#ifndef LIB_TINYUSB_SRC_CLASS_BTH_BTH_HOST_H_
#define LIB_TINYUSB_SRC_CLASS_BTH_BTH_HOST_H_


#ifdef __cplusplus
 extern "C" {
#endif


#define TUH_BT_APP_CLASS                    (TUSB_CLASS_WIRELESS_CONTROLLER)
#define TUH_BT_APP_SUBCLASS                 0x01
#define TUH_BT_PROTOCOL_PRIMARY_CONTROLLER  0x01
#define TUH_BT_PROTOCOL_AMP_CONTROLLER      0x02

//--------------------------------------------------------------------+
// Internal Class Driver API
//--------------------------------------------------------------------+
void bthh_init       (void);
bool bthh_open       (uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *itf_desc, uint16_t max_len);
bool bthh_set_config (uint8_t dev_addr, uint8_t itf_num);
bool bthh_xfer_cb    (uint8_t dev_addr, uint8_t ep_addr, xfer_result_t event, uint32_t xferred_bytes);
void bthh_close      (uint8_t dev_addr);


//--------------------------------------------------------------------+
// Write API
//--------------------------------------------------------------------+

// Write to bth interface
// Read from bth interface
uint32_t tuh_bth_read (uint8_t idx, void* buffer, uint32_t bufsize);
bool tuh_bth_send_acl(uint8_t idx, const uint8_t* packet, uint16_t len);
bool tuh_bth_send_cmd(uint8_t idx, const uint8_t * packet, uint16_t len);
bool tuh_bth_can_send_now(uint8_t idx);
//--------------------------------------------------------------------+
// BTH APPLICATION CALLBACKS
//--------------------------------------------------------------------+

// Invoked when a device with BTH interface is mounted
// idx is index of bth interface in the internal pool.
TU_ATTR_WEAK extern void tuh_bth_mount_cb(uint8_t idx);

// Invoked when a device with BTH interface is unmounted
TU_ATTR_WEAK extern void tuh_bth_umount_cb(uint8_t idx);

// Invoked when received new data
TU_ATTR_WEAK extern void tuh_bth_rx_acl_cb(uint8_t idx);

// Invoked when a TX is complete and therefore space becomes available in TX buffer
TU_ATTR_WEAK extern void tuh_bth_send_acl_cb(uint8_t idx);
TU_ATTR_WEAK extern void tuh_bth_send_cmd_cb(uint8_t idx);

// Invoked when received notificaion
TU_ATTR_WEAK extern void tuh_bth_event_cb(uint8_t idx, uint8_t * buffer, uint16_t size);


#ifdef __cplusplus
 }
#endif

#endif /* LIB_TINYUSB_SRC_CLASS_BTH_BTH_HOST_H_ */
