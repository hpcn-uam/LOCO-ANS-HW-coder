# -*- coding: utf-8 -*-
# ==================================================================================================
# -----------------------------------------------------------------------------
# Copyright (c) 2018. All rights reserved
# -----------------------------------------------------------------------------
#  @Filename      : LOCOANSOverlay.py
#  @Author        : Tobías Alonso
#  @Email         : tobi.alonso@gmail.com
#  @Created On    : 2021-07-26 10:30:13
#  @Description   :
# 
# 
# 
#  Last Modified : 2021-08-03 19:45:07 
# 
#  Revision      : 
# 
#  
# ==================================================================================================

from pynq import Overlay, allocate
import subprocess
import numpy as np
from pynq.ps import Clocks
import time 
import cv2


class LOCOANSOverlay(Overlay):
    def __init__(
        self,
        bitfile_name,
        device=None,
        download=True,
        rand_seed=0
    ):
#         super().__init__(bitfile_name, download=download, device=device)
        super().__init__(bitfile_name, download=download)
        self.idma = self.idma_0
        self.odma = self.LOCO_ANS_Encoder.odma_0
        try:
            self.decorrelator = codec.LOCO_ANS_Encoder.LOCO_decorrelator_0
            self.near_support=True
        except:
            self.decorrelator = codec.LOCO_ANS_Encoder.LOCO_decorrelator_LS_0
            self.near_support=False
        self.BLK_HEADER_BYTES = 4
        
        self.in_buf = np.array([])
        self.out_buf = np.array([])
        np.random.seed(rand_seed)
        self.compressed_img =  "/run/user/1000/compressed_image.jls_ans"
        self.decoded_img =  ""

        self._near = 0
        self.input_img = None

        # TODO: get the next parameters from hardware
        self.num_of_vertical_tiles = 1
        self.ee_buffer_exp = 6
    
    
    def obj_status(self,obj):
        status_msj=""
        status = obj.read(0x00)
        
        if status &0x2:
            status_msj+="Done, "

        if status &0x4:
            status_msj+="Idle, "

        if status &0x8:
            status_msj+="Ready, "
        return status_msj
            
    def print_codec_status(self):
        modules = ["idma", "odma", "decorrelator"]
        for module in modules:
            obj = getattr(self,module)
            status_msj = self.obj_status(obj)
            print(module,": ",status_msj)
   
    @property
    def near(self):
        return self._near
    
    @near.setter
    def near(self,new_near):
        assert self.near_support or new_near == 0, "Error: near setting (new near =%d) is not supported" % new_near
        self._near=new_near  

    
    
    
    @property
    def clock(self):
        return Clocks.fclk0_mhz
    
    @clock.setter
    def clock(self,clk_in_mhz):
        Clocks.fclk0_mhz=clk_in_mhz
        
    def allocate_buffers(self,in_block_size):
        in_buf_size = in_block_size
        out_buf_size = 4+in_block_size*10
        if len(self.in_buf)<in_buf_size:
            del self.in_buf
            self.in_buf = allocate(in_block_size, dtype=np.uint8)
        
        if len(self.out_buf)<out_buf_size:
            del self.out_buf
            self.out_buf = allocate(out_buf_size, dtype=np.uint8)

    def load_image(self,img=None):
        if img is None:
            img = self.input_img

        assert not img is None, "No image to load"

        img = img.flatten()
        num_of_px = len(img)
        self.allocate_buffers(num_of_px)
        self.in_buf[:num_of_px] = img
        self.in_buf.flush()

    def load_image_from_path(self,image_path):
        self.input_img = cv2.imread(image_path,cv2.IMREAD_GRAYSCALE)
        self.load_image(self.input_img)

    def load_constant_image(self,rows,cols,val = 120):
        self.input_img = np.zeros((rows,cols),dtype=np.uint8)+val
        self.load_image(self.input_img)

    def load_random_image(self,rows,cols,seed = None):
        if not seed is None:
            np.random.seed(seed)

        self.input_img = np.random.randint(0,256,size=(rows,cols),dtype=np.uint8)
        self.load_image(self.input_img)
    
    def print_outbuf(elems=30):
        if elem == -1:
            print("Complete outbuf")
            print( " ".join(["%02X" % x for x in codec.out_buf] ))
        else:
            print("First %d elements of outbuf" %elems)
            print( " ".join(["%02X" % x for x in codec.out_buf[:elems]] ))
    
    def config_hw(self,cols=None,rows=None):
        if cols is None or rows is None:
            rows,cols = self.input_img.shape
        assert not (rows is None or cols is None), "Missing image size data"

        in_block_size = cols*rows
        self.idma.write(0x10,self.in_buf.device_address)
        self.idma.write(0x1c,in_block_size)

        self.odma.write(0x10,self.out_buf.device_address)

        # config LOCO decorrelator 
        self.decorrelator.write(0x10,cols)
        self.decorrelator.write(0x18,rows)
        if self.near_support:
            self.decorrelator.write(0x20,self._near)

    def print_odma_status(self):

        status = self.odma.read(0x00)
        if status &0x2:
            print("Done")

        if status &0x4:
            print("Idle")

        if status &0x8:
            print("Ready")
            
    def odma_done(self):
        return (self.odma.read(0x00)&0x2) != 0
            
    def run_hw(self,blocking=False):
        self.idma.write(0,1)
        self.decorrelator.write(0,1)
        self.odma.write(0,1)
        if blocking:
            # first while to avoid query before done goes down (it was happening)
            while(not self.odma_done()): 
                pass
            while(self.odma_done()):
                pass

    def generate_global_header(self):
        global_header = np.zeros(12,dtype=np.uint8)
        img_height,img_width = self.input_img.shape
        # global_header[0] |= 0 #predictor
        # global_header[0] |= 0<<2 #color_profile
        global_header[0] |= 2<<6 #version

        global_header[1] = self.ee_buffer_exp #(// buffer_size = 32* 2^ee_buffer_exp). 

        global_header[2] = 8 #ibbp

        global_header[3] = self._near #near

        #blk_height
        blk_height = img_height
        global_header[4] = blk_height &0xFF
        global_header[5] = (blk_height >>8)&0xFF

        #blk_width
        blk_width= img_width//self.num_of_vertical_tiles
        assert img_width//self.num_of_vertical_tiles == img_width/self.num_of_vertical_tiles,(
            "blocks of different sizes are not supported by the header")
        global_header[6] = blk_width &0xFF
        global_header[7] = (blk_width >>8)&0xFF

        #img_height
        global_header[8] = img_height &0xFF
        global_header[9] = (img_height>>8)&0xFF
        
        #img_width
        global_header[10] = img_width &0xFF
        global_header[11] = (img_width >>8)&0xFF

        return global_header

    def store_out_binary(self,compressed_img=None):
        if compressed_img is not None:
            self.compressed_img = compressed_img

        out_data = self.generate_global_header()
        block_bytes = self.get_out_binary_bytes()+self.BLK_HEADER_BYTES
        out_data = np.append(out_data,self.out_buf[:block_bytes])
        out_data.tofile(self.compressed_img)

    def get_out_binary_bytes(self):
        
        output_bytes = 0
        for i in range(self.BLK_HEADER_BYTES):
            output_bytes |= self.out_buf[i] <<(8*i)
        return output_bytes
    
    def get_bpp(self):
        # Adding header's 12 bytes 
        return (self.get_out_binary_bytes()+12)*8/self.input_img.flatten().size
        
    def print_out_binary(self):
        
        print("Out bytes:")
        offset = self.BLK_HEADER_BYTES
        output_bytes = self.get_out_binary_bytes()
        for i in range(offset,offset+output_bytes):
            print("{:4d} ({:4d}): {:02X}".format(i,i-offset,self.out_buf[i]))

    def check_out_binary(self):
        # first sanity check
        
        assert self.get_out_binary_bytes() < (len(self.out_buf)-self.BLK_HEADER_BYTES)
        
        self.store_out_binary()

        self.decoded_img =  "/run/user/1000/decoded_image.pgm"
        # decode image with sw codec
        cmd =["./loco_ans_codec/loco_ans_codec"]
        cmd += ["1"] # decode mode
        cmd += [self.compressed_img ]
        cmd += [self.decoded_img]
        process = subprocess.run(cmd,  stdout=subprocess.PIPE , stderr=subprocess.PIPE)

        assert process.returncode == 0, "Software decoder Failed"
        
        #load decoded image
        rx_img = cv2.imread(self.decoded_img ,cv2.IMREAD_GRAYSCALE)
        #compare to input_img taking into account self._near
        diffimg = cv2.absdiff(self.input_img,rx_img)
        max_error =  max(diffimg.flatten())
        correct= self._near >= max_error

        return correct, max_error
        
    def run_test(self,image_path=None,random_image=False,cols=None,rows=None,run_iterations = 5):   
        perf_times = dict()
        t1 = time.perf_counter()
        if not image_path is None:      
            self.load_image_from_path(image_path)
        else:
            assert not(cols is None or rows is None),"Error: Need to provide image size to generate image"
            if random_image:
                self.load_random_image(rows,cols)
            else:
                self.load_constant_image(rows,cols)
        t2= time.perf_counter()
        perf_times["load_image"] = t2-t1
        
        t1 = time.perf_counter()
        self.config_hw()
        t2= time.perf_counter()
        perf_times["config_hw"] = t2-t1
        
        
        t1 = time.perf_counter()
        for i in range(run_iterations):
            self.run_hw(blocking=True)
        t2= time.perf_counter()
        perf_times["run_hw"] = (t2-t1)/run_iterations
        
        self.out_buf.invalidate()
        
        # check output
        success = self.check_out_binary()
        
        
        return success, perf_times
    