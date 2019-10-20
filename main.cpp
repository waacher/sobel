#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <cmath>
#include <iostream>
#include <stdio.h>

using namespace cv;
using namespace std;

Mat to442_grayscale(Mat frame)
{
	CV_Assert(frame.type() == CV_8UC3);

	int channels = frame.channels();
	int nRows = frame.rows;
	int nCols = frame.cols * channels;

	for(int i = 0; i < nRows; i++)
	{
		uchar *row = frame.ptr(i);
		for(int j = 0; j < nCols; j+=3)
		{
			uchar pixelB = (int)row[j]*.0722;
			uchar pixelG = (int)row[j+1]*.7152;
			uchar pixelR = (int)row[j+2] *.2126;
			int  grey = pixelB + pixelG + pixelR;
	
			if(grey > 255)
			{
				grey = 255;
			}

			row[j] = grey;
			row[j+1] = grey;
			row[j+2] = grey;
		}
	}
	return frame;
}	

Mat to442_sobel(Mat frame)
{
    CV_Assert(frame.type() == CV_8UC3);
	Mat dst = frame.clone();	//destination mat that will have black border
	CV_Assert(dst.type() == CV_8UC3);
	int top, bottom, left, right;
	top = 1 ; 	//amount we are adding to our frame
	bottom = top;
	left = 1 ;
	right = left;
	copyMakeBorder(frame, dst, top, bottom, left, right, 1, 0);
	//copy make border, frame is source, dst is destination
	//top, botoom,left and right = 1
	//this will add one row to top and bottom and
	//one column to left and right of our frame
	Mat new_dest = dst.clone();
	int channels = dst.channels();
	int  nRows = dst.rows;
	int nCols = dst.cols * channels;

	for(int i = 1; i < nRows-1; i++)
	{
		uchar *row_dest = new_dest.ptr(i);
		for(int j = 3; j < nCols-3; j+=3)
		{
			
			uchar pixelUL = dst.ptr(i-1)[j-3];
			uchar pixelUC = dst.ptr(i-1)[j];
			uchar pixelUR = dst.ptr(i-1)[j+3];
			uchar pixelCL = dst.ptr(i)[j-3];
			
			uchar pixelCR = dst.ptr(i)[j+3];
			uchar pixelBL = dst.ptr(i+1)[j-3];
			uchar pixelBC = dst.ptr(i+1)[j];
			uchar pixelBR = dst.ptr(i+1)[j+3];
			
			int gxUL = pixelUL * -1;
			int gxUR = pixelUR;
			int gxCL = pixelCL * -2;
			int gxCR = pixelCR * 2;
			int gxBL = pixelBL * -1;
			int gxBR = pixelBR;
			
			int gyUL = pixelUL;
			int gyUC = pixelUC * 2;
			int gyUR = pixelUR;
			int gyBL = pixelBL * -1;
			int gyBC = pixelBC * -2;
			int gyBR = pixelBR * -1;
			
			int gx_scal = abs(gxUL + gxUR + gxCL + gxCR + gxBL + gxBR);
			int gy_scal = abs(gyUL + gyUC + gyUR + gyBL + gyBC + gyBR);
			
			int g = gx_scal + gy_scal;
			if(g > 255)
			{
				g = 255;
			}
			row_dest[j] = g;
			row_dest[j+1] =g;
			row_dest[j+2] = g;	
		}
	}
	return new_dest;
}

int main(int argc, char *argv[])
{
	VideoCapture cap(argv[1]);
	Mat frame;
	
	while(true)
	{
		cap.read(frame);
		if(frame.empty())
		{
			break;
		}
		CV_Assert(frame.type() == CV_8UC3);
		
		Mat frame2 = to442_grayscale(frame);
		Mat frame3 = frame;
		frame3 = to442_sobel(frame3);
		
        imshow("hello_andy", frame3);
		waitKey(100);
	}
	cap.release();
	destroyAllWindows();
	return 0;
}