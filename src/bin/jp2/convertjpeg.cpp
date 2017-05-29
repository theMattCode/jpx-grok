/*
*    Copyright (C) 2016-2017 Grok Image Compression Inc.
*
*    This source code is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This source code is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/


#include "opj_apps_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


extern "C" {
#include "openjpeg.h"
#include "convert.h"
#include "color.h"
}


#ifndef GROK_HAVE_LIBJPEG
# error GROK_HAVE_LIBJPEG_NOT_DEFINED
#endif /* GROK_HAVE_LIBPNG */


#include "jpeglib.h"
#include <setjmp.h>
#include <cassert>


/*
* ERROR HANDLING:
*
* The JPEG library's standard error handler (jerror.c) is divided into
* several "methods" which you can override individually.  This lets you
* adjust the behavior without duplicating a lot of code, which you might
* have to update with each future release.
*
* Our example here shows how to override the "error_exit" method so that
* control is returned to the library's caller when a fatal error occurs,
* rather than calling exit() as the standard error_exit method does.
*
* We use C's setjmp/longjmp facility to return control.  This means that the
* routine which calls the JPEG library must first execute a setjmp() call to
* establish the return point.  We want the replacement error_exit to do a
* longjmp().  But we need to make the setjmp buffer accessible to the
* error_exit routine.  To do this, we make a private extension of the
* standard JPEG error handler object.  (If we were using C++, we'd say we
* were making a subclass of the regular error handler.)
*
* Here's the extended error handler struct:
*/

struct my_error_mgr {
	struct jpeg_error_mgr pub;    /* "public" fields */

	jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct my_error_mgr *my_error_ptr;

/*
* Here's the routine that will replace the standard error_exit method:
*/

METHODDEF(void)
my_error_exit(j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr)cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}


/*
* SOME FINE POINTS:
*
* In the code below, we ignored the return value of jpeg_read_scanlines,
* which is the number of scanlines actually read.  We could get away with
* this because we asked for only one line at a time and we weren't using
* a suspending data source.  See libjpeg.txt for more info.
*
* We cheated a bit by calling alloc_sarray() after jpeg_start_decompress();
* we should have done it beforehand to ensure that the space would be
* counted against the JPEG max_memory setting.  In some systems the above
* code would risk an out-of-memory error.  However, in general we don't
* know the output image dimensions before jpeg_start_decompress(), unless we
* call jpeg_calc_output_dimensions().  See libjpeg.txt for more about this.
*
* Scanlines are returned in the same order as they appear in the JPEG file,
* which is standardly top-to-bottom.  If you must emit data bottom-to-top,
* you can use one of the virtual arrays provided by the JPEG memory manager
* to invert the data.  See wrbmp.c for an example.
*
* As with compression, some operating modes may require temporary files.
* On some systems you may need to set up a signal handler to ensure that
* temporary files are deleted if the program is interrupted.  See libjpeg.txt.
*/

opj_image_t* jpegtoimage(const char *filename, opj_cparameters_t *parameters)
{
	opj_image_t *image = NULL;
	bool success = true;
	int32_t* planes[3];
	JDIMENSION w=0, h=0;
	int bps = 0, numcomps = 0;
	convert_XXx32s_C1R cvtJpegTo32s;
	OPJ_COLOR_SPACE color_space = OPJ_CLRSPC_UNKNOWN;
	opj_image_cmptparm_t cmptparm[3]; /* mono or RGB */
	convert_32s_CXPX cvtCxToPx;
	int32_t* buffer32s = nullptr;

	/* This struct contains the JPEG decompression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	*/
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	struct my_error_mgr jerr;
	/* More stuff */
	FILE *infile;                 /* source file */
	JSAMPARRAY buffer;            /* Output row buffer */
	int row_stride;               /* physical row width in output buffer */

								  /* In this example we want to open the input file before doing anything else,
								  * so that the setjmp() error recovery below can assume the file is open.
								  * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
								  * requires it in order to read binary files.
								  */

	if ((infile = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		return 0;
	}

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		* We need to clean up the JPEG object, close the input file, and return.
		*/
		jpeg_destroy_decompress(&cinfo);
		success = false;
		goto cleanup;
	}
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */
	jpeg_stdio_src(&cinfo, infile);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void)jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	*   (a) suspension is not possible with the stdio data source, and
	*   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	* See libjpeg.txt for more info.
	*/

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	* jpeg_read_header(), so we do nothing here.
	*/

	/* Step 5: Start decompressor */

	(void)jpeg_start_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	bps = cinfo.data_precision;
	numcomps = cinfo.output_components;
	w = cinfo.image_width;
	h = cinfo.image_height;
	cvtJpegTo32s = convert_XXu32s_C1R_LUT[bps];
	memset(&cmptparm[0], 0, 3 * sizeof(opj_image_cmptparm_t));
	cvtCxToPx = convert_32s_CXPX_LUT[numcomps];

	if (cinfo.output_components==3)
		color_space = OPJ_CLRSPC_SRGB;
	else
		 color_space = OPJ_CLRSPC_GRAY;

	for (int j = 0; j < cinfo.output_components; j++) {
		cmptparm[j].prec = bps;
		cmptparm[j].dx = 1;
		cmptparm[j].dy = 1;
		cmptparm[j].w = w;
		cmptparm[j].h = h;
	}
	
	image = opj_image_create(numcomps, &cmptparm[0], color_space);
	if (!image) {
		success = false;
		goto cleanup;
	}
	/* set image offset and reference grid */
	image->x0 = parameters->image_offset_x0;
	image->x1 = !image->x0 ? (w - 1) * 1 + 1 :
		image->x0 + (w - 1) * 1 + 1;
	if (image->x1 <= image->x0) {
		fprintf(stderr, "jpegtoimage: Bad value for image->x1(%d) vs. "
			"image->x0(%d)\n\tAborting.\n", image->x1, image->x0);
		success = false;
		goto cleanup;
	}

	image->y0 = parameters->image_offset_y0;
	image->y1 = !image->y0 ? (h - 1) * 1 + 1 :
		image->y0 + (h - 1) * 1 + 1;

	if (image->y1 <= image->y0) {
		fprintf(stderr, "jpegtoimage: Bad value for image->y1(%d) vs. "
			"image->y0(%d)\n\tAborting.\n", image->y1, image->y0);
		success = false;
		goto cleanup;
	}


	for (int j = 0; j < numcomps; j++) {
		planes[j] = image->comps[j].data;
	}

	buffer32s = new int32_t[w * numcomps];

	/* We may need to do some setup of our own at this point before reading
	* the data.  After jpeg_start_decompress() we have the correct scaled
	* output image dimensions available, as well as the output colormap
	* if we asked for color quantization.
	* In this example, we need to make an output work buffer of the right size.
	*/
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
	if (!buffer) {
		success = false;
		goto cleanup;
	}

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	*/
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		* Here the array is only one element long, but you could ask for
		* more than one scanline at a time if that's more convenient.
		*/
		(void)jpeg_read_scanlines(&cinfo, buffer, 1);

		cvtJpegTo32s(buffer[0], buffer32s, (size_t)w * numcomps, false);
		cvtCxToPx(buffer32s, planes, (size_t)w);

		planes[0] += w;
		planes[1] += w;
		planes[2] += w;
	}

	/* Step 7: Finish decompression */

	(void)jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

cleanup:

	/* After finish_decompress, we can close the input file.
	* Here we postpone it until after no more JPEG errors are possible,
	* so as to simplify the setjmp error logic above.  (Actually, I don't
	* think that jpeg_destroy can do an error exit, but why assume anything...)
	*/
	if (infile)
		fclose(infile);
	
	if (buffer32s)
		delete[] buffer32s;

	/* At this point you may want to check to see whether any corrupt-data
	* warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	*/
	assert(jerr.pub.num_warnings == 0);

	if (success) {
		return image;
	}
	if (image)
		opj_image_destroy(image);
	return NULL;
}/* jpegtoimage() */









