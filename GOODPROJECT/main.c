#include "lodepng.c"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
void gaussianBlur(unsigned char* image, unsigned char* blurred, int radius, float sigma, unsigned image_width, unsigned image_height){
    int kernel_size=2*radius+1;
    float* kernel=(float*)malloc(kernel_size*kernel_size*sizeof(float));
    float kernel_sum=0.0f;
    for (int i=-radius;i<=radius;i++){
        for (int j=-radius;j<=radius;j++){
            int idx = (i+radius)*kernel_size+(j+radius);
            kernel[idx]=expf(-(i*i+j*j)/(2.0f*sigma*sigma))/(2.0f*M_PI*sigma*sigma);
            kernel_sum+=kernel[idx];
        }
    }
    for (int i=0;i<kernel_size*kernel_size;i++){
        kernel[i]/=kernel_sum;
    }
    for (int y=0;y<image_height;y++){
        for (int x=0;x<image_width;x++){
            float r=0.0f,g=0.0f,b=0.0f;
            for (int i = -radius; i <= radius; i++){
                for (int j=-radius;j<=radius;j++){
                    int nx=x+j;
                    int ny=y+i;
                    if (nx<0) nx=0;
                    if (nx>=image_width) nx=image_width-1;
                    if (ny<0) ny=0;
                    if (ny>=image_height) ny=image_height-1;

                    int offset=(ny*image_width+nx)*4;
                    float kernel_value=kernel[(i+radius)*kernel_size+(j+radius)];
                    r+=image[offset]*kernel_value;
                    g+=image[offset+1]*kernel_value;
                    b+=image[offset+2]*kernel_value;
                }
            }

            int current_offset=(y*image_width+x)*4;
            blurred[current_offset]=(unsigned char)fminf(255.0f,r);
            blurred[current_offset+1]=(unsigned char)fminf(255.0f,g);
            blurred[current_offset+2]=(unsigned char)fminf(255.0f,b);
            blurred[current_offset+3]=255;
        }
    }
    free(kernel);
}
void applyRobertsOperator(unsigned char* image, int width, int height) {
    unsigned char* edge_image=malloc(width*height*4*sizeof(unsigned char));
    for (int i=0;i<height-1;i++){
        for (int j=0;j<width-1;j++){
            int gradient_x=abs(image[4*(i*width+j)]-image[4*((i+1)*width+(j+1))]);
            int gradient_y=abs(image[4*(i*width+(j+1))]-image[4*((i+1)*width+j)]);
            int gradient=(int)sqrt((double)(gradient_x*gradient_x+gradient_y*gradient_y));
            for (int k=0;k<3;k++){
                edge_image[4*(i*width+j)+k]=(gradient>30)?255:image[4*(i*width+j)+k];
            }
            edge_image[4*(i*width+j)+3]=image[4*(i*width+j)+3];
        }
    }
    for (int i=0;i<height*width*4;i++){
        image[i]=edge_image[i];
    }
    free(edge_image);
}
void applySobelOperator(unsigned char* image, unsigned width, unsigned height){
    unsigned char* edge_image=malloc(width*height*4*sizeof(unsigned char));
    int sobel_horizontal[3][3]={
        {-1,0,1},
        {-2,0,2},
        {-1,0,1}
    };
    int sobel_vertical[3][3]={
        {1,2,1},
        {0,0,0},
        {-1,-2,-1}
    };
    for (int y=1;y<height-1;y++){
        for (int x=1;x<width-1;x++){
            int gx=0, gy=0;
            for (int i=-1;i<=1;i++){
                for (int j=-1;j<=1;j++){
                    int idx=((y+i)*width+(x+j))*4;
                    gx+=image[idx]*sobel_horizontal[i+1][j+1];
                    gy+=image[idx]*sobel_vertical[i+1][j+1];
                }
            }
            int idx=(y*width+x)*4;
            float gradient=sqrt(pow(gx,2)+pow(gy,2));
            for (int k=0;k<3;k++){
                edge_image[idx+k]=(gradient>50)?255:image[idx+k];
            }
            edge_image[idx+3]=255;
        }
    }
    for (int i=0;i<height*width*4;i++){
        image[i]=edge_image[i];
    }
    free(edge_image);
}
void hysteresisThresholding(unsigned char* edge_image, int x, int y, unsigned width, unsigned height){
    for (int i=-1;i<=1;i++){
        for(int j=-1;j<=1;j++){
            int nx=x+i;
            int ny=y+j;
            if(nx>=0 && nx<width && ny>=0 && ny<height){
                int idx=4*(ny*width+nx);
                for(int k=0;k<3;k++){
                     if (edge_image[idx+k]==100){
                        edge_image[idx+k]=255;
                        hysteresisThresholding(edge_image,nx,ny,width,height);
                    }
                }

            }
        }
    }
}
void detectorCanny(unsigned char* image, unsigned width, unsigned height,unsigned upper_threshold,unsigned lower_threshold){
    unsigned char* edge_image=calloc(width*height*4,sizeof(unsigned char));
    unsigned char* gradient=calloc(width*height*4,sizeof(unsigned char));
    unsigned char* edge_direction=calloc(width*height*4,sizeof(unsigned char));
    int sobel_horizontal[3][3]={
        {-1,0,1},
        {-2,0,2},
        {-1,0,1}
    };
    int sobel_vertical[3][3]={
        {-1,-2,-1},
        {0,0,0},
        {1,2,1}
    };
    for (int y=1;y<height-1;y++){
        for (int x=1;x<width-1;x++){
            int gx=0, gy=0;
            for (int i=-1;i<=1;i++){
                for (int j=-1;j<=1;j++){
                    int idx=((y+i)*width+(x+j))*4;
                    gx+=image[idx]*sobel_horizontal[i+1][j+1];
                    gy+=image[idx]*sobel_vertical[i+1][j+1];
                }
            }
            int idx=4*(y*width+x);
            float grad=sqrt(pow(gx,2)+pow(gy,2));
            float arctan=atan2(gy,gx);
            for (int k=0;k<3;k++){
                gradient[idx+k]=grad;
                edge_direction[idx+k]=arctan;
            }
        }
    }
    for (int y=1;y<height-1;y++){
        for (int x=1;x<width-1;x++){
            int dx=0,dy=0;
            int idx=4*(y*width+x);
            int value=gradient[idx];
            if (edge_direction[idx]<22.5 || edge_direction[idx]>=157.5){
                dx=1;
                dy=0;
            }else if (edge_direction[idx]>=22.5 && edge_direction[idx]<67.5){
                dx=1;
                dy=-1;
            }else if (edge_direction[idx]>67.5 && edge_direction[idx]<112.5){
                dx=0;
                dy=-1;
            }else{
                dx=-1;
                dy=-1;
            }
            int val1=gradient[(y+dy)*width+(x+dx)];
            int val2=gradient[(y-dy)*width+(x-dx)];
            for (int k=0;k<3;k++){
                if (value>=val1 && value>=val2){
                edge_image[idx+k]=value;
                }else{
                    edge_image[idx+k]=0;
                }
                if (gradient[idx+k]>=upper_threshold){
                    edge_image[idx+k]=255;
                }else if (gradient[idx+k]>=lower_threshold){
                    edge_image[idx+k]=100;
                }else{
                    edge_image[idx+k]=0;
                }
            }
            edge_image[idx+3]=255;
        }
    }
    for (int y=1;y<height-1;y++){
        for (int x=1;x<width-1;x++){
            int idx=4*(y*width+x);
            if(edge_image[idx]==255){
                hysteresisThresholding(edge_image,x,y,width,height);
            }
        }
    }
    for (int i=0;i<height*width*4;i++){
        image[i]=edge_image[i];
    }
    free(gradient);
    free(edge_direction);
    free(edge_image);
}
unsigned char* convert_to_grayscale(unsigned char* image, unsigned size) {
    unsigned char* bw_image=malloc(size*sizeof(unsigned char));
    for (unsigned i=0;i<size;i+=4) {
        unsigned char gray=(image[i]+image[i+1]+image[i+2])/3;
        gray=(gray<=255)?gray:255;
        bw_image[i]=bw_image[i+1]=bw_image[i+2]=gray;
        bw_image[i+3]=255;

    }
    return bw_image;
}
typedef struct{
    int src;
    int dest;
}Edge;
typedef struct Set{
    int parent;
    int rank;
}Set;
void Make_Set(Set* sets,int v){
    sets[v].parent=v;
    sets[v].rank=0;
}
int Find_Set(Set* sets,int v){
    if(v!=sets[v].parent){
        sets[v].parent=Find_Set(sets,sets[v].parent);
    }
    return sets[v].parent;
}
void Union(Set* sets, int i, int j){
    int root1=Find_Set(sets,i);
    int root2=Find_Set(sets,j);
    if(root1!=root2){
        if (sets[root1].rank<sets[root2].rank){
            sets[root1].parent=root2;
        }else if(sets[root1].rank>sets[root2].rank){
            sets[root2].parent=root1;
        }else{
            sets[root2].parent=root1;
            sets[root1].rank++;
        }
    }
}
void Paint(Set* sets,unsigned char* image,int V,unsigned R,unsigned G,unsigned B){
    int* roots=malloc(V*sizeof(int));
    int** components=malloc(V*sizeof(int*));
    for (int i=0;i<V;i++){components[i]=calloc(1,sizeof(int));}
    for (int i=0;i<V;i++){
        int root=Find_Set(sets,i);
        roots[i]=root;
        components[root]=realloc(components[root],((components[root][0])+2)*sizeof(int));
        components[root][0]++;
        components[root][components[root][0]]=i;
    }
    for (int i=0;i<V;i++){
        if(roots[i]==i){
            R=(R+137)%256;
            G=(G+215)%256;
            B=(B+123)%256;
            for (int j=1;j<=components[i][0];j++){
                int current=components[i][j]+1;
                if ((4*current+3)>=4*V) continue;
                image[4*current]=R;
                image[4*current+1]=G;
                image[4*current+2]=B;
                image[4*current+3]=255;
            }
        }
    }
    for(int i=0;i<V;i++){
            free(components[i]);
    }
    free(components);
    free(roots);
}
int main(){
    unsigned w, h;
    const char* filename="1.png";
    unsigned char* image=NULL;
    unsigned error=lodepng_decode32_file(&image, &w, &h, filename);
    unsigned char* bw_image=convert_to_grayscale(image,4*w*h);
    unsigned char* blurred_image=malloc(4*w*h*sizeof(unsigned char));
    gaussianBlur(bw_image, blurred_image,4,2,w,h);
    detectorCanny(blurred_image,w,h,17,15);
//    applySobelOperator(bw_image,w,h);
//    applyRobertsOperator(bw_image,w,h);
    unsigned error1=lodepng_encode32_file("output1.png",blurred_image, w, h);
    int V=h*w;
    struct Set* sets=malloc(V*sizeof(Set));
    for (int i=0;i<V;i++) Make_Set(sets,i);
    unsigned char* image1=calloc(4*w*h,sizeof(unsigned char));
    unsigned char delta=2;
    unsigned char R=0;
    unsigned char G=0;
    unsigned char B=0;
    int i,j=0;
    image1[0]=R;
    image1[1]=G;
    image1[2]=B;
    image1[3]=255;
    for (i=0;i<h;i++){
        for (j=0;j<w;j++){
            int pixel0=4*(i*w+j);
            int vertex0=i*w+j;
            int pixel1=4*(i*w+j+1);
            int vertex1=i*w+j+1;
            int pixel2=4*((i+1)*w+j);
            int vertex2=(i+1)*w+j;
            int pixel3=4*((i+1)*w+j+1);
            int vertex3=(i+1)*w+j+1;
            if(pixel0+3>=4*h*w || pixel1+3>=4*h*w || pixel2+3>=4*h*w || pixel3+3>=4*h*w){
                continue;
            }
            unsigned char x0=blurred_image[pixel0];
            unsigned char x1=(j+1<w)?blurred_image[pixel1]:-1;
            unsigned char x2=(i+1<h)?blurred_image[pixel2]:-1;
            unsigned char x3=((i+1<h) && (j+1<w))?blurred_image[pixel3]:-1;
            if(abs(x0-x1)<=delta && x1!=-1){
                Union(sets,vertex0,vertex1);
            }
            if(abs(x0-x2)<=delta && x2!=-1){
                Union(sets,vertex0,vertex2);
            }
            if(abs(x0-x3)<=delta && x3!=-1){
                Union(sets,vertex0,vertex3);
            }
        }
    }
    Paint(sets,image1,V,R,G,B);
    unsigned error2=lodepng_encode32_file("output3.png", image1, w, h);

    free(sets);
    free(image);
    free(bw_image);
    free(blurred_image);
    free(image1);
    return 0;
}
