#include "diskImp.h"


#include <ctype.h>
#include "tusb.h"
#include "ff.h"
#include "diskIO.h"

//------------- Elm Chan FatFS -------------//
static volatile bool _disk_busy[CFG_TUH_DEVICE_MAX];

void disk_io_init(){
	for(size_t i=0; i<CFG_TUH_DEVICE_MAX; i++) _disk_busy[i] = false;
}


//--------------------------------------------------------------------+
// DiskIO
//--------------------------------------------------------------------+

static void wait_for_disk_io(BYTE pdrv)
{
  while(_disk_busy[pdrv])
  {
    tuh_task();
  }
}

static bool disk_io_complete(uint8_t dev_addr, tuh_msc_complete_data_t const * cb_data)
{
  (void) dev_addr; (void) cb_data;
  _disk_busy[dev_addr-1] = false;
  return true;
}

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
  uint8_t dev_addr = pdrv + 1;
  return tuh_msc_mounted(dev_addr) ? 0 : STA_NODISK;
}

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
  (void) pdrv;
	return 0; // nothing to do
}

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	uint8_t const dev_addr = pdrv + 1;
	uint8_t const lun = 0;

	_disk_busy[pdrv] = true;
	tuh_msc_read10(dev_addr, lun, buff, sector, (uint16_t) count, disk_io_complete, 0);
	wait_for_disk_io(pdrv);

	return RES_OK;
}

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	uint8_t const dev_addr = pdrv + 1;
	uint8_t const lun = 0;

	_disk_busy[pdrv] = true;
	tuh_msc_write10(dev_addr, lun, buff, sector, (uint16_t) count, disk_io_complete, 0);
	wait_for_disk_io(pdrv);

	return RES_OK;
}

#endif

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
  uint8_t const dev_addr = pdrv + 1;
  uint8_t const lun = 0;
  switch ( cmd )
  {
    case CTRL_SYNC:
      // nothing to do since we do blocking
      return RES_OK;

    case GET_SECTOR_COUNT:
      *((DWORD*) buff) = (WORD) tuh_msc_get_block_count(dev_addr, lun);
      return RES_OK;

    case GET_SECTOR_SIZE:
      *((WORD*) buff) = (WORD) tuh_msc_get_block_size(dev_addr, lun);
      return RES_OK;

    case GET_BLOCK_SIZE:
      *((DWORD*) buff) = 1;    // erase block size in units of sector size
      return RES_OK;

    default:
      return RES_PARERR;
  }

	return RES_OK;
}
