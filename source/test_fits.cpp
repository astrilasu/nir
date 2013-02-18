
#include <iostream>
#include <cstdio>

#include <fitsio.h>

void printerror(int status)
{
	if(status){
		fits_report_error(stderr, status);
		exit(status);
	}
	return;
}
 
int main()
{

	char filename[] = "myfits.fit";
	fitsfile *fptr = NULL;
	long fpixel =1;
	int bitpix = USHORT_IMG;
	long naxis = 2;
	long naxes[2] = {1024, 1024};
	unsigned short pixel[100*100];
	int status;
	int exposure = 10;

	

	status = 0;
	


	printf ("Creating file ..\n");
	if(fits_create_file(&fptr, filename, &status))	{
		printf ("Unable to create file ..\n");
		printerror(status);
	}
	
	
	if (fits_create_img(fptr, bitpix, naxis, naxes, &status)) {
		printf ("Unable to create image ..\n");
		//printerror(status);
	}

	if(fits_write_img(fptr, TUSHORT, fpixel, 1024*1024, pixel, &status)) {
		printf ("Unable to write image ..\n");
		//printerror(status);
	}

	

	if(fits_update_key(fptr, TLONG, "EXPOSURE", &exposure, "Total time", &status)) {
		printf ("Unable to update key ..\n");
		//printerror(status);
	}

	if(fits_close_file(fptr, &status)) {
		printf ("Unable to close file ..\n");
		//printerror(status);	
	}
	

	return 0;
}
