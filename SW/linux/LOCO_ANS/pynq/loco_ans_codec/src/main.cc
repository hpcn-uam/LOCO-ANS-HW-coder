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

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <iostream>
#include <string>
#include <algorithm>
#include <cstdlib>

#include "codec.h"

#include <sys/time.h>

using namespace std;

int main(int arg, char** argv ){
  
  int chroma_mode=CHROMA_MODE_GRAY; //default. (chroma_mode!=CHROMA_MODE_GRAY) not currently supported
  int encode_mode=ENCODER_MODE_ENCODE; //default
  int encode_prediction =ENCODER_PRED_LOCO; //default (deprecated)
  int NEAR =0; //default lossless
  int ibpp=8;

  if( arg < 3) {
    printf("Args: encode(0)/decode(1) args \n");
    printf("Encode args: 0 src_img_path out_compressed_img_path [NEAR] [bit_reduction] [encode_mode]  [blk_height]  [blk_width]   \n");
    printf("Decode args: 1 compressed_img_path path_to_out_image  \n");
    return 1;
  }


  bool decode= atoi(argv[1]);

  if( ! decode) {
    char * img_path= argv[2];
    char * out_file= argv[3];
  
    cv::Mat img_orig = cv::imread( img_path ,cv::IMREAD_UNCHANGED);
    if(img_orig.empty()){
      std::cerr<<"Empty image file"<<std::endl;
      return 2;
    }

    int blk_height=img_orig.rows;
    int blk_width=img_orig.cols;

    if (arg>4) {
      NEAR = atoi(argv[4]);
    }


    if (img_orig.type() == CV_8UC1){
      ibpp=8;
    }else if (img_orig.type() == CV_16UC1){
      ibpp=16;
    }else{
      std::cout<< "input has to be either a 8 or 16 bit gray image"<<std::endl;
      return -1;
    }

    if (arg>5) {
      int bit_reduction = atoi(argv[5]);
      if (bit_reduction >0){
      std::cout<< "Converting image | ";
      double div_factor = pow(2,-bit_reduction);
      ibpp -= bit_reduction;
      cout<<"In type "<< img_orig.type()<<"| Depth reduction (ibpp): "<<bit_reduction<<" ("<<ibpp<<")"<<endl;
      // img_orig/=div_factor;
      img_orig.convertTo(img_orig,-1,div_factor,0);
      // img_orig.convertTo(img_orig,-1,div_factor,.5);
      }
    }

    if (arg>6) {
  		encode_mode= atoi(argv[6]);
  	}

    if (arg>7 && (atoi(argv[7])>0) ) {
      blk_height = atoi(argv[7]);
    }

    if (arg>8 && (atoi(argv[8])>0) ) {
      blk_width = atoi(argv[8]);
    }


    /*if (arg>8) {
      encode_prediction= atoi(argv[8]);
    }

    if (arg>9) {
	 	 chroma_mode= atoi(argv[9]); 
    }*/

    std::cout<<" Encoder configuration ";
    std::cout<<"| NEAR: "<<NEAR; 
    std::cout<<"| Ibpp: "<<ibpp; 
    std::cout<<"| blk_height: "<<blk_height; 
    std::cout<<"| blk_width: "<<blk_width; 
    if(encode_mode != ENCODER_MODE_ENCODE) {
      std::cout<<"| encode_mode: "<<encode_mode;
    }
    // std::cout<<"| encode_prediction: "<<encode_prediction;
    std::cout<< std::endl;


 
    if(NEAR < 0) {
      std::cerr<<" Error NEAR should be >= 0"<<std::endl;
      return 1;
    }


    if(chroma_mode != CHROMA_MODE_GRAY) {
      std::cerr<< "chroma_mode != CHROMA_MODE_GRAY.";
      std::cerr<< " Only gray images are currently supported ";
      std::cerr<<std::endl;
      //Other modes are not currently supported 
      throw 1;
    }

    if(ibpp > 8) {
      std::cerr<< "ibpp > 8 |";
      std::cerr<< " Only images 8 or less input bit per pixel currently supported ";
      std::cerr<<std::endl;;
      //Other modes are not currently supported 
      throw 1;
    }

  //encode
    timespec fin,ini;
    int compress_img_size;
    clock_gettime(CLOCK_MONOTONIC, &ini);
    compress_img_size=encoder(img_orig,out_file,blk_width,blk_height,
                    chroma_mode,encode_prediction,NEAR,encode_mode,ibpp);
    clock_gettime(CLOCK_MONOTONIC, &fin);

    float enc_time = ((fin.tv_sec+fin.tv_nsec* 1E-9)-(ini.tv_sec+ini.tv_nsec* 1E-9));
    float enc_bw = img_orig.cols*img_orig.rows/(1024*1024*enc_time);
    printf("Encoder time: %.3f | BW: %.3f MP/s |", enc_time,enc_bw );
    printf(" Achieved bpp: %.3f \n",float(compress_img_size*8)/(img_orig.cols*img_orig.rows));
    if (compress_img_size<0){
      std::cerr<<"there's been an error in trying to encode the image"<<std::endl;
      return -1;
    }

  }else{
    char * compressed_img= argv[2];
    char * out_path= argv[3];
    std::cout<<"Compressed image:"<<compressed_img<<std::endl;
    std::cout<<"Out decoded image path: "<<out_path<<std::endl;

    cv::Mat decode_img;
    // struct timeval fin,ini;
    timespec fin,ini;
    bool scale_depth = true;
    // gettimeofday(&ini,NULL);
    clock_gettime(CLOCK_MONOTONIC, &ini);
    int deco_status = decoder(compressed_img,decode_img,scale_depth);
    clock_gettime(CLOCK_MONOTONIC, &fin);
    // gettimeofday(&fin,NULL);
    float dec_time = ((fin.tv_sec+fin.tv_nsec* 1E-9)-(ini.tv_sec+ini.tv_nsec* 1E-9));
    float dec_bw = decode_img.cols*decode_img.rows/(1024*1024*dec_time);
    printf("Decoder time: %.3f | BW: %.3f MP/s \n", dec_time,dec_bw );

    if (deco_status)
    {
      std::cerr<<"there's been an error in trying to decode the image"<<std::endl;
      return deco_status;
    }else{
      cv::imwrite(out_path,decode_img);
    }

  }

  return 0;
}


