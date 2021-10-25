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
#  Last Modified : 2021-08-11 15:14:56 
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
from warnings import warn

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
        self.idma = list()
        self.odma = list()
        self.decorrelator = list()

        self.in_buf = list()
        self.out_buf = list()

        #check get coder. It can support near >0 or be lossless only
        try:
            aux_deco = self.LOCO_ANS_Encoder.LOCO_decorrelator_0
            self.near_support=True
            self.max_near = 255
        except:
            aux_deco = self.LOCO_ANS_Encoder.LOCO_decorrelator_LS_0
            self.near_support=False
            self.max_near = 0

        self.num_of_vertical_tiles = 0

        while True:
            tile_post="_%d" % self.num_of_vertical_tiles
            try:
                self.idma += [getattr(self,"idma"+tile_post) ]
                self.odma += [getattr(self.LOCO_ANS_Encoder,"odma"+tile_post) ]

                if self.near_support:
                    self.decorrelator += [getattr(self.LOCO_ANS_Encoder,"LOCO_decorrelator"+tile_post) ]
                    
                else:
                    self.decorrelator += [getattr(self.LOCO_ANS_Encoder,"LOCO_decorrelator_LS"+tile_post) ]

                self.in_buf += [np.array([])]
                self.out_buf += [np.array([])]
                self.num_of_vertical_tiles +=1

            except:
                break

        self.tiles_in_use = self.num_of_vertical_tiles
        self.BLK_HEADER_BYTES = 4 
        
        np.random.seed(rand_seed)
        self.compressed_img =  "/run/user/1000/compressed_image.jls_ans"
        self.decoded_img =  ""

        self._near = 0
        self.input_img = None

        # TODO: get the next parameters from hardware
        
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
            for i in range(self.num_of_vertical_tiles):
                status_msj = self.obj_status(obj[i])
                print(module," %d: " %i,status_msj)
   
    @property
    def near(self):
        return self._near
    
    @near.setter
    def near(self,new_near):
        assert self.near_support or new_near == 0, "Error: near setting (new near =%d) is not supported" % new_near
        self._near=new_near  
            
    @property
    def clock0(self):
        return Clocks.fclk0_mhz
    
    @clock0.setter
    def clock0(self,clk_in_mhz):
        Clocks.fclk0_mhz=clk_in_mhz
        
    @property
    def clock1(self):
        return Clocks.fclk1_mhz
    
    @clock1.setter
    def clock1(self,clk_in_mhz):
        Clocks.fclk1_mhz=clk_in_mhz   
        
    @property
    def clock2(self):
        return Clocks.fclk1_mhz
    
    @clock1.setter
    def clock2(self,clk_in_mhz):
        Clocks.fclk1_mhz=clk_in_mhz
    

        
    def allocate_buffers(self,in_block_size):
        in_buf_size = in_block_size
        out_buf_size = 4+in_block_size*10
        if len(self.in_buf[0])<in_buf_size:
            del self.in_buf
            del self.out_buf
        
        self.in_buf = []
        self.out_buf = []
        for i in range(self.tiles_in_use):
            self.in_buf += [allocate(in_block_size, dtype=np.uint8,cacheable=1 )]
            self.out_buf += [allocate(out_buf_size, dtype=np.uint8,cacheable=1 )]

    def load_image(self,img=None):
        if img is None:
            img = self.input_img

        assert not img is None, "No image to load"

        self.tiles_in_use = self.num_of_vertical_tiles
        rows,cols = img.shape
        assert self.tiles_in_use >= 1
        while cols % self.tiles_in_use != 0:
            self.tiles_in_use-=1

        if self.tiles_in_use != self.num_of_vertical_tiles:
            warn("Using %d of %d tiles for this images due to cols to tiles ratio" % (
                    self.tiles_in_use,self.num_of_vertical_tiles))
        
        cols_per_tile = cols//self.tiles_in_use #This division result is exact
        px_per_tile = cols_per_tile*rows
        self.allocate_buffers(px_per_tile)
        for i in range(self.tiles_in_use):
            sub_img = img[:,i*cols_per_tile:(i+1)*cols_per_tile].flatten()
            self.in_buf[i][:px_per_tile] = sub_img
            self.in_buf[i].flush()

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
    
    def print_outbuf(self,elems=30):
        for i in range(self.tiles_in_use):
            if elem == -1:
                print("Complete outbuf %d" % i)
                print( " ".join(["%02X" % x for x in codec.out_buf[i]] ))
            else:
                print("First %d elements of outbuf %d" %(elems,i))
                print( " ".join(["%02X" % x for x in codec.out_buf[i][:elems]] ))
    
    def config_hw(self,cols=None,rows=None):
        if cols is None or rows is None:
            rows,cols = self.input_img.shape
        assert not (rows is None or cols is None), "Missing image size data"

        assert cols%self.tiles_in_use == 0
        cols_per_tile = cols//self.tiles_in_use #This division result is exact

        in_block_size = cols_per_tile*rows
        for i in range(self.tiles_in_use):
            self.idma[i].write(0x10,self.in_buf[i].device_address)
            self.idma[i].write(0x1c,in_block_size)

            self.odma[i].write(0x10,self.out_buf[i].device_address)

            # config LOCO decorrelator 
            self.decorrelator[i].write(0x10,cols_per_tile)
            self.decorrelator[i].write(0x18,rows)
            if self.near_support:
                self.decorrelator[i].write(0x20,self._near)

    def print_odma_status(self):

        for i in range(self.tiles_in_use):
            status = self.odma[i].read(0x00)
            print("Odma %d: "%i,end="")
            if status &0x2:
                print("Done")

            if status &0x4:
                print("Idle")

            if status &0x8:
                print("Ready")
            
    def odmas_done(self):
        all_done = True
        for i in range(self.tiles_in_use):
            if self.tiles_running[i]:
                all_done = all_done and ((self.odma[i].read(0x00)&0x2) != 0)
        return all_done
            
    def odmas_running(self):
        all_running = True
        for i in range(self.tiles_in_use):
            if not self.tiles_running[i]:
                self.tiles_running[i] = (self.odma[i].read(0x00)&0x2) != 0
                all_running = all_running and self.tiles_running[i]
        return all_running
    
    def run_hw(self,blocking=False):
        self.tiles_running = []
        for i in range(self.tiles_in_use):
            self.idma[i].write(0,1)
            self.tiles_running +=[False]
            self.decorrelator[i].write(0,1)
            self.odma[i].write(0,1)

        if blocking:
            # first while to avoid query before done goes down (it was happening)
            while(not self.odmas_running()): 
                pass
            while(self.odmas_done()):
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
        blk_width= img_width//self.tiles_in_use
        assert img_width %self.tiles_in_use == 0,(
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
        for i in range(self.tiles_in_use):
            block_bytes = self.get_out_binary_bytes(i)+self.BLK_HEADER_BYTES
            out_data = np.append(out_data,self.out_buf[i][:block_bytes])

        out_data.tofile(self.compressed_img)

    def get_out_binary_bytes(self,tile_idx=0):
        
        output_bytes = 0
        for i in range(self.BLK_HEADER_BYTES):
            output_bytes |= self.out_buf[tile_idx][i] <<(8*i)
        return output_bytes
    
    def get_bpp(self):
        # Adding header's 12 bytes 
        blks_bytes = sum( [self.get_out_binary_bytes(i)+self.BLK_HEADER_BYTES 
                                for i in range(self.tiles_in_use)])
        return ( blks_bytes+ 12)*8/self.input_img.size
        
    def print_out_binary(self):
        
        print("Out bytes:")
        offset = self.BLK_HEADER_BYTES
        for i in self.tiles_in_use:
            output_bytes = self.get_out_binary_bytes(i)
            for b in range(offset,offset+output_bytes):
                print("{:4d} ({:4d}): {:02X}".format(b,b-offset,self.out_buf[i][b]))

    def check_out_binary(self):
        # first sanity check
        for i in range(self.tiles_in_use):
            assert self.get_out_binary_bytes(i) < (len(self.out_buf[i])-self.BLK_HEADER_BYTES)
        
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
        
        for i in range(self.tiles_in_use):
            self.out_buf[i].invalidate()
        
        # check output
        success = self.check_out_binary()
        
        
        return success, perf_times