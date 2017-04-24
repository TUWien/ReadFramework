/*M///////////////////////////////////////////////////////////////////////////////////////
 //
 //  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 //
 //  By downloading, copying, installing or using the software you agree to this license.
 //  If you do not agree to this license, do not download, install,
 //  copy or use the software.
 //
 //
 //                           License Agreement
 //                For Open Source Computer Vision Library
 //
 // Copyright (C) 2014, Biagio Montesano, all rights reserved.
 // Third party copyrights are property of their respective owners.
 //
 // Redistribution and use in source and binary forms, with or without modification,
 // are permitted provided that the following conditions are met:
 //
 //   * Redistribution's of source code must retain the above copyright notice,
 //     this list of conditions and the following disclaimer.
 //
 //   * Redistribution's in binary form must reproduce the above copyright notice,
 //     this list of conditions and the following disclaimer in the documentation
 //     and/or other materials provided with the distribution.
 //
 //   * The name of the copyright holders may not be used to endorse or promote products
 //     derived from this software without specific prior written permission.
 //
 // This software is provided by the copyright holders and contributors "as is" and
 // any express or implied warranties, including, but not limited to, the implied
 // warranties of merchantability and fitness for a particular purpose are disclaimed.
 // In no event shall the Intel Corporation or contributors be liable for any direct,
 // indirect, incidental, special, exemplary, or consequential damages
 // (including, but not limited to, procurement of substitute goods or services;
 // loss of use, data, or profits; or business interruption) however caused
 // and on any theory of liability, whether in contract, strict liability,
 // or tort (including negligence or otherwise) arising in any way out of
 // the use of this software, even if advised of the possibility of such damage.
 //
 //M*/

#pragma once

#include <map>
#include <vector>
#include <list>

#if defined _MSC_VER && _MSC_VER <= 1700
#include <stdint.h>
#else
#include <inttypes.h>
#endif

#include <stdio.h>
#include <iostream>

#include "opencv2/core/utility.hpp"
//#include "opencv2/core/private.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/core.hpp"

/* define data types */
typedef uint64_t UINT64;
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint8_t UINT8;

/* define constants */
#define UINT64_1 ((UINT64)0x01)
#define UINT32_1 ((UINT32)0x01)

namespace lsd
{

//! @addtogroup line_descriptor
//! @{

/** @brief A class to represent a line

As aformentioned, it is been necessary to design a class that fully stores the information needed to
characterize completely a line and plot it on image it was extracted from, when required.

*KeyLine* class has been created for such goal; it is mainly inspired to Feature2d's KeyPoint class,
since KeyLine shares some of *KeyPoint*'s fields, even if a part of them assumes a different
meaning, when speaking about lines. In particular:

-   the *class_id* field is used to gather lines extracted from different octaves which refer to
    same line inside original image (such lines and the one they represent in original image share
    the same *class_id* value)
-   the *angle* field represents line's slope with respect to (positive) X axis
-   the *pt* field represents line's midpoint
-   the *response* field is computed as the ratio between the line's length and maximum between
    image's width and height
-   the *size* field is the area of the smallest rectangle containing line

Apart from fields inspired to KeyPoint class, KeyLines stores information about extremes of line in
original image and in octave it was extracted from, about line's length and number of pixels it
covers.
 */
struct CV_EXPORTS KeyLine
{
 public:
  /** orientation of the line */
  float angle;

  /** object ID, that can be used to cluster keylines by the line they represent */
  int class_id;

  /** octave (pyramid layer), from which the keyline has been extracted */
  int octave;

  /** coordinates of the middlepoint */
  cv::Point2f pt;

  /** the response, by which the strongest keylines have been selected.
   It's represented by the ratio between line's length and maximum between
   image's width and height */
  float response;

  /** minimum area containing line */
  float size;

  /** lines's extremes in original image */
  float startPointX;
  float startPointY;
  float endPointX;
  float endPointY;

  /** line's extremes in image it was extracted from */
  float sPointInOctaveX;
  float sPointInOctaveY;
  float ePointInOctaveX;
  float ePointInOctaveY;

  /** the length of line */
  float lineLength;

  /** number of pixels covered by the line */
  int numOfPixels;

  /** Returns the start point of the line in the original image */
  cv::Point2f getStartPoint() const
  {
    return cv::Point2f(startPointX, startPointY);
  }

  /** Returns the end point of the line in the original image */
  cv::Point2f getEndPoint() const
  {
    return cv::Point2f(endPointX, endPointY);
  }

  /** Returns the start point of the line in the octave it was extracted from */
  cv::Point2f getStartPointInOctave() const
  {
    return cv::Point2f(sPointInOctaveX, sPointInOctaveY);
  }

  /** Returns the end point of the line in the octave it was extracted from */
  cv::Point2f getEndPointInOctave() const
  {
    return cv::Point2f(ePointInOctaveX, ePointInOctaveY);
  }

  /** constructor */
  KeyLine()
  {
  }
};

/**
Lines extraction methodology
----------------------------

The lines extraction methodology described in the following is mainly based on @cite EDL . The
extraction starts with a Gaussian pyramid generated from an original image, downsampled N-1 times,
blurred N times, to obtain N layers (one for each octave), with layer 0 corresponding to input
image. Then, from each layer (octave) in the pyramid, lines are extracted using LSD algorithm.

Differently from EDLine lines extractor used in original article, LSD furnishes information only
about lines extremes; thus, additional information regarding slope and equation of line are computed
via analytic methods. The number of pixels is obtained using *LineIterator*. Extracted lines are
returned in the form of KeyLine objects, but since extraction is based on a method different from
the one used in *BinaryDescriptor* class, data associated to a line's extremes in original image and
in octave it was extracted from, coincide. KeyLine's field *class_id* is used as an index to
indicate the order of extraction of a line inside a single octave.
*/
class CV_EXPORTS LSDDetector : public cv::Algorithm
{
public:

/* constructor */
/*CV_WRAP*/
LSDDetector()
{
}
;

/** @brief Creates ad LSDDetector object, using smart pointers.
 */
static cv::Ptr<LSDDetector> createLSDDetector();

/** @brief Detect lines inside an image.

@param image input image
@param keypoints vector that will store extracted lines for one or more images
@param scale scale factor used in pyramids generation
@param numOctaves number of octaves inside pyramid
@param mask mask matrix to detect only KeyLines of interest
 */
void detect( const cv::Mat& image, CV_OUT std::vector<KeyLine>& keypoints, int scale, int numOctaves, const cv::Mat& mask = cv::Mat() );

/** @overload
@param images input images
@param keylines set of vectors that will store extracted lines for one or more images
@param scale scale factor used in pyramids generation
@param numOctaves number of octaves inside pyramid
@param masks vector of mask matrices to detect only KeyLines of interest from each input image
*/
void detect( const std::vector<cv::Mat>& images, std::vector<std::vector<KeyLine> >& keylines, int scale, int numOctaves,
const std::vector<cv::Mat>& masks = std::vector<cv::Mat>() ) const;

private:
/* compute Gaussian pyramid of input image */
void computeGaussianPyramid( const cv::Mat& image, int numOctaves, int scale );

/* implementation of line detection */
void detectImpl( const cv::Mat& imageSrc, std::vector<KeyLine>& keylines, int numOctaves, int scale, const cv::Mat& mask ) const;

/* matrices for Gaussian pyramids */
std::vector<cv::Mat> gaussianPyrs;
};

}
