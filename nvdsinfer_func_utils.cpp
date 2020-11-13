/**
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 */

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>

#include "nvdsinfer_func_utils.h"

namespace nvdsinfer {

/* Logger for TensorRT info/warning/errors */
class NvDsInferLogger : public nvinfer1::ILogger
{
public:
    void log(Severity severity, const char* msg) override
    {
        switch (severity)
        {
            case Severity::kINTERNAL_ERROR:
            case Severity::kERROR:
                dsInferError("[TRT]: %s", msg);
                break;
            case Severity::kWARNING:
                dsInferWarning("[TRT]: %s", msg);
                break;
            case Severity::kINFO:
                dsInferInfo("[TRT]: %s", msg);
                break;
            case Severity::kVERBOSE:
                dsInferDebug("[TRT]: %s", msg);
                break;
            default:
                dsInferLogPrint__((NvDsInferLogLevel)100, "[TRT][severity:%d]: %s",
                        (int)severity, msg);
                return;
        }
    }
};

std::unique_ptr<nvinfer1::ILogger> gTrtLogger(new NvDsInferLogger);

DlLibHandle::DlLibHandle(const std::string& path, int mode)
    : m_LibPath(path)
{
    assert(!path.empty());
    m_LibHandle = dlopen(path.c_str(), mode);
    if (!m_LibHandle)
    {
        dsInferError("Could not open lib: %s, error string: %s", path.c_str(),
                dlerror());
    }
}

DlLibHandle::~DlLibHandle()
{
    if (m_LibHandle)
    {
        dlclose(m_LibHandle);
    }
}

nvinfer1::Dims
ds2TrtDims(const NvDsInferDimsCHW& dims)
{
    return nvinfer1::Dims{3, {(int)dims.c, (int)dims.h, (int)dims.w}};
}

nvinfer1::Dims
ds2TrtDims(const NvDsInferDims& dims)
{
    nvinfer1::Dims ret;
    ret.nbDims = dims.numDims;
    std::copy(dims.d, dims.d + dims.numDims, ret.d);
    return ret;
}

NvDsInferDims
trt2DsDims(const nvinfer1::Dims& dims)
{
    NvDsInferDims ret;
    ret.numDims = dims.nbDims;
    int sum = 1;
    for (int i = 0; i < dims.nbDims; ++i)
    {
        ret.d[i] = dims.d[i];
        if (dims.d[i] < 0)
        {
            sum = 0;
        }
        else
        {
            sum *= dims.d[i];
        }
    }
    //Min num elements has to be 1 to support empty tensors
    ret.numElements = (dims.nbDims ? sum : 1);
    return ret;
}

nvinfer1::Dims
CombineDimsBatch(const NvDsInferDims& dims, int batch)
{
    nvinfer1::Dims ret;
    ret.nbDims = dims.numDims + 1;
    /* Set batch size as 0th dim and copy rest of the dims. */
    ret.d[0] = batch;
    std::copy(dims.d, dims.d + dims.numDims, &ret.d[1]);
    return ret;
}

void
SplitFullDims(const nvinfer1::Dims& fullDims, NvDsInferDims& dims, int& batch)
{
    if (!fullDims.nbDims)
    {
        dims.numDims = 0;
        dims.numElements = 0;
        batch = 0;
    }
    else
    {
        /* Use 0th dim as batch size and get rest of the dims. */
        batch = fullDims.d[0];
        dims.numDims = fullDims.nbDims - 1;
        std::copy(fullDims.d + 1, fullDims.d + fullDims.nbDims, dims.d);
        normalizeDims(dims);
    }
}

void
normalizeDims(NvDsInferDims& dims)
{
    if (hasWildcard(dims) || !dims.numDims)
    {
        dims.numElements = 0;
    }
    else
    {
        dims.numElements = std::accumulate(dims.d, dims.d + dims.numDims, 1,
            [](int s, int i) { return s * i; });
    }
}

std::string
dims2Str(const nvinfer1::Dims& d)
{
    std::stringstream s;
    for (int i = 0; i < d.nbDims - 1; ++i)
    {
        s << d.d[i] << "x";
    }
    s << d.d[d.nbDims - 1];

    return s.str();
}

std::string
dims2Str(const NvDsInferDims& d)
{
    return dims2Str(ds2TrtDims(d));
}

std::string
batchDims2Str(const NvDsInferBatchDims& d)
{
    return dims2Str(CombineDimsBatch(d.dims, d.batchSize));
}

std::string
dataType2Str(const nvinfer1::DataType type)
{
    switch (type)
    {
        case nvinfer1::DataType::kFLOAT:
            return "kFLOAT";
        case nvinfer1::DataType::kHALF:
            return "kHALF";
        case nvinfer1::DataType::kINT8:
            return "kINT8";
        case nvinfer1::DataType::kINT32:
            return "kINT32";
        default:
            return "UNKNOWN";
    }
}

std::string
dataType2Str(const NvDsInferDataType type)
{
    switch (type)
    {
        case FLOAT:
            return "kFLOAT";
        case HALF:
            return "kHALF";
        case INT8:
            return "kINT8";
        case INT32:
            return "kINT32";
        default:
            return "UNKNOWN";
    }
}

std::string
networkMode2Str(const NvDsInferNetworkMode type)
{
    switch (type)
    {
        case NvDsInferNetworkMode_FP32:
            return "fp32";
        case NvDsInferNetworkMode_INT8:
            return "int8";
        case NvDsInferNetworkMode_FP16:
            return "fp16";
        default:
            return "UNKNOWN";
    }
}

bool
hasWildcard(const nvinfer1::Dims& dims)
{
    return std::any_of(
        dims.d, dims.d + dims.nbDims, [](int d) { return d == -1; });
}

bool
hasWildcard(const NvDsInferDims& dims)
{
    return std::any_of(
        dims.d, dims.d + dims.numDims, [](int d) { return d == -1; });
}

bool
operator<=(const nvinfer1::Dims& a, const nvinfer1::Dims& b)
{
    assert(a.nbDims == b.nbDims);
    for (int i = 0; i < a.nbDims; ++i)
    {
        if (a.d[i] > b.d[i])
            return false;
    }
    return true;
}

bool
operator>(const nvinfer1::Dims& a, const nvinfer1::Dims& b)
{
    return !(a <= b);
}

bool
operator<=(const NvDsInferDims& a, const NvDsInferDims& b)
{
    assert(a.numDims == b.numDims);
    for (uint32_t i = 0; i < a.numDims; ++i)
    {
        if (a.d[i] > b.d[i])
            return false;
    }
    return true;
}

bool
operator==(const nvinfer1::Dims& a, const nvinfer1::Dims& b)
{
    if (a.nbDims != b.nbDims)
        return false;

    for (int i = 0; i < a.nbDims; ++i)
    {
        if (a.d[i] != b.d[i])
            return false;
    }
    return true;
}

bool
operator!=(const nvinfer1::Dims& a, const nvinfer1::Dims& b)
{
    return !(a == b);
}

bool
operator>(const NvDsInferDims& a, const NvDsInferDims& b)
{
    return !(a <= b);
}

bool
operator==(const NvDsInferDims& a, const NvDsInferDims& b)
{
    if (a.numDims != b.numDims)
        return false;

    for (uint32_t i = 0; i < a.numDims; ++i)
    {
        if (a.d[i] != b.d[i])
            return false;
    }
    return true;
}

bool
operator!=(const NvDsInferDims& a, const NvDsInferDims& b)
{
    return !(a == b);
}

static const char*
strLogLevel(NvDsInferLogLevel l)
{
    switch (l)
    {
        case NVDSINFER_LOG_ERROR:
            return "ERROR";
        case NVDSINFER_LOG_WARNING:
            return "WARNING";
        case NVDSINFER_LOG_INFO:
            return "INFO";
        case NVDSINFER_LOG_DEBUG:
            return "DEBUG";
        default:
            return "UNKNOWN";
    }
}

struct LogEnv
{
    NvDsInferLogLevel levelLimit = NVDSINFER_LOG_INFO;
    std::mutex printMutex;
    LogEnv()
    {
        const char* cEnv = std::getenv("NVDSINFER_LOG_LEVEL");
        if (cEnv)
        {
            levelLimit = (NvDsInferLogLevel)std::stoi(cEnv);
        }
    }
};

static LogEnv gLogEnv;

void dsInferLogPrint__(NvDsInferLogLevel level, const char* fmt, ...)
{
    if (level > gLogEnv.levelLimit)
    {
        return;
    }
    constexpr int kMaxBufLen = 4096;

    va_list args;
    va_start(args, fmt);
    std::array<char, kMaxBufLen> logMsgBuffer{{'\0'}};
    vsnprintf(logMsgBuffer.data(), kMaxBufLen - 1, fmt, args);
    va_end(args);

    FILE* f = (level <= NVDSINFER_LOG_ERROR) ? stderr : stdout;

    std::unique_lock<std::mutex> locker(gLogEnv.printMutex);
    fprintf(f, "%s: %s\n", strLogLevel(level), logMsgBuffer.data());
}

} // namespace nvdsinfer

__attribute__ ((visibility ("default")))
const char*
NvDsInferStatus2Str(NvDsInferStatus status)
{
#define CHECK_AND_RETURN_STRING(status_iter) \
    if (status == status_iter)               \
    return #status_iter

    CHECK_AND_RETURN_STRING(NVDSINFER_SUCCESS);
    CHECK_AND_RETURN_STRING(NVDSINFER_CONFIG_FAILED);
    CHECK_AND_RETURN_STRING(NVDSINFER_CUSTOM_LIB_FAILED);
    CHECK_AND_RETURN_STRING(NVDSINFER_INVALID_PARAMS);
    CHECK_AND_RETURN_STRING(NVDSINFER_OUTPUT_PARSING_FAILED);
    CHECK_AND_RETURN_STRING(NVDSINFER_CUDA_ERROR);
    CHECK_AND_RETURN_STRING(NVDSINFER_TENSORRT_ERROR);
    CHECK_AND_RETURN_STRING(NVDSINFER_RESOURCE_ERROR);
    CHECK_AND_RETURN_STRING(NVDSINFER_UNKNOWN_ERROR);

    return "NVDSINFER_NULL";
#undef CHECK_AND_RETURN_STRING
}
