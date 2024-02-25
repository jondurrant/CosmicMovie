/***
 * Cosmic Movie
 *
 * Jon Durrant
 * 25-Feb-2024
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

extern "C" {
#include "tusb.h"
#include "ff.h"

}

#include "diskImp.h"
#include "tuhMount.h"
#include <spng.h>


void listRoot(){
  const char* dpath = "/";

  DIR dir;
  if ( FR_OK != f_opendir(&dir, dpath) )
  {
    printf("cannot access '%s': No such file or directory\r\n", dpath);
    return;
  }

  FILINFO fno;
  while( (f_readdir(&dir, &fno) == FR_OK) && (fno.fname[0] != 0) )
  {
    if ( fno.fname[0] != '.' ) // ignore . and .. entry
    {
      if ( fno.fattrib & AM_DIR )
      {
        // directory
        printf("/%s\r\n", fno.fname);
      }else
      {
        printf("%-40s", fno.fname);
        if (fno.fsize < 1024)
        {
          printf("%lu B\r\n", fno.fsize);
        }else
        {
          printf("%lu KB\r\n", fno.fsize / 1024);
        }
      }
    }
  }

  f_closedir(&dir);
}

int spng_read(spng_ctx *ctx, void *user, void *dst_src, size_t length){
	FIL *pfi = (FIL *) user;
	UINT count = 0;

	if (FR_OK == f_read(pfi, dst_src , length, &count)){
		printf("Read(%u) = %u\n", length, count);
		return 0;
	} else {
		printf("Read(%u) Failed\n");
		return SPNG_IO_ERROR;
	}
}


void checkPng(const char * name){
	size_t size;
	struct spng_ihdr ihdr;
	int res;
	FIL fi;
	uint32_t *out = NULL;
	struct spng_row_info row_info;

	if ( FR_OK != f_open(&fi, name, FA_READ) ){
		printf("Failed to open %s\n", name);
		return;
	}

	spng_ctx *ctx = spng_ctx_new(0);

	res = spng_set_png_stream(
			ctx,
			spng_read,
			&fi);

	res = spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &size);
	printf("(%d)PNG Out Buf size is %d\n", res, size);
	out = (uint32_t *)malloc(size);
	memset(out, 0, size);

	res = spng_get_ihdr(ctx, &ihdr);
	printf("(%d)PNG (%u, %u)\n", res, ihdr.width, ihdr.height);

	spng_decode_image(ctx, out, size, SPNG_FMT_RGBA8, SPNG_DECODE_PROGRESSIVE);
	printf("Decoded progressive\n");

	uint line = 0;
	while (line < 10){
		res = spng_decode_row( ctx,  out,  32*4);
		printf("(%d)Row decode\n", res);
		for (int i = (32*32); i > 0; i--){
			if (out[i] != 0){
				printf("Last Non Zero %d\n", i);
				break;
			}
		}

		res = spng_get_row_info( ctx,  &row_info);
		printf("(%d)Row Info %d \n", res, row_info.row_num);
		if (res == 0){
			line = row_info.row_num;
		} else {
			break;
		}
	}


	spng_ctx_free(ctx);
	 f_close(&fi);
	 free(out);

}



int png32x32Frames(const char * name){
	size_t size;
	struct spng_ihdr ihdr;
	int res;
	FIL fi;
	uint32_t *out = NULL;
	struct spng_row_info row_info;

	if ( FR_OK != f_open(&fi, name, FA_READ) ){
		printf("Failed to open %s\n", name);
		return -1;
	}

	spng_ctx *ctx = spng_ctx_new(0);

	res = spng_set_png_stream(
			ctx,
			spng_read,
			&fi);

	res = spng_get_ihdr(ctx, &ihdr);

	if (res == 0){
		if (ihdr.width != 32){
			res = -1;
		} else {
			res = ihdr.height/32;
		}
	}

	spng_ctx_free(ctx);
	f_close(&fi);

	return res;
}




int main(){
	bool once = false;
	 stdio_init_all();
	 sleep_ms(2000);
	 printf("Go\n");

	 tuh_init(BOARD_TUH_RHPORT);

	 disk_io_init();


	 for (;;){

		 tuh_task();

		 if (tuh_msc_is_mounted()){

			 if  (!once){
				 listRoot();
				 //checkPng("/youtube_logo_icon_168737.png");
				 //checkPng("/instagram_logo_icon_181738.png");
				 //checkPng("/earth.png");

				 printf("Earth %d\n", png32x32Frames("/earth.png"));
				 printf("YouTube %d\n", png32x32Frames("/youtube_logo_icon_168737.png"));
				 printf("Moon %d\n", png32x32Frames("/moon.png"));

				 sleep_ms(3000);
				 once = true;
			 }
		 }

	 }



	 for (;;){
		 sleep_ms(3000);
	 }

}


