/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "precomp.hpp"

using namespace cv;
using namespace cv::gpu;

#if !defined (HAVE_CUDA) || defined (CUDA_DISABLER)

void cv::gpu::gemm(const GpuMat&, const GpuMat&, double, const GpuMat&, double, GpuMat&, int, Stream&) { throw_no_cuda(); }
void cv::gpu::transpose(const GpuMat&, GpuMat&, Stream&) { throw_no_cuda(); }
void cv::gpu::flip(const GpuMat&, GpuMat&, int, Stream&) { throw_no_cuda(); }
void cv::gpu::LUT(const GpuMat&, const Mat&, GpuMat&, Stream&) { throw_no_cuda(); }
void cv::gpu::magnitude(const GpuMat&, GpuMat&, Stream&) { throw_no_cuda(); }
void cv::gpu::magnitudeSqr(const GpuMat&, GpuMat&, Stream&) { throw_no_cuda(); }
void cv::gpu::magnitude(const GpuMat&, const GpuMat&, GpuMat&, Stream&) { throw_no_cuda(); }
void cv::gpu::magnitudeSqr(const GpuMat&, const GpuMat&, GpuMat&, Stream&) { throw_no_cuda(); }
void cv::gpu::phase(const GpuMat&, const GpuMat&, GpuMat&, bool, Stream&) { throw_no_cuda(); }
void cv::gpu::cartToPolar(const GpuMat&, const GpuMat&, GpuMat&, GpuMat&, bool, Stream&) { throw_no_cuda(); }
void cv::gpu::polarToCart(const GpuMat&, const GpuMat&, GpuMat&, GpuMat&, bool, Stream&) { throw_no_cuda(); }
void cv::gpu::normalize(const GpuMat&, GpuMat&, double, double, int, int, const GpuMat&) { throw_no_cuda(); }
void cv::gpu::normalize(const GpuMat&, GpuMat&, double, double, int, int, const GpuMat&, GpuMat&, GpuMat&) { throw_no_cuda(); }
void cv::gpu::copyMakeBorder(const GpuMat&, GpuMat&, int, int, int, int, int, const Scalar&, Stream&) { throw_no_cuda(); }
void cv::gpu::integral(const GpuMat&, GpuMat&, Stream&) { throw_no_cuda(); }
void cv::gpu::integralBuffered(const GpuMat&, GpuMat&, GpuMat&, Stream&) { throw_no_cuda(); }
void cv::gpu::sqrIntegral(const GpuMat&, GpuMat&, Stream&) { throw_no_cuda(); }

#else /* !defined (HAVE_CUDA) */

////////////////////////////////////////////////////////////////////////
// gemm

#ifdef HAVE_CUBLAS

namespace
{
    #define error_entry(entry)  { entry, #entry }

    struct ErrorEntry
    {
        int code;
        const char* str;
    };

    struct ErrorEntryComparer
    {
        int code;
        ErrorEntryComparer(int code_) : code(code_) {}
        bool operator()(const ErrorEntry& e) const { return e.code == code; }
    };

    const ErrorEntry cublas_errors[] =
    {
        error_entry( CUBLAS_STATUS_SUCCESS ),
        error_entry( CUBLAS_STATUS_NOT_INITIALIZED ),
        error_entry( CUBLAS_STATUS_ALLOC_FAILED ),
        error_entry( CUBLAS_STATUS_INVALID_VALUE ),
        error_entry( CUBLAS_STATUS_ARCH_MISMATCH ),
        error_entry( CUBLAS_STATUS_MAPPING_ERROR ),
        error_entry( CUBLAS_STATUS_EXECUTION_FAILED ),
        error_entry( CUBLAS_STATUS_INTERNAL_ERROR )
    };

    const size_t cublas_error_num = sizeof(cublas_errors) / sizeof(cublas_errors[0]);

    static inline void ___cublasSafeCall(cublasStatus_t err, const char* file, const int line, const char* func)
    {
        if (CUBLAS_STATUS_SUCCESS != err)
        {
            size_t idx = std::find_if(cublas_errors, cublas_errors + cublas_error_num, ErrorEntryComparer(err)) - cublas_errors;

            const char* msg = (idx != cublas_error_num) ? cublas_errors[idx].str : "Unknown error code";
            String str = cv::format("%s [Code = %d]", msg, err);

            cv::error(cv::Error::GpuApiCallError, str, func, file, line);
        }
    }
}

#if defined(__GNUC__)
    #define cublasSafeCall(expr)  ___cublasSafeCall(expr, __FILE__, __LINE__, __func__)
#else /* defined(__CUDACC__) || defined(__MSVC__) */
    #define cublasSafeCall(expr)  ___cublasSafeCall(expr, __FILE__, __LINE__, "")
#endif

#endif

void cv::gpu::gemm(const GpuMat& src1, const GpuMat& src2, double alpha, const GpuMat& src3, double beta, GpuMat& dst, int flags, Stream& stream)
{
#ifndef HAVE_CUBLAS
    (void)src1;
    (void)src2;
    (void)alpha;
    (void)src3;
    (void)beta;
    (void)dst;
    (void)flags;
    (void)stream;
    CV_Error(cv::Error::StsNotImplemented, "The library was build without CUBLAS");
#else
    // CUBLAS works with column-major matrices

    CV_Assert(src1.type() == CV_32FC1 || src1.type() == CV_32FC2 || src1.type() == CV_64FC1 || src1.type() == CV_64FC2);
    CV_Assert(src2.type() == src1.type() && (src3.empty() || src3.type() == src1.type()));

    if (src1.depth() == CV_64F)
    {
        if (!deviceSupports(NATIVE_DOUBLE))
            CV_Error(cv::Error::StsUnsupportedFormat, "The device doesn't support double");
    }

    bool tr1 = (flags & GEMM_1_T) != 0;
    bool tr2 = (flags & GEMM_2_T) != 0;
    bool tr3 = (flags & GEMM_3_T) != 0;

    if (src1.type() == CV_64FC2)
    {
        if (tr1 || tr2 || tr3)
            CV_Error(cv::Error::StsNotImplemented, "transpose operation doesn't implemented for CV_64FC2 type");
    }

    Size src1Size = tr1 ? Size(src1.rows, src1.cols) : src1.size();
    Size src2Size = tr2 ? Size(src2.rows, src2.cols) : src2.size();
    Size src3Size = tr3 ? Size(src3.rows, src3.cols) : src3.size();
    Size dstSize(src2Size.width, src1Size.height);

    CV_Assert(src1Size.width == src2Size.height);
    CV_Assert(src3.empty() || src3Size == dstSize);

    dst.create(dstSize, src1.type());

    if (beta != 0)
    {
        if (src3.empty())
        {
            if (stream)
                stream.enqueueMemSet(dst, Scalar::all(0));
            else
                dst.setTo(Scalar::all(0));
        }
        else
        {
            if (tr3)
            {
                transpose(src3, dst, stream);
            }
            else
            {
                if (stream)
                    stream.enqueueCopy(src3, dst);
                else
                    src3.copyTo(dst);
            }
        }
    }

    cublasHandle_t handle;
    cublasSafeCall( cublasCreate_v2(&handle) );

    cublasSafeCall( cublasSetStream_v2(handle, StreamAccessor::getStream(stream)) );

    cublasSafeCall( cublasSetPointerMode_v2(handle, CUBLAS_POINTER_MODE_HOST) );

    const float alphaf = static_cast<float>(alpha);
    const float betaf = static_cast<float>(beta);

    const cuComplex alphacf = make_cuComplex(alphaf, 0);
    const cuComplex betacf = make_cuComplex(betaf, 0);

    const cuDoubleComplex alphac = make_cuDoubleComplex(alpha, 0);
    const cuDoubleComplex betac = make_cuDoubleComplex(beta, 0);

    cublasOperation_t transa = tr2 ? CUBLAS_OP_T : CUBLAS_OP_N;
    cublasOperation_t transb = tr1 ? CUBLAS_OP_T : CUBLAS_OP_N;

    switch (src1.type())
    {
    case CV_32FC1:
        cublasSafeCall( cublasSgemm_v2(handle, transa, transb, tr2 ? src2.rows : src2.cols, tr1 ? src1.cols : src1.rows, tr2 ? src2.cols : src2.rows,
            &alphaf,
            src2.ptr<float>(), static_cast<int>(src2.step / sizeof(float)),
            src1.ptr<float>(), static_cast<int>(src1.step / sizeof(float)),
            &betaf,
            dst.ptr<float>(), static_cast<int>(dst.step / sizeof(float))) );
        break;

    case CV_64FC1:
        cublasSafeCall( cublasDgemm_v2(handle, transa, transb, tr2 ? src2.rows : src2.cols, tr1 ? src1.cols : src1.rows, tr2 ? src2.cols : src2.rows,
            &alpha,
            src2.ptr<double>(), static_cast<int>(src2.step / sizeof(double)),
            src1.ptr<double>(), static_cast<int>(src1.step / sizeof(double)),
            &beta,
            dst.ptr<double>(), static_cast<int>(dst.step / sizeof(double))) );
        break;

    case CV_32FC2:
        cublasSafeCall( cublasCgemm_v2(handle, transa, transb, tr2 ? src2.rows : src2.cols, tr1 ? src1.cols : src1.rows, tr2 ? src2.cols : src2.rows,
            &alphacf,
            src2.ptr<cuComplex>(), static_cast<int>(src2.step / sizeof(cuComplex)),
            src1.ptr<cuComplex>(), static_cast<int>(src1.step / sizeof(cuComplex)),
            &betacf,
            dst.ptr<cuComplex>(), static_cast<int>(dst.step / sizeof(cuComplex))) );
        break;

    case CV_64FC2:
        cublasSafeCall( cublasZgemm_v2(handle, transa, transb, tr2 ? src2.rows : src2.cols, tr1 ? src1.cols : src1.rows, tr2 ? src2.cols : src2.rows,
            &alphac,
            src2.ptr<cuDoubleComplex>(), static_cast<int>(src2.step / sizeof(cuDoubleComplex)),
            src1.ptr<cuDoubleComplex>(), static_cast<int>(src1.step / sizeof(cuDoubleComplex)),
            &betac,
            dst.ptr<cuDoubleComplex>(), static_cast<int>(dst.step / sizeof(cuDoubleComplex))) );
        break;
    }

    cublasSafeCall( cublasDestroy_v2(handle) );
#endif
}

////////////////////////////////////////////////////////////////////////
// transpose

namespace arithm
{
    template <typename T> void transpose(PtrStepSz<T> src, PtrStepSz<T> dst, cudaStream_t stream);
}

void cv::gpu::transpose(const GpuMat& src, GpuMat& dst, Stream& s)
{
    CV_Assert( src.elemSize() == 1 || src.elemSize() == 4 || src.elemSize() == 8 );

    dst.create( src.cols, src.rows, src.type() );

    cudaStream_t stream = StreamAccessor::getStream(s);

    if (src.elemSize() == 1)
    {
        NppStreamHandler h(stream);

        NppiSize sz;
        sz.width  = src.cols;
        sz.height = src.rows;

        nppSafeCall( nppiTranspose_8u_C1R(src.ptr<Npp8u>(), static_cast<int>(src.step),
            dst.ptr<Npp8u>(), static_cast<int>(dst.step), sz) );

        if (stream == 0)
            cudaSafeCall( cudaDeviceSynchronize() );
    }
    else if (src.elemSize() == 4)
    {
        arithm::transpose<int>(src, dst, stream);
    }
    else // if (src.elemSize() == 8)
    {
        if (!deviceSupports(NATIVE_DOUBLE))
            CV_Error(cv::Error::StsUnsupportedFormat, "The device doesn't support double");

        arithm::transpose<double>(src, dst, stream);
    }
}

////////////////////////////////////////////////////////////////////////
// flip

namespace
{
    template<int DEPTH> struct NppTypeTraits;
    template<> struct NppTypeTraits<CV_8U>  { typedef Npp8u npp_t; };
    template<> struct NppTypeTraits<CV_8S>  { typedef Npp8s npp_t; };
    template<> struct NppTypeTraits<CV_16U> { typedef Npp16u npp_t; };
    template<> struct NppTypeTraits<CV_16S> { typedef Npp16s npp_t; };
    template<> struct NppTypeTraits<CV_32S> { typedef Npp32s npp_t; };
    template<> struct NppTypeTraits<CV_32F> { typedef Npp32f npp_t; };
    template<> struct NppTypeTraits<CV_64F> { typedef Npp64f npp_t; };

    template <int DEPTH> struct NppMirrorFunc
    {
        typedef typename NppTypeTraits<DEPTH>::npp_t npp_t;

        typedef NppStatus (*func_t)(const npp_t* pSrc, int nSrcStep, npp_t* pDst, int nDstStep, NppiSize oROI, NppiAxis flip);
    };

    template <int DEPTH, typename NppMirrorFunc<DEPTH>::func_t func> struct NppMirror
    {
        typedef typename NppMirrorFunc<DEPTH>::npp_t npp_t;

        static void call(const GpuMat& src, GpuMat& dst, int flipCode, cudaStream_t stream)
        {
            NppStreamHandler h(stream);

            NppiSize sz;
            sz.width  = src.cols;
            sz.height = src.rows;

            nppSafeCall( func(src.ptr<npp_t>(), static_cast<int>(src.step),
                dst.ptr<npp_t>(), static_cast<int>(dst.step), sz,
                (flipCode == 0 ? NPP_HORIZONTAL_AXIS : (flipCode > 0 ? NPP_VERTICAL_AXIS : NPP_BOTH_AXIS))) );

            if (stream == 0)
                cudaSafeCall( cudaDeviceSynchronize() );
        }
    };
}

void cv::gpu::flip(const GpuMat& src, GpuMat& dst, int flipCode, Stream& stream)
{
    typedef void (*func_t)(const GpuMat& src, GpuMat& dst, int flipCode, cudaStream_t stream);
    static const func_t funcs[6][4] =
    {
        {NppMirror<CV_8U, nppiMirror_8u_C1R>::call, 0, NppMirror<CV_8U, nppiMirror_8u_C3R>::call, NppMirror<CV_8U, nppiMirror_8u_C4R>::call},
        {0,0,0,0},
        {NppMirror<CV_16U, nppiMirror_16u_C1R>::call, 0, NppMirror<CV_16U, nppiMirror_16u_C3R>::call, NppMirror<CV_16U, nppiMirror_16u_C4R>::call},
        {0,0,0,0},
        {NppMirror<CV_32S, nppiMirror_32s_C1R>::call, 0, NppMirror<CV_32S, nppiMirror_32s_C3R>::call, NppMirror<CV_32S, nppiMirror_32s_C4R>::call},
        {NppMirror<CV_32F, nppiMirror_32f_C1R>::call, 0, NppMirror<CV_32F, nppiMirror_32f_C3R>::call, NppMirror<CV_32F, nppiMirror_32f_C4R>::call}
    };

    CV_Assert(src.depth() == CV_8U || src.depth() == CV_16U || src.depth() == CV_32S || src.depth() == CV_32F);
    CV_Assert(src.channels() == 1 || src.channels() == 3 || src.channels() == 4);

    dst.create(src.size(), src.type());

    funcs[src.depth()][src.channels() - 1](src, dst, flipCode, StreamAccessor::getStream(stream));
}

////////////////////////////////////////////////////////////////////////
// LUT

void cv::gpu::LUT(const GpuMat& src, const Mat& lut, GpuMat& dst, Stream& s)
{
    const int cn = src.channels();

    CV_Assert( src.type() == CV_8UC1 || src.type() == CV_8UC3 );
    CV_Assert( lut.depth() == CV_8U );
    CV_Assert( lut.channels() == 1 || lut.channels() == cn );
    CV_Assert( lut.rows * lut.cols == 256 && lut.isContinuous() );

    dst.create(src.size(), CV_MAKE_TYPE(lut.depth(), cn));

    NppiSize sz;
    sz.height = src.rows;
    sz.width = src.cols;

    Mat nppLut;
    lut.convertTo(nppLut, CV_32S);

    int nValues3[] = {256, 256, 256};

    Npp32s pLevels[256];
    for (int i = 0; i < 256; ++i)
        pLevels[i] = i;

    const Npp32s* pLevels3[3];

#if (CUDA_VERSION <= 4020)
    pLevels3[0] = pLevels3[1] = pLevels3[2] = pLevels;
#else
    GpuMat d_pLevels;
    d_pLevels.upload(Mat(1, 256, CV_32S, pLevels));
    pLevels3[0] = pLevels3[1] = pLevels3[2] = d_pLevels.ptr<Npp32s>();
#endif

    cudaStream_t stream = StreamAccessor::getStream(s);
    NppStreamHandler h(stream);

    if (src.type() == CV_8UC1)
    {
#if (CUDA_VERSION <= 4020)
        nppSafeCall( nppiLUT_Linear_8u_C1R(src.ptr<Npp8u>(), static_cast<int>(src.step),
            dst.ptr<Npp8u>(), static_cast<int>(dst.step), sz, nppLut.ptr<Npp32s>(), pLevels, 256) );
#else
        GpuMat d_nppLut(Mat(1, 256, CV_32S, nppLut.data));
        nppSafeCall( nppiLUT_Linear_8u_C1R(src.ptr<Npp8u>(), static_cast<int>(src.step),
            dst.ptr<Npp8u>(), static_cast<int>(dst.step), sz, d_nppLut.ptr<Npp32s>(), d_pLevels.ptr<Npp32s>(), 256) );
#endif
    }
    else
    {
        const Npp32s* pValues3[3];

        Mat nppLut3[3];
        if (nppLut.channels() == 1)
        {
#if (CUDA_VERSION <= 4020)
            pValues3[0] = pValues3[1] = pValues3[2] = nppLut.ptr<Npp32s>();
#else
            GpuMat d_nppLut(Mat(1, 256, CV_32S, nppLut.data));
            pValues3[0] = pValues3[1] = pValues3[2] = d_nppLut.ptr<Npp32s>();
#endif
        }
        else
        {
            cv::split(nppLut, nppLut3);

#if (CUDA_VERSION <= 4020)
            pValues3[0] = nppLut3[0].ptr<Npp32s>();
            pValues3[1] = nppLut3[1].ptr<Npp32s>();
            pValues3[2] = nppLut3[2].ptr<Npp32s>();
#else
            GpuMat d_nppLut0(Mat(1, 256, CV_32S, nppLut3[0].data));
            GpuMat d_nppLut1(Mat(1, 256, CV_32S, nppLut3[1].data));
            GpuMat d_nppLut2(Mat(1, 256, CV_32S, nppLut3[2].data));

            pValues3[0] = d_nppLut0.ptr<Npp32s>();
            pValues3[1] = d_nppLut1.ptr<Npp32s>();
            pValues3[2] = d_nppLut2.ptr<Npp32s>();
#endif
        }

        nppSafeCall( nppiLUT_Linear_8u_C3R(src.ptr<Npp8u>(), static_cast<int>(src.step),
            dst.ptr<Npp8u>(), static_cast<int>(dst.step), sz, pValues3, pLevels3, nValues3) );
    }

    if (stream == 0)
        cudaSafeCall( cudaDeviceSynchronize() );
}

////////////////////////////////////////////////////////////////////////
// NPP magnitide

namespace
{
    typedef NppStatus (*nppMagnitude_t)(const Npp32fc* pSrc, int nSrcStep, Npp32f* pDst, int nDstStep, NppiSize oSizeROI);

    inline void npp_magnitude(const GpuMat& src, GpuMat& dst, nppMagnitude_t func, cudaStream_t stream)
    {
        CV_Assert(src.type() == CV_32FC2);

        dst.create(src.size(), CV_32FC1);

        NppiSize sz;
        sz.width = src.cols;
        sz.height = src.rows;

        NppStreamHandler h(stream);

        nppSafeCall( func(src.ptr<Npp32fc>(), static_cast<int>(src.step), dst.ptr<Npp32f>(), static_cast<int>(dst.step), sz) );

        if (stream == 0)
            cudaSafeCall( cudaDeviceSynchronize() );
    }
}

void cv::gpu::magnitude(const GpuMat& src, GpuMat& dst, Stream& stream)
{
    npp_magnitude(src, dst, nppiMagnitude_32fc32f_C1R, StreamAccessor::getStream(stream));
}

void cv::gpu::magnitudeSqr(const GpuMat& src, GpuMat& dst, Stream& stream)
{
    npp_magnitude(src, dst, nppiMagnitudeSqr_32fc32f_C1R, StreamAccessor::getStream(stream));
}

////////////////////////////////////////////////////////////////////////
// Polar <-> Cart

namespace cv { namespace gpu { namespace cudev
{
    namespace mathfunc
    {
        void cartToPolar_gpu(PtrStepSzf x, PtrStepSzf y, PtrStepSzf mag, bool magSqr, PtrStepSzf angle, bool angleInDegrees, cudaStream_t stream);
        void polarToCart_gpu(PtrStepSzf mag, PtrStepSzf angle, PtrStepSzf x, PtrStepSzf y, bool angleInDegrees, cudaStream_t stream);
    }
}}}

namespace
{
    inline void cartToPolar_caller(const GpuMat& x, const GpuMat& y, GpuMat* mag, bool magSqr, GpuMat* angle, bool angleInDegrees, cudaStream_t stream)
    {
        using namespace ::cv::gpu::cudev::mathfunc;

        CV_Assert(x.size() == y.size() && x.type() == y.type());
        CV_Assert(x.depth() == CV_32F);

        if (mag)
            mag->create(x.size(), x.type());
        if (angle)
            angle->create(x.size(), x.type());

        GpuMat x1cn = x.reshape(1);
        GpuMat y1cn = y.reshape(1);
        GpuMat mag1cn = mag ? mag->reshape(1) : GpuMat();
        GpuMat angle1cn = angle ? angle->reshape(1) : GpuMat();

        cartToPolar_gpu(x1cn, y1cn, mag1cn, magSqr, angle1cn, angleInDegrees, stream);
    }

    inline void polarToCart_caller(const GpuMat& mag, const GpuMat& angle, GpuMat& x, GpuMat& y, bool angleInDegrees, cudaStream_t stream)
    {
        using namespace ::cv::gpu::cudev::mathfunc;

        CV_Assert((mag.empty() || mag.size() == angle.size()) && mag.type() == angle.type());
        CV_Assert(mag.depth() == CV_32F);

        x.create(mag.size(), mag.type());
        y.create(mag.size(), mag.type());

        GpuMat mag1cn = mag.reshape(1);
        GpuMat angle1cn = angle.reshape(1);
        GpuMat x1cn = x.reshape(1);
        GpuMat y1cn = y.reshape(1);

        polarToCart_gpu(mag1cn, angle1cn, x1cn, y1cn, angleInDegrees, stream);
    }
}

void cv::gpu::magnitude(const GpuMat& x, const GpuMat& y, GpuMat& dst, Stream& stream)
{
    cartToPolar_caller(x, y, &dst, false, 0, false, StreamAccessor::getStream(stream));
}

void cv::gpu::magnitudeSqr(const GpuMat& x, const GpuMat& y, GpuMat& dst, Stream& stream)
{
    cartToPolar_caller(x, y, &dst, true, 0, false, StreamAccessor::getStream(stream));
}

void cv::gpu::phase(const GpuMat& x, const GpuMat& y, GpuMat& angle, bool angleInDegrees, Stream& stream)
{
    cartToPolar_caller(x, y, 0, false, &angle, angleInDegrees, StreamAccessor::getStream(stream));
}

void cv::gpu::cartToPolar(const GpuMat& x, const GpuMat& y, GpuMat& mag, GpuMat& angle, bool angleInDegrees, Stream& stream)
{
    cartToPolar_caller(x, y, &mag, false, &angle, angleInDegrees, StreamAccessor::getStream(stream));
}

void cv::gpu::polarToCart(const GpuMat& magnitude, const GpuMat& angle, GpuMat& x, GpuMat& y, bool angleInDegrees, Stream& stream)
{
    polarToCart_caller(magnitude, angle, x, y, angleInDegrees, StreamAccessor::getStream(stream));
}

////////////////////////////////////////////////////////////////////////
// normalize

void cv::gpu::normalize(const GpuMat& src, GpuMat& dst, double a, double b, int norm_type, int dtype, const GpuMat& mask)
{
    GpuMat norm_buf;
    GpuMat cvt_buf;
    normalize(src, dst, a, b, norm_type, dtype, mask, norm_buf, cvt_buf);
}

void cv::gpu::normalize(const GpuMat& src, GpuMat& dst, double a, double b, int norm_type, int dtype, const GpuMat& mask, GpuMat& norm_buf, GpuMat& cvt_buf)
{
    double scale = 1, shift = 0;
    if (norm_type == NORM_MINMAX)
    {
        double smin = 0, smax = 0;
        double dmin = std::min(a, b), dmax = std::max(a, b);
        minMax(src, &smin, &smax, mask, norm_buf);
        scale = (dmax - dmin) * (smax - smin > std::numeric_limits<double>::epsilon() ? 1.0 / (smax - smin) : 0.0);
        shift = dmin - smin * scale;
    }
    else if (norm_type == NORM_L2 || norm_type == NORM_L1 || norm_type == NORM_INF)
    {
        scale = norm(src, norm_type, mask, norm_buf);
        scale = scale > std::numeric_limits<double>::epsilon() ? a / scale : 0.0;
        shift = 0;
    }
    else
    {
        CV_Error(cv::Error::StsBadArg, "Unknown/unsupported norm type");
    }

    if (mask.empty())
    {
        src.convertTo(dst, dtype, scale, shift);
    }
    else
    {
        src.convertTo(cvt_buf, dtype, scale, shift);
        cvt_buf.copyTo(dst, mask);
    }
}

////////////////////////////////////////////////////////////////////////
// copyMakeBorder

namespace cv { namespace gpu { namespace cudev
{
    namespace imgproc
    {
        template <typename T, int cn> void copyMakeBorder_gpu(const PtrStepSzb& src, const PtrStepSzb& dst, int top, int left, int borderMode, const T* borderValue, cudaStream_t stream);
    }
}}}

namespace
{
    template <typename T, int cn> void copyMakeBorder_caller(const PtrStepSzb& src, const PtrStepSzb& dst, int top, int left, int borderType, const Scalar& value, cudaStream_t stream)
    {
        using namespace ::cv::gpu::cudev::imgproc;

        Scalar_<T> val(saturate_cast<T>(value[0]), saturate_cast<T>(value[1]), saturate_cast<T>(value[2]), saturate_cast<T>(value[3]));

        copyMakeBorder_gpu<T, cn>(src, dst, top, left, borderType, val.val, stream);
    }
}

#if defined __GNUC__ && __GNUC__ > 2 && __GNUC_MINOR__  > 4
typedef Npp32s __attribute__((__may_alias__)) Npp32s_a;
#else
typedef Npp32s Npp32s_a;
#endif

void cv::gpu::copyMakeBorder(const GpuMat& src, GpuMat& dst, int top, int bottom, int left, int right, int borderType, const Scalar& value, Stream& s)
{
    CV_Assert(src.depth() <= CV_32F && src.channels() <= 4);
    CV_Assert(borderType == IPL_BORDER_REFLECT_101 || borderType == IPL_BORDER_REPLICATE || borderType == IPL_BORDER_CONSTANT || borderType == IPL_BORDER_REFLECT || borderType == IPL_BORDER_WRAP);

    dst.create(src.rows + top + bottom, src.cols + left + right, src.type());

    cudaStream_t stream = StreamAccessor::getStream(s);

    if (borderType == IPL_BORDER_CONSTANT && (src.type() == CV_8UC1 || src.type() == CV_8UC4 || src.type() == CV_32SC1 || src.type() == CV_32FC1))
    {
        NppiSize srcsz;
        srcsz.width  = src.cols;
        srcsz.height = src.rows;

        NppiSize dstsz;
        dstsz.width  = dst.cols;
        dstsz.height = dst.rows;

        NppStreamHandler h(stream);

        switch (src.type())
        {
        case CV_8UC1:
            {
                Npp8u nVal = saturate_cast<Npp8u>(value[0]);
                nppSafeCall( nppiCopyConstBorder_8u_C1R(src.ptr<Npp8u>(), static_cast<int>(src.step), srcsz,
                    dst.ptr<Npp8u>(), static_cast<int>(dst.step), dstsz, top, left, nVal) );
                break;
            }
        case CV_8UC4:
            {
                Npp8u nVal[] = {saturate_cast<Npp8u>(value[0]), saturate_cast<Npp8u>(value[1]), saturate_cast<Npp8u>(value[2]), saturate_cast<Npp8u>(value[3])};
                nppSafeCall( nppiCopyConstBorder_8u_C4R(src.ptr<Npp8u>(), static_cast<int>(src.step), srcsz,
                    dst.ptr<Npp8u>(), static_cast<int>(dst.step), dstsz, top, left, nVal) );
                break;
            }
        case CV_32SC1:
            {
                Npp32s nVal = saturate_cast<Npp32s>(value[0]);
                nppSafeCall( nppiCopyConstBorder_32s_C1R(src.ptr<Npp32s>(), static_cast<int>(src.step), srcsz,
                    dst.ptr<Npp32s>(), static_cast<int>(dst.step), dstsz, top, left, nVal) );
                break;
            }
        case CV_32FC1:
            {
                Npp32f val = saturate_cast<Npp32f>(value[0]);
                Npp32s nVal = *(reinterpret_cast<Npp32s_a*>(&val));
                nppSafeCall( nppiCopyConstBorder_32s_C1R(src.ptr<Npp32s>(), static_cast<int>(src.step), srcsz,
                    dst.ptr<Npp32s>(), static_cast<int>(dst.step), dstsz, top, left, nVal) );
                break;
            }
        }

        if (stream == 0)
            cudaSafeCall( cudaDeviceSynchronize() );
    }
    else
    {
        typedef void (*caller_t)(const PtrStepSzb& src, const PtrStepSzb& dst, int top, int left, int borderType, const Scalar& value, cudaStream_t stream);
        static const caller_t callers[6][4] =
        {
            {   copyMakeBorder_caller<uchar, 1>  ,    copyMakeBorder_caller<uchar, 2>   ,    copyMakeBorder_caller<uchar, 3>  ,    copyMakeBorder_caller<uchar, 4>},
            {0/*copyMakeBorder_caller<schar, 1>*/, 0/*copyMakeBorder_caller<schar, 2>*/ , 0/*copyMakeBorder_caller<schar, 3>*/, 0/*copyMakeBorder_caller<schar, 4>*/},
            {   copyMakeBorder_caller<ushort, 1> , 0/*copyMakeBorder_caller<ushort, 2>*/,    copyMakeBorder_caller<ushort, 3> ,    copyMakeBorder_caller<ushort, 4>},
            {   copyMakeBorder_caller<short, 1>  , 0/*copyMakeBorder_caller<short, 2>*/ ,    copyMakeBorder_caller<short, 3>  ,    copyMakeBorder_caller<short, 4>},
            {0/*copyMakeBorder_caller<int,   1>*/, 0/*copyMakeBorder_caller<int,   2>*/ , 0/*copyMakeBorder_caller<int,   3>*/, 0/*copyMakeBorder_caller<int  , 4>*/},
            {   copyMakeBorder_caller<float, 1>  , 0/*copyMakeBorder_caller<float, 2>*/ ,    copyMakeBorder_caller<float, 3>  ,    copyMakeBorder_caller<float ,4>}
        };

        caller_t func = callers[src.depth()][src.channels() - 1];
        CV_Assert(func != 0);

        int gpuBorderType;
        CV_Assert(tryConvertToGpuBorderType(borderType, gpuBorderType));

        func(src, dst, top, left, gpuBorderType, value, stream);
    }
}

////////////////////////////////////////////////////////////////////////
// integral

void cv::gpu::integral(const GpuMat& src, GpuMat& sum, Stream& s)
{
    GpuMat buffer;
    integralBuffered(src, sum, buffer, s);
}

namespace cv { namespace gpu { namespace cudev
{
    namespace imgproc
    {
        void shfl_integral_gpu(const PtrStepSzb& img, PtrStepSz<unsigned int> integral, cudaStream_t stream);
    }
}}}

void cv::gpu::integralBuffered(const GpuMat& src, GpuMat& sum, GpuMat& buffer, Stream& s)
{
    CV_Assert(src.type() == CV_8UC1);

    cudaStream_t stream = StreamAccessor::getStream(s);

    cv::Size whole;
    cv::Point offset;

    src.locateROI(whole, offset);

    if (deviceSupports(WARP_SHUFFLE_FUNCTIONS) && src.cols <= 2048
        && offset.x % 16 == 0 && ((src.cols + 63) / 64) * 64 <= (static_cast<int>(src.step) - offset.x))
    {
        ensureSizeIsEnough(((src.rows + 7) / 8) * 8, ((src.cols + 63) / 64) * 64, CV_32SC1, buffer);

        cv::gpu::cudev::imgproc::shfl_integral_gpu(src, buffer, stream);

        sum.create(src.rows + 1, src.cols + 1, CV_32SC1);
        if (s)
            s.enqueueMemSet(sum, Scalar::all(0));
        else
            sum.setTo(Scalar::all(0));

        GpuMat inner = sum(Rect(1, 1, src.cols, src.rows));
        GpuMat res = buffer(Rect(0, 0, src.cols, src.rows));

        if (s)
            s.enqueueCopy(res, inner);
        else
            res.copyTo(inner);
    }
    else
    {
#ifndef HAVE_OPENCV_GPUNVIDIA
    throw_no_cuda();
#else
        sum.create(src.rows + 1, src.cols + 1, CV_32SC1);

        NcvSize32u roiSize;
        roiSize.width = src.cols;
        roiSize.height = src.rows;

        cudaDeviceProp prop;
        cudaSafeCall( cudaGetDeviceProperties(&prop, cv::gpu::getDevice()) );

        Ncv32u bufSize;
        ncvSafeCall( nppiStIntegralGetSize_8u32u(roiSize, &bufSize, prop) );
        ensureSizeIsEnough(1, bufSize, CV_8UC1, buffer);

        NppStStreamHandler h(stream);

        ncvSafeCall( nppiStIntegral_8u32u_C1R(const_cast<Ncv8u*>(src.ptr<Ncv8u>()), static_cast<int>(src.step),
            sum.ptr<Ncv32u>(), static_cast<int>(sum.step), roiSize, buffer.ptr<Ncv8u>(), bufSize, prop) );

        if (stream == 0)
            cudaSafeCall( cudaDeviceSynchronize() );
#endif
    }
}

//////////////////////////////////////////////////////////////////////////////
// sqrIntegral

void cv::gpu::sqrIntegral(const GpuMat& src, GpuMat& sqsum, Stream& s)
{
#ifndef HAVE_OPENCV_GPUNVIDIA
    (void) src;
    (void) sqsum;
    (void) s;
    throw_no_cuda();
#else
    CV_Assert(src.type() == CV_8U);

    NcvSize32u roiSize;
    roiSize.width = src.cols;
    roiSize.height = src.rows;

    cudaDeviceProp prop;
    cudaSafeCall( cudaGetDeviceProperties(&prop, cv::gpu::getDevice()) );

    Ncv32u bufSize;
    ncvSafeCall(nppiStSqrIntegralGetSize_8u64u(roiSize, &bufSize, prop));
    GpuMat buf(1, bufSize, CV_8U);

    cudaStream_t stream = StreamAccessor::getStream(s);

    NppStStreamHandler h(stream);

    sqsum.create(src.rows + 1, src.cols + 1, CV_64F);
    ncvSafeCall(nppiStSqrIntegral_8u64u_C1R(const_cast<Ncv8u*>(src.ptr<Ncv8u>(0)), static_cast<int>(src.step),
            sqsum.ptr<Ncv64u>(0), static_cast<int>(sqsum.step), roiSize, buf.ptr<Ncv8u>(0), bufSize, prop));

    if (stream == 0)
        cudaSafeCall( cudaDeviceSynchronize() );
#endif
}

#endif /* !defined (HAVE_CUDA) */
