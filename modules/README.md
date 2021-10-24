# Platform modules sources

High Level Synthesis code to create and test the encoder accelerator system

![alt text](../images/Hw_test_block_diagram.svg "LOCO-ANS encoder hardware accelerator test platform")

## Encoder arch:

![alt text](../images/LOCO_HW_block_diag.png "Mean bits per pixel (bpp) obtained by JPEG-LS, JPEG-LS without run mode and LOCO-ANS")


### Pixel Decorrelator pipeline
![alt text](../images/Decorrelator_pipeline.png )



### Double lane TSG coder Hierarchy 

This module can receive the output of two independent Pixel Decorrelators and process it without clock penalties

![alt text](../images/TSG_coder_block_diagram.png )


#### Input buffers

![alt text](../images/input_buffers_block_diagram.png )


#### Subsymbol Generator
![alt text](../images/subsymb_gen.png )

#### tANS coder
![alt text](../images/ANS_coder_block_diagram.png )




