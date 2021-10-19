/*
  Copyright 2021 Tobías Alonso, Autonomous University of Madrid

  This file is part of LOCO-ANS.

  LOCO-ANS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LOCO-ANS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LOCO-ANS.  If not, see <https://www.gnu.org/licenses/>.


 */
#ifndef COLOR_SPACE_TRANSFORMS
#define COLOR_SPACE_TRANSFORMS

#include <opencv2/imgproc/imgproc.hpp>

#include "codec_core.h"
#include "img_proc_utils.h"


  void rgb2yuv(const cv::Mat& src,cv::Mat&  dst,char chroma_mode){
    cv::Mat tmp;
    bool gray_input = true;
    if(chroma_mode==CHROMA_MODE_GRAY) {
      tmp =cv::Mat::zeros(src.size(),CV_8UC1);
      for (int row = 0; row < src.rows; ++row){
        for (int col = 0; col < src.cols; ++col){
          int32_t r = src.at<cv::Vec3b>(row,col)[2];
          int32_t g = src.at<cv::Vec3b>(row,col)[1];
          int32_t b = src.at<cv::Vec3b>(row,col)[0];

          if(r ==g && r == b) {
            tmp.at<unsigned char>(row,col)= r;
          }else{
            gray_input = false;
            tmp.at<unsigned char>(row,col)= ((r*76  + g*150  + b*29  +128)>>8)    ; //y
          }
          // tmp.at<unsigned char>(row,col)= ((r*-43 + g*-84  + b*127 +128)>>8)+128; //u
          // tmp.at<unsigned char>(row,col)= ((r*127 + g*-106 + b*-21 +128)>>8)+128; //v
        }


      }
      if( not gray_input) {
        std::cerr<<"Chrominance mode: Gray, but source image is not gray"<<std::endl;
      }
    }else{
      tmp =cv::Mat::zeros(src.size(),src.type());
      for (int row = 0; row < src.rows; ++row){
        for (int col = 0; col < src.cols; ++col){
          int32_t r = src.at<cv::Vec3b>(row,col)[2];
          int32_t g = src.at<cv::Vec3b>(row,col)[1];
          int32_t b = src.at<cv::Vec3b>(row,col)[0];

          tmp.at<cv::Vec3b>(row,col)[0]= ((r*76  + g*150  + b*29  +128)>>8)    ; //y
          tmp.at<cv::Vec3b>(row,col)[1]= ((r*-43 + g*-84  + b*127 +128)>>8)+128; //u
          tmp.at<cv::Vec3b>(row,col)[2]= ((r*127 + g*-106 + b*-21 +128)>>8)+128; //v
        }

      }
    }

    dst=tmp.clone();
  }

  void yuv2rgb(const cv::Mat src, cv::Mat& dst,char chroma_mode){
    //cv::Mat tmp=cv::Mat::zeros(src.size(),src.type());
    int rows=src.rows, cols=src.cols;
    for (int row = 0; row < rows; ++row){
      for (int col = 0; col < cols; ++col){
        int16_t  y = src.at<cv::Vec3b>(row,col)[0];
        int16_t  u = src.at<cv::Vec3b>(row,col)[1]-128;
        int16_t  v = src.at<cv::Vec3b>(row,col)[2]-128;

        dst.at<cv::Vec3b>(row,col)[2]= clamp((y*257 + u     + v*363  +128)/256); //r
        dst.at<cv::Vec3b>(row,col)[1]= clamp((y*257 + u*-89 + v*-183 +128)/256); //g/home/tobi/Documents/Proyectos/LHE/sw/Experiments/encode_n_deco_blocks_stats/src/
        dst.at<cv::Vec3b>(row,col)[0]= clamp((y*257 + u*458  +v      +128)/256); //b

        /* approximation of
          tmp.at<cv::Vec3b>(row,col)[2]= clamp(y +    + v*1.13983 );  //r
          tmp.at<cv::Vec3b>(row,col)[1]= clamp(y + u*-.39465 + v*-.5806); //g
          tmp.at<cv::Vec3b>(row,col)[0]= clamp((y + u*2.03211    ));  //b*/

      }

    }
    //tmp.copyTo(dst);
  }


#endif /* COLOR_SPACE_TRANSFORMS */