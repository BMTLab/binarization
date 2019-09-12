#include "binarization.h"

#define CONTOUR_BLACK (unsigned char) 0
#define CONTOUR_WHITE (unsigned char) 0xFF

void threshold(
	unsigned char* src, 
	unsigned char* out,
	const uint_fast16_t width,
	const uint_fast16_t height)
{
	#if USE_STD_DEV
	const uint_fast8_t  log_w_ext = 1;
	const uint_fast8_t  std_dev_max = 0xF0;
	#endif
	const uint_fast16_t width_w = width >> 2;
	const uint_fast16_t width_w_half = width_w >> 1;
	float k = 0;

	register uint_fast16_t i = 0, j = 0;

	uint_fast8_t  min_image = 0;
	uint_fast16_t index = 0;
	uint_fast16_t count_w = 0;
	uint_fast16_t count_white = 0;
	#if USE_STD_DEV
	uint_fast16_t count_w_ext = 0;
	#endif

	int_fast16_t  x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	#if USE_STD_DEV
	int_fast16_t  x1_ext = 0, x2_ext = 0, y1_ext = 0, y2_ext = 0;
	#endif

	unsigned long* integral_image = NULL;
	unsigned long  sum_image = 0;
	unsigned long  sum_w = 0;
	#if USE_STD_DEV
	unsigned long  sum_w_ext = 0;
	#endif
	unsigned long  threshold = 0;

	#if USE_STD_DEV
	double dev = 0;
	double std_dev = 0;
	#endif
	double average_w = 0;
	#if USE_STD_DEV
	double average_w_ext = 0;
	#endif

	/* Memory allocation for integral */
	integral_image = calloc(width * height, sizeof(unsigned long*));
	if (!integral_image) 
		goto out;

	/* The calculation of the integral sum and min of the image */
	count_white = 0;
	min_image = src[0];

	for (i = 0; i < width; i++)
	{
		sum_image = 0;
		for (j = 0; j < height; j++)
		{
			index = j * width + i;
			sum_image += src[index];

			integral_image[index] = i == 0
				? sum_image
				: integral_image[index - 1] + sum_image;

			if (index != 0 && src[index] < min_image)
				min_image = src[index];

			/*
			if (src[index] <= (unsigned char) BLACK_BRIGHT)
				count_white++;
			*/
		}
	}

	k = 0.3; //(float) 100 * (count_white) / (width * height);

	/* Loop for each pixel */
	for (i = 0; i < width; i++)
	{
		for (j = 0; j < height; j++)
		{
			index = j * width + i;

			/* Finding the coordinates of the corners of the window */
			x1 = i - width_w_half;
			x2 = i + width_w_half;
			y1 = j - width_w_half;
			y2 = j + width_w_half;

			if (x1 < 0)
				x1 = 0;
			if (x2 >= width)
				x2 = width - 1;
			if (y1 < 0)
				y1 = 0;
			if (y2 >= height)
				y2 = height - 1;

			/* Finding the coordinates of the corners of the slightly larger window */
			#if USE_STD_DEV
			x1_ext = x1 - (width_w >> log_w_ext);
			x2_ext = x2 + (width_w >> log_w_ext);
			y1_ext = y1 - (width_w >> log_w_ext);
			y2_ext = y2 + (width_w >> log_w_ext);

			if (x1_ext < 0)
				x1_ext = 0;
			if (x2_ext >= width)
				x2_ext = width - 1;
			if (y1_ext < 0)
				y1_ext = 0;
			if (y2_ext >= height)
				y2_ext = height - 1;
			#endif

			/* Calculating the sum of pixel values for window and extended window */
			sum_w =
				integral_image[y2 * width + x2] - 
				integral_image[y1 * width + x2] -
				integral_image[y2 * width + x1] + 
				integral_image[y1 * width + x1];

			#if USE_STD_DEV
			sum_w_ext =
				integral_image[y2_ext * width + x2_ext] - 
				integral_image[y1_ext * width + x2_ext] -
				integral_image[y2_ext * width + x1_ext] + 
				integral_image[y1_ext * width + x1_ext];
			#endif

			/* Calculating the count of pixel for window and extended window */
			count_w = (y2 - y1) * (x2 - x1);

			#if USE_STD_DEV
			count_w_ext = (y2_ext - y1_ext) * (x2_ext - x1_ext);
			#endif

			/* Calculating the mean of pixel values for window and extended window */
			average_w = sum_w / count_w;			

			#if USE_STD_DEV
			average_w_ext = sum_w_ext / count_w_ext;

			/* Calculation of standard deviation */
			dev = 0;
			for (i = x1_ext; i < x2_ext; i++)
			{
				for (j = y1_ext; j < y2_ext; j++)
				{
					dev += sqr(src[j * width + i] - average_w_ext);
				}
			}
			std_dev = sqrt(dev / count_w_ext);
			#endif

			/* Threshold Calculation */
			threshold =
				(1 - k) * average_w	+ 
				k * min_image
			#if USE_STD_DEV
			+ k * (std_dev / std_dev_max) * (average_w - min_image)
			#endif
			;

			/* Segmentation */
			out[index] = (unsigned long) src[index] <= (unsigned long) threshold
				? CONTOUR_BLACK
				: CONTOUR_WHITE;
		}
	}

out:
	/* Clean-up */
	if (integral_image) 
		free(integral_image);
}