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

#ifndef IMG_PROC_UTILS
#define IMG_PROC_UTILS

#include "codec_core.h"

/*
*##################   Codec commons  ########################
*/

  inline unsigned int clamp(int v, unsigned MAXVAL = 255){  //cv::saturate_cast<uchar>
    return (unsigned int)((unsigned)v <= MAXVAL ? v : v > 0 ? MAXVAL : 0);
  }


  int get_num_of_symbs(int rows, int cols, char chroma_mode){
    int num_symb;
    switch(chroma_mode){
      case CHROMA_MODE_YUV422:
        if(cols%2!=0) {
          std::cerr<<" For this chroma mode the number of columns should be even" <<std::endl;
          num_symb=rows*cols + 2*rows*(cols/2+1) ;
        }else{
          num_symb=rows*cols*2;  
        }
        break;

      case CHROMA_MODE_YUV420:
          

        if(cols%2!=0) {
          if(rows!=1) { //this height is supported cause encoder takes this into account
            std::cerr<<" For this chroma mode the number of columns should be even" <<std::endl;
          }

          if(rows%2!=0) {
            if(rows!=1){ //better than throwing it to NULL?
              std::cerr<<" For this chroma mode the number of rows should be even" <<std::endl;
            }
            num_symb=rows*cols + (rows/2+1)*(cols/2+1) ;
          }else{
            num_symb=rows*cols + rows*(cols/2+1) ;
          }
        }else{
          if(rows%2!=0) {
            if(rows!=1){ //better than throwing it to NULL?
              std::cerr<<" For this chroma mode the number of rows should be even" <<std::endl;
            }
            num_symb= rows*cols + cols*(rows/2+1);
          }else{
            num_symb= rows*cols*1.5;
          }
        }

        break;
      default:
       std::cout<<" Not a valid chroma mode. Switching to YUV444"<<std::endl;
       /* no break */
      case CHROMA_MODE_YUV444:
        num_symb=rows*cols*3;
        break;
      case CHROMA_MODE_GRAY:
        num_symb=rows*cols;
        break;
    }

    return num_symb;
  }
  // inline unsigned int get_value( RowBuffer &src,int row, int col, int channel=0){
  //   return src.retrieve( row,col);
  // }

  // inline unsigned int get_value(const cv::Mat &src,int row, int col, int channel){
  //   return src.data[src.step[0]*row+src.step[1]*col+channel];
  // }

  inline unsigned int get_value(const cv::Mat &src,int row, int col){
    return src.data[src.step[0]*row+src.step[1]*col];
  }

  // inline unsigned int get_value_16(const cv::Mat &src,int row, int col, int channel){
  //   return src.at<unsigned>(row,col)[channel];
  // }

  inline unsigned int get_value_16(const cv::Mat &src,int row, int col){
    return src.at<unsigned short>(row,col);
  }


  inline void set_value(cv::Mat &src,int row, int col, int value){
    src.data[src.step[0]*row+src.step[1]*col]=value;
  }

  inline void set_value_16(cv::Mat &src,int row, int col, int value){
    src.at<unsigned short>(row,col)=value;
  }

/*
*##################   quality_measures  ########################
*/

  double mse(const cv::Mat img0,const cv::Mat img1){
    cv::Mat tmp(img0.rows,img0.cols,CV_32F);
    //cv::subtract(img0, img1, tmp);
    cv::absdiff(img0, img1, tmp);
    cv::multiply(tmp, tmp, tmp);
    cv::Scalar mse_per_chn=cv::mean(tmp);

    double mse =0;
    for(int i=0;i< img0.channels();i++ ){
     mse += mse_per_chn.val[i];
    }
    mse /= img0.channels();

    return mse;
  }

  double psnr(const cv::Mat img0,const cv::Mat img1,int max_value=255){
    double imgs_mse=mse(img0,img1);

    double psnr=10*std::log10(pow(max_value,2)/imgs_mse);

    return psnr;
  }





#endif /* IMG_PROC_UTILS */