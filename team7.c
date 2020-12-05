#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define N 330 /* frame dimension for QCIF format */
#define M 440 /* frame dimension for QCIF format */
#define filename "dog_440x330.yuv"
#define file_yuv "normalized.yuv"
#define file_yuv_rgb "normalized_RGB.yuv"

/* code for armulator*/
#pragma arm section zidata="ram"
int current_y[N][M];
int temp[N+2][M+2];
int I_x[N][M];
int I_y[N][M];
int gray_image[N][M];
int gaussian_image[N][M];
int norm_image[N][M];
int norm_Y[N][M];
int norm_U[N][M];
int norm_V[N][M];
int Gauss[3][3];
int Sobel_x[3][3];
int Sobel_y[3][3];
float theta[N][M];
float magnitude[N][M];
float diff;
int max,min;
#pragma arm section

int i,j;


void read()
{
    FILE *frame_c;
    if((frame_c=fopen(filename,"rb"))==NULL)
    {
        printf("current frame doesn't exist\n");
        exit(-1);
    }

    for(i=0;i<N;i++)
    {
        for(j=0;j<M;j+=2)
        {
            gray_image[i][j]=fgetc(frame_c);
            gray_image[i][j+1]=fgetc(frame_c);
        }
    }
    fclose(frame_c);
}

void write()
{
    FILE *frame_yuv;
    frame_yuv=fopen(file_yuv,"wb");

    for(i=0;i<N;i++)
    {
        for(j=0;j<M;j+=2)
        {
            fputc(norm_image[i][j],frame_yuv);
            fputc(norm_image[i][j+1],frame_yuv);
        }
    }

    fclose(frame_yuv);
}

void write_color()
{
    FILE *frame_yuv_rgb;
    frame_yuv_rgb=fopen(file_yuv_rgb,"wb");

    for(i=0;i<N;i++)
    {
        for(j=0;j<M;j+=2)
        {
            fputc(norm_Y[i][j],frame_yuv_rgb);
            fputc(norm_Y[i][j+1],frame_yuv_rgb);
        }
    }
    for(i=0;i<N;i++)
    {
        for(j=0;j<M;j+=2)
        {
            fputc(norm_U[i][j],frame_yuv_rgb);
            fputc(norm_U[i][j+1],frame_yuv_rgb);
        }
    }
    for(i=0;i<N;i++)
    {
        for(j=0;j<M;j+=2)
        {
            fputc(norm_V[i][j],frame_yuv_rgb);
            fputc(norm_V[i][j+1],frame_yuv_rgb);
        }
    }
    fclose(frame_yuv_rgb);
}

/////////////////////////////////////
void convolution(int (in)[N][M], int (kernel)[3][3], int (out)[N][M]){
    //insert image to temp array
    for(i=0;i<(N+2);++i)
    {
        for(j=0;j<(M+2);j+=4)
        {
            temp[i][j]=0;
            temp[i][j+1]=0;
            temp[i][j+2]=0;
            temp[i][j+3]=0;
        }
    }
    for(i=1;i<(N+1);++i)
    {
        for(j=1;j<(M+1);j+=2)
        {
            temp[i][j]=in[i-1][j-1]; // join these two to save clocks if possible
            temp[i][j+1]=in[i-1][j];
        }
    }
    for(i=1;i<(N+1);++i)
    {
        for(j=1;j<(M+1);++j)
        {
            out[i-1][j-1]=temp[i-1][j-1]*kernel[0][0]+temp[i-1][j]*kernel[0][1]+temp[i-1][j+1]*kernel[0][2]+temp[i][j-1]*kernel[1][0]+temp[i][j]*kernel[1][1]+temp[i][j+1]*kernel[1][2]+temp[i+1][j-1]*kernel[2][0]+temp[i+1][j]*kernel[2][1]+temp[i+1][j+1]*kernel[2][2];
        }
    }
}

////////////////////////////////////////
int main()
{
    // read grayscale image
    read();

    //create gaussian filter
    Gauss[0][0]=1;Gauss[0][1]=2;Gauss[0][2]=1;
    Gauss[1][0]=2;Gauss[1][1]=4;Gauss[1][2]=2;
    Gauss[2][0]=1;Gauss[2][1]=2;Gauss[2][2]=1;

    //Perform convolution and get gaussian image
    convolution(gray_image, Gauss, gaussian_image);

    //create Sobel filters
    Sobel_x[0][0]=-1;Sobel_x[0][1]=0;Sobel_x[0][2]=1;
    Sobel_x[1][0]=-2;Sobel_x[1][1]=0;Sobel_x[1][2]=2;
    Sobel_x[2][0]=-1;Sobel_x[2][1]=0;Sobel_x[2][2]=1;

    Sobel_y[0][0]=-1;Sobel_y[0][1]=-2;Sobel_y[0][2]=-1;
    Sobel_y[1][0]=0;Sobel_y[1][1]=0;Sobel_y[1][2]=0;
    Sobel_y[2][0]=1;Sobel_y[2][1]=2;Sobel_y[2][2]=1;
    
    //Get the image gradients Ix and Iy
    convolution(gaussian_image, Sobel_x, I_x);
    convolution(gaussian_image, Sobel_y, I_y);

    //compute Gradient theta and magnitude
    //(JOIN in same loop) find Max and min magnitude value
    max=0;
    min=10000;
    for(i=0;i<N;++i)
    {
        for(j=0;j<M;j+=2)
        {
            theta[i][j] = atan2(I_x[i][j],I_y[i][j]);
            magnitude[i][j] = sqrt(I_x[i][j]*I_x[i][j] + I_y[i][j]*I_y[i][j]);

            theta[i][j+1] = atan2(I_x[i][j+1],I_y[i][j+1]);
            magnitude[i][j+1] = sqrt(I_x[i][j+1]*I_x[i][j+1] + I_y[i][j+1]*I_y[i][j+1]);

            if(magnitude[i][j] > max ){
                max = magnitude[i][j];
            }else if (magnitude[i][j] < min ){
                min = magnitude[i][j];
            }
            if(magnitude[i][j+1] > max ){
                max = magnitude[i][j+1];
            }else if (magnitude[i][j+1] < min ){
                min = magnitude[i][j+1];
            }
        }
    }

    //normalize to 0-255 and determine edge color
    diff = max - min;
    for(i=0;i<N;++i)
    {
        for(j=0;j<M;++j)
        {
            norm_image[i][j] = ((magnitude[i][j] - min)/diff)*255; //norm to 0-255 range
            if ( norm_image[i][j] > 45) {
                if (((abs(theta[i][j] - 1.57)) < 0.4) || ((abs(theta[i][j] + 1.57)) < 0.4)) {
                    //yellow for vertical lines
                    norm_Y[i][j] = 225;
                    norm_U[i][j] = 0;
                    norm_V[i][j] =  148;
                }else if (((abs(theta[i][j])) < 0.4) || ((abs(theta[i][j] - 3.14)) < 0.4)) {
                    //blue for horizontal lines
                    norm_Y[i][j] = 29;
                    norm_U[i][j] = 255;
                    norm_V[i][j] = 107;
                }else if (((abs(theta[i][j] - 0.78)) < 0.4) || ((abs(theta[i][j] + 2.35)) < 0.4)) {
                    //red for 45deg lines
                    norm_Y[i][j] = 76;
                    norm_U[i][j] = 84;
                    norm_V[i][j] = 255;
                }else if (((abs(theta[i][j] + 0.78)) < 0.4) || ((abs(theta[i][j] - 2.35)) < 0.4)) {
                    //green for -45deg lines
                    norm_Y[i][j] = 149;
                    norm_U[i][j] = 43;
                    norm_V[i][j] = 21 ;
                }
            }else{
                norm_Y[i][j] = 0;
                norm_U[i][j] = 128;
                norm_V[i][j] = 128;
            }
        }
    }
    write();
    write_color();
}
