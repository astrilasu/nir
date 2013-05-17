
#include <iostream>
#include <cstdio>
using namespace std;

#include <fitsio.h>

void printerror(int status)
{
	if(status){
		fits_report_error(stderr, status);
		exit(status);
	}
	return;
}

int main (int argc, char* argv[])
{
  fitsfile* fptr;
  int status = 0, nkeys;
  char card[1024];

  int ret = fits_open_file (&fptr, argv[1], READONLY, &status);
  cout << ret << endl;
  fits_get_hdrspace (fptr, &nkeys, NULL, &status);

  for (int i = 1; i <= nkeys; i++) {
    fits_read_record (fptr, i, card, &status);
    printf ("%s\n", card);
  }

  fits_close_file (fptr, &status);

  return 0;
}
 
//int main()
//{
//
//	char filename[] = "myfits.fit";
//	fitsfile *fptr = NULL;
//	long fpixel =1;
//	int bitpix = USHORT_IMG;
//	long naxis = 2;
//	long naxes[2] = {1024, 1024};
//	unsigned short pixel[100*100];
//	int status;
//	int exposure = 10;
//
//	
//
//	status = 0;
//	
//
//
//	printf ("Creating file ..\n");
//	if(fits_create_file(&fptr, filename, &status))	{
//		printf ("Unable to create file ..\n");
//		printerror(status);
//	}
//	
//	
//	if (fits_create_img(fptr, bitpix, naxis, naxes, &status)) {
//		printf ("Unable to create image ..\n");
//		//printerror(status);
//	}
//
//	if(fits_write_img(fptr, TUSHORT, fpixel, 1024*1024, pixel, &status)) {
//		printf ("Unable to write image ..\n");
//		//printerror(status);
//	}
//
//	
//
//	if(fits_update_key(fptr, TLONG, "EXPOSURE", &exposure, "Total time", &status)) {
//		printf ("Unable to update key ..\n");
//		//printerror(status);
//	}
//
//	if(fits_close_file(fptr, &status)) {
//		printf ("Unable to close file ..\n");
//		//printerror(status);	
//	}
//	
//
//	return 0;
//}
