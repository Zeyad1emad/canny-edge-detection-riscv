
#ifndef GAUSSIAN_BLUR_H
#define GAUSSIAN_BLUR_H

#include <cstdint>
#include <cstdlib>
#include <vector> // for dynamic array

//initiallized at guassian_blur.cpp 
//This ensures the this functions are general
extern const int16_t kernel5x5[5][5];
extern const int16_t kernel1D[5];
/*PixelT-> represents the pixel type in general it would be a byte
AccumT-> represents the sum variable type in general it would be large to avoid overflow 
KernelT-> represents the kernal type in general it would be sined as the cernal may contain -ve nums 
*/
template <typename PixelT, typename AccumT, typename KernelT>
void gaussian_blur_2d(const PixelT* input, PixelT* output, int width, int height) {
    //it takes type of pixels , the location of the input image,location of the output image after filtering and also the width and height of them . 

    // these two loops to loop around each pixel of the image
    for (int y = 0; y < height; ++y) { 
        for (int x = 0; x < width; ++x) {
            AccumT sum = 0; //initializing the sum with 0 as we will fill it 

//these two loops to loop arround each elemnt of our kernal 
            for (int ky = -2; ky <= 2; ++ky) {
                for (int kx = -2; kx <= 2; ++kx) {
                    int py = y + ky, px = x + kx;
                    if (py >= 0 && py < height && px >= 0 && px < width) // to ensure zero padding
                        sum += (AccumT)input[py * width + px] * (KernelT)kernel5x5[ky+2][kx+2];  //multiplay each pixel with its corresponding lernal value and add
                }
            }
            sum /= 273; // normalizing the sum to make sure that the output pixel within the valid range [0:255]
            if (sum < 0)   sum = 0;
            if (sum > 255) sum = 255;
            output[y * width + x] = (PixelT)sum;
        }
    }
}
template <typename PixelT, typename AccumT, typename KernelT>
void gaussian_blur_separable(const PixelT* input, PixelT* output, int width, int height) {
    // A temporary buffer to store the results of the horizontal pass before processing the vertical pass
    std::vector<AccumT> temp(width * height);

    //  Horizontal Pass
    // Loops through each row and column to apply the 1D kernel horizontally
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            AccumT sum = 0; 

            // Inner loop for the horizontal 1D kernel (1x5)
            for (int kx = -2; kx <= 2; ++kx) {
                int px = x + kx;
                // Boundary check: ensures we don't read pixels outside the image width
                if (px >= 0 && px < width)
                    // Multiply the input pixel by the corresponding 1D kernel value and accumulate
                    sum += (AccumT)input[y * width + px] * (KernelT)kernel1D[kx+2];
            }
            // Store the intermediate result in the temp buffer
            temp[y * width + x] = sum; //this represents intermediate array 
        }
    }

    // Vertical Pass 
    // Loops through the temporary buffer to apply the 1D kernel vertically
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            AccumT sum = 0;

            // Inner loop for the vertical 1D kernel (5x1)
            for (int ky = -2; ky <= 2; ++ky) {
                int py = y + ky;
                // Boundary check: ensures we don't read pixels outside the image height
                if (py >= 0 && py < height)
                    // Multiply the intermediate pixel from 'temp' by the 1D kernel value and accumulate
                    sum += temp[py * width + x] * (KernelT)kernel1D[ky+2];
            }
            
            // Normalize the final sum.
            sum /= 256; //256??-> (sum of H_kernal *sum of V_kernal)=16*16=256 
            
            // Clamp the value to ensure it stays within the valid 8-bit range [0:255]
            if (sum > 255) sum = 255;
            if (sum < 0)   sum = 0; 
            
            // Cast the final result back to the pixel type and store it in the output buffer
            output[y * width + x] = (PixelT)sum;
        }
    }
}

#endif