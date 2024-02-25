/*
 * tuhMount.c
 *
 *  Created on: 25 Feb 2024
 *      Author: jondurrant
 */

#include "tusb.h"
#include "ff.h"

#include "diskImp.h"

static scsi_inquiry_resp_t inquiry_resp;
static FATFS fatfs[CFG_TUH_DEVICE_MAX]; // for simplicity only support 1 LUN per device


bool _mounted = false;


bool tuh_msc_is_mounted(){
	return _mounted;
}


//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

void tuh_mount_cb(uint8_t dev_addr)
{
  (void) dev_addr;
  printf("Mount CB\n");
}

void tuh_umount_cb(uint8_t dev_addr)
{
  (void) dev_addr;
  printf("Unmount CB\n");
  _mounted = false;
}


bool inquiry_complete_cb(uint8_t dev_addr, tuh_msc_complete_data_t const * cb_data)
{
  msc_cbw_t const* cbw = cb_data->cbw;
  msc_csw_t const* csw = cb_data->csw;

  if (csw->status != 0)
  {
    printf("Inquiry failed\r\n");
    return false;
  }

  // Print out Vendor ID, Product ID and Rev
  printf("%.8s %.16s rev %.4s\r\n", inquiry_resp.vendor_id, inquiry_resp.product_id, inquiry_resp.product_rev);

  // Get capacity of device
  uint32_t const block_count = tuh_msc_get_block_count(dev_addr, cbw->lun);
  uint32_t const block_size = tuh_msc_get_block_size(dev_addr, cbw->lun);

  printf("Disk Size: %lu MB\r\n", block_count / ((1024*1024)/block_size));
  // printf("Block Count = %lu, Block Size: %lu\r\n", block_count, block_size);

  // For simplicity: we only mount 1 LUN per device
  uint8_t const drive_num = dev_addr-1;
  char drive_path[3] = "0:";
  drive_path[0] += drive_num;

  if ( f_mount(&fatfs[drive_num], drive_path, 1) != FR_OK )
  {
    puts("mount failed");
  }

  // change to newly mounted drive
  f_chdir(drive_path);

  _mounted = true;


  return true;
}

//------------- IMPLEMENTATION -------------//
void tuh_msc_mount_cb(uint8_t dev_addr)
{
  printf("A MassStorage device is mounted\r\n");

  uint8_t const lun = 0;
  tuh_msc_inquiry(dev_addr, lun, &inquiry_resp, inquiry_complete_cb, 0);
}

void tuh_msc_umount_cb(uint8_t dev_addr)
{
  printf("A MassStorage device is unmounted\r\n");

  uint8_t const drive_num = dev_addr-1;
  char drive_path[3] = "0:";
  drive_path[0] += drive_num;

  f_unmount(drive_path);

}
