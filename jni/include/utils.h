#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

// Android stuff start
#include <android/log.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>

#define LOG_TAG "linearconcalc"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
// Android stuff end

// Named constants start
#define STRUCT_ALIGN32 32

#define BASE 10
#define INPUTS 3
#define INPUT_MAX 25

#define BUF_DEF 1024
// Named constants end

// Define convenient type
typedef char InMatrix[INPUTS][INPUT_MAX];

typedef struct __attribute__((aligned(STRUCT_ALIGN32))) {
    int64_t num_a;
    int64_t num_b;
    int64_t gcd;
    int64_t mod;
} Numbers;

// Function declarations
void    simplify_eq(Numbers *nums);
int64_t get_inverse_mod(Numbers *nums);
int64_t get_solutions(Numbers *nums, int64_t inverse_mod, int64_t results[]);

void    populate_data(InMatrix inputs, Numbers *all_nums);
void    sanitize_inputs(Numbers *all_nums);
int64_t calculate_gcd(int64_t num1, int64_t num2);

#endif
