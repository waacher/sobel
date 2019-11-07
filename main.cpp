#include <pthread.h>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <cmath>
#include <iostream>
#include <stdio.h>
#include <time.h>


using namespace cv;
using namespace std;

Mat *top_grey;
Mat *top_sobel;

void * to442_grayscale(Mat * frame)
{
    Mat *ptr = frame;
    //CV_Assert(frame.type() == CV_8UC3);

    int channels = frame->channels();
    int nRows = frame->rows;
    int nCols = frame->cols * channels;

    int i, j;
    #pragma omp simd collapse(2)
	for( i = 1; i < nRows-1; i++)
    {
        //uchar *row = frame->ptr(i);
        for( j = 3; j < nCols-3; j+=3)
        {
            uchar pixelB = frame->ptr(i)[j] * 0.0722;
            uchar pixelG = frame->ptr(i)[j+1]*.7152;
            uchar pixelR = frame->ptr(i)[j+2] *.2126;
            int  grey = pixelB + pixelG + pixelR;

            /* No if statements in pragma
            if(grey > 255)
            {
                grey = 255;
            } */

            frame->ptr(i)[j] = grey;
            frame->ptr(i)[j+1] = grey;
            frame->ptr(i)[j+2] = grey;
        }
    }
    return (void *)ptr;
}

void * to442_sobel(Mat *frame)
{
    Mat cpy = frame->clone();
    Mat *ptr = frame;
    CV_Assert(frame->type() == CV_8UC3);
    int channels = frame->channels();
    int nRows = frame->rows;
    int nCols = frame -> cols * channels;

    int i, j;
    #pragma omp simd  collapse(2)
    for( i = 1; i < nRows-1; i++)
    {
        for( j = 3; j < nCols-3; j+=3)
        {
	    uchar pixelUL = cpy.ptr(i-1)[j-3];
	    uchar pixelUC = cpy.ptr(i-1)[j];
     	    uchar pixelUR = cpy.ptr(i-1)[j+3];
	    uchar pixelCL = cpy.ptr(i)[j-3];
            uchar pixelCR = cpy.ptr(i)[j+3];
	    uchar pixelBL = cpy.ptr(i+1)[j-3];
    	    uchar pixelBC = cpy.ptr(i+1)[j];
	    uchar pixelBR = cpy.ptr(i+1)[j+3];

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
	    frame->ptr(i)[j] = g;
	    frame->ptr(i)[j+1] =g;
	    frame->ptr(i)[j+2] = g;
        }
    }
    return (void *) ptr;
}

Mat combine(Mat original, Mat frame_top, Mat frame_bot)
{
    int channels = original.channels();
    int nRows = original.rows;
    int nCols = original.cols * channels;

    for(int i = 0; i < nRows/2+1; i++)
    {
	uchar *row = original.ptr(i);
        for(int j = 0; j < nCols; j++)
        {
            row[j] = frame_top.ptr(i)[j];
        }
    }
    int x = 2;
    int y = 0;
    for(int i = nRows/2-1; i < nRows; i++)
    {
        uchar *row = original.ptr(i);
        for(int j = 0; j < nCols; j++)
        {
            row[j] = frame_bot.ptr(x)[y++];
        }
        y = 0;
        x++;
    }
    return original;
}

void * thread_func(void * arg)
{
    Mat *frame = (Mat *) arg;
    top_grey = (Mat *)to442_grayscale(frame);
    top_sobel = (Mat *)to442_sobel(top_grey);
    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    VideoCapture cap("video_forcv.mp4");
    Mat frame;
    clock_t t;
    Mat *frame_pointer_bot_grey;
    frame_pointer_bot_grey = &frame;
    Mat dst;
    Mat *frame_pointer_bot_sobel;
    int top, bottom, left, right;
    top = 1 ; 	//amount we add to our frame
    bottom = top;
    left = 1 ;
    right = left;
    pthread_t t1;
    int num_frames = 0;

    t = clock();
    while(true)
    {
	cap.read(frame);

        CV_Assert(frame.type() == CV_8UC3);
        num_frames++;
	if(frame.empty())
	{
	     break;
	}
        // copy make border, frame is source, dst is destination
        // top, botoom,left and right = 1
        // this will add one row to top and bottom and
        // one column to left and right of our frame
	copyMakeBorder(frame, dst, top, bottom, left, right, 0, 0);
        Mat cpy = dst.clone();
        Mat croppedFrameTop = dst(Rect(0, 0, dst.cols, dst.rows/2+1));
        Mat croppedFrameBot = dst(Rect(0, dst.rows/2-1 ,dst.cols, dst.rows/2+1));

        // Grayscale and Sobel on bottom
        pthread_create(&t1, NULL, thread_func, &croppedFrameTop);

        // Grayscale and Sobel on top
        frame_pointer_bot_grey =(Mat*)to442_grayscale(&croppedFrameBot);
	frame_pointer_bot_sobel  = (Mat*)to442_sobel(frame_pointer_bot_grey);

        pthread_join(t1, NULL);

        Mat combined_frame = combine(cpy, *top_sobel, *frame_pointer_bot_sobel);
        imshow("top_half", combined_frame);
        waitKey(50);
    }
    t = clock() -t ;
    printf("frames per second: %f\n",num_frames/( ((float)t) / CLOCKS_PER_SEC));
    cap.release();
    destroyAllWindows();
    return 0;
}
