#include "Matrix.h"

namespace binding {

v8::Persistent<FunctionTemplate> Matrix::constructor;

cv::Scalar setColor(Local<Object> objColor);
cv::Point setPoint(Local<Object> objPoint);
cv::Rect* setRect(Local<Object> objRect);


void
Matrix::Init(Handle<Object> target) {
	HandleScope scope;

	//Class
	v8::Local<v8::FunctionTemplate> m = v8::FunctionTemplate::New(New);
	m->SetClassName(v8::String::NewSymbol("Matrix"));

	// Constructor
	constructor = Persistent<FunctionTemplate>::New(m);
	constructor->InstanceTemplate()->SetInternalFieldCount(1);
	constructor->SetClassName(String::NewSymbol("Matrix"));

	// Prototype
	//Local<ObjectTemplate> proto = constructor->PrototypeTemplate();

/*
	NODE_SET_PROTOTYPE_METHOD(constructor, "row", Row);
	NODE_SET_PROTOTYPE_METHOD(constructor, "col", Col);

	NODE_SET_PROTOTYPE_METHOD(constructor, "pixelRow", PixelRow);
	NODE_SET_PROTOTYPE_METHOD(constructor, "pixelCol", PixelCol);

	NODE_SET_PROTOTYPE_METHOD(constructor, "empty", Empty);
	NODE_SET_PROTOTYPE_METHOD(constructor, "get", Get);
	NODE_SET_PROTOTYPE_METHOD(constructor, "set", Set);
	NODE_SET_PROTOTYPE_METHOD(constructor, "pixel", Pixel);
  */
	NODE_SET_PROTOTYPE_METHOD(constructor, "width", Width);
	NODE_SET_PROTOTYPE_METHOD(constructor, "height", Height);
  /*
	NODE_SET_PROTOTYPE_METHOD(constructor, "size", Size);
	NODE_SET_PROTOTYPE_METHOD(constructor, "clone", Clone);
	NODE_SET_PROTOTYPE_METHOD(constructor, "toBuffer", ToBuffer);
	NODE_SET_PROTOTYPE_METHOD(constructor, "toBufferAsync", ToBufferAsync);
	NODE_SET_PROTOTYPE_METHOD(constructor, "ellipse", Ellipse);
	NODE_SET_PROTOTYPE_METHOD(constructor, "rectangle", Rectangle);
	NODE_SET_PROTOTYPE_METHOD(constructor, "line", Line);
	NODE_SET_PROTOTYPE_METHOD(constructor, "save", Save);
	NODE_SET_PROTOTYPE_METHOD(constructor, "saveAsync", SaveAsync);
	NODE_SET_PROTOTYPE_METHOD(constructor, "resize", Resize);
	NODE_SET_PROTOTYPE_METHOD(constructor, "rotate", Rotate);
	NODE_SET_PROTOTYPE_METHOD(constructor, "copyTo", CopyTo);
	NODE_SET_PROTOTYPE_METHOD(constructor, "pyrDown", PyrDown);
	NODE_SET_PROTOTYPE_METHOD(constructor, "pyrUp", PyrUp);
	NODE_SET_PROTOTYPE_METHOD(constructor, "channels", Channels);

	NODE_SET_PROTOTYPE_METHOD(constructor, "convertGrayscale", ConvertGrayscale);
    NODE_SET_PROTOTYPE_METHOD(constructor, "convertHSVscale", ConvertHSVscale);
	NODE_SET_PROTOTYPE_METHOD(constructor, "gaussianBlur", GaussianBlur);
	NODE_SET_PROTOTYPE_METHOD(constructor, "copy", Copy);
	NODE_SET_PROTOTYPE_METHOD(constructor, "flip", Flip);
	NODE_SET_PROTOTYPE_METHOD(constructor, "roi", ROI);
	NODE_SET_PROTOTYPE_METHOD(constructor, "ptr", Ptr);
	NODE_SET_PROTOTYPE_METHOD(constructor, "absDiff", AbsDiff);
	NODE_SET_PROTOTYPE_METHOD(constructor, "addWeighted", AddWeighted);
	NODE_SET_PROTOTYPE_METHOD(constructor, "bitwiseXor", BitwiseXor);
	NODE_SET_PROTOTYPE_METHOD(constructor, "countNonZero", CountNonZero);
	//NODE_SET_PROTOTYPE_METHOD(constructor, "split", Split);
	NODE_SET_PROTOTYPE_METHOD(constructor, "canny", Canny);
    NODE_SET_PROTOTYPE_METHOD(constructor, "dilate", Dilate);
	NODE_SET_PROTOTYPE_METHOD(constructor, "erode", Erode);

	NODE_SET_PROTOTYPE_METHOD(constructor, "findContours", FindContours);
	NODE_SET_PROTOTYPE_METHOD(constructor, "drawContour", DrawContour);
	NODE_SET_PROTOTYPE_METHOD(constructor, "drawAllContours", DrawAllContours);

  NODE_SET_PROTOTYPE_METHOD(constructor, "goodFeaturesToTrack", GoodFeaturesToTrack);
  NODE_SET_PROTOTYPE_METHOD(constructor, "houghLinesP", HoughLinesP);

	NODE_SET_PROTOTYPE_METHOD(constructor, "inRange", inRange);
	NODE_SET_PROTOTYPE_METHOD(constructor, "adjustROI", AdjustROI);
	NODE_SET_PROTOTYPE_METHOD(constructor, "locateROI", LocateROI);

	NODE_SET_PROTOTYPE_METHOD(constructor, "threshold", Threshold);
	NODE_SET_PROTOTYPE_METHOD(constructor, "adaptiveThreshold", AdaptiveThreshold);
	NODE_SET_PROTOTYPE_METHOD(constructor, "meanStdDev", MeanStdDev);
    
    NODE_SET_PROTOTYPE_METHOD(constructor, "cvtColor", CvtColor);
    NODE_SET_PROTOTYPE_METHOD(constructor, "split", Split);
    NODE_SET_PROTOTYPE_METHOD(constructor, "merge", Merge);
    NODE_SET_PROTOTYPE_METHOD(constructor, "equalizeHist", EqualizeHist);

	NODE_SET_PROTOTYPE_METHOD(constructor, "floodFill", FloodFill);

  NODE_SET_PROTOTYPE_METHOD(constructor, "matchTemplate", MatchTemplate);
  NODE_SET_PROTOTYPE_METHOD(constructor, "minMaxLoc", MinMaxLoc);

  NODE_SET_PROTOTYPE_METHOD(constructor, "pushBack", PushBack);

	NODE_SET_METHOD(constructor, "Eye", Eye);

*/
	target->Set(String::NewSymbol("Matrix"), m->GetFunction());
};


Handle<Value>
Matrix::New(const Arguments &args) {
	HandleScope scope;

	if (args.This()->InternalFieldCount() == 0)
		return v8::ThrowException(v8::Exception::TypeError(v8::String::New("Cannot instantiate without new")));

	Matrix *mat;

  std::cout << "New called" << std::endl;

	if (args.Length() == 0){
		mat = new Matrix;
    std::cout << "Length 0" << std::endl;
	} else if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()){
			mat = new Matrix(args[0]->IntegerValue(), args[1]->IntegerValue());
      std::cout << "Length 2" << std::endl;
	} else if (args.Length() == 5) {
		Matrix *other = ObjectWrap::Unwrap<Matrix>(args[0]->ToObject());
		int x = args[1]->IntegerValue();
		int y = args[2]->IntegerValue();
		int w = args[3]->IntegerValue();
		int h = args[4]->IntegerValue();
		mat = new Matrix(other->mat, cv::Rect(x, y, w, h));
    std::cout << "Length 5" << std::endl;
	}

  int h = mat->mat.size().height;
  std::cout << "Height: " << h << std::endl;

	mat->Wrap(args.Holder());
	return scope.Close(args.Holder());
}


Matrix::Matrix(): ObjectWrap() {
	mat = cv::Mat();

  std::cout << "Length 0 Constructor" << std::endl;
}


Matrix::Matrix(int rows, int cols): ObjectWrap() {
  mat = cv::Mat(rows, cols, CV_32FC3);

  std::cout << mat.size().width << std::endl;
}

Matrix::Matrix(cv::Mat m, cv::Rect roi): ObjectWrap() {
	mat = cv::Mat(m, roi);
}


bool Matrix::HasInstance(Handle<Value> val)
{
    if (!val->IsObject()) {
        return false;
    }
    return constructor->HasInstance(val->ToObject());
}


Handle<Value>
Matrix::Width(const Arguments& args){
  HandleScope scope;
  Matrix *self = ObjectWrap::Unwrap<Matrix>(args.This());

  return scope.Close(Number::New(self->mat.size().width));
}

Handle<Value>
Matrix::Height(const Arguments& args){
  HandleScope scope;
  Matrix *self = ObjectWrap::Unwrap<Matrix>(args.This());

  return scope.Close(Number::New(self->mat.size().height));
}

}