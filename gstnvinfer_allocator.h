/**
 * Copyright (c) 2018-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 */

#ifndef __GSTNVINFERALLOCATOR_H__
#define __GSTNVINFERALLOCATOR_H__

#include <cuda_runtime_api.h>
#include <gst/gst.h>
#include <vector>
#ifdef IS_TEGRA
#include "cudaEGL.h"
#endif
#include "nvbufsurface.h"

/**
 * This file describes the custom memory allocator for the Gstreamer TensorRT
 * plugin. The allocator allocates memory for a specified batch_size of frames
 * of resolution equal to the network input resolution and RGBA color format.
 * The frames are allocated on device memory.
 */

/**
 * Holds the pointer for the allocated memory.
 */
typedef struct
{
  NvBufSurface *surf;
#ifdef IS_TEGRA
  /** Vector of cuda resources created by registering the above egl images in CUDA. */
  std::vector<CUgraphicsResource> cuda_resources;
  /** Vector of CUDA eglFrames created by mapping the above cuda resources. */
  std::vector<CUeglFrame> egl_frames;
#else
  /** Pointer to the memory allocated for the batch of frames (DGPU). */
  void *dev_memory_ptr;
#endif
  /** Vector of pointer to individual frame memories in the batch memory */
  std::vector<void *> frame_memory_ptrs;
} GstNvInferOnnxMemory;

/**
 * Get GstNvInferOnnxMemory structure associated with buffer allocated using
 * GstNvInferOnnxAllocator.
 *
 * @param buffer GstBuffer allocated by this allocator.
 *
 * @return Pointer to the associated GstNvInferOnnxMemory structure
 */
GstNvInferOnnxMemory *gst_nvinfer_buffer_get_memory (GstBuffer * buffer);

/**
 * Create a new GstNvInferOnnxAllocator with the given parameters.
 *
 * @param width Width of the network input, in pixels.
 * @param height Height of the network input, in pixels.
 * @param color_format Color format of the buffers in the pool.
 * @param batch_size Max size of batch that will be inferred.
 * @param gpu_id ID of the gpu where the batch memory will be allocated.
 *
 * @return Pointer to the GstNvInferOnnxAllocator structure cast as GstAllocator
 */
GstAllocator *gst_nvinfer_allocator_new (guint width, guint height,
    NvBufSurfaceColorFormat color_format, guint batch_size, guint gpu_id);

#endif
