#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <png.h>
#include <dirent.h>
#include <scan_dir.h>

#include "stdint.h"
#include "binarization.h"

#define PNG_BYTES_TO_CHECK 4
#define SOURCE_IMAGES_PATH "..\\images\\"
#define OUTPUT_IMAGES_PATH "..\\images\\output\\"

#define println(x)   \
	printf("\r\n "); \
	printf(x);       \
	printf("\r\n")

int write_data(const char *file_path, uint_fast16_t width, uint_fast16_t height, unsigned char *image, const char *title)
{
	int_fast8_t err_code = 0;
	FILE *file = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytep row = NULL;

	file = fopen(file_path, "wb");
	if (file == NULL)
	{
		perror("Could not open file for writing \r\n");
		goto out;
	}

	/* Initialize write structure */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		perror("Could not allocate write struct \r\n");
		goto out;
	}

	/* Initialize info structure */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		perror("Could not allocate info struct \r\n");
		goto out;
	}

	/* Setup Exception handling */
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		perror("Error during png creation \r\n");
		goto out;
	}

	png_init_io(png_ptr, file);

	png_set_IHDR(png_ptr, info_ptr, width, height,
				 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	if (title != NULL)
	{
		png_text title_text;
		title_text.compression = PNG_TEXT_COMPRESSION_NONE;
		title_text.key = (png_charp)"Title";
		title_text.text = (png_charp)title;
		png_set_text(png_ptr, info_ptr, &title_text, 1);
	}

	png_write_info(png_ptr, info_ptr);

	row = (png_bytep)malloc(width * sizeof(png_byte));

	register uint_fast16_t x, y;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			row[x] = image[y * width + x];
		}
		png_write_row(png_ptr, row);
	}

	png_write_end(png_ptr, NULL);
	err_code = 1;

out:
	if (file != NULL)
		fclose(file);
	if (info_ptr != NULL)
		png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL)
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	if (row != NULL)
		free(row);

	return err_code;
}

int get_data(const char *name, uint_fast16_t *width, uint_fast16_t *height, unsigned char **raw)
{
	int err_code = 0;
	png_structp png = NULL;
	png_bytep row_pointer = NULL;
	png_infop info = NULL;	
	FILE *file = fopen(name, "rb");
	
	if (!file)
	{
		perror("Could not open a file \r\n");
		goto out;
	}

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
	{
		perror("Could not create png structure \r\n");
		goto out;
	}

	if (setjmp(png_jmpbuf(png)))
	{
		perror("Could not create png buffer \r\n");
		goto out;
	}

	info = png_create_info_struct(png);
	if (!info)
	{
		perror("Could not read png info \r\n");
		goto out;
	}

	png_init_io(png, file);
	png_read_info(png, info);

	/* Configure for 8bpp gray-scale input */
	png_uint_16 trns = 0;
	const png_byte color = png_get_color_type(png, info);
	const png_byte bits = png_get_bit_depth(png, info);
	const png_byte interlace_type = png_get_interlace_type(png, info);

	if (color & PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);
	if (color == PNG_COLOR_TYPE_GRAY && bits < 8)
		png_set_expand_gray_1_2_4_to_8(png);
	if ((trns = png_get_valid(png, info, PNG_INFO_tRNS)))
		png_set_tRNS_to_alpha(png);
	if (bits == 16)
		png_set_strip_16(png);
	if (trns || color & PNG_COLOR_MASK_ALPHA)
		png_set_strip_alpha(png);
	if (color & PNG_COLOR_MASK_COLOR)
		png_set_rgb_to_gray_fixed(png, 1, -1, -1);

	uint_fast8_t number_passes = 1;

	if (interlace_type != PNG_INTERLACE_NONE)
		number_passes = png_set_interlace_handling(png);

	png_read_update_info(png, info);

	/* Allocate image */
	*width = (uint_fast16_t)png_get_image_width(png, info);
	*height = (uint_fast16_t)png_get_image_height(png, info);

	*raw = calloc(*width * *height, sizeof(png_uint_16));

	if (!(raw && width > 0 && height > 0))
		goto out;

	/* Fill buffer */
	row_pointer = calloc(*width, sizeof(png_uint_16));

	for (uint_fast8_t pass = 0; pass < number_passes; pass++)
	{
		for (uint_fast16_t y = 0; y < *height; y++)
		{
			row_pointer = *raw + y * *width;
			png_read_rows(png, &row_pointer, NULL, 1);
		}
	}

	err_code = 1;

out:
	/* Clean-up */
	if (png)
	{
		png_read_end(png, NULL);

		if (info)
			png_destroy_read_struct((png_structpp)&png, &info, (png_infopp)NULL);
		else
			png_destroy_read_struct((png_structpp)&png, (png_infopp)NULL, (png_infopp)NULL);
	}
	
	if (file)
		fclose(file);

	return err_code;
}

int check_if_png(const char *file_path)
{
	FILE *file;
	int err_code = 0;

	unsigned char buf[PNG_BYTES_TO_CHECK];

	if (!(file = fopen(file_path, "rb")))
	{
		perror("Could not open file \r\n");
		goto out;
	}

	if (fread(buf, 1, PNG_BYTES_TO_CHECK, file) != PNG_BYTES_TO_CHECK)
	{
		perror("Could not cread png header \r\n");
		goto out;
	}

	if (!png_sig_cmp(buf, (png_size_t)0, PNG_BYTES_TO_CHECK))
		err_code = 1;

out:
	/* Clean-up */
	if (file)
		fclose(file);

	return err_code;
}

const char *open_and_choose_file(const char *dir_path, char *filename)
{
	println("Choose one of of exiting file in directory:");
	scan_dir((char *)dir_path);

	printf("\r\n Select a file: ");

	uint_fast8_t selected_file_index;
	scanf("%hd", &selected_file_index);

	if (selected_file_index == 0)
		system("@cls||clear");

	const char *file_path = concat(dir_path, current_file[selected_file_index]);
	strcpy(filename, current_file[selected_file_index]);

	return file_path;
}

int main(void)
{
	int exit_code = -1;

	uint_fast16_t width = 0;
	uint_fast16_t height = 0;

	unsigned char *raw = NULL;
	unsigned char *out = NULL;

	char *filename = (char *)malloc(0x10);
	const char *file_source = open_and_choose_file(SOURCE_IMAGES_PATH, filename);
	const char *file_path_dst = concat(OUTPUT_IMAGES_PATH, filename);

	if (!(filename && file_source && file_path_dst))
	{
		perror("Path cannot be created \r\n");
		goto out;
	}

	if (!check_if_png(file_source))
	{
		perror("This file is not valid \r\n");
		goto out;
	}

	if (!get_data(file_source, &width, &height, &raw))
	{
		perror("PNG read failed \r\n");
		goto out;
	}

	out = (unsigned char *)calloc(width * height, sizeof(unsigned char *));

	threshold(raw, out, width, height);

	if (!write_data(file_path_dst, width, height, out, "output"))
	{
		perror("Couldn't save image \r\n");
		goto out;
	}

	exit_code = 0;

out:
	/* Clean-up */
	if (filename)
		free(filename);
	if (raw)
		free(raw);
	if (out)
		free(out);

	if (!exit_code)
	{
		println("Success");
	}
	else
	{
		println("Unsuccess");
	}

	return exit_code;
	exit(exit_code);
}
