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

#include "codec.h"
#include "coder_config.h"


int encoder(const cv::Mat& src_img,char* out_file,int block_width,int block_height, 
  int chroma_mode, char prediction,int NEAR, char encoder_mode, int ibpp ){

  if(NEAR > MAX_NEAR) {
    std::cerr<<" The header used in this version does not support NEAR > "<<MAX_NEAR<<std::endl;
    return 1;
  }else if(NEAR < 0) {
    std::cerr<<" Error: NEAR should be >= 0"<<std::endl;
    return 1;
  }
  
  bool save_to_file = true;


  //get number of image blocks
    //image height and width in number of blocks
    int blk_rows=ceil(src_img.rows/float(block_height));
    int blk_cols=ceil(src_img.cols/float(block_width));
    
    if(ibpp > MAX_IBPP) {
      std::cerr<<"Input image not supported. Input bpp >"<<MAX_IBPP<<"."<<std::endl;
      std::cerr<<"Input bpp:"<< int(ibpp)<<std::endl;
      throw 1;
    }
      
    //save file header
      struct global_header header;
      header.color_profile= chroma_mode;
      header.ibpp=ibpp ;        
      header.predictor = prediction;
      header.ee_buffer_exp = uint(std::log2(EE_BUFFER_SIZE/32));
      header.NEAR = NEAR ;        
      header.blk_height = block_height;
      header.blk_width = block_width;
      header.img_height =src_img.rows; 
      header.img_width = src_img.cols;

      std::ofstream binary_out_file(out_file,std::ios::binary); //out file
      binary_out_file.write((char*)&(header),sizeof(header));
    

  //generate blocks
    cv::Mat block, codec_input_img;
    
    if(src_img.channels()== 3 && chroma_mode == CHROMA_MODE_GRAY){ //I assume it's RGB image 
      std::cerr<<"src_img.channels()== 3"<<std::endl;
      throw 1;
      // rgb2yuv(src_img, codec_input_img,CHROMA_MODE_GRAY);
    }else{
      codec_input_img = src_img;
    }

    int compress_img_size=sizeof(global_header);
    uint8_t* block_buffer;
    uint32_t max_output_size=((block_height*block_width*MAX_SUPPORTED_BPP/8)/(sizeof (*block_buffer))+1); //for no chroma sub-sampling
    block_buffer = new uint8_t[max_output_size];

    for (int blk_row = 0; blk_row < blk_rows; ++blk_row) {
      for (int blk_col = 0; blk_col < blk_cols; ++blk_col) {
        //map a portion of the input image to a block
        int row_low = blk_row*block_height;
        int row_high = MIN((blk_row+1)*block_height,src_img.rows);
        int col_low = blk_col*block_width;
        int col_high = MIN((blk_col+1)*block_width,src_img.cols); 
        block=codec_input_img(cv::Range(row_low,row_high),cv::Range(col_low,col_high)); 
        cv::Mat quant_block;
        uint32_t out_file_size;

        out_file_size=encode_core(block,quant_block,block_buffer,chroma_mode,
                          prediction,NEAR, encoder_mode,ibpp);
      

        compress_img_size+= (int)out_file_size;
        compress_img_size+= sizeof(block_header);
        if(save_to_file) {
          //store block header
            struct block_header block_header;
            block_header.size = out_file_size;
            binary_out_file.write((char*)&(block_header),sizeof(block_header));

          binary_out_file.write((char*)block_buffer,out_file_size); //store block data
        }
      }
    }


    binary_out_file.close();
  

    delete[] block_buffer;
    return compress_img_size;
}



int decoder(char* in_file,cv::Mat &dst_img, bool scale_depth){
  //open LOCO-ANS-coded image
    std::ifstream binary_in_file(in_file, std::ios::binary );

  //extract file header
    struct global_header header;
    binary_in_file.read((char*)&header,sizeof(header));

    if(!header.check_version() ) {
      std::cerr<<"Compressed image format not supported. version mismatch."<<std::endl;
      std::cerr<<"Compressed image version:"<< int(header.version)<<std::endl;
      throw 1;
    }
    
    uint32_t blk_height=header.blk_height;
    uint32_t blk_width=header.blk_width;

    uint32_t img_height = header.img_height;
    uint32_t img_width  = header.img_width;

    int blk_rows=ceil(img_height/float(blk_height));
    int blk_cols=ceil(img_width/float(blk_width));

    int chroma_mode = header.color_profile;
    uint ee_buffer_size = 32 * (1<<header.ee_buffer_exp);
    int fix_predictor = header.predictor;
    int NEAR = (int)header.NEAR ;

    std::cout<<" Encoded image configuration ";
    std::cout<<"| NEAR: "<<NEAR; 
    std::cout<<"| ibpp: "<<int(header.ibpp); 
    std::cout<<"| blk_height: "<<blk_height; 
    std::cout<<"| blk_width: "<<blk_width; 
    std::cout<<"| img_height: "<<img_height; 
    std::cout<<"| img_width: "<<img_width; 
    std::cout<< std::endl;

    int num_of_channels;
    switch(chroma_mode){
      case CHROMA_MODE_YUV420 :
      case CHROMA_MODE_YUV422 :
      case CHROMA_MODE_YUV444 :
        num_of_channels = 3;
        break;
      case CHROMA_MODE_BAYER :
      case CHROMA_MODE_GRAY :
        num_of_channels = 1;
        break;
      default:
        std::cerr<<"Unknown chroma mode. Quitting"<<std::endl;
        throw 1;
    }

  //variable initiation for decoder loop
    char* block_binary_data;
    // max_block_data_size has to be a bit bigger than actual max size to avoid
    // segfaults due to how the binary is read (binary buffer will read an 
    // additional word)
    size_t max_block_data_size=(blk_height*blk_width*num_of_channels*
                            (1+int(MAX_SUPPORTED_BPP/8)/sizeof(*block_binary_data))+8);
    block_binary_data = new char[max_block_data_size];

  //loop to decode every block

    //create out image object
      if(header.ibpp > MAX_IBPP) {
        std::cerr<<"Compressed image format not supported. Input bpp >"<<MAX_IBPP
              <<"."<<std::endl;
        std::cerr<<"Input bpp:"<< int(header.ibpp)<<std::endl;
        throw 1;
      }
      int img_depth_type = header.ibpp > 8? CV_16U:CV_8U;
      if (header.color_profile==CHROMA_MODE_GRAY){
        dst_img=cv::Mat::zeros(img_height,img_width,CV_MAKETYPE(img_depth_type,1));
      }else{
        dst_img=cv::Mat::zeros(img_height,img_width,CV_MAKETYPE(img_depth_type,3));
      }
    
    cv::Mat block;
    char codec_mode= (chroma_mode==CHROMA_MODE_YUV420 && blk_height==1)? 1 : 0;
    
    //omp_set_num_threads(threads);
    for (uint16_t blk_row = 0; blk_row < blk_rows; ++blk_row) {
    //#pragma omp parallel for

      for (uint16_t blk_col = 0; blk_col < blk_cols; ++blk_col) {
        //map a range of full image to block (they share image data)
        int row_low = blk_row*blk_height;
        int row_high = MIN((blk_row+1)*blk_height,dst_img.rows);
        int col_low = blk_col*blk_width;
        int col_high = MIN((blk_col+1)*blk_width,dst_img.cols); 
        block=dst_img(cv::Range(row_low,row_high),cv::Range(col_low,col_high)); 

        //get block id and length of the generated binary
          struct block_header block_header;
          binary_in_file.read((char*)&(block_header),sizeof(block_header));


        //get binary
          binary_in_file.read(block_binary_data, block_header.size);
          decode_core((unsigned char*)block_binary_data,block,chroma_mode,
                                    fix_predictor, NEAR,ee_buffer_size,header.ibpp
                                    ,codec_mode);
        


      }
    }


  if(scale_depth && header.ibpp !=8 && header.ibpp!=16) {
    int bit_increase = header.ibpp<8? 8-header.ibpp:16-header.ibpp ;
    std::cout<< "Scaling image depth up "<<bit_increase<<" bits"<<std::endl;
    double scale_factor = pow(2,bit_increase);
    dst_img.convertTo(dst_img,-1,scale_factor);
  }
  delete[] block_binary_data;
  return 0;
}

