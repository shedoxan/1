#include "lodepng.c"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
void gaussian_blur(unsigned char* image, unsigned char* blurred, int radius, unsigned image_width, unsigned image_height){
    float kernel[2*radius+1];
    float sum=0.0f;
    for (int i=-radius;i<=radius;i++){
        kernel[i+radius]=expf(-(i*i)/(2.0f*radius*radius));
        sum+=kernel[i+radius];
    }
    for (int i=0;i<2*radius+1;i++){
        kernel[i]/=sum;
    }
    for (int y=0;y<image_height;y++){
        for (int x=0;x<image_width;x++){
            float r=0.0f,g=0.0f,b=0.0f;
            for (int i=-radius;i<=radius;i++){
                int nx=x+i;
                if (nx<0) nx=0;
                if (nx>=image_width) nx=image_width-1;
                int offset=(y*image_width+nx)*4;
                r+=image[offset+0]*kernel[i+radius];
                g+=image[offset+1]*kernel[i+radius];
                b+=image[offset+2]*kernel[i+radius];
            }
            int offset=(y*image_width+x)*4;
            if (offset+3<4*image_height*image_width){
                 blurred[offset+0]=(unsigned char)fminf(255.0f,r);
                blurred[offset+1]=(unsigned char)fminf(255.0f,g);
                blurred[offset+2]=(unsigned char)fminf(255.0f,b);
                blurred[offset+3]=255;
            }
        }
    }
}
unsigned char AVG(unsigned char a,unsigned char b) {
    return (a+b)/2;
}
unsigned char* convert_to_grayscale(unsigned char* image, unsigned size) {
    unsigned char* bw_image=malloc(size*sizeof(unsigned char));
    for (unsigned i=0;i<size;i+=4) {
        unsigned char gray=(image[i]+image[i + 1]+image[i+2])/3;
        bw_image[i]=bw_image[i+1]=bw_image[i+2]=gray;
        bw_image[i+3]=255;
    }
    return bw_image;
}
int main(){
    unsigned w, h;
    const char* filename="1.png";
    unsigned char* image=NULL;
    unsigned error=lodepng_decode32_file(&image, &w, &h, filename);
    unsigned char* bw_image=convert_to_grayscale(image,4*w*h);
    unsigned char* blurred_image=malloc(4*w*h*sizeof(unsigned char));
    gaussian_blur(bw_image, blurred_image,1,w,h);
    unsigned error1 = lodepng_encode32_file("output1.png", blurred_image, w, h);

    unsigned char* image1=calloc(4*w*h,sizeof(unsigned char));
    unsigned char delta=10;
    unsigned char R=125;
    unsigned char G=100;
    unsigned char B=50;
    int i,j=0;
//    printf("%d %d ",h,w);
    image1[0]=R;
    image1[1]=G;
    image1[2]=B;
    image1[3]=255;
    for (i=0;i<h;i++){
        for (j=0;j<w;j++){
            int pixel0=4*(i*w+j);
            int pixel1=4*(i*w+j+1);
            int pixel2=4*((i+1)*w+j);
            int pixel3=4*((i+1)*w+j+1);
            if(pixel0+3>=4*h*w || pixel1+3>=4*h*w || pixel2+3>=4*h*w || pixel3+3>=4*h*w){
                continue;
            }
            unsigned char x0=blurred_image[pixel0];
            unsigned char x1=(j+1<w)?blurred_image[pixel1]:-1;
            unsigned char x2=(i+1<h)?blurred_image[pixel2]:-1;
            unsigned char x3=((i+1<h) && (j+1<w))?blurred_image[pixel3]:-1;
            unsigned char local_delta=0;
            int count=0;
            if (x1!=-1){
                local_delta+=abs(x0-x1);
                count++;
            }
            if (x2!=-1){
                local_delta+=abs(x0-x2);
                count++;
            }
            if (x3!=-1){
                local_delta+=abs(x0-x3);
                count++;
            }
            local_delta=(count==0)?delta:local_delta/count;
            if ((abs(x0-x1)>2*local_delta) && (abs(x0-x2)>2*local_delta) && (abs(x0-x3)>2*local_delta) && x1!=-1 && x2!=-1 && x3!=-1){
                R=(R+20+R*211)%256;
                G=(G+25+G*121)%256;
                B=(B+20+B*341)%256;
                if(abs(x1-x2)<local_delta){
                    image1[pixel1]=R;
                    image1[pixel1+1]=G;
                    image1[pixel1+ 2]=B;
                    image1[pixel1+3]=255;

                    image1[pixel2]=R;
                    image1[pixel2+1]=G;
                    image1[pixel2+2]=B;
                    image1[pixel2+3]=255;
                }
                if(abs(x1-x3)<local_delta){
                    image1[pixel1]=R;
                    image1[pixel1+1]=G;
                    image1[pixel1+2]=B;
                    image1[pixel1+3]=255;

                    image1[pixel3]=R;
                    image1[pixel3+1]=G;
                    image1[pixel3+2]=B;
                    image1[pixel3+3]=255;
                }
                if(abs(x2-x3)<local_delta){
                    image1[pixel2]=R;
                    image1[pixel2+1]=G;
                    image1[pixel2+2]=B;
                    image1[pixel2+3]=255;

                    image1[pixel3]=R;
                    image1[pixel3+1]=G;
                    image1[pixel3+2]=B;
                    image1[pixel3+3]=255;
                }
            }
            if (abs(x0-x1)<local_delta && x1!=-1){
                image1[pixel1]=R;
                image1[pixel1+1]=G;
                image1[pixel1+2]=B;
                image1[pixel1+3]=255;
            }
            if (abs(x0-x2)<local_delta && x2!=-1){
                image1[pixel2]=R;
                image1[pixel2+1]=G;
                image1[pixel2+2]=B;
                image1[pixel2+3]=255;
            }
            if(abs(x0-x1)<local_delta && abs(x0-x2)<local_delta && x1!=-1 && x2!=-1){
                image1[pixel1]=R;
                image1[pixel1+1]=G;
                image1[pixel1+2]=B;
                image1[pixel1+3]=255;

                image1[pixel2]=R;
                image1[pixel2+1]=G;
                image1[pixel2+2]=B;
                image1[pixel2+3]=255;

                R=(R+137)%256;
                G=(G+215)%256;
                B=(B+123)%256;

                image1[pixel3]=R;
                image1[pixel3+1]=G;
                image1[pixel3+2]=B;
                image1[pixel3+3]=255;
            }
            if(abs(x0-x2)<local_delta && abs(x0-x3)<local_delta && x2!=-1 && x3!=-1){
                image1[pixel2]=R;
                image1[pixel2+1]=G;
                image1[pixel2+2]=B;
                image1[pixel2+3]=255;

                image1[pixel3]=R;
                image1[pixel3+1]=G;
                image1[pixel3+2]=B;
                image1[pixel3+3]=255;

                R=(R+137)%256;
                G=(G+215)%256;
                B=(B+123)%256;

                image1[pixel1]=R;
                image1[pixel1+1]=G;
                image1[pixel1+2]=B;
                image1[pixel1+3]=255;
            }
            if(abs(x0-x1)<local_delta && abs(x0-x3)<local_delta && x1!=-1 && x3!=-1){
                image1[pixel1]=R;
                image1[pixel1+1]=G;
                image1[pixel1+2]=B;
                image1[pixel1+3]=255;

                image1[pixel3]=R;
                image1[pixel3+1]=G;
                image1[pixel3+2]=B;
                image1[pixel3+3]=255;

                R=(R+137)%256;
                G=(G+215)%256;
                B=(B+123)%256;

                image1[pixel2]=R;
                image1[pixel2+1]=G;
                image1[pixel2+2]=B;
                image1[pixel2+3]=255;
            }
        }
    }

    unsigned error2 = lodepng_encode32_file("output4.png", image1, w, h);

    free(image);
    free(bw_image);
    free(blurred_image);
    free(image1);

    return 0;
}
