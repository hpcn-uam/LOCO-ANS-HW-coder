from pynq import Overlay, allocate
import subprocess
import numpy as np
from pynq.ps import Clocks
import time 

class TSGCoderOverlay(Overlay):
    def __init__(
        self,
        bitfile_name,
        device=None,
        download=True,
    ):
#         super().__init__(bitfile_name, download=download, device=device)
        super().__init__(bitfile_name, download=download)
        self.idma = self.idma_TSG_0
        self.odma = self.odma_0
        self.HEADER_BYTES = 4
        
        self.in_buf = np.array([])
        self.out_buf = np.array([])
        
    @property
    def clock(self):
        return Clocks.fclk0_mhz
    
    @clock.setter
    def clock(self,clk_in_mhz):
        Clocks.fclk0_mhz=clk_in_mhz
        
    def allocate_buffers(self,in_block_size):
        in_buf_size = in_block_size
        out_buf_size = 4+in_block_size*2
        if len(self.in_buf)<in_buf_size:
            del self.in_buf
            self.in_buf = allocate(in_block_size, dtype=np.uint32)
        
        if len(self.out_buf)<out_buf_size:
            del self.out_buf
            self.out_buf = allocate(out_buf_size, dtype=np.uint8)
        
    def generate_test_in_block(self,block_size,blk_idx=0):
        NUM_ANS_THETA_MODES = (15)
        NUM_ANS_P_MODES = (32) 

        INPUT_BPP =  (8)
        Z_SIZE =  (INPUT_BPP-1)
        LOG2_Z_SIZE =  (3)
        Y_SIZE =  (1)
        THETA_SIZE =  (5) #32 tables
        P_SIZE =  (5) #32 tables
        REM_RED_BITS_SIZE = 3


        input_vector = np.zeros(block_size, dtype=np.uint32)

        def GET_MASK(x):
            return ((1<<x)-1) 

        for i in range(block_size):
            last = 1 if (i == block_size-1) else 0 ;
            val = i;
            z = val&0x1 if blk_idx <= 3 else val & 0xF if (blk_idx <= 5) else val & 0x7F ;
            y = 1 if val & 0x80 else 0 ; 
            theta_id = NUM_ANS_THETA_MODES-1 if i >= NUM_ANS_THETA_MODES else i ;
            p_id = blk_idx//2 ;

            dma_data = 0;
            acc_bits = 0;
            dma_data = p_id & GET_MASK(P_SIZE);
            acc_bits +=P_SIZE;

            dma_data |= (theta_id & GET_MASK(THETA_SIZE))<<acc_bits;
            acc_bits +=THETA_SIZE;

            dma_data |= (y & GET_MASK(Y_SIZE))<<acc_bits;
            acc_bits +=Y_SIZE;

            dma_data |= (z & GET_MASK(Z_SIZE))<<acc_bits;
            acc_bits +=Z_SIZE;
              
            dma_data |= (Z_SIZE & GET_MASK(REM_RED_BITS_SIZE))<<acc_bits;
            acc_bits +=REM_RED_BITS_SIZE;
            
            # last bit
            dma_data |= (last & GET_MASK(1))<<acc_bits;
            acc_bits +=1;

            input_vector[i]=dma_data;


        return input_vector
    
    def config_hw(self,in_block_size):
        self.idma.write(0x10,self.in_buf.device_address)
        self.idma.write(0x1c,in_block_size)

        self.odma.write(0x10,self.out_buf.device_address)
      
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
        self.odma.write(0,1)
        if blocking:
            # first while to avoid query before done gones down (it was happening)
            while(not self.odma_done()): 
                pass
            while(self.odma_done()):
                pass
        
    def store_out_binary(self,outfile=None):
        if outfile is None:
            outfile = "/run/user/1000/out_bytes.loco_ans"
        
        # remove fixt pixel value
        new_out_bytes=self.get_out_binary_bytes()-1
        out_binary = np.zeros(4, dtype=np.uint8)
        for i in range(self.HEADER_BYTES):
            out_binary[i] =  new_out_bytes>>(8*i)
        out_binary = np.append(out_binary,self.out_buf[5:])
        out_binary.tofile(outfile)
    
    def get_out_binary_bytes(self):
        
        output_bytes = 0
        for i in range(self.HEADER_BYTES):
            output_bytes |= self.out_buf[i] <<(8*i)
        return output_bytes
    
    def print_out_binary(self):
        
        print("Out bytes:")
        offset = self.HEADER_BYTES
        output_bytes = self.get_out_binary_bytes()
        for i in range(offset,offset+output_bytes):
            print("{:4d} ({:4d}): {:02X}".format(i,i-offset,self.out_buf[i]))

    def check_out_binary(self,block_size,block_idx):
        # first sanity check
        assert self.get_out_binary_bytes() < (len(self.out_buf)-self.HEADER_BYTES)
        
        #use C++ checker to validate output binary
        outfile = "/run/user/1000/out_bytes.loco_ans"
        self.store_out_binary(outfile=outfile)
        cmd =["/home/xilinx/loco_ans/tsg_coder_test/TSG_checker/TSG_checker"]
        cmd += [outfile]
        cmd += [str(block_size)]
        cmd += [str(block_idx)]
        process = subprocess.run(cmd,  stdout=subprocess.PIPE , stderr=subprocess.PIPE)
        return process.returncode == 0, process
        
    def run_test(self,block_size = 2048,block_idx = 0):      
        perf_times = dict()
        t1 = time.perf_counter()
        self.allocate_buffers(block_size)
        t2= time.perf_counter()
        perf_times["allocate_buffers"] = t2-t1
        
        t1 = time.perf_counter()
        self.in_buf[:block_size] = self.generate_test_in_block(block_size,block_idx)
        self.in_buf.flush()
        t2= time.perf_counter()
        perf_times["generate_test_in_block"] = t2-t1
        
        t1 = time.perf_counter()
        self.config_hw(block_size)
        t2= time.perf_counter()
        perf_times["config_hw"] = t2-t1
        
        
        t1 = time.perf_counter()
        for i in range(5):
            self.run_hw(blocking=True)
        t2= time.perf_counter()
        perf_times["run_hw"] = (t2-t1)/5
        
        self.out_buf.invalidate()
        
        # check output
        success, process = self.check_out_binary(block_size,block_idx)
        
        
        
        return success, perf_times
        