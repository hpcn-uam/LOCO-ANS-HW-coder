
#ifndef LOCO_FUNCTIONS_HPP
#define LOCO_FUNCTIONS_HPP

#include "LOCO_decorrelator.hpp"

#include "context.hpp"


inline px_t clamp(int v){  //cv::saturate_cast<uchar>
  #pragma HLS inline
  return (px_t)((unsigned)v <= 255 ? v : v > 0 ? 255 : 0);
}


template<int _COLS>
class image_buffer
{

public:
  //opt: partition?
  px_t buffer[_COLS]; //don't init to allow URAM
  px_t b_reg;
private:
  col_ptr_t col_ptr;
  col_ptr_t px_remaining_in_col;
  ap_uint<1> first_col;
  px_t prev_1st_a;
  // col_ptr_t read_addr;
public:
  // int rows;
  ap_uint<LOG2_MAX_COLS+1> cols;
  // Map of pixels:
  //   |c|b|d |
  // |e|a|x|
  px_t a,b,c,d;
  // err_t d0,d1,d2,dxy;
  #ifdef DEBUG
  sw_impl::RowBuffer row_buffer;
  int row_ptr ;
  #endif

  image_buffer(){};

  image_buffer(int _cols):col_ptr(0),first_col(1),
    // rows(_row),
    cols(_cols),
    px_remaining_in_col(_cols-1),
    // read_addr(2),
    a(0),b(0), c(0), d(0),prev_1st_a(0),b_reg(0)
    // ,d0(0),d1(0),d2(0),dxy(0)
    {

    #ifdef DEBUG
    row_buffer.set_cols(_cols);
    row_ptr =0;
    #endif
  }

  void init(int _cols){
    // rows=_row;
    cols=_cols;
    col_ptr=0;
    first_col=1;
    px_remaining_in_col=_cols-1;
    a=0;
    b=0;
    c=0;
    d=0;
    prev_1st_a=0;
    b_reg=0;

    // ,d0(0),d1(0),d2(0),dxy(0)

    #ifdef DEBUG
    row_buffer.set_cols(_cols);
    row_ptr =0;
    #endif
  }

  void get_fixed_prediction(int &context,int &fixed_prediction, int &sign){
  // px_t get_fixed_prediction(px_t a,px_t b,px_t c){
    #pragma HLS inline
    // compute fixed prediction
    int dx, dy, dxy, s,s_1;
    dy = c - a;
    dx = b - c; // OPT: this is the same gradient as d0 in prev iteration
    dxy = a -b;
    s = (dy ^ dx)<0?  1 : 0 ;
    s_1 = (dy ^ dxy)<0? a :b;
    // prev line as as optimal(in RTL terms) to:
    //   auto s_1 = (dy[dy.width-1] ^ dxy[dxy.width-1])==0? a:b ;
    fixed_prediction = s ? s_1 : b - dy ;


    #ifdef DEBUG
      px_t dbg_pred;
      if(c > std::max(a,b)) {
        dbg_pred = std::min(a,b);
      }else if(c < std::min(a,b)) {
        dbg_pred = std::max(a,b);
      }else{
        dbg_pred = a+b-c;
      }

      ASSERT(int(fixed_prediction), ==,int(dbg_pred), "| row_ptr:"<<int(row_ptr)<<
         "| col_ptr:"<<int(col_ptr)<<
          "\n| a:"<<int(a)<<"| b:"<<int(b)<<"| c:"<<int(c)<<"| d:"<<int(d)
          <<"\n| dy:"<<int(dy)<<"| dx:"<<int(dx)<<"| dxy:"<<int(dxy));
    #endif

    map_gradients_to_int(d-b,dx,dy,context,sign);
    
  }


  //update current row
  void update(px_t new_px){
    // 1° Row: b=c=d=0 
    // 1° Col: a=b, c=a(row-1) 
    // Last Col d = b
    //
    // 1° Row:    Pred = a  | Context = (0,0,-a)
    // 1° Col:      Pred = b  | Context = (d-b,b - b_prev, b_prev - b)
    // Last Col : Pred =  Normal  | Context = (0,b-c, c-a)
    #pragma HLS inline

    #ifdef DEBUG
      row_buffer.update(new_px, col_ptr);
    #endif


    #if END_OF_LINE_CALL
      a = new_px;
      c = b; 
      b = d;

      int read_addr = col_ptr >= cols-2? cols-1: col_ptr+2 ;
      // d = buffer[read_addr];
      d = first_col==1? px_t(0): buffer[read_addr];

      buffer[col_ptr]= new_px;

      if(col_ptr == 0){
        b_reg = new_px;
      }


      col_ptr++; 

    #else
      col_ptr_t read_addr = col_ptr == cols -1? col_ptr_t(1): 
            (col_ptr == cols-2? col_ptr_t(cols-1):col_ptr_t(col_ptr+2));

      if(col_ptr == cols -1) {
        //assuming image width of at least 10 px
        #pragma HLS occurrence cycle=10 
        b = b_reg;
        a = b;
        c = prev_1st_a; 
        prev_1st_a = a;

        #ifdef DEBUG
        row_buffer.end_row();
        row_ptr ++;
        #endif
      }else{
        a = new_px;
        c = b; 
        b = d;
      }

        // d = buffer[1];
      // d =  buffer[read_addr];
      d = first_col==1? px_t(0): buffer[read_addr];

      /*d2 = c - a;
      d1 = b - c;
      d0 = d - b;
      dxy = b - a;*/

      buffer[col_ptr]= new_px;

      if(col_ptr == 0){
        #pragma HLS occurrence cycle=10 
        b_reg = new_px;
      }

      if(col_ptr == cols -2) {
        #pragma HLS occurrence cycle=10 
        first_col = 0;
      }
      // first_col = col_ptr == cols -2? ap_uint<1>(0):first_col;

      col_ptr = col_ptr == cols -1? col_ptr_t(0): col_ptr_t(col_ptr+1);
    
      
      // OPT pre calc next values
    #endif


    #ifdef DEBUG
    if(col_ptr<cols) {
      int dbg_a , dbg_b, dbg_c, dbg_d;
      row_buffer.get_teplate( col_ptr,
                     dbg_a, dbg_b, dbg_c, dbg_d);
      ASSERT(int(a),==,dbg_a,"| row_ptr:"<<row_ptr<<"| col_ptr:"<<col_ptr)
      ASSERT(int(b),==,dbg_b,"| row_ptr:"<<row_ptr<<"| col_ptr:"<<col_ptr)
      ASSERT(int(c),==,dbg_c,"| row_ptr:"<<row_ptr<<"| col_ptr:"<<col_ptr)
      ASSERT(int(d),==,dbg_d,"| row_ptr:"<<row_ptr<<"| col_ptr:"<<col_ptr)
    }
    #endif


  }

  void end_of_line(){
    
    #pragma HLS inline

    #if END_OF_LINE_CALL
      
      b = b_reg;
      // b = buffer[0];
      a = b;
      c = prev_1st_a; 
      prev_1st_a = a;
      d = buffer[1];

      #ifdef DEBUG
      row_buffer.end_row();
      row_ptr ++;
      #endif

      col_ptr =0;
      first_col = 0;
      // read_addr = 2;
      //update existing pxs
      // opt partition the first cols to avoid mem reads
      #ifdef DEBUG
        int dbg_a , dbg_b, dbg_c, dbg_d;
        row_buffer.get_teplate( col_ptr,
                       dbg_a, dbg_b, dbg_c, dbg_d);
        ASSERT(a,==,dbg_a,"| row_ptr:"<<row_ptr<<"| col_ptr:"<<col_ptr)
        ASSERT(b,==,dbg_b,"| row_ptr:"<<row_ptr<<"| col_ptr:"<<col_ptr)
        ASSERT(c,==,dbg_c,"| row_ptr:"<<row_ptr<<"| col_ptr:"<<col_ptr)
        ASSERT(d,==,dbg_d,"| row_ptr:"<<row_ptr<<"| col_ptr:"<<col_ptr)
      #endif
    #endif
  }


  // check if it's initialized every time
  void end_of_image(){
    // reset
    prev_1st_a=a=b=c=d=b_reg=0;
    col_ptr = 0;
    first_col = 1;
  }


};







#endif // LOCO_FUNCTIONS_HPP
