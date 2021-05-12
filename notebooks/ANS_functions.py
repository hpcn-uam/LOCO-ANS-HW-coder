import numpy as np
from random import seed
from random import random
from warnings import warn

def print_War(text):
    print("\x1b[31m " +text + "\x1b[0m")
    
def get_class_table(probs,I):

    acc_probs = sum(probs)
    if acc_probs>1:
        print("Error: the sum of the prob vector should be <=1 (last symb prob is the diff)")
        raise
    elif not np.isclose(acc_probs,1):
        probs = probs + (1-acc_probs,)
        
    src_card = len(probs)
    num_of_slots = I[1]-I[0]+1
    max_states_per_symb = np.ceil(np.array(probs)*num_of_slots )
    
    #init
    symb_cnt = [0]*src_card
    priority = 1/(2*np.array(probs))
    
    table = []
    missing_symbols = {x for x in range(len(probs))} - set(table)
    for i in range(num_of_slots):
        remaining_slots = num_of_slots - i
        
        #print(priority)
        symb_min = np.argmin(priority)
        min_vec = priority == priority[symb_min]
        if sum(min_vec) >1:
            aux = [p if ismin else np.inf for p,ismin in zip(probs,min_vec)]
            symb_min = np.argmin(aux)
        
        if len(missing_symbols) == remaining_slots and symb_min not in missing_symbols:
            break

        table += [symb_min]
        symb_cnt[symb_min] += 1
        if symb_cnt[symb_min] < max_states_per_symb[symb_min]:
            priority[symb_min] += 1/probs[symb_min]
        else:
            priority[symb_min] = np.inf
        
        missing_symbols = {x for x in range(len(probs))} - set(table)
        
    
    # Did every symbol get at least 1 state?
    missing_symbols = {x for x in range(len(probs))} - set(table)
    if len(missing_symbols)> 0:
        table += list(missing_symbols)
        print_War(
            "Suboptimal table for source:"+str(probs) 
            + "\nHeuristic algorithm wasn't able to include the following symbols:"+ str(missing_symbols))
        
    
    assert len(table) == num_of_slots

        
    return table*2

def Compile_ANS_table(table,I):
    if len(table) < I[1]-1:
        print("Error: Table is too short")
        print("  Remember to deliver the table upto state ", I[1],
                  ".Not just I range ")
        raise
    
    #create decoding table
    tANS_d = []
    symb_cnts ={}
    symb_list = set()
    for symb in table:
        if symb in symb_cnts:
            symb_cnts[symb] += 1
        else:
            symb_cnts[symb] = 0
            symb_list.add(symb)
            
        prev_state = symb_cnts[symb]
        tANS_d += [(symb,prev_state)]
    tANS_d = tuple(tANS_d)
    
    #create encoding table
    
    #get Is and list for each symb
    Is_high={}
    state_class={}
    for i in range(len(table)):
        symb = tANS_d[i][0]
        Is_high[symb] = tANS_d[i][1]
        
        if symb in state_class:
            state_class[symb] += [i]
        else:
            state_class[symb] = [i]
    
    tANS_e = {}
    for symb in symb_list:
        enc_table = []
        for state in range(I[0],len(table)):
            out_bits = []
            while Is_high[symb] < state:
                out_bits += [state%2]
                state //=2
            new_state = state_class[symb][state]
            enc_table += [(new_state, len(out_bits))]
        tANS_e[symb] = tuple(enc_table)
        
    return tANS_e,tANS_d

def ANS_encode(tANS_e=None,state_off=None,sym_source=None,verbose=0):
    
    if tANS_e==None or state_off==None or sym_source==None:
        print("Error, need more args")
        
    state = state_off
    out_seq=[]

    for s in sym_source:
        if verbose == 1:
            print(state," ->", end=" ")
        new_state, num_out_bits = tANS_e[s][state-state_off]
        
        out_bits = []
        for i in range(num_out_bits):
            out_bits += [state%2]
            state //=2
            
        out_seq += list(out_bits )
        state = new_state
        
        if verbose == 1:
            print("{} | {:5d} {:10b} |".format(s,state,state),out_bits)
    while state >0:
        out_seq += [state%2]
        state //=2
    
    return out_seq

def ANS_decode(tANS_d=None,state_off = None,bit_seq=None,sym_len=None,symb_seq=None,verbose=0):
    
    if tANS_d==None or state_off==None or bit_seq==None or sym_len==None:
        print("Error, need more args")
    
    state = 0
    deco_symbols = 0
    out_symbs = []
    while len(bit_seq) >0 or state > state_off:
        while state <state_off:
            state = state*2 + bit_seq.pop()
        
        if verbose == 1:
            print(state," ->", end=" ")
        s,state = tANS_d[state]
        
        #verification
        if verbose == 1:
            print("{} | {:5d} {:10b} ".format(s,state,state))      
        if symb_seq !=None and s != symb_seq[-deco_symbols-1]:
            print("Error decoding")
            break
            
        deco_symbols +=1
        out_symbs += [s]
        if deco_symbols == sym_len:
            break
    return out_symbs


 #aux functions

def symb_generator(p,l):
    if sum(p)> 1:
        print("Error: the sum of the prob vector should be <=1 (last symb prob is the diff)")
        raise
    seed(5)
    
    accum_probs = []
    cum_prob = 0
    for symb in range(len(p)):
        cum_prob += p[symb]
        accum_probs += [cum_prob]
        
    for i in range(l):
        rnum = random()
        for symb, acc_p in enumerate(accum_probs):
            if rnum < acc_p:
                yield symb
                break
        else:
            yield len(p)
            
    #test code:
    # symb_cnt = {}
    # num_of_symbs = 100000
    # p = (.2,.4,.15,.15,.1)
    # for s in symb_generator(p,num_of_symbs):
    #     if s in symb_cnt:
    #         symb_cnt[s] += 1
    #     else:
    #         symb_cnt[s] = 1

    # for symb,cnt in symb_cnt.items():
    #     print(symb, cnt/num_of_symbs)

def get_entropy(probs):
    return sum([-p*np.log2(p) for p in probs if p != 0])

def get_tANS_sweet_pnt(tANS_e=None,I=None,sym_len = 10000,higher_probs=(),
                                       test_pnt=None,aprox_point=False,rng_limits=None):
    src_card = len(tANS_e)
    recur_level = len(higher_probs)
    step = 0.01
    min_prob = step
    max_prob = 1-sum(higher_probs)
    
    
    # get probability search space
    if rng_limits is not None:
        min_prob,max_prob,step = rng_limits[recur_level]
        probs = np.arange(min_prob,max_prob,step)
    elif test_pnt is None:
        probs = np.arange(min_prob,max_prob,step)
    else:
        if len(test_pnt) != src_card:
            print("test point must have the same cardinality as the symb source")
            raise
        if not np.isclose(sum(test_pnt),1):
            print("bad test point")
            raise
        if aprox_point:
            rng = .5/(len(tANS_e[0])/src_card)
            step = .25/(len(tANS_e[0])/src_card)/100
            min_prob = step
            min_prob = max(min_prob,test_pnt[recur_level]-rng/2)
            max_prob = min(max_prob,test_pnt[recur_level]+ rng/2)
            probs = np.arange(min_prob,max_prob,step)
        else:
            probs = (test_pnt[recur_level],)
        
    dH=[]
    if recur_level < src_card -2:
        min_dH = np.inf
        for p,i in zip(probs,range(len(probs))):
            if recur_level == 0:
                print("\rProcesing: {:4.1f}%".format((p-min_prob)/(max_prob-min_prob)*100),end="")
            new_pnt = higher_probs + (p,)
            idx,probs,best_dH,dH_l = get_tANS_sweet_pnt(tANS_e,I,sym_len,new_pnt,test_pnt,aprox_point,rng_limits)
            if best_dH < min_dH:
                min_dH = best_dH
                min_idx = (i,) + idx
                min_probs = (p,) + probs

        #dH += [dH_l]
        if recur_level == 0:
            print("\r                      ")
    else:
        for p in probs:
            vp = higher_probs + (p,)
            out_seq = ANS_encode(tANS_e,I[0],symb_generator(vp,sym_len))
            H = get_entropy(vp + (1-sum(vp),) )
            B = len(out_seq)/sym_len
            dH+=[B-H]
        min_idx = np.argmin(dH)
        min_probs = (probs[min_idx],1-sum(higher_probs)-probs[min_idx])
        min_dH = dH[min_idx]
        min_idx = (min_idx,)
    
    
    return min_idx,min_probs,min_dH,dH

def get_ANS_tables(probs, address_size):
    I = (2**(address_size),2**(address_size+1)-1)
    L = I[1]-I[0]+1
    table = get_class_table(probs,I)
    tANS_e,tANS_d = Compile_ANS_table(table,I)
    return tANS_e,tANS_d

def test_probs_4_tANS(probs, address_size):
    #get tANS system
    I = (2**(address_size),2**(address_size+1)-1)
    L = I[1]-I[0]+1
    table = get_class_table(probs,I)
    tANS_e,tANS_d = Compile_ANS_table(table,I)

    min_idx,min_probs,min_dH,dH = get_tANS_sweet_pnt(tANS_e,I,sym_len=1000000,test_pnt=probs)
    #print("Chosen probs:",np.round(min_probs,5)," |dH: ",min_dH)
    return min_dH