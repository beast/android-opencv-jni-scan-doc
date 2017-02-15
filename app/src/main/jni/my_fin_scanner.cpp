//
// Created by yang yao on 15/02/2017.
//

#include "my_fin_scanner.h"

#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <string>
#include <vector>
#include <android/log.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <android/bitmap.h>
#define APPNAME "my_fin_scanner"
using namespace cv;
using namespace std;


extern "C" {

/*
 * File: scannerLite.cpp
 * Author: daisygao
 * An OpenCV program implementing the recognition feature of the app "CamScanner".
 * It extracts the main document object from an image and adjust it to A4 size.
 */

/**
 * Get edges of an image
 * @param gray - grayscale input image
 * @param canny - output edge image
 */
void getCanny(Mat gray, Mat &canny) {
  Mat thres;
  double high_thres = threshold(gray, thres, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU), low_thres = high_thres * 0.5;
  cv::Canny(gray, canny, low_thres, high_thres);
}

struct Line {
  Point _p1;
  Point _p2;
  Point _center;

  Line(Point p1, Point p2) {
    _p1 = p1;
    _p2 = p2;
    _center = Point((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
  }
};

bool cmp_y(const Line &p1, const Line &p2) {
  return p1._center.y < p2._center.y;
}

bool cmp_x(const Line &p1, const Line &p2) {
  return p1._center.x < p2._center.x;
}

/**
 * Compute intersect point of two lines l1 and l2
 * @param l1
 * @param l2
 * @return Intersect Point
 */
Point2f computeIntersect(Line l1, Line l2) {
  int x1 = l1._p1.x, x2 = l1._p2.x, y1 = l1._p1.y, y2 = l1._p2.y;
  int x3 = l2._p1.x, x4 = l2._p2.x, y3 = l2._p1.y, y4 = l2._p2.y;
  if (float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4)) {
    Point2f pt;
    pt.x = ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) / d;
    pt.y = ((x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4)) / d;
    return pt;
  }
  return Point2f(-1, -1);
}

Mat scan(Mat img) {

  /* get input image */
  //Mat img = imread(file);

  // resize input image to img_proc to reduce computation
  Mat img_proc;
  int w = img.size().width, h = img.size().height, min_w = 200;
  double scale = min(10.0, w * 1.0 / min_w);
  int w_proc = w * 1.0 / scale, h_proc = h * 1.0 / scale;
  resize(img, img_proc, Size(w_proc, h_proc));
  Mat img_dis = img_proc.clone();

  /* get four outline edges of the document */
  // get edges of the image
  Mat gray, canny;
  cvtColor(img_proc, gray, CV_BGR2GRAY);
  getCanny(gray, canny);

  // extract lines from the edge image
  vector<Vec4i> lines;
  vector<Line> horizontals, verticals;
  HoughLinesP(canny, lines, 1, CV_PI / 180, w_proc / 3, w_proc / 3, 20);
  for (size_t i = 0; i < lines.size(); i++) {
    Vec4i v = lines[i];
    double delta_x = v[0] - v[2], delta_y = v[1] - v[3];
    Line l(Point(v[0], v[1]), Point(v[2], v[3]));
    // get horizontal lines and vertical lines respectively
    if (fabs(delta_x) > fabs(delta_y)) {
      horizontals.push_back(l);
    } else {
      verticals.push_back(l);
    }
    // for visualization only
//    if (debug)
//      line(img_proc, Point(v[0], v[1]), Point(v[2], v[3]), Scalar(0, 0, 255), 1, CV_AA);
  }

  // edge cases when not enough lines are detected
  if (horizontals.size() < 2) {
    if (horizontals.size() == 0 || horizontals[0]._center.y > h_proc / 2) {
      horizontals.push_back(Line(Point(0, 0), Point(w_proc - 1, 0)));
    }
    if (horizontals.size() == 0 || horizontals[0]._center.y <= h_proc / 2) {
      horizontals.push_back(Line(Point(0, h_proc - 1), Point(w_proc - 1, h_proc - 1)));
    }
  }
  if (verticals.size() < 2) {
    if (verticals.size() == 0 || verticals[0]._center.x > w_proc / 2) {
      verticals.push_back(Line(Point(0, 0), Point(0, h_proc - 1)));
    }
    if (verticals.size() == 0 || verticals[0]._center.x <= w_proc / 2) {
      verticals.push_back(Line(Point(w_proc - 1, 0), Point(w_proc - 1, h_proc - 1)));
    }
  }
  // sort lines according to their center point
  sort(horizontals.begin(), horizontals.end(), cmp_y);
  sort(verticals.begin(), verticals.end(), cmp_x);
  // for visualization only
/*  if (debug) {
    line(img_proc, horizontals[0]._p1, horizontals[0]._p2, Scalar(0, 255, 0), 2, CV_AA);
    line(img_proc, horizontals[horizontals.size() - 1]._p1, horizontals[horizontals.size() - 1]._p2, Scalar(0, 255, 0), 2, CV_AA);
    line(img_proc, verticals[0]._p1, verticals[0]._p2, Scalar(255, 0, 0), 2, CV_AA);
    line(img_proc, verticals[verticals.size() - 1]._p1, verticals[verticals.size() - 1]._p2, Scalar(255, 0, 0), 2, CV_AA);
  } */

  /* perspective transformation */

  // define the destination image size: A4 - 200 PPI
  int w_a4 = 1654, h_a4 = 2339;
  //int w_a4 = 595, h_a4 = 842;
  Mat dst = Mat::zeros(h_a4, w_a4, CV_8UC3);

  // corners of destination image with the sequence [tl, tr, bl, br]
  vector<Point2f> dst_pts, img_pts;
  dst_pts.push_back(Point(0, 0));
  dst_pts.push_back(Point(w_a4 - 1, 0));
  dst_pts.push_back(Point(0, h_a4 - 1));
  dst_pts.push_back(Point(w_a4 - 1, h_a4 - 1));

  // corners of source image with the sequence [tl, tr, bl, br]
  img_pts.push_back(computeIntersect(horizontals[0], verticals[0]));
  img_pts.push_back(computeIntersect(horizontals[0], verticals[verticals.size() - 1]));
  img_pts.push_back(computeIntersect(horizontals[horizontals.size() - 1], verticals[0]));
  img_pts.push_back(computeIntersect(horizontals[horizontals.size() - 1], verticals[verticals.size() - 1]));

  // convert to original image scale
  for (size_t i = 0; i < img_pts.size(); i++) {
    // for visualization only
  /*  if (debug) {
      circle(img_proc, img_pts[i], 10, Scalar(255, 255, 0), 3);
    } */
    img_pts[i].x *= scale;
    img_pts[i].y *= scale;
  }

  // get transformation matrix
  Mat transmtx = getPerspectiveTransform(img_pts, dst_pts);

  // apply perspective transformation
  warpPerspective(img, dst, transmtx, dst.size());

  // save dst img
  return dst;
}

jobject mat_to_bitmap(JNIEnv * env, Mat & src, bool needPremultiplyAlpha, jobject bitmap_config){
    jclass java_bitmap_class = (jclass)env->FindClass("android/graphics/Bitmap");
    jmethodID mid = env->GetStaticMethodID(java_bitmap_class,
                                           "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");

    jobject bitmap = env->CallStaticObjectMethod(java_bitmap_class,
                                                 mid, src.size().width, src.size().height, bitmap_config);
    AndroidBitmapInfo  info;
    void*              pixels = 0;

    try {
        CV_Assert(AndroidBitmap_getInfo(env, bitmap, &info) >= 0);
        CV_Assert(src.type() == CV_8UC1 || src.type() == CV_8UC3 || src.type() == CV_8UC4);
        CV_Assert(AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0);
        CV_Assert(pixels);
        if(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888){
            Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(src.type() == CV_8UC1){
                cvtColor(src, tmp, CV_GRAY2RGBA);
            }else if(src.type() == CV_8UC3){
                cvtColor(src, tmp, CV_RGB2RGBA);
            }else if(src.type() == CV_8UC4){
                if(needPremultiplyAlpha){
                    cvtColor(src, tmp, COLOR_RGBA2mRGBA);
                }else{
                    src.copyTo(tmp);
                }
            }
        }else{
            // info.format == ANDROID_BITMAP_FORMAT_RGB_565
            Mat tmp(info.height, info.width, CV_8UC2, pixels);
            if(src.type() == CV_8UC1){
                cvtColor(src, tmp, CV_GRAY2BGR565);
            }else if(src.type() == CV_8UC3){
                cvtColor(src, tmp, CV_RGB2BGR565);
            }else if(src.type() == CV_8UC4){
                cvtColor(src, tmp, CV_RGBA2BGR565);
            }
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return bitmap;
    }catch(cv::Exception e){
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("org/opencv/core/CvException");
        if(!je) je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return bitmap;
    }catch (...){
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nMatToBitmap}");
        return bitmap;
    }
}


JNIEXPORT jobject JNICALL Java_my_fin_scanner_OpenCVHelper_scan
  (JNIEnv *env, jobject obj, jobject bitmap){

    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Scanning start");
    int ret;
    AndroidBitmapInfo info;
    void* pixels = 0;

    if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME,"AndroidBitmap_getInfo() failed ! error=%d", ret);
        return NULL;
    }

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888 )
    {       __android_log_print(ANDROID_LOG_VERBOSE, APPNAME,"Bitmap format is not RGBA_8888!");
        return NULL;
    }

    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME,"AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }

    // init our output image
    Mat mbgra(info.height, info.width, CV_8UC4, pixels);
    Mat dst = scan(mbgra);

    //get source bitmap's config
    jclass java_bitmap_class = (jclass)env->FindClass("android/graphics/Bitmap");
    jmethodID mid = env->GetMethodID(java_bitmap_class, "getConfig", "()Landroid/graphics/Bitmap$Config;");
    jobject bitmap_config = env->CallObjectMethod(bitmap, mid);
    jobject _bitmap = mat_to_bitmap(env, dst, false, bitmap_config);

    AndroidBitmap_unlockPixels(env, bitmap);
    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Scanning complete");
    return _bitmap;
  }

}