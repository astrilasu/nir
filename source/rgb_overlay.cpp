#include <iostream>
using namespace std;

#include <cv.h>
#include <highgui.h>
using namespace cv;

int main (int argc, char *argv[])
{
  if (argc < 5) {
    cout << "Enter arguments \n 1)Red file\n 2) Green file\n "
             "3) Blue file\n 4) Luminance file\n 5) Output file\n\n";
    return -1;
  }

  string redfile = argv[1];
  string greenfile = argv[2];
  string bluefile = argv[3];
  string lumfile = argv[4];
  string outfile = argv[5];

  Mat red;
  red = imread (redfile.c_str ());
  Mat red_g (red.rows, red.cols, CV_8U);
  cvtColor (red, red_g, CV_BGR2GRAY);

  Mat green;
  green = imread (greenfile.c_str ());
  Mat green_g (green.rows, green.cols, CV_8U);
  cvtColor (green, green_g, CV_BGR2GRAY);

  Mat blue;
  blue = imread (bluefile.c_str ());
  Mat blue_g (blue.rows, blue.cols, CV_8U);
  cvtColor (blue, blue_g, CV_BGR2GRAY);

  Mat lum;
  lum = imread (lumfile.c_str ());
  Mat lum_g (lum.rows, lum.cols, CV_8U);
  cvtColor (lum, lum_g, CV_BGR2GRAY);

  Mat rgb_im (red.rows, red.cols, CV_8UC3);


  for (int i = 0; i <red.rows; i++) {
    for (int j = 0; j <red.cols; j++) {
      uchar r = red_g.at<uchar> (i, j);
      uchar g = green_g.at<uchar> (i, j);
      uchar b = blue_g.at<uchar> (i, j);
      uchar l = lum_g.at<uchar> (i, j);

      uchar R = ceil ((float (r) / (r+g+b)) * l);
      uchar G = ceil ((float (g) / (r+g+b)) * l);
      uchar B = ceil ((float (b) / (r+g+b)) * l);

      rgb_im.at<Vec3b> (i, j) = Vec3b (B, G, R);
    }
  }

  cout << "Saving file " << outfile << endl;
  imwrite (outfile.c_str (), rgb_im);

  return 0;
}
