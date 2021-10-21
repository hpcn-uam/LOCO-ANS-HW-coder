# LOCO-ANS codec source code

## Usage
  Args: encode(0)/decode(1) args

### Encode 
command: ./loco_ans_codec 0 src_img_path out_compressed_img_path [NEAR] [encode_mode] [blk_height] [blk_width]   \n");

Args:
- src_img_path: input image to encode
- out_compressed_img_path: path to output encoded image
- NEAR (optional. Default: 0) : Maximum allowed error in the space domain.
- encode_mode (optional. Default: 0) : 
  - 0: Encode only
  - 1: Encode and analysis (get entropy,mean iterations,..)


- blk_height (optional. Default: image height) : the image can be coded on blocks blk_height tall
- blk_width (optional. Default: image width) : the image can be coded on blocks blk_width wide

### Decode 
command: ./opt_jpegls_codec 1 compressed_img_path path_to_out_image  

Args:
- compressed_img_path: path to input encoded image
- path_to_out_image: path to output decoded image (pgm for single channel and ppm for RGB are recommended )

## Build
Run 'make'

Prerequisites:
- OpenCV C++ lib and devs (any version >=2.4 should be fine)


## Code
The main encode and decode procedures are coded in: codec_core.cc
functions:
- image_scanner:  encodes input image block
- binary_scanner: decodes input binary block


## Change ANS coder parameters
Use the jupiter notebook coder_config_gen.ipynb under repo_root/notebooks to select the coder parameters and generate the configuration file and ANS tables.