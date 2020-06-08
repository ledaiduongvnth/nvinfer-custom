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

#include "gstnvinfer.h"
#include "gstnvinfer_impl.h"

void attach_metadata_detector (GstNvInferOnnx * nvinfer, GstMiniObject * tensor_out_object,
        GstNvInferOnnxFrame & frame, NvDsInferDetectionOutput & detection_output);

void attach_metadata_classifier (GstNvInferOnnx * nvinfer, GstMiniObject * tensor_out_object,
        GstNvInferOnnxFrame & frame, GstNvInferOnnxObjectInfo & object_info);

void merge_classification_output (GstNvInferOnnxObjectHistory & history,
    GstNvInferOnnxObjectInfo  &new_result);

void attach_metadata_segmentation (GstNvInferOnnx * nvinfer, GstMiniObject * tensor_out_object,
        GstNvInferOnnxFrame & frame, NvDsInferSegmentationOutput & segmentation_output);

/* Attaches the raw tensor output to the GstBuffer as metadata. */
void attach_tensor_output_meta (GstNvInferOnnx *nvinfer, GstMiniObject * tensor_out_object,
        GstNvInferOnnxBatch *batch, NvDsInferContextBatchOutput *batch_output);
