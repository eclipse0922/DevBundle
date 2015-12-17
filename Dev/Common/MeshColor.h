#ifndef MESHCOLOR_H
#define MESHCOLOR_H
#include <iostream>
#include <stdint-gcc.h>
#include <common_global.h>
#include <armadillo>
namespace ColorArray
{
    extern "C"{
        typedef union{
            struct{
                uint8_t r0;
                uint8_t g0;
                uint8_t b0;
                uint8_t a0;
                uint8_t r1;
                uint8_t g1;
                uint8_t b1;
                uint8_t a1;
            }rgba;
            uint64_t color;
        }RGB64;

        typedef union{
            uint32_t color;
            struct{
                uint8_t r;
                uint8_t g;
                uint8_t b;
                uint8_t a;
            }rgba;
        }RGB32;

        const int32_t  DefaultColorNum_ = 59;
        extern RGB32 DefaultColor[DefaultColorNum_];
    }

    typedef struct RGBArray{
        static const uint8_t dim_ = 3;
        long size_ = 0;
        uint8_t* data_=NULL;
        void reset(long size,uint8_t r,uint8_t g, uint8_t b);
        ~RGBArray(){if(data_)delete[]data_;}
        }RGBArray;
    void COMMONSHARED_EXPORT Lab2RGB(const arma::fmat& Lab, arma::Mat<uint8_t>& rgb);
    void COMMONSHARED_EXPORT RGB2Lab(const arma::Mat<uint8_t>& rgb, arma::fmat& Lab);
    void COMMONSHARED_EXPORT RGB2Lab(const arma::Col<uint8_t>& rgb, arma::fvec& Lab);
    void COMMONSHARED_EXPORT Lab2BGR(const arma::fmat& Lab, arma::Mat<uint8_t>& rgb);
    void COMMONSHARED_EXPORT BGR2Lab(const arma::Mat<uint8_t>& rgb, arma::fmat& Lab);
    void COMMONSHARED_EXPORT BGR2Lab(const arma::Col<uint8_t>& rgb, arma::fvec& Lab);

    extern const float COMMONSHARED_EXPORT Lab_L_min;
    extern const float COMMONSHARED_EXPORT Lab_L_max;
    extern const float COMMONSHARED_EXPORT Lab_ab_min;
    extern const float COMMONSHARED_EXPORT Lab_ab_max;
}

template <typename M>
class MeshColor
{
    typedef M Mesh;
public:
    MeshColor(const Mesh&);
    MeshColor(const MeshColor &);
    ColorArray::RGBArray vertex_colors_array(void);
    void* vertex_colors(void);
    void fromlabel(const arma::uvec&);
protected:
    ColorArray::RGBArray v_colors;
private:
    const Mesh& Ref_;//the ref Mesh that bound with this custom color
};
#include "MeshColor.hpp"
#endif // MESHCOLOR_H
