#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#include <stdbool.h>

#define BLS12_377
// #define BLS12_381 // Initially define BLS12_381, commented out for illustration

#ifdef BLS12_377
    #undef BLS12_381
#endif

#ifdef BLS12_381
    #undef BLS12_377
#endif

// #define VERBOSE_DEBUG // Print out detailed intermediate results for debugging purpose

///// BLS12-377 Curve
//! Curve information:
//! * Base field: q = 258664426012969094010652733694893533536393512754914660539884262666720468348340822774968888139573360124440321458177
//! * Scalar field: r = 8444461749428370424248824938781546531375899335154063827935233455917409239041
//! * valuation(q - 1, 2) = 46
//! * valuation(r - 1, 2) = 47
//! * G1 curve equation: y^2 = x^3 + 1
//! * G2 curve equation: y^2 = x^3 + B, where
//!    * B = Fq2(0, 155198655607781456406391640216936120121836107652948796323930557600032281009004493664981332883744016074664192874906)

/////  BLS12-381 Curve
//! Curve information:
//! * Base field: q = 4002409555221667393417789825735904156556882819939007885332058136124031650490837864442687629129015664037894272559787
//! * Scalar field: r = 52435875175126190479447740508185965837690552500527637822603658699938581184513
//! * valuation(q - 1, 2) = 1
//! * valuation(r - 1, 2) = 32
//! * G1 curve equation: y^2 = x^3 + 4
//! * G2 curve equation: y^2 = x^3 + Fq2(4, 4)

// I follow the video here to implement the following code: https://www.google.com/search?sca_esv=d2e1279bafa3c31c&rlz=1C5CHFA_enUS1072US1072&tbm=vid&sxsrf=ACQVn0_PPMh9jbaEB8q86mJadJ7qYFmC7Q:1710426149951&q=weierstrass+elliptic+curve+scalar+multiplication&sa=X&ved=2ahUKEwjr5dP4-fOEAxXNGtAFHWJfDOEQ8ccDegQICxAI&biw=1708&bih=912&dpr=2#fpstate=ive&vld=cid:8a85a018,vid:iydGkrjJkSM,st:0
// I hope it works XDD

// The following function does not work for all potential function calls, 
// The reasons are still unclear as of now.
void mpz_mod_sub(mpz_t* result, mpz_t* a, mpz_t* b, mpz_t* q){
  // return ( a - b ) % q
  mpz_t a_b;
  mpz_init(a_b);
  mpz_init(result);
  mpz_sub(a_b, a, b);
  mpz_mod(result, a_b, q);
  mpz_clear(a_b);
};

/*
  rlt_x_q, rlt_y_q are x and y coordinate of the result point,
  x, y are coordinate of input point.
*/
void point_doubling(mpz_t* rlt_x_q, mpz_t* rlt_y_q, mpz_t x, mpz_t y){
  mpz_t scalar, result;
  mpz_t const_2, const_3, const_R, q; 
  mpz_t x_square, x_square_q, x_square_times_3, x_square_times_3_q, y_times_2, y_times_2_q, beta;
  mpz_t beta_q, beta_square, beta_square_q, beta_square_q_sub_x, beta_square_q_sub_x_q;
  mpz_t rlt_x, rlt_y;
  mpz_t x_sub_rst_x, x_sub_rst_x_q, x_sub_rst_x_beta, x_sub_rst_x_beta_q, x_sub_rst_x_beta_q_sub_y, y_times_2_q_inverse;
  
  mpz_init_set_str(const_2, "2", 16);
  mpz_init_set_str(const_3, "3", 16);
  mpz_init_set_str(const_R, "39402006196394479212279040100143613805079739270465446667948293404245721771497210611414266254884915640806627990306816", 10);
#ifdef BLS12_381
  mpz_init_set_str(q, "4002409555221667393417789825735904156556882819939007885332058136124031650490837864442687629129015664037894272559787", 10); // BLS12-381
#elif defined BLS12_377
  mpz_init_set_str(q, "258664426012969094010652733694893533536393512754914660539884262666720468348340822774968888139573360124440321458177", 10); // BLS12-377
#endif
  // gmp_printf("Input -- X: %Zx\n *\n Y: %Zx\n --------------------\n" , x, y);
 
  mpz_init(result);
  mpz_init(x_square);
  mpz_init(x_square_q);
  mpz_init(x_square_times_3);
  mpz_init(x_square_times_3_q);
  mpz_init(y_times_2);
  mpz_init(y_times_2_q);
  mpz_init(beta);
  mpz_init(beta_q);
  mpz_init(beta_square);
  mpz_init(beta_square_q);
  mpz_init(beta_square_q_sub_x);
  mpz_init(beta_square_q_sub_x_q);
  mpz_init(rlt_x);
  mpz_init(rlt_x_q);
  mpz_init(rlt_y);
  mpz_init(rlt_y_q);
  mpz_init(x_sub_rst_x);
  mpz_init(x_sub_rst_x_q);
  mpz_init(x_sub_rst_x_beta);
  mpz_init(x_sub_rst_x_beta_q);
  mpz_init(x_sub_rst_x_beta_q_sub_y);
  mpz_init(y_times_2_q_inverse);

  mpz_mul(x_square, x, x);
  mpz_mod(x_square_q, x_square, q);

  mpz_mul(x_square_times_3, x_square_q, const_3);
  mpz_mod(x_square_times_3_q, x_square_times_3, q);
  mpz_mul(y_times_2, const_2, y);
  mpz_mod(y_times_2_q, y_times_2, q);
  mpz_invert(y_times_2_q_inverse, y_times_2_q, q);

  // For curve y^2 =  x^3 + ax + b
  // Given point (x_1, y_1)
  // When tangent: beta = (3*x_1^2 + a)/2y_1
  mpz_mul(beta, x_square_times_3_q, y_times_2_q_inverse);
  mpz_mod(beta_q, beta, q);

  mpz_mul(beta_square, beta_q, beta_q);
  mpz_mod(beta_square_q, beta_square, q);
  
  // mpz_mod_sub(&beta_square_q_sub_x_q, beta_square_q, x, q);
  mpz_sub(beta_square_q_sub_x, beta_square_q, x);
  mpz_mod(beta_square_q_sub_x_q, beta_square_q_sub_x, q);

  // mpz_mod_sub(&rlt_x_q, beta_square_q_sub_x_q, x, q);
  mpz_sub(rlt_x, beta_square_q_sub_x_q, x);
  mpz_mod(rlt_x_q, rlt_x, q);

  // mpz_mod_sub(&x_sub_rst_x_q, &x, rlt_x_q, q);
  mpz_sub(x_sub_rst_x, x, rlt_x_q);
  mpz_mod(x_sub_rst_x_q, x_sub_rst_x, q);

  mpz_mul(x_sub_rst_x_beta, x_sub_rst_x_q, beta_q);
  mpz_mod(x_sub_rst_x_beta_q, x_sub_rst_x_beta, q);

  // mpz_mod_sub(&rlt_y_q, x_sub_rst_x_beta_q, y, q);
  mpz_sub(x_sub_rst_x_beta_q_sub_y, x_sub_rst_x_beta_q, y);
  mpz_mod(rlt_y_q, x_sub_rst_x_beta_q_sub_y, q);

#ifdef VERBOSE_DEBUG
  gmp_printf(" Doubling Result: \n X: %Zx\n Y: %Zx\n --------------------\n", rlt_x_q, rlt_y_q);
#endif 

  /* free used memory */
  mpz_clear(result);
  mpz_clear(x_square);
  mpz_clear(x_square_q);
  mpz_clear(x_square_times_3);
  mpz_clear(x_square_times_3_q);
  mpz_clear(y_times_2);
  mpz_clear(y_times_2_q);
  mpz_clear(beta);
  mpz_clear(beta_q);
  mpz_clear(beta_square);
  mpz_clear(beta_square_q);
  mpz_clear(beta_square_q_sub_x);
  mpz_clear(beta_square_q_sub_x_q);
  mpz_clear(rlt_x);
  mpz_clear(rlt_y);
  mpz_clear(x_sub_rst_x);
  mpz_clear(x_sub_rst_x_q);
  mpz_clear(x_sub_rst_x_beta);
  mpz_clear(x_sub_rst_x_beta_q);
  mpz_clear(x_sub_rst_x_beta_q_sub_y);
  mpz_clear(y_times_2_q_inverse);

};


void point_addition(mpz_t* x3, mpz_t* y3, mpz_t x1, mpz_t y1, mpz_t x2, mpz_t y2, mpz_t q){
  // beta = (y2 - y1) / (x2 - x1)
  // x3 = beta^2 - x1 - x2
  // y3 = beta * (x1 - x3) - y1
  mpz_t y2_y1, x2_x1, y2_y1_q, x2_x1_q, x2_x1_q_inv, x2_x1_q_inv_q;
  mpz_t beta, beta_q, beta_q_square, beta_q_square_q;
  mpz_t beta_q_square_q_x1, beta_q_square_q_x1_q, beta_q_square_q_x1_q_x2, beta_q_square_q_x1_q_x2_q; // (x3=beta_q_square_q_x1_q_x2_q)
  mpz_t x1_x3, x1_x3_q, beta_q_x1_x3_q, beta_q_x1_x3_q_q, beta_q_x1_x3_q_q_y1, beta_q_x1_x3_q_q_y1_q; // (y3=beta_x1_x3_q_q_y1)
  
  mpz_init(y2_y1);
  mpz_init(x2_x1);
  mpz_init(y2_y1_q);
  mpz_init(x2_x1_q);
  mpz_init(x2_x1_q_inv);
  mpz_init(x2_x1_q_inv_q);
  mpz_init(beta);
  mpz_init(beta_q);
  mpz_init(beta_q_square);
  mpz_init(beta_q_square_q);
  mpz_init(beta_q_square_q_x1);
  mpz_init(beta_q_square_q_x1_q);
  mpz_init(beta_q_square_q_x1_q_x2);
  mpz_init(beta_q_square_q_x1_q_x2_q);
  mpz_init(x3);
  mpz_init(x1_x3);
  mpz_init(x1_x3_q);
  mpz_init(beta_q_x1_x3_q);
  mpz_init(beta_q_x1_x3_q_q);
  mpz_init(beta_q_x1_x3_q_q_y1);
  mpz_init(beta_q_x1_x3_q_q_y1_q);
  mpz_init(y3);

  // beta = (y2 - y1) / (x2 - x1)
  mpz_sub(y2_y1, y2, y1);
  mpz_mod(y2_y1_q, y2_y1, q);

  mpz_sub(x2_x1, x2, x1);
  mpz_mod(x2_x1_q, x2_x1, q);

  mpz_invert(x2_x1_q_inv, x2_x1_q, q);
  mpz_mod(x2_x1_q_inv_q, x2_x1_q_inv, q);
  
  mpz_mul(beta, x2_x1_q_inv_q, y2_y1_q);
  mpz_mod(beta_q, beta, q);

  // x3 = beta^2 - x1 - x2
  mpz_mul(beta_q_square, beta_q, beta_q);
  mpz_mod(beta_q_square_q, beta_q_square, q);

  mpz_sub(beta_q_square_q_x1, beta_q_square_q, x1);
  mpz_mod(beta_q_square_q_x1_q, beta_q_square_q_x1, q);

  mpz_sub(beta_q_square_q_x1_q_x2, beta_q_square_q_x1_q, x2);
  mpz_mod(x3, beta_q_square_q_x1_q_x2, q);

  // y3 = beta * (x1 - x3) - y1
  mpz_sub(x1_x3, x1, x3);
  mpz_mod(x1_x3_q, x1_x3, q);

  mpz_mul(beta_q_x1_x3_q, beta_q, x1_x3_q);
  mpz_mod(beta_q_x1_x3_q_q, beta_q_x1_x3_q, q);

  mpz_sub(beta_q_x1_x3_q_q_y1, beta_q_x1_x3_q_q, y1);
  mpz_mod(y3, beta_q_x1_x3_q_q_y1, q);

  mpz_clear(y2_y1);
  mpz_clear(x2_x1);
  mpz_clear(y2_y1_q);
  mpz_clear(x2_x1_q);
  mpz_clear(x2_x1_q_inv);
  mpz_clear(x2_x1_q_inv_q);
  mpz_clear(beta);
  mpz_clear(beta_q);
  mpz_clear(beta_q_square);
  mpz_clear(beta_q_square_q);
  mpz_clear(beta_q_square_q_x1);
  mpz_clear(beta_q_square_q_x1_q);
  mpz_clear(beta_q_square_q_x1_q_x2);
  mpz_clear(x1_x3);
  mpz_clear(x1_x3_q);
  mpz_clear(beta_q_x1_x3_q);
  mpz_clear(beta_q_x1_x3_q_q);
  mpz_clear(beta_q_x1_x3_q_q_y1);

#ifdef VERBOSE_DEBUG
  gmp_printf(" Addition Result: \n X: %Zx\n Y: %Zx\n --------------------\n", x3, y3);
#endif 
}


void bit_iteration(){
    int accumulate = 0;
    int point_base = 2;
    mpz_t scalar;
    mpz_init_set_str(scalar, "0000000000000000000000000000000000000000000000000000000000000003", 16);

    // Determine the number of bits in scalar.
    size_t num_bits = mpz_sizeinbase(scalar, 2);

    for (size_t i = 0; i < num_bits; ++i) {
        // Check if the current bit is set.
        if (mpz_tstbit(scalar, i)) {
            accumulate += point_base; // Add to accumulate if bit is set.
        }
        point_base *= 2; // Double the point_base for the next bit.
    }

#ifdef VERBOSE_DEBUG
    printf("Accumulate: %d\n", accumulate);
#endif 

    // Clean up
    mpz_clear(scalar);

    return 0;
}

void scalar_multiplication(
    mpz_t* result_point_x, mpz_t* result_point_y,
    mpz_t base_point_x, mpz_t base_point_y,
    mpz_t scalar, mpz_t modulus_q) {

    // Temporary variables for intermediate results
    mpz_t current_point_x, current_point_y, temp_point_x, temp_point_y;
    mpz_t temp_acc_point_x, temp_acc_point_y;
    mpz_init(current_point_x);
    mpz_init(current_point_y);
    mpz_init(temp_point_x);
    mpz_init(temp_point_y);
    mpz_init(temp_acc_point_x);
    mpz_init(temp_acc_point_y);

    // Initialize result_point as the identity element (0,0) or the neutral element of addition on your curve
    mpz_init(result_point_x);
    mpz_init(result_point_y);

    // Copy base point to current_point
    mpz_set(current_point_x, base_point_x);
    mpz_set(current_point_y, base_point_y);

    size_t num_bits = mpz_sizeinbase(scalar, 2);

    for (size_t i = 0; i < num_bits; ++i) {
        if (mpz_tstbit(scalar, i)) {
            if (mpz_cmp_ui(*result_point_x, 0) == 0 && mpz_cmp_ui(*result_point_y, 0) == 0) {
                // result_point is the identity element, so just copy current_point to it
                mpz_set(*result_point_x, current_point_x);
                mpz_set(*result_point_y, current_point_y);
            } else {
                // Perform point addition
                mpz_t tmp_current_point_x, tmp_current_point_y;
                mpz_inits(tmp_current_point_x, tmp_current_point_y, NULL);
                mpz_set(tmp_current_point_x, *result_point_x);
                mpz_set(tmp_current_point_y, *result_point_y);
                point_addition(&temp_acc_point_x, &temp_acc_point_y,
                               tmp_current_point_x, tmp_current_point_y,
                               current_point_x, current_point_y, modulus_q);
                mpz_set(*result_point_x, temp_acc_point_x);
                mpz_set(*result_point_y, temp_acc_point_y);
            }
        }

        // Prepare for next iteration by doubling current_point
        if(i < num_bits - 1){
          mpz_set(temp_point_x, current_point_x);
          mpz_set(temp_point_y, current_point_y);
          point_doubling(&current_point_x, &current_point_y, temp_point_x, temp_point_y);
        }

#ifdef VERBOSE_DEBUG
      gmp_printf(" Current Result: \n X: %Zx\n Y: %Zx\n --------------------\n\n\n", result_point_x, result_point_y);
#endif    
    }

    // Clean up
    mpz_clear(current_point_x);
    mpz_clear(current_point_y);
    mpz_clear(temp_point_x);
    mpz_clear(temp_point_y);
}



void parse_two_mpz_from_line(mpz_t* out, mpz_t* out2, const char* buffer) {
    char* value_start = strchr(buffer, '(');
    if (!value_start) return;

    value_start++; // Move past the quote
    char* value_end = strchr(value_start, ')');
    if (!value_end) return;

    size_t value_length = value_end - value_start;
    char* value_str = malloc(value_length + 1);
    strncpy(value_str, value_start, value_length);
    value_str[value_length] = '\0';
    mpz_set_str(out, value_str, 16);
    // Second part

    char* value_start2 = strchr(value_end, '(');
    value_start2++; // Move past the quote

    char* value_end2 = strchr(value_start2, ')');

    size_t value_length2 = value_end2 - value_start2;
    char* value_str2 = malloc(value_length2 + 1);
    strncpy(value_str2, value_start2, value_length2);
    value_str2[value_length2] = '\0';
    mpz_set_str(out2, value_str2, 16);
#ifdef VERBOSE_DEBUG
    gmp_printf(" input X: %Zx \n Y: %Zx \n", out, out2);
#endif
    free(value_str2);
}

// Assumes buffer is a line from your CSV containing an mpz_t in the format zz: FpXXX "value"
void parse_mpz_from_line(mpz_t* out, const char* buffer) {
    char* value_start = strchr(buffer, '(');
    if (!value_start) return;

    value_start++; // Move past the quote
    char* value_end = strchr(value_start, ')');
    if (!value_end) return;
    buffer = value_end;

    size_t value_length = value_end - value_start;
    char* value_str = malloc(value_length + 1);
    strncpy(value_str, value_start, value_length);
    value_str[value_length] = '\0';
    mpz_set_str(out, value_str, 16);

#ifdef VERBOSE_DEBUG
    printf("scalar string: %s \n", value_str);
    gmp_printf(" scalar: %Zx \n", out);
#endif

    free(value_str);
}

void multi_scalar_multiplication(const char* points_csv_path, const char* scalars_csv_path) {
    FILE* points_file = fopen(points_csv_path, "r");
    FILE* scalars_file = fopen(scalars_csv_path, "r");
    if (!points_file || !scalars_file) {
        // Handle error
        return;
    }


    char points_buffer[2048];
    char scalars_buffer[1024];
    mpz_t result_x, result_y, copy_result_x, copy_result_y, q;
    mpz_inits(result_x, result_y, copy_result_x, copy_result_y, NULL);
  #ifdef BLS12_381
    mpz_init_set_str(q, "4002409555221667393417789825735904156556882819939007885332058136124031650490837864442687629129015664037894272559787", 10); // BLS12-381
  #elif defined BLS12_377
    mpz_init_set_str(q, "258664426012969094010652733694893533536393512754914660539884262666720468348340822774968888139573360124440321458177", 10); // BLS12-377
  #endif
    bool initial_addition = true;
    while (fgets(points_buffer, sizeof(points_buffer), points_file) && fgets(scalars_buffer, sizeof(scalars_buffer), scalars_file)) {
        mpz_t point_x, point_y, scalar, cur_result_x, cur_result_y;
        mpz_inits(point_x, point_y, scalar, cur_result_x, cur_result_y,  NULL);

        // Assume modulus_q is set correctly for your curve
        // mpz_set_str(modulus_q, "Your modulus here", 10);

        parse_two_mpz_from_line(&point_x, &point_y, points_buffer);
        parse_mpz_from_line(&scalar, scalars_buffer);
#ifdef VERBOSE_DEBUG
        gmp_printf(" Input X: %Zx\n Y: %Zx \n* scalar: %Zx\n --------------------\n", point_x, point_y, scalar);
#endif

        // Perform scalar multiplication
        scalar_multiplication(&cur_result_x, &cur_result_y, point_x, point_y, scalar, q);
#ifdef VERBOSE_DEBUG
        gmp_printf(" cur_result:\n   X: %Zx\n   Y: %Zx\n --------------------\n", cur_result_x, cur_result_y);
#endif

        if(initial_addition){
          mpz_set(result_x, cur_result_x);
          mpz_set(result_y, cur_result_y);
          initial_addition = false;
        }
        else{
          mpz_set(copy_result_x, result_x);
          mpz_set(copy_result_y, result_y);
          point_addition(&result_x, &result_y, cur_result_x, cur_result_y, copy_result_x, copy_result_y, q);
        }
        // Print or store result_x and result_y as needed

        mpz_clears(point_x, point_y, scalar, cur_result_x, cur_result_y, NULL);
    }
    gmp_printf(" Final Result:\n   X: %Zx\n   Y: %Zx\n --------------------\n", result_x, result_y);

    fclose(points_file);
    fclose(scalars_file);
}


void specific_value_test(){
  char* input_x = "16A86B7BADC3C532B1ACE6D225E0A80425DDF81EE696F1C03FE66BDD58F1F1A787E5FE407C838FAD95721F5E3B8AB1D2";
  char* input_y = "00E835EB1E998D7F12AA207B0312815073EAF5C31879BD08F0CB2F8B48D32012EDF384B4B18AAA536B45C453C8AF46D8";
  
  int a = 0, b = 4;

  mpz_t x, y;
  mpz_t scalar, q;

#ifdef BLS12_381
  mpz_init_set_str(q, "4002409555221667393417789825735904156556882819939007885332058136124031650490837864442687629129015664037894272559787", 10); // BLS12-381
#elif defined BLS12_377
  mpz_init_set_str(q, "258664426012969094010652733694893533536393512754914660539884262666720468348340822774968888139573360124440321458177", 10); // BLS12-377
#endif
  mpz_init_set_str(x, input_x, 16);
  mpz_init_set_str(y, input_y, 16);
  mpz_init_set_str(scalar, "0000000000000000000000000000000000000000000000000000000000000005", 16);

#ifdef VERBOSE_DEBUG
  gmp_printf(" Input \n X: %Zx\n Y: %Zx \n * scalar: %Zx\n --------------------\n", x, y, scalar);
#endif

  mpz_t result_x_q, result_y_q;
  scalar_multiplication(&result_x_q, &result_y_q, x, y, scalar, q);

#ifdef VERBOSE_DEBUG
  gmp_printf(" Final Result:\n X: %Zx\n Y: %Zx\n --------------------\n", result_x_q, result_y_q);
#endif
}

/*
  ./program "/home/ubuntu/work/ZPrize-23-Prize1/Prize 1A/test_code/zprize_msm_curve_377_bases_dim_16_seed_0.csv" "/home/ubuntu/work/ZPrize-23-Prize1/Prize 1A/test_code/zprize_msm_curve_377_scalars_dim_16_seed_0.csv"
*/
int main(int argc, char *argv[]) {
  if (argc != 3) {
      printf("Usage: %s <input_x> <input_y>\n", argv[0]);
      return 1; // Exit with an error code
  }

  // Assign command line arguments to input_x and input_y
  char* point_input_file = argv[1];
  char* scalar_input_file = argv[2];

  multi_scalar_multiplication(point_input_file, scalar_input_file);

  return 0;
}
