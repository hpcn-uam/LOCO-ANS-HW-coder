import numpy as np

def Uniform_quantizer(x,size,low,high):
    assert low<high
    assert size > 0
    assert size == int(size)
    assert low <= x <=high
    
    R =high-low
    bin_size = R/size
    idx = np.floor((x-low)/bin_size)
    return low + bin_size*(idx+.5)


def Uniform_quantizer_mean_low_complexity(Acc,t,precision,max_idx=None,centered = True ):
    ''' Valid for positive values. 
    Conceived to obtain a probability of Bernulli trials where Acc is the sum of experiments
    trials at time t.
    When centered = True, first and last bins (if max_idx is set to 2**precision) have double 
    of the precision. In this case the num of reconstruction values is not a power of 2, but 2**n+1.
    For a coder, this is not a problem as the center idx corresponds to p=.5, then the outcome of
    the trial doesn't need to be coded.
    
    '''
    up_bound = (t>>1) if centered else t 
    val = Acc*2**precision 
    idx = 0
    while val > up_bound:
        idx+=1
        up_bound += t
        if max_idx is not None and max_idx == idx:
            break
    
    if centered:
        rec_val = 2**-(precision+2) if idx == 0 else idx/2**precision 
        if max_idx is not None and max_idx == idx:
            rec_val -= 2**-(precision+2)
    else:
        rec_val = 2**-(precision+1) + idx/2**precision
    return idx, rec_val

def Constant_ratio_quantizer_size(ratio,Initial_val,Max_number):
    assert 0< ratio < 1
    assert Initial_val>0
    assert Max_number > 0 
    lg2 = np.log2
    f = ratio
    x1=Initial_val
    M = Max_number
    return np.floor((lg2(x1)-lg2(M))/(2*lg2(f)) +3/2)

def Const_ratio_quantizer(x, ratio=1/2**.5,Initial_val=2**-4):
    '''Quantizer of real numbers >=0 such that reconstruction values are always
    x*ratio <= Q(x) < x/ratio.
    '''
    assert 0< ratio < 1
    assert Initial_val>0
    assert x > 0
    n = Constant_ratio_quantizer_size(ratio,Initial_val,x)
    return Initial_val/ratio**(2*(n-1))

def Constant_ratio_quantizer_reconstruction_values(ratio,Initial_val,Max_number):
    assert 0< ratio < 1
    assert Initial_val>0
    assert Max_number > 0 
    
    lg2 = np.log2
    f = ratio
    x1=Initial_val
    M = Max_number
    xn = x1
    quants = [xn]
    while(xn<M):
        xn = xn/f**2
        quants += [xn]

    return quants



## sequencial quantization functions
def Simplified_const_ratio_quantizer(acc,t,Initial_val_exp=-4):
    '''Constant_ratio_quantizer_reconstruction_values with all bounds in powers of 2
    the first bound is at 2**(Initial_val_exp)
    It simulates the behav of parameter estimation in JPEGLS.
    The algorithm computes, acc/t and quantizes it using the mentioned boundaries.
    JPEG-LS has Initial_val_exp=0, here negative values are also allowed.
    '''
    n = 0
    simp_ratio = 1/2**.5
    p = Initial_val_exp
    if p <0:
        acc = acc*2**(-p) #acc<<(-p)
        p=0
    while acc > (t<<(p+n)):
        n+=1
    return n,(1<<n)*2**Initial_val_exp*simp_ratio


def Uniform_sequential_quantizer(Acc,t, x, new_val,precision, reset_exponent = 5 ):
    reset = 1<<reset_exponent
    Acc += (new_val<<precision)-x
    t += 1
    Li =-((t+1)>>1); 
    Ls = (t>>1); 

    if (Li >= Acc):
#         print("Below")
        x-=1
        Acc += t
    elif (Acc > Ls):
#         print("Above")
        x+=1
        Acc -= t 
        
    if t >= reset:
        t >>= 1
        Acc /=2
    return Acc,t,x

def kld_minimizing_rec_value_for_const_ratio_uniform(l,h):
    lg = np.log2
    C = lg(1-l) - lg(1-h) + l/(1-l)*lg(l) - h/(1-h)*lg(h)
    C1 = C/(l/(1-l)-h/(1-h))
    return 2**C1

def Simplified_const_ratio_uniform_quantizer(acc,t,Initial_val_exp=-4):
    '''Constant_ratio_quantizer_reconstruction_values with all bounds in powers of 2
    the first bound is at 2**(Initial_val_exp)
    It simulates the behav of parameter estimation in JPEGLS.
    The algorithm computes, acc/t and quantizes it using the mentioned boundaries.
    JPEG-LS has Initial_val_exp=0, here negative values are also allowed.
    '''
    n = 0
    simp_ratio = 1/2**.5
    p = Initial_val_exp
    if p <0:
        acc = acc*2**(-p) #acc<<(-p)
        p=0
    
    l = t
    while acc > l:
        n+=1
        l<<=1
    idx = n<<1
    if acc > (l - (l>>2) ):
        idx +=1
    
    # reconstruction
    #low_bound
    if (0x01&idx) >0:
        low_b = 3*2**((idx>>1)-2 + Initial_val_exp)
    else:
        low_b = 1*2**((idx>>1)-1 + Initial_val_exp)
        
    #high bound
    idx_h = idx +1
    if (0x01&idx_h) >0:
        high_b = 3*2**((idx_h>>1)-2 + Initial_val_exp)
    else:
        high_b = 1*2**((idx_h>>1)-1 + Initial_val_exp)
    
    rx = kld_minimizing_rec_value_for_const_ratio_uniform(low_b/(low_b+1),high_b/(high_b+1))
    qSt = rx/(1-rx)

    return idx, qSt

if __name__ == '__main__':
    def Test_Uniform_sequential_quantizer():
        print("Test_Uniform_sequential_quantizer:" ,end="")
        np.random.seed(0)
        precision = 3
        factor = 2**precision


        iter_exponent= 8
        for ratio in [.1,.3,.5,.7]:
            for init_sum in [0,1,3]:
                # init
                t =5
                x = int(init_sum/t*2**precision+1/2)

                test_t = t
                test_Acc = init_sum


                assert (x-.5)/factor <= test_Acc/test_t  < (x+.5)/factor
                Acc = init_sum*factor- x*t

                for i in range(1<<iter_exponent -t-1):
                    new_val = 1 if np.random.rand()<ratio else 0

                    Acc,t, x = Uniform_sequential_quantizer(Acc,t, x, new_val,precision , reset_exponent = iter_exponent )

                    test_Acc += new_val
                    test_t += 1
                    assert np.isclose(test_Acc/test_t, (Acc+ t*x)/t/factor)
                    assert (x-.5)/factor < test_Acc/test_t  <= (x+.5)/factor, (
                            "Failed for new_val={}, x={} ,t={}, Acc ={} | condition {:.4f} <= {:.4f} < {:.4f}  is not true".format(new_val,x,t,Acc,(x-.5)/factor , test_Acc/test_t  , (x+.5)/factor) )
                    assert x >=0

        print(" OK")

    Test_Uniform_sequential_quantizer()
    
    
    def Test_Uniform_sequential_quantizer_2():
        print("Test_Uniform_sequential_quantizer_2:" ,end="")
        np.random.seed(0)
        precision = 3
        factor = 2**precision

        # init
        init_sum = 10
        t =20
        x = int(init_sum/t*2**precision+1/2)
        Acc = init_sum*factor- x*t

        assert (x-.5)/2**precision < (Acc+ t*x)/t/factor  <= (x+.5)/2**precision


        for i in range(100):
            new_val = 1 if np.random.rand()<0.3 else 0
            Acc,t, x = Uniform_sequential_quantizer(Acc,t, x, new_val,precision , reset_exponent = 5 )

            assert (x-.5) < (Acc+ t*x)/t  <= (x+.5), (
                    "Failed for new_val={}, x={} ,t={}, Acc ={} | condition {:.4f} <= {:.4f} < {:.4f}  is not true".format(new_val,x,t,Acc,(x-.5)/factor , (Acc+ t*x)/t/factor  , (x+.5)/factor) )
            assert x >=0

        print(" OK")

    Test_Uniform_sequential_quantizer_2()
    
    def Test_Simplified_const_ratio_quantizer():
        print("Test_Simplified_const_ratio_quantizer:" ,end="")
        Initial_val_exp = -4
        simp_ratio = 1/2**.5
        np.random.seed(0)
        for i in range(5):
            for t in [1,4,64,2000]:
                val = np.random.rand()*200
                qval = Const_ratio_quantizer(val, ratio=simp_ratio ,Initial_val=2**Initial_val_exp*simp_ratio)
                qval_s = Simplified_const_ratio_quantizer(val*t,t,Initial_val_exp)[1]
                assert np.isclose(qval_s,qval),("Fails for: val={}({}), t={} | qval_s= {} , qval={}".format(val,i,t,qval_s,qval)
                                                + "| Bound:"+str(min(qval,qval_s)/simp_ratio)
                                               )
        print(" OK")

    Test_Simplified_const_ratio_quantizer()