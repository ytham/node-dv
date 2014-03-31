#ifndef MATRIX_H
#define MATRIX_H

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include <node_version.h>
#include <node_buffer.h>
//#include <cv.h>
//#include <highgui.h>
#include <string.h>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace v8;
using namespace node;

namespace binding {

class Matrix : public node::ObjectWrap 
{
  public:

  	cv::Mat mat;
    static Persistent<FunctionTemplate> constructor;
    static void Init(Handle<Object> target);
    static Handle<Value> New(const Arguments &args);
    Matrix();
    Matrix(cv::Mat other, cv::Rect roi);
    Matrix(int rows, int cols);
    Matrix(int rows, int cols, int typ);

    static bool HasInstance(v8::Handle<v8::Value> val);
/*
    static double DblGet(cv::Mat mat, int i, int j);

    static v8::Handle<v8::Value> Eye(const v8::Arguments& args); // factory

    static v8::Handle<v8::Value> Get(const v8::Arguments& args); // at
    static v8::Handle<v8::Value> Set(const v8::Arguments& args);

    static v8::Handle<v8::Value> Row(const v8::Arguments& args);
    static v8::Handle<v8::Value> PixelRow(const v8::Arguments& args);
    static v8::Handle<v8::Value> Col(const v8::Arguments& args);
    static v8::Handle<v8::Value> PixelCol(const v8::Arguments& args);

    static v8::Handle<v8::Value> Size(const v8::Arguments& args);
    */
    static v8::Handle<v8::Value> Width(const v8::Arguments& args);
    static v8::Handle<v8::Value> Height(const v8::Arguments& args);
    /*
    static v8::Handle<v8::Value> Channels(const v8::Arguments& args);
    static v8::Handle<v8::Value> Clone(const v8::Arguments& args);
    static v8::Handle<v8::Value> Ellipse(const v8::Arguments& args);
    static v8::Handle<v8::Value> Rectangle(const v8::Arguments& args);
    static v8::Handle<v8::Value> Line(const v8::Arguments& args);
    static v8::Handle<v8::Value> Empty(const v8::Arguments& args);

    static v8::Handle<v8::Value> Save(const v8::Arguments& args);
    static v8::Handle<v8::Value> SaveAsync(const v8::Arguments& args);

    static v8::Handle<v8::Value> ToBuffer(const v8::Arguments& args);
    static v8::Handle<v8::Value> ToBufferAsync(const v8::Arguments& args);

    static v8::Handle<v8::Value> Resize(const v8::Arguments& args);
    static v8::Handle<v8::Value> Rotate(const v8::Arguments& args);
    static v8::Handle<v8::Value> PyrDown(const v8::Arguments& args);
    static v8::Handle<v8::Value> PyrUp(const v8::Arguments& args);

    static v8::Handle<v8::Value> ConvertGrayscale(const v8::Arguments& args);
    static v8::Handle<v8::Value> ConvertHSVscale(const v8::Arguments& args);
    static v8::Handle<v8::Value> GaussianBlur(const v8::Arguments& args);
    static v8::Handle<v8::Value> Copy(const v8::Arguments& args);
    static v8::Handle<v8::Value> Flip(const v8::Arguments& args);
    static v8::Handle<v8::Value> ROI(const v8::Arguments& args);
    static v8::Handle<v8::Value> Ptr(const v8::Arguments& args);
    static v8::Handle<v8::Value> AbsDiff(const v8::Arguments& args);
    static v8::Handle<v8::Value> AddWeighted(const v8::Arguments& args);
    static v8::Handle<v8::Value> BitwiseXor(const v8::Arguments& args);
    static v8::Handle<v8::Value> CountNonZero(const v8::Arguments& args);
    //static v8::Handle<v8::Value> Split(const v8::Arguments& args);
    static v8::Handle<v8::Value> Canny(const v8::Arguments& args);
    static v8::Handle<v8::Value> Dilate(const v8::Arguments& args);
    static v8::Handle<v8::Value> Erode(const v8::Arguments& args);

    static v8::Handle<v8::Value> FindContours(const v8::Arguments& args);
    static v8::Handle<v8::Value> DrawContour(const v8::Arguments& args);
    static v8::Handle<v8::Value> DrawAllContours(const v8::Arguments& args);

    // Feature Detection
    static v8::Handle<v8::Value> GoodFeaturesToTrack(const v8::Arguments& args);
    static v8::Handle<v8::Value> HoughLinesP(const v8::Arguments& args);

    static v8::Handle<v8::Value> inRange(const v8::Arguments& args);

    static v8::Handle<v8::Value> LocateROI(const v8::Arguments& args);
    static v8::Handle<v8::Value> AdjustROI(const v8::Arguments& args);

    static v8::Handle<v8::Value> Threshold(const v8::Arguments& args);
    static v8::Handle<v8::Value> AdaptiveThreshold(const v8::Arguments& args);
    static v8::Handle<v8::Value> MeanStdDev(const v8::Arguments& args);

    static v8::Handle<v8::Value> CopyTo(const v8::Arguments& args);
    static v8::Handle<v8::Value> CvtColor(const v8::Arguments& args);
    static v8::Handle<v8::Value> Split(const v8::Arguments& args);
    static v8::Handle<v8::Value> Merge(const v8::Arguments& args);
    static v8::Handle<v8::Value> EqualizeHist(const v8::Arguments& args);
    static v8::Handle<v8::Value> Pixel(const v8::Arguments& args);
    static v8::Handle<v8::Value> FloodFill(const v8::Arguments& args);

    static v8::Handle<v8::Value> MatchTemplate(const v8::Arguments& args);
    static v8::Handle<v8::Value> MinMaxLoc(const v8::Arguments& args);

    static v8::Handle<v8::Value> PushBack(const v8::Arguments& args);*/
/*
	static Handle<Value> Val(const Arguments& args);
	static Handle<Value> RowRange(const Arguments& args);
	static Handle<Value> ColRange(const Arguments& args);
	static Handle<Value> Diag(const Arguments& args);
	static Handle<Value> Clone(const Arguments& args);
	static Handle<Value> CopyTo(const Arguments& args);
	static Handle<Value> ConvertTo(const Arguments& args);
    static Handle<Value> AssignTo(const Arguments& args);
    static Handle<Value> SetTo(const Arguments& args);
    static Handle<Value> Reshape(const Arguments& args);
    static Handle<Value> Transpose(const Arguments& args);
    static Handle<Value> Invert(const Arguments& args);
    static Handle<Value> Multiply(const Arguments& args);
    static Handle<Value> Cross(const Arguments& args);
    static Handle<Value> Dot(const Arguments& args);
    static Handle<Value> Zeroes(const Arguments& args);
    static Handle<Value> Ones(const Arguments& args);
// create, increment, release
    static Handle<Value> PushBack(const Arguments& args);
    static Handle<Value> PopBack(const Arguments& args);
    static Handle<Value> Total(const Arguments& args);
    static Handle<Value> IsContinous(const Arguments& args);
    static Handle<Value> Type(const Arguments& args);
    static Handle<Value> Depth(const Arguments& args);
    static Handle<Value> Channels(const Arguments& args);
    static Handle<Value> StepOne(const Arguments& args);


*/

};

}

#endif