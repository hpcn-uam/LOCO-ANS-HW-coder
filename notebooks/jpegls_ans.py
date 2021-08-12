
import probability_functions as prob_func
import ANS_functions as ANS
import Quantization_functions as qfunc # my quantization functions
import numpy as np


template_config = """/*
 *
 *      Author: Tobías Alonso
 */

#ifndef CODER_CONFIG_H
#define CODER_CONFIG_H

#include "codec_core.h"

//quantizers
#define CTX_ST_FINER_QUANT ($CTX_ST_FINER_QUANT$) 
#define CTX_ST_PRECISION ($CTX_ST_PRECISION$) // number of fractional bits
#define MAX_ST_IDX  ($MAX_ST_IDX$)//31//25 //(12)

#define CTX_NT_CENTERED_QUANT ($CTX_NT_CENTERED_QUANT$)
#define CTX_NT_PRECISION ($CTX_NT_PRECISION$) // number of fractional bits
#define HALF_Y_CODER ($HALF_Y_CODER$) 

// ANS coder 
#define ANS_MAX_SRC_CARDINALITY ($ANS_MAX_SRC_CARDINALITY$)
#define EE_MAX_ITERATIONS ($EE_MAX_ITERATIONS$)
#define EE_BUFFER_SIZE ($EE_BUFFER_SIZE$)

#define NUM_ANS_THETA_MODES ($NUM_ANS_THETA_MODES$) // supported theta_modes
#define NUM_ANS_P_MODES ($NUM_ANS_P_MODES$) // supported p_modes
#define NUM_ANS_STATES ($NUM_ANS_STATES$)// NUM_ANS_STATES only considers I range

$CARDINALITY_ARRAYS$

#endif
"""



def get_p_modes(Nt_PRECISION,Nt_quantizer_centered,Max_Nt_idx,Half_Bernuulli_coder):
    ''' Simple function to get p reconstruction values.
    It provides a reconstruction values in the middle of the bin.
    This does not optimize the KLD.
    '''
    precision = Nt_PRECISION
    max_id =2**(Nt_PRECISION-1)-1  if Half_Bernuulli_coder else 2**Nt_PRECISION-1 # assuming centered 0.5 id is handled differently
    max_id = min(Max_Nt_idx,max_id)
    modes = dict()
    for idx in range(max_id+1):
        if Nt_quantizer_centered:
            rec_val = 2**-(precision+2) if idx == 0 else idx/2**precision 
        else:
            rec_val = 2**-(precision+1) + idx/2**precision
        
        modes[idx] = rec_val
    
    return modes


def get_St_modes_const_ratio(St_PRECISION,MAX_ST):
    ''' Simple function to get theta reconstruction values.
    It provides a reconstruction values accoding to the rule:
        rec_val = up_bound*ratio = low_bound/ratio.
        
    This function sets bounds at power of 2, so the ratio is set to 1/2**.5
    
    This does not optimize the KLD.
    '''
    
    t = 100
    max_st_idx = qfunc.Simplified_const_ratio_quantizer(MAX_ST*t,t,Initial_val_exp=-St_PRECISION)[0]
    modes = dict()
    simp_ratio = 1/2**.5
    for idx in range(max_st_idx+1):
        rec_val = (1<<idx)*2**-St_PRECISION * simp_ratio
        modes[idx] = rec_val
        
    return modes

def get_St_modes_const_ratio_uniform_1(St_PRECISION,MAX_ST):
    ''' Simple function to get theta reconstruction values.
        Takes get_St_modes_const_ratio quantization bins, and
        divides them in 2 (equal size)
    '''
    
    t = 100
    max_st_idx = qfunc.Simplified_const_ratio_uniform_quantizer(MAX_ST*t,t,Initial_val_exp=-St_PRECISION)[0]
    modes = dict()
    simp_ratio = 1/2**.5
    for idx in range(max_st_idx+1):
        if (0x01&idx) >0:
            low_b = 3*2**((idx>>1)-2  -St_PRECISION)
        else:
            low_b = 1*2**((idx>>1)-1  -St_PRECISION)

        #high bound
        idx_h = idx +1
        if (0x01&idx_h) >0:
            high_b = 3*2**((idx_h>>1)-2  -St_PRECISION)
        else:
            high_b = 1*2**((idx_h>>1)-1  -St_PRECISION)

        rx = qfunc.kld_minimizing_rec_value_for_const_ratio_uniform(low_b/(low_b+1),high_b/(high_b+1))
        qSt = rx/(1-rx)
        
        modes[idx] = qSt
        
    return modes



def get_2d_c_array(ls,name = "var",row_name=None, format_funt=str):
    if row_name is None:
        row_name = ["" for x in range(len(ls))]
    print("{:s}[{:d}][{:d}] = {{".format(name,len(ls),len(ls[0])))
    for i,sub_ls in enumerate(ls[:-1]):
        print("\t{",",".join([format_funt(v) for v in sub_ls]) ,"}, //",row_name[i])
    print("\t{",",".join([format_funt(v) for v in ls[-1]]),"}  //",row_name[-1])
    print("};")
    
def round_to_fixed_bits(num, num_of_bits):
    EP=1e-10
    factor = 2**num_of_bits
    sign = 1 if num >=0 else -1
    return round(num*factor-sign*EP)/factor



def get_symbol_src_geo(theta,max_code_len,max_symbols,just_pow_of_2 = True,min_symbols = 2):
    '''
    Function to get probability vectors for each theta

    The cardinality of the vectors is constrained by the precision of the ANS coder and the 
    requirement of the module code to have (2^n +1) cardinalities, where n is an integer 

    '''
    
#     assert max_code_len >=6
#     min_p = .66/2**(max_code_len) # works for max_code_len >=6
#     max_p = 1- max_p # works for max_code_len >=6
    probs = []
    for i in range(max_symbols):
        new_sym_prob = prob_func.Geometric_P(i,theta)
        rem_prob = 1 - (sum(probs) + new_sym_prob)
        code_len_rem = -np.log2(rem_prob) 
#         code_len_rem = -np.log2(rem_prob) 
        if code_len_rem > max_code_len and i>= min_symbols-1:
            break
        probs += [new_sym_prob]
    
    if just_pow_of_2:
        final_symbols = len(probs)
        max_total_symbols = int(2**np.floor(np.log2(final_symbols)))
        probs = probs[:max_total_symbols]
    
    
                                     
    probs += [1- sum(probs)]
    
    return probs



def get_adaptable_ANS_tables(ANS_address_size,ec_modes, symbol_source_generator, max_src_card,HW_tables = False ):
    ''' 
    Creates encoding and decoding array of ANS tables.
    out: encoding array enc[len(ec_modes)][state][max_src_card][2] 
       decoding array dec[len(ec_modes)][state][2]    
       
    args:
    ec_modes: dict with table_id-> parameter
    '''
    
    ANS_modes = len(ec_modes)
    ANS_states = 2**ANS_address_size
    ANS_state_offset = ANS_states
    # create empty structure table[NUM_ANS_MODES][NUM_ANS_STATES][ANS_MAX_SRC_CARDINALITY][2]
    if HW_tables:
        table_symbol_cardinality = int(2**(np.ceil(np.log2(max_src_card))))
    else:
        table_symbol_cardinality = max_src_card
    encoding_array = [ [ [[0,-1] for sy in range(table_symbol_cardinality)] 
                                  for st in range(ANS_states)] 
                                          for m in range(ANS_modes)]

    # create empty structure table[NUM_ANS_MODES][NUM_ANS_STATES][2]
    decoding_array = [ [ [0,-1]  for st in range(ANS_states)] 
                                      for m in range(ANS_modes)]

    #fill selected ANS modes
    max_code_len = ANS_address_size
    for mode_id,mode_parameter in ec_modes.items():
        probs = symbol_source_generator(mode_parameter)

        tANS_e, tANS_d = ANS.get_ANS_tables(probs,ANS_address_size)
        num_of_symbs = len(tANS_e)
        assert len(probs) == num_of_symbs, ("Error with mode_id = " + str(mode_id)
                 + "(mode_parameter = "+ str(mode_parameter)+ "): Compile error")


        for state in range(ANS_states):
            decoding_array[mode_id][state][0] = tANS_d[ANS_state_offset+state][0] # symbol
            decoding_array[mode_id][state][1] = tANS_d[ANS_state_offset+state][1] # prev state

            for symbol in range(num_of_symbs):
                new_state, num_out_bits = tANS_e[symbol][state]
                encoding_array[mode_id][state][symbol][0] = new_state - ANS_state_offset
                encoding_array[mode_id][state][symbol][1] = num_out_bits
    
    return encoding_array,decoding_array



def store_adaptative_ANS_encoding_tables(output_encoder_file,encoding_array,ec_modes):
    ANS_states = len(encoding_array[0])
    ANS_symbols = len(encoding_array[0][0])
    ANS_modes = len(ec_modes)
    
    state_digits = int(np.ceil(np.log10(ANS_states)))
    out_bit_digits = int(np.ceil(np.log10(np.ceil(np.log2(ANS_states)))))
    entry_format="{{{:"+str(state_digits)+"d}, {:"+str(out_bit_digits)+"d}}}"
    with open(output_encoder_file,'w') as file:
        for mode_id,mode_parameter in ec_modes.items():
            file.write("{{ // mode: {:d} | param: {:8.6f} \n".format(mode_id,mode_parameter))
            for state in range(ANS_states):    
                file.write("{") 
                for symbol in range(ANS_symbols):
                    new_state = encoding_array[mode_id][state][symbol][0]
                    num_out_bits = encoding_array[mode_id][state][symbol][1]
                    file.write(entry_format.format(new_state,num_out_bits) ) 

                    if symbol < ANS_symbols-1:
                        file.write(", ") 

                if state < (ANS_states - 1):
                    file.write("}}, // state: {}\n".format(state))
                else:
                    file.write("}}  // state: {}\n".format(state))

            if mode_id < (ANS_modes - 1):
                file.write("}, ") 
            else:
                file.write("}\n")


def store_adaptative_ANS_decoding_tables(output_decoder_file,decoding_array,ec_modes):
    ANS_states = len(decoding_array[0])
    ANS_modes = len(ec_modes)
    
    state_digits = int(np.ceil(np.log10(ANS_states)))
    symbol_digits = 3
    entry_format="{{{:"+str(state_digits)+"d}, {:"+str(symbol_digits)+"d}}}"
    with open(output_decoder_file,'w') as file:
        for mode_id,mode_parameter in ec_modes.items():
            file.write("{{ // mode_id: {:3d} | parameter: {:8.6f} \n".format(mode_id,mode_parameter))
            for state in range(ANS_states):    
                prev_state = decoding_array[mode_id][state][0]
                symbol = decoding_array[mode_id][state][1]
                file.write(entry_format.format(prev_state,symbol) )
                    
                if state < (ANS_states - 1):
                    file.write(", // state: {} \n".format(state))
                else:
                    file.write("  // state: {} \n".format(state))

            if mode_id < (ANS_modes - 1):
                file.write("}, ")
            else:
                file.write("} \n")
                
                
                
def get_cardinality_array(ec_modes,max_code_len,max_symbols):
    b_address_size = 5
    type_idx = 0 # 8
    pow_of_2_size = True
    available_bit_sizes = [8,16,32,64]


    table_data_type_bits = available_bit_sizes[type_idx]
    aux_table = []
    max_idx = 0
    for mode_id,mode_parameter in ec_modes.items():
        St = mode_parameter
        theta = St/(St+1)
        probs = get_symbol_src_geo(theta,max_code_len,max_symbols,just_pow_of_2 = True)
        
        idx= mode_id
        card =len(probs)
        if card >= 2**table_data_type_bits:
            type_idx += 1
            table_data_type_bits = available_bit_sizes[type_idx]
            assert type_idx< len(available_bit_sizes), "biggest type is not enought "
        aux_table += [(idx,card ) ]
        if idx>max_idx: max_idx = idx
        
            
    table_size = max_idx+1
    if pow_of_2_size:
        old_size = table_size
        table_size = int(2**(np.ceil(np.log2(table_size))))


    aux_table.sort()

    table_ptr = 0
    cardinality_table = []
    for i in range(table_size):

        if table_ptr < len(aux_table) and aux_table[table_ptr][0]==i:
            cardinality_table += [aux_table[table_ptr][1]-1]
            table_ptr +=1
        else:
            cardinality_table += [0]

    for mode_id,mode_parameter in ec_modes.items():
        idx= mode_id

    assert table_data_type_bits in [8,16,32,64]

    
    out_string ="static const uint{}_t tANS_cardinality_table[{}] = {{".format(table_data_type_bits,table_size)
    out_string +="\t"+",".join(str(c) for c in  cardinality_table)+"};\n"

    out_string +=""
    out_string +="static const int32_t max_module_per_cardinality_table[{}] = {{".format(table_size)
    out_string +="\t"+",".join(str(c)+'*EE_MAX_ITERATIONS' for c in  cardinality_table)+"};\n"
    return out_string

# max achieved cardinality
def get_max_z_coder_cardinality(ec_modes,max_code_len,max_symbols):
    max_card = 0
    for mode_id,mode_parameter in ec_modes.items():
        St = mode_parameter
        theta = St/(St+1)
        probs = get_symbol_src_geo(theta,max_code_len,max_symbols,just_pow_of_2 = True)
        
        if max_card < len(probs):
            max_card = len(probs)
            
    return max_card


