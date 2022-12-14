///////////////////////////////////////////////////////////////////////////////
//
//      TargaImage.cpp                          Author:     Stephen Chenney
//                                              Modified:   Eric McDaniel
//                                              Date:       Fall 2004
//
//      Implementation of TargaImage methods.  You must implement the image
//  modification functions.
//
///////////////////////////////////////////////////////////////////////////////

#include "Globals.h"
#include "TargaImage.h"
#include "libtarga.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
using namespace std;

// constants
const int           RED             = 0;                // red channel
const int           GREEN           = 1;                // green channel
const int           BLUE            = 2;                // blue channel
const unsigned char BACKGROUND[3]   = { 0, 0, 0 };      // background color


// Computes n choose s, efficiently
double Binomial(int n, int s)
{
    double        res;

    res = 1;
    for (int i = 1 ; i <= s ; i++)
        res = (n - i + 1) * res / i ;

    return res;
}// Binomial


///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage() : width(0), height(0), data(NULL)
{}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h) : width(w), height(h)
{
   data = new unsigned char[width * height * 4];
   ClearToBlack();
}// TargaImage



///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables to values given.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h, unsigned char *d)
{
    int i;

    width = w;
    height = h;
    data = new unsigned char[width * height * 4];

    for (i = 0; i < width * height * 4; i++)
	    data[i] = d[i];
}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Copy Constructor.  Initialize member to that of input
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(const TargaImage& image) 
{
   width = image.width;
   height = image.height;
   data = NULL; 
   if (image.data != NULL) {
      data = new unsigned char[width * height * 4];
      memcpy(data, image.data, sizeof(unsigned char) * width * height * 4);
   }
}


///////////////////////////////////////////////////////////////////////////////
//
//      Destructor.  Free image memory.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::~TargaImage()
{
    if (data)
        delete[] data;
}// ~TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Converts an image to RGB form, and returns the rgb pixel data - 24 
//  bits per pixel. The returned space should be deleted when no longer 
//  required.
//
///////////////////////////////////////////////////////////////////////////////
unsigned char* TargaImage::To_RGB(void)
{
    unsigned char   *rgb = new unsigned char[width * height * 3];
    int		    i, j;

    if (! data)
	    return NULL;

    // Divide out the alpha
    for (i = 0 ; i < height ; i++)
    {
	    int in_offset = ((i * width) << 2);
	    int out_offset = i * width * 3;

	    for (j = 0 ; j < width ; j++)
        {
	        RGBA_To_RGB(data + (in_offset + j*4), rgb + (out_offset + j*3));
	    }
    }

    return rgb;
}// TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Save the image to a targa file. Returns 1 on success, 0 on failure.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Save_Image(const char *filename)
{
    TargaImage	*out_image = Reverse_Rows();

    if (! out_image)
	    return false;

    if (!tga_write_raw(filename, width, height, out_image->data, TGA_TRUECOLOR_32))
    {
	    cout << "TGA Save Error: %s\n", tga_error_string(tga_get_last_error());
	    return false;
    }

    delete out_image;

    return true;
}// Save_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Load a targa image from a file.  Return a new TargaImage object which 
//  must be deleted by caller.  Return NULL on failure.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Load_Image(char *filename)
{
    unsigned char   *temp_data;
    TargaImage	    *temp_image;
    TargaImage	    *result;
    int		        width, height;

    if (!filename)
    {
        cout << "No filename given." << endl;
        return NULL;
    }// if

    temp_data = (unsigned char*)tga_load(filename, &width, &height, TGA_TRUECOLOR_32);
    if (!temp_data)
    {
        cout << "TGA Error: %s\n", tga_error_string(tga_get_last_error());
	    width = height = 0;
	    return NULL;
    }
    temp_image = new TargaImage(width, height, temp_data);
    free(temp_data);

    result = temp_image->Reverse_Rows();

    delete temp_image;

    return result;
}// Load_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Convert image to grayscale.  Red, green, and blue channels should all 
//  contain grayscale value.  Alpha channel shoould be left unchanged.  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::To_Grayscale() {
    for (int i = 0; i < width * height * 4; i += 4) {
        data[i] = data[i] * 0.299 + data[i + 1] * 0.587 + data[i + 2] * 0.114;
        data[i + 2] = data[i + 1] = data[i];
    }

	return true;
}// To_Grayscale


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using uniform quantization.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Uniform() {
    for (int i = 0; i < width * height * 4; i += 4) {
        data[i + 0] = data[i + 0] >> 5 << 5; // R 3bit
        data[i + 1] = data[i + 1] >> 5 << 5; // G 3bit
        data[i + 2] = data[i + 2] >> 6 << 6; // B 2bit
    }
    return false;
}// Quant_Uniform


///////////////////////////////////////////////////////////////////////////////
//
//      Convert the image to an 8 bit image using populosity quantization.  
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Populosity() {
    std::map<uint32_t, int> colors;
    for (int i = 0; i < this->width * this->height * 4; i += 4) {
        uint8_t R_val = this->data[i + 0] >> 3;
        uint8_t G_val = this->data[i + 1] >> 3;
        uint8_t B_val = this->data[i + 2] >> 3;
        uint32_t key = (R_val << 10) + (G_val << 5) + B_val;
        if (colors.find(key) == colors.end()){
            colors[key] = 1;
        }else{
            colors[key]++;
        }
    }


    std::vector<std::pair<uint32_t , int>> Sorted(colors.begin(), colors.end());
    std::sort(Sorted.begin(), Sorted.end(),
              [](const std::pair<uint32_t, int>& a, const std::pair<uint32_t, int>& b)
              { return a.second > b.second; });

    for (int i = 0; i < this->width * this->height * 4; i += 4) {
        int ind = 0;
        uint8_t R_val = this->data[i + 0];
        uint8_t G_val = this->data[i + 1];
        uint8_t B_val = this->data[i + 2];

        uint32_t min_dis = (1<<31);
        pair<uint32_t,int> min_it;

        for (const auto& j : Sorted){
            uint8_t jR_val = ((j.first >> 10) & 0b11111) << 3;
            uint8_t jG_val = ((j.first >> 05) & 0b11111) << 3;
            uint8_t jB_val = ((j.first >> 00) & 0b11111) << 3;

            uint32_t dis =
                    (jR_val - R_val) * (jR_val - R_val)+
                    (jG_val - G_val) * (jG_val - G_val)+
                    (jB_val - B_val) * (jB_val - B_val);

            if(dis < min_dis){
                min_dis = dis;
                min_it = j;
            }

            if(ind == 255){
                break;
            }
            ind++;
        }

        data[i + 0] = ((min_it.first >> 10) & 0b11111) << 3;
        data[i + 1] = ((min_it.first >> 05) & 0b11111) << 3;
        data[i + 2] = ((min_it.first >> 00) & 0b11111) << 3;

    }

    return true;
}// Quant_Populosity


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image using a threshold of 1/2.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Threshold() {
    for (int i = 0; i < width * height * 4; i += 4) {
        data[i] = data[i] * 0.299 + data[i + 1] * 0.587 + data[i + 2] * 0.114;
        data[i] = (data[i] < 128) ? 0 : 255;
        data[i + 2] = data[i + 1] = data[i];
    }
    return true;
}// Dither_Threshold


///////////////////////////////////////////////////////////////////////////////
//
//      Dither image using random dithering.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Random(){
    srand(time(NULL));
    for (int i = 0; i < width * height * 4; i += 4) {

        double grayval = data[i] * 0.299 + data[i + 1] * 0.587 + data[i + 2] * 0.114;
        double rd = ((rand() % 103) - 51);

        grayval += rd;
        grayval = grayval < 255 ? grayval > 0 ? grayval : 0 : 255;

        data[i] = static_cast<uint8_t>(grayval);
        data[i + 2] = data[i + 1] = data[i];
    }
    this->Dither_Bright();
    return true;
}// Dither_Random


///////////////////////////////////////////////////////////////////////////////
//
//      Perform Floyd-Steinberg dithering on the image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_FS() {
    uint32_t* new_data = new uint32_t [width * height * 4];
    for (int i = 0; i < width * height * 4; i += 4) {
        data[i] = data[i] * 0.299 + data[i + 1] * 0.587 + data[i + 2] * 0.114;
        data[i + 2] = data[i + 1] = data[i];
        new_data[i] = new_data[i + 1] = new_data[i + 2] = data[i];
        new_data[i + 3] = data[3];
    }

    for (int i = 0; i < height; i++) {
        if (i % 2 == 0) {
            for (int j = 0; j < width; j++) {

                size_t index = ((i * width) * 4) + j * 4;
                data[index] = (new_data[index] < 128) ? 0 : 255;
                data[index + 1] = data[index + 2] = data[index];

                uint32_t diff = new_data[index] - data[index];

                if (i + 1 < height && j > 0) {
                    new_data[(((i + 1) * width) << 2) + (j - 1) * 4] += diff * ((float)3 / 16);
                }
                if (i + 1 < height) {
                    new_data[(((i + 1) * width) << 2) + j * 4] += diff * ((float)5 / 16);
                }
                if (i + 1 < height && j + 1 < width) {
                    new_data[(((i + 1) * width) << 2) + (j + 1) * 4] += diff * ((float)1 / 16);
                }
                if (j + 1 < width) {
                    new_data[((i * width) << 2) + (j - 1) * 4] += diff * ((float)7 / 16);
                }
            }
        }else {
            for (int j = width - 1; j >= 0; j--) {

                size_t index = ((i * width) * 4) + j * 4;
                data[index] = (new_data[index] < 128) ? 0 : 255;
                data[index + 1] = data[index + 2] = data[index];

                uint32_t diff = new_data[index] - data[index];

                if (i + 1 < height && j + 1 < width) {
                    new_data[(((i + 1) * width) << 2) + (j + 1) * 4] += diff * ((float) 3 / 16);
                }
                if (i + 1 < height) {
                    new_data[(((i + 1) * width) << 2) + j * 4] += diff * ((float) 5 / 16);
                }
                if (i + 1 < height && j > 0) {
                    new_data[(((i + 1) * width) << 2) + (j - 1) * 4] += diff * ((float) 1 / 16);
                }
                if (j > 0) {
                    new_data[((i * width) << 2) + (j - 1) * 4] += diff * ((float) 7 / 16);
                }
            }
        }
    }

    delete[] new_data;
    ClearToBlack();
    return true;
}// Dither_FS


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image while conserving the average brightness.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Bright() {
    double sum_of_brightness = 0.0;
    vector<int> bright_cnt(256, 0);
    for (int i = 0; i < width * height * 4; i += 4) {
        double grayval = data[i] * 0.299 + data[i + 1] * 0.587 + data[i + 2] * 0.114;
        data[i] = static_cast<uint8_t>(grayval);
        sum_of_brightness += grayval;
        bright_cnt[static_cast<int>(grayval)]++;
    }

    int br_after_thres = 0;
    int thres_val = 255;
    for( ; thres_val > -1 ; thres_val--){
        br_after_thres += (bright_cnt[thres_val] * 255);
        if(br_after_thres >= sum_of_brightness){
            break;
        }
    }

    for (int i = 0; i < (width * height) << 2; i += 4) {
        data[i] = (data[i] < thres_val) ? 0 : 255;
        data[i + 2] = data[i + 1] = data[i];
    }

    return true;
}// Dither_Bright


///////////////////////////////////////////////////////////////////////////////
//
//      Perform clustered differing of the image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Cluster() {
    this->To_Grayscale();
    int mask[4][4] = {{180, 90, 150, 60},
                      {15, 240, 210, 105},
                      {120, 195, 225, 30},
                      {45, 135, 75, 165}};
    for (int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++){
            size_t index = ((i * width) << 2) + (j << 2);
            data[index] = data[index] >= mask[i & 0b11][ j & 0b11 ] ? 255 : 0;
            data[index + 2] = data[index + 1] = data[index];
        }
    }
    return true;
}// Dither_Cluster


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using Floyd-Steinberg dithering over
//  a uniform quantization - the same quantization as in Quant_Uniform.
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Color()
{
    ClearToBlack();
    return false;
}// Dither_Color


///////////////////////////////////////////////////////////////////////////////
//
//      Composite the current image over the given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Over(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout <<  "Comp_Over: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Over


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "in" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_In(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_In: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_In


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "out" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Out(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Out: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Out


///////////////////////////////////////////////////////////////////////////////
//
//      Composite current image "atop" given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Atop(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Atop: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Atop


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image with given image using exclusive or (XOR).  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Xor(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Xor: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Xor


///////////////////////////////////////////////////////////////////////////////
//
//      Calculate the difference bewteen this imag and the given one.  Image 
//  dimensions must be equal.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Difference(TargaImage* pImage)
{
    if (!pImage)
        return false;

    if (width != pImage->width || height != pImage->height)
    {
        cout << "Difference: Images not the same size\n";
        return false;
    }// if

    for (int i = 0 ; i < width * height * 4 ; i += 4)
    {
        unsigned char        rgb1[3];
        unsigned char        rgb2[3];

        RGBA_To_RGB(data + i, rgb1);
        RGBA_To_RGB(pImage->data + i, rgb2);

        data[i] = abs(rgb1[0] - rgb2[0]);
        data[i+1] = abs(rgb1[1] - rgb2[1]);
        data[i+2] = abs(rgb1[2] - rgb2[2]);
        data[i+3] = 255;
    }

    return true;
}// Difference


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 box filter on this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Box() {

    uint8_t * new_data = new uint8_t[(this->height * this->width) << 2];

    for (int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++){
            uint32_t Rtotal = 0, Gtotal = 0, Btotal = 0;
            int32_t count = 0;
            size_t index = ((i * width) << 2) + j * 4;

            new_data[index + 3] = data[index + 3];

            for(int m = 0; m < 5; m++){
                for(int n = 0; n < 5; n++){
                    static size_t shift_x, shift_y;
                    shift_x = (i + m - 2);
                    shift_y = (j + n - 2);

                    if(shift_x < 0 || shift_x >= height){ continue; }
                    if(shift_y < 0 || shift_y >= width){ continue; }

                    static size_t in_index;
                    in_index = shift_x * width * 4 + shift_y * 4;
                    Rtotal += data[in_index+0];
                    Gtotal += data[in_index+1];
                    Btotal += data[in_index+2];
                    count ++;
                }
            }

            new_data[index + 0] = static_cast<uint8_t>(Rtotal / count);
            new_data[index + 1] = static_cast<uint8_t>(Gtotal / count);
            new_data[index + 2] = static_cast<uint8_t>(Btotal / count);
        }
    }
    delete[] data;
    data = new_data;
    return true;
}// Filter_Box


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Bartlett filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Bartlett() {

    uint8_t * new_data = new uint8_t[(this->height * this->width) << 2];

    const static int filter[5][5] = {
        {1, 3, 5, 3, 1},
        {3, 9, 15, 9, 3},
        {5, 15, 25, 15, 5},
        {3, 9, 15, 9, 3},
        {1, 3, 5, 3, 1}
    };

    for (int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++){
            uint32_t Rtotal = 0, Gtotal = 0, Btotal = 0;
            int32_t count = 0;
            size_t index = ((i * width) << 2) + j * 4;

            new_data[index + 3] = data[index + 3];

            for(int m = 0; m < 5; m++){
                for(int n = 0; n < 5; n++){
                    static size_t shift_x, shift_y;
                    shift_x = (i + m - 2);
                    shift_y = (j + n - 2);

                    if(shift_x < 0 || shift_x >= height){ continue; }
                    if(shift_y < 0 || shift_y >= width){ continue; }

                    static size_t in_index;
                    in_index = shift_x * width * 4 + shift_y * 4;
                    Rtotal += data[in_index+0] * filter[m][n];
                    Gtotal += data[in_index+1] * filter[m][n];
                    Btotal += data[in_index+2] * filter[m][n];
                    count  += filter[m][n];
                }
            }

            new_data[index + 0] = static_cast<uint8_t>(Rtotal / count);
            new_data[index + 1] = static_cast<uint8_t>(Gtotal / count);
            new_data[index + 2] = static_cast<uint8_t>(Btotal / count);
        }
    }
    delete[] data;
    data = new_data;
    return true;
}// Filter_Bartlett


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Gaussian() {
    uint8_t * new_data = new uint8_t[(this->height * this->width) << 2];

    const static int filter[5][5] = {
        {1, 4, 7, 4, 1},
        {4, 16, 26, 16, 4},
        {7, 26, 41, 26, 7},
        {4, 16, 26, 16, 4},
        {1, 4, 7, 4, 1}
    };

    for (int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++){
            uint32_t Rtotal = 0, Gtotal = 0, Btotal = 0;
            int32_t count = 0;
            size_t index = ((i * width) << 2) + (j << 2);

            new_data[index + 3] = data[index + 3];

            for(int m = 0; m < 5; m++){
                for(int n = 0; n < 5; n++){
                    static size_t shift_x, shift_y;
                    shift_x = (i + m - 2);
                    shift_y = (j + n - 2);

                    if(shift_x < 0 || shift_x >= height){ continue; }
                    if(shift_y < 0 || shift_y >= width){ continue; }

                    static size_t in_index;
                    in_index = shift_x * width * 4 + shift_y * 4;
                    Rtotal += data[in_index+0] * filter[m][n];
                    Gtotal += data[in_index+1] * filter[m][n];
                    Btotal += data[in_index+2] * filter[m][n];
                    count  += filter[m][n];
                }
            }

            new_data[index + 0] = static_cast<uint8_t>(Rtotal / count);
            new_data[index + 1] = static_cast<uint8_t>(Gtotal / count);
            new_data[index + 2] = static_cast<uint8_t>(Btotal / count);
        }
    }
    delete[] data;
    data = new_data;
    return true;
}// Filter_Gaussian

///////////////////////////////////////////////////////////////////////////////
//
//      Perform NxN Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////

bool TargaImage::Filter_Gaussian_N( unsigned int N ) {
    ClearToBlack();
   return false;
}// Filter_Gaussian_N


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 edge detect (high pass) filter on this image.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Edge() {
    ClearToBlack();
    return true;
}// Filter_Edge


///////////////////////////////////////////////////////////////////////////////
//
//      Perform a 5x5 enhancement filter to this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Enhance() {
    ClearToBlack();
    return false;
}// Filter_Enhance


///////////////////////////////////////////////////////////////////////////////
//
//      Run simplified version of Hertzmann's painterly image filter.
//      You probably will want to use the Draw_Stroke funciton and the
//      Stroke class to help.
// Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::NPR_Paint() {
    ClearToBlack();
    return false;
}



///////////////////////////////////////////////////////////////////////////////
//
//      Halve the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Half_Size() {

    uint8_t * new_data = new uint8_t[(this->height>>1) * (this->width>>1) << 2];

    const static int filter[3][3] = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };

    for (int i = 0; i < (height >> 1); i++) {
        for(int j = 0; j < (width >> 1); j++){
            uint32_t Rtotal = 0, Gtotal = 0, Btotal = 0;
            int32_t count = 0;
            size_t index = (i * (width >> 1))  * 4 + j * 4;

            for(int m = 0; m < 3; m++){
                for(int n = 0; n < 3; n++){
                    static size_t shift_x, shift_y;
                    shift_x = ((i << 1) + m - 1);
                    shift_y = ((j << 1) + n - 1);

                    if(shift_x < 0 || shift_x >= height){ continue; }
                    if(shift_y < 0 || shift_y >= width){ continue; }

                    static size_t in_index;
                    in_index = shift_x * width * 4 + shift_y * 4;
                    Rtotal += data[in_index+0] * filter[m][n];
                    Gtotal += data[in_index+1] * filter[m][n];
                    Btotal += data[in_index+2] * filter[m][n];
                    new_data[index + 3] = data[in_index + 3];
                    count  += filter[m][n];
                }
            }

            new_data[index + 0] = static_cast<uint8_t>(Rtotal / count);
            new_data[index + 1] = static_cast<uint8_t>(Gtotal / count);
            new_data[index + 2] = static_cast<uint8_t>(Btotal / count);
        }
    }

    delete[] data;
    data = new_data;

    this->height >>= 1;
    this->width >>= 1;

    return true;
}// Half_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Double the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Double_Size() {
    ClearToBlack();
    return false;
}// Double_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Scale the image dimensions by the given factor.  The given factor is 
//  assumed to be greater than one.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Resize(float scale) {
    ClearToBlack();
    return false;
}// Resize


//////////////////////////////////////////////////////////////////////////////
//
//      Rotate the image clockwise by the given angle.  Do not resize the 
//  image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Rotate(float angleDegrees) {
    uint32_t* new_data = new uint32_t [height*width*4];

    double filter[4][4] = {
        {1, 3, 3, 1},
        {3, 9, 9, 3},
        {3, 9, 9, 3},
        {1, 3, 3, 1}
    };

    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            for(int k = 0; k < 4; k++){
                double sum = 0, cnt = 0;
                for(int m = 0; m < 4; m++){
                    for(int n = 0; n < 4; n++){
                        if((i + m - 2) < 0 || (j + n - 2) < 0 || (i + m - 2) >= height || (j + n - 2) >= width){
                            continue;
                        }
                        size_t index = (((i + m - 2) * width) << 2) + ((j + n - 2) << 2);
                        sum += data[index + k] * filter[m][n];
                        cnt += filter[m][n];
                    }
                }
            new_data[((i * width) << 2) + (j << 2) + k] = sum / cnt;
            }
        }
    }

    ClearToBlack();
    angleDegrees = -angleDegrees;
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            for(int k = 0; k < 4; k++){
                int FixI = i - height / 2, FixJ = j - width / 2;

                int rotated_i = cos(angleDegrees * c_pi / 180.f) * FixI
                    + sin(angleDegrees * c_pi / 180.f) * FixJ + height / 2;
                int rotated_j = cos(angleDegrees * c_pi / 180.f) * FixJ
                    - sin(angleDegrees * c_pi / 180.f) * FixI + width / 2;

                if(rotated_i < 0 || rotated_j < 0 || rotated_i >= height || rotated_j >= width){
                    continue;
                }
                data[((i * width) << 2) + (j << 2) + k]
                    = new_data[((rotated_i * width) << 2) + (rotated_j << 2) + k];
            }
        }
    }
    delete[] new_data;

    return true;
}// Rotate


//////////////////////////////////////////////////////////////////////////////
//
//      Given a single RGBA pixel return, via the second argument, the RGB
//      equivalent composited with a black background.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::RGBA_To_RGB(unsigned char *rgba, unsigned char *rgb)
{
    const unsigned char	BACKGROUND[3] = { 0, 0, 0 };

    unsigned char  alpha = rgba[3];

    if (alpha == 0)
    {
        rgb[0] = BACKGROUND[0];
        rgb[1] = BACKGROUND[1];
        rgb[2] = BACKGROUND[2];
    }
    else
    {
	    float	alpha_scale = (float)255 / (float)alpha;
	    int	val;
	    int	i;

	    for (i = 0 ; i < 3 ; i++)
	    {
	        val = (int)floor(rgba[i] * alpha_scale);
	        if (val < 0)
		    rgb[i] = 0;
	        else if (val > 255)
		    rgb[i] = 255;
	        else
		    rgb[i] = val;
	    }
    }
}// RGA_To_RGB


///////////////////////////////////////////////////////////////////////////////
//
//      Copy this into a new image, reversing the rows as it goes. A pointer
//  to the new image is returned.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Reverse_Rows(void)
{
    unsigned char   *dest = new unsigned char[width * height * 4];
    TargaImage	    *result;
    int 	        i, j;

    if (! data)
    	return NULL;

    for (i = 0 ; i < height ; i++)
    {
	    int in_offset = (height - i - 1) * width * 4;
	    int out_offset = ((i * width) << 2);

	    for (j = 0 ; j < width ; j++)
        {
	        dest[out_offset + j * 4] = data[in_offset + j * 4];
	        dest[out_offset + j * 4 + 1] = data[in_offset + j * 4 + 1];
	        dest[out_offset + j * 4 + 2] = data[in_offset + j * 4 + 2];
	        dest[out_offset + j * 4 + 3] = data[in_offset + j * 4 + 3];
        }
    }

    result = new TargaImage(width, height, dest);
    delete[] dest;
    return result;
}// Reverse_Rows


///////////////////////////////////////////////////////////////////////////////
//
//      Clear the image to all black.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::ClearToBlack()
{
    memset(data, 0, width * height * 4);
}// ClearToBlack


///////////////////////////////////////////////////////////////////////////////
//
//      Helper function for the painterly filter; paint a stroke at
// the given location
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::Paint_Stroke(const Stroke& s) {
   int radius_squared = (int)s.radius * (int)s.radius;
   for (int x_off = -((int)s.radius); x_off <= (int)s.radius; x_off++) {
      for (int y_off = -((int)s.radius); y_off <= (int)s.radius; y_off++) {
         int x_loc = (int)s.x + x_off;
         int y_loc = (int)s.y + y_off;
         // are we inside the circle, and inside the image?
         if ((x_loc >= 0 && x_loc < width && y_loc >= 0 && y_loc < height)) {
            int dist_squared = x_off * x_off + y_off * y_off;
            if (dist_squared <= radius_squared) {
               data[(y_loc * width + x_loc) * 4 + 0] = s.r;
               data[(y_loc * width + x_loc) * 4 + 1] = s.g;
               data[(y_loc * width + x_loc) * 4 + 2] = s.b;
               data[(y_loc * width + x_loc) * 4 + 3] = s.a;
            } else if (dist_squared == radius_squared + 1) {
               data[(y_loc * width + x_loc) * 4 + 0] = 
                  (data[(y_loc * width + x_loc) * 4 + 0] + s.r) / 2;
               data[(y_loc * width + x_loc) * 4 + 1] = 
                  (data[(y_loc * width + x_loc) * 4 + 1] + s.g) / 2;
               data[(y_loc * width + x_loc) * 4 + 2] = 
                  (data[(y_loc * width + x_loc) * 4 + 2] + s.b) / 2;
               data[(y_loc * width + x_loc) * 4 + 3] = 
                  (data[(y_loc * width + x_loc) * 4 + 3] + s.a) / 2;
            }
         }
      }
   }
}

///////////////////////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Compare(TargaImage* pImage) {
    if (width != pImage->width) {
        return false;
    }
    if (height != pImage->height) {
        return false;
    }
    for (int i = 0; i < width * height * 4; i++) {
            if (data[i] != pImage->data[i]) return false;
        }
    return true;
}


///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke() {}

///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke(unsigned int iradius, unsigned int ix, unsigned int iy,
               unsigned char ir, unsigned char ig, unsigned char ib, unsigned char ia) :
   radius(iradius),x(ix),y(iy),r(ir),g(ig),b(ib),a(ia)
{
}

