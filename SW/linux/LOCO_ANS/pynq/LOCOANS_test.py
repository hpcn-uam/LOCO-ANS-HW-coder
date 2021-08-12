# -*- coding: utf-8 -*-
# ==================================================================================================
# -----------------------------------------------------------------------------
# Copyright (c) 2018. All rights reserved
# -----------------------------------------------------------------------------
#  @Filename      : LOCOANS_test.py
#  @Author        : Tobías Alonso
#  @Email         : tobi.alonso@gmail.com
#  @Created On    : 2021-08-11 15:15:16
#  @Description   :
# 
# 
# 
#  Last Modified : 2021-08-11 15:47:44 
# 
#  Revision      : 
# 
#  
# ==================================================================================================


import argparse
import numpy as np
import cv2
import LOCOANSOverlay
CodecOverlay=LOCOANSOverlay.LOCOANSOverlay
import os

parser = argparse.ArgumentParser(description='LOCO ANS coder test')
parser.add_argument('--bit', dest='bit', default='platforms/platform.bit',help='Platform bit file')
parser.add_argument('--clk0', dest='clk0', default=None,help='Clk 0 new freq',type=float)
parser.add_argument('--clk1', dest='clk1', default=None,help='Clk 1 new freq',type=float)
args = parser.parse_args()


codec = CodecOverlay(args.bit)

if not args.clk0 is None:
    print("Prev. clock 0:",codec.clock0)
    codec.clock0=args.clk0

print("Current. clock 0:",codec.clock0)

if  not args.clk1 is None:
    print("Prev. clock 1:",codec.clock1)
    codec.clock1=args.clk1
    
print("Current. clock 1:",codec.clock1)

print("*** Platform Info ***")
print("Max near:", codec.max_near)
print("Num. of tiles:", codec.num_of_vertical_tiles)


print("\n#############  Code image set   #############")

iters = 1
img_dir="./images/"
max_near= 4
max_near = max_near if max_near<= codec.max_near else codec.max_near
for file in os.listdir(img_dir):
    if not (file.endswith(".pgm")):
        continue
    image_name = str(file)
    image_path = img_dir+ image_name
    input_img = cv2.imread(image_path,cv2.IMREAD_GRAYSCALE)
    rows,cols = input_img.shape
    print("Image: ",file,"| Size ",rows,"x",cols)
    
    for near in range(max_near+1):
        print("\tNear ",near," | ",end="")
        codec.near = near
        status,time_prof = codec.run_test(image_path,iters)
        correct, max_error = status
        assert correct,"Error running test using file %s with near %d: peak error: %d " % (
                image_name,near, max_error)

        img_pxs= codec.input_img.flatten().size
        bpp=codec.get_bpp() 
        print("Bw: %f Mpixels/s | bpp: %5.3f" % (img_pxs/ (time_prof["run_hw"]*10**6),bpp))
        



print("\n#############  Code Constant images (Best case scenario)  #############")

iters = 1
codec.near = 0
img_dir="./images/"
for rows,cols in [(10,10),(100,100),(1000,1000),(1080,2048)]:
    print("Size ",rows,"x",cols)
    status,time_prof = codec.run_test(cols=cols,rows=rows,random_image=False,run_iterations=iters)
    correct, max_error = status
    assert correct,"Error running test using size %s with near %d: peak error: %d " % (
            image_nsizeame,near, max_error)
    
    img_pxs= codec.input_img.flatten().size
    bpp=codec.get_bpp() 
    print("\tBw: %f Mpixels/s | bpp: %5.3f" % (img_pxs/ (time_prof["run_hw"]*10**6),bpp))
    

print("\n#############  Code Random images (uniformly distributed pixels, worst case scenario)   #############")

iters = 1
max_near=4
img_dir="./images/"
np.random.seed(0)
max_near = max_near if max_near<= codec.max_near else codec.max_near
for rows,cols in [(10,10),(100,100),(1000,1000),(1080,2048)]:
    print("Size ",rows,"x",cols)
    for near in range(max_near+1):
        print("\tNear ",near," | ",end="")
        codec.near = near
        status,time_prof = codec.run_test(cols=cols,rows=rows,random_image=True,run_iterations=iters)
        correct, max_error = status
        assert correct,"Error running test using size %s with near %d: peak error: %d " % (
                image_nsizeame,near, max_error)

        img_pxs= codec.input_img.flatten().size
        bpp=codec.get_bpp() 
        print("Bw: %f Mpixels/s | bpp: %5.3f" % (img_pxs/ (time_prof["run_hw"]*10**6),bpp))
