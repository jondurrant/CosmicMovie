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
#include <vector>
#include <string>
#include <iostream>

#include "libraries/pico_graphics/pico_graphics.hpp"
#include "cosmic_unicorn.hpp"

using namespace pimoroni;
PicoGraphics_PenRGB888 graphics(32, 32, nullptr);
CosmicUnicorn unicorn;

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

void listDir(std::vector<std::string> *list, const char * ext = NULL){
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
	  if ( fno.fname[0] != '.' ) {
		  if (ext == NULL){
			  list->push_back(fno.fname);
		  } else {
			  if (strstr(fno.fname, ext) != NULL){
				  list->push_back(fno.fname);
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
		//printf("Read(%u) = %u\n", length, count);
		return 0;
	} else {
		//printf("Read(%u) Failed\n");
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


bool pngFrame(const char * name, uint frame){
	size_t size;
	struct spng_ihdr ihdr;
	int res;
	FIL fi;
	uint32_t out[32];
	struct spng_row_info row_info;
	bool result = true;

	if ( FR_OK != f_open(&fi, name, FA_READ) ){
		printf("Failed to open %s\n", name);
		return false;
	}

	spng_ctx *ctx = spng_ctx_new(0);

	res = spng_set_png_stream(
			ctx,
			spng_read,
			&fi);

	res = spng_get_ihdr(ctx, &ihdr);
	if (ihdr.width > 32){
		printf("Image is too wide %u > 32\n", ihdr.width);
		result = false;
	}
	uint frames = ihdr.height / 32;
	if (frame > frames ){
		printf("Frame %u > %u\n", ihdr.width, frames);
		result = false;
	}
	//printf("(%d)PNG (%u, %u)\n", res, ihdr.width, ihdr.height);

	if (result){
		uint32_t startRow = frame * 32;
		uint32_t endRow = startRow + 31;
		spng_decode_image(ctx, out, size, SPNG_FMT_RGBA8, SPNG_DECODE_PROGRESSIVE);

		uint line = 0;
		while (line < endRow){
			res = spng_decode_row( ctx,  out,  32*4);
			//printf("(%d)Row decode\n", res);
			if (res == SPNG_EOI){
				//printf("EOF Reached\n");
				break;
			}
			if (res != 0){
				printf("Read decode failed\n");
				result = false;
				break;
			}

			res = spng_get_row_info( ctx,  &row_info);
			//printf("(%d)Row Info %d \n", res, row_info.row_num);
			if (res == 0){
				line = row_info.row_num;
				if (  ( line >= startRow) &&
						( line <= endRow)){
					//printf("Copy in row %u for frame %u\n", row_info.row_num, frame);

					for (int32_t x=0; x <  ihdr.width; x++){
						uint8_t r = (out[x] & 0x000000FF) ;
						uint8_t g = (out[x] & 0x0000FF00) >> 8;
						uint8_t b = (out[x] & 0x00FF0000) >> 16;
						graphics.set_pen(r, g, b);
						graphics.pixel({x, line - startRow});
					}

				}
			} else {
				result = false;
				break;
			}
		}
	}

	spng_ctx_free(ctx);
	 f_close(&fi);
	 return result;
}

bool playPng(const char * name, uint8_t fps = 0){
	size_t size;
	struct spng_ihdr ihdr;
	int res;
	FIL fi;
	uint32_t out[32];
	struct spng_row_info row_info;
	bool result = true;

	if ( FR_OK != f_open(&fi, name, FA_READ) ){
		printf("Failed to open %s\n", name);
		return false;
	}

	spng_ctx *ctx = spng_ctx_new(0);

	res = spng_set_png_stream(
			ctx,
			spng_read,
			&fi);

	res = spng_get_ihdr(ctx, &ihdr);
	if (ihdr.width > 32){
		printf("Image is too wide %u > 32\n", ihdr.width);
		result = false;
	}
	uint frames = ihdr.height / 32;
	//printf("(%d)PNG (%u, %u)\n", res, ihdr.width, ihdr.height);

	if (result){
		uint32_t frame = 0;
		spng_decode_image(ctx, out, size, SPNG_FMT_RGBA8, SPNG_DECODE_PROGRESSIVE);

		uint line = 0;
		for(;;){
			res = spng_decode_row( ctx,  out,  32*4);
			//printf("(%d)Row decode\n", res);
			if (res == SPNG_EOI){
				//printf("EOF Reached\n");
				break;
			}
			if (res != 0){
				printf("Read decode failed\n");
				result = false;
				break;
			}

			res = spng_get_row_info( ctx,  &row_info);
			//printf("(%d)Row Info %d \n", res, row_info.row_num);
			if (res == 0){
				uint32_t last = to_ms_since_boot (get_absolute_time());
				uint32_t msPerFrame = 1000 / fps;
				frame = (row_info.row_num -1)/32;
				line = (row_info.row_num  -1)% 32;

				if (line == 0){
					 graphics.set_pen(0, 0, 0);
					 graphics.clear();
				}

				for (int32_t x=0; x <  ihdr.width; x++){
					uint8_t r = (out[x] & 0x000000FF) ;
					uint8_t g = (out[x] & 0x0000FF00) >> 8;
					uint8_t b = (out[x] & 0x00FF0000) >> 16;
					graphics.set_pen(r, g, b);
					graphics.pixel({x, line});
				}

				if (line == 31){
					uint32_t now = to_ms_since_boot (get_absolute_time());

					if ((last + msPerFrame)  > now ){
						uint32_t remain = (last + msPerFrame) - now;
						sleep_ms(remain);
					}
					unicorn.update(&graphics);
					last = now;
				}

			} else {
				result = false;
				break;
			}
		}
	}

	spng_ctx_free(ctx);
	 f_close(&fi);
	 return result;
}




int main(){
	bool once = false;
	std::vector<std::string> list;
	 stdio_init_all();
	 //unicorn.init();
	 sleep_ms(2000);
	 printf("Go\n");

	 tuh_init(BOARD_TUH_RHPORT);

	 disk_io_init();


	 unicorn.init();


	 for (;;){

		 tuh_task();

		 if (tuh_msc_is_mounted()){

			 listDir(&list, ".png");
			 std::vector<std::string>::iterator iter = list.begin();
			 for(iter; iter < list.end(); iter++){
				 uint32_t now = to_ms_since_boot (get_absolute_time());
				 uint32_t end = now + 20000;
				 while (now < end){
					 playPng(iter->c_str(), 4);
					 now = to_ms_since_boot (get_absolute_time());
				 }
			  }
			 list.clear();
		 }

	 }



	 for (;;){
		 sleep_ms(3000);
	 }

}


