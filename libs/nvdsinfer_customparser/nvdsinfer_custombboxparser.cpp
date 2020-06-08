/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <cstring>
#include <iostream>
#include "nvdsinfer_custom_impl.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CLIP(a,min,max) (MAX(MIN(a, max), min))
#define DIVIDE_AND_ROUND_UP(a, b) ((a + b - 1) / b)

/* This is a sample bounding box parsing function for the sample Resnet10
 * detector model provided with the SDK. */

/* C-linkage to prevent name-mangling */
extern "C"
bool NvDsInferParseCustomResnet (std::vector<NvDsInferLayerInfo> const &outputLayersInfo,
        NvDsInferNetworkInfo  const &networkInfo,
        NvDsInferParseDetectionParams const &detectionParams,
        std::vector<NvDsInferObjectDetectionInfo> &objectList);

/* This is a sample bounding box parsing function for the tensorflow SSD models
 * detector model provided with the SDK. */

/* C-linkage to prevent name-mangling */
extern "C"
bool NvDsInferParseCustomTfSSD (std::vector<NvDsInferLayerInfo> const &outputLayersInfo,
        NvDsInferNetworkInfo  const &networkInfo,
        NvDsInferParseDetectionParams const &detectionParams,
        std::vector<NvDsInferObjectDetectionInfo> &objectList);


extern "C"
bool NvDsInferParseCustomResnet (std::vector<NvDsInferLayerInfo> const &outputLayersInfo,
        NvDsInferNetworkInfo  const &networkInfo,
        NvDsInferParseDetectionParams const &detectionParams,
        std::vector<NvDsInferObjectDetectionInfo> &objectList)
{
  static NvDsInferDimsCHW covLayerDims;
  static NvDsInferDimsCHW bboxLayerDims;
  static int bboxLayerIndex = -1;
  static int covLayerIndex = -1;
  static bool classMismatchWarn = false;
  int numClassesToParse;

  /* Find the bbox layer */
  if (bboxLayerIndex == -1) {
    for (unsigned int i = 0; i < outputLayersInfo.size(); i++) {
      if (strcmp(outputLayersInfo[i].layerName, "conv2d_bbox") == 0) {
        bboxLayerIndex = i;
        getDimsCHWFromDims(bboxLayerDims, outputLayersInfo[i].inferDims);
        break;
      }
    }
    if (bboxLayerIndex == -1) {
    std::cerr << "Could not find bbox layer buffer while parsing" << std::endl;
    return false;
    }
  }

  /* Find the cov layer */
  if (covLayerIndex == -1) {
    for (unsigned int i = 0; i < outputLayersInfo.size(); i++) {
      if (strcmp(outputLayersInfo[i].layerName, "conv2d_cov/Sigmoid") == 0) {
        covLayerIndex = i;
        getDimsCHWFromDims(covLayerDims, outputLayersInfo[i].inferDims);
        break;
      }
    }
    if (covLayerIndex == -1) {
    std::cerr << "Could not find bbox layer buffer while parsing" << std::endl;
    return false;
    }
  }

  /* Warn in case of mismatch in number of classes */
  if (!classMismatchWarn) {
    if (covLayerDims.c != detectionParams.numClassesConfigured) {
      std::cerr << "WARNING: Num classes mismatch. Configured:" <<
        detectionParams.numClassesConfigured << ", detected by network: " <<
        covLayerDims.c << std::endl;
    }
    classMismatchWarn = true;
  }

  /* Calculate the number of classes to parse */
  numClassesToParse = MIN (covLayerDims.c, detectionParams.numClassesConfigured);

  int gridW = covLayerDims.w;
  int gridH = covLayerDims.h;
  int gridSize = gridW * gridH;
  float gcCentersX[gridW];
  float gcCentersY[gridH];
  float bboxNormX = 35.0;
  float bboxNormY = 35.0;
  float *outputCovBuf = (float *) outputLayersInfo[covLayerIndex].buffer;
  float *outputBboxBuf = (float *) outputLayersInfo[bboxLayerIndex].buffer;
  int strideX = DIVIDE_AND_ROUND_UP(networkInfo.width, bboxLayerDims.w);
  int strideY = DIVIDE_AND_ROUND_UP(networkInfo.height, bboxLayerDims.h);

  for (int i = 0; i < gridW; i++)
  {
    gcCentersX[i] = (float)(i * strideX + 0.5);
    gcCentersX[i] /= (float)bboxNormX;

  }
  for (int i = 0; i < gridH; i++)
  {
    gcCentersY[i] = (float)(i * strideY + 0.5);
    gcCentersY[i] /= (float)bboxNormY;

  }

  for (int c = 0; c < numClassesToParse; c++)
  {
    float *outputX1 = outputBboxBuf + (c * 4 * bboxLayerDims.h * bboxLayerDims.w);

    float *outputY1 = outputX1 + gridSize;
    float *outputX2 = outputY1 + gridSize;
    float *outputY2 = outputX2 + gridSize;

    float threshold = detectionParams.perClassPreclusterThreshold[c];
    for (int h = 0; h < gridH; h++)
    {
      for (int w = 0; w < gridW; w++)
      {
        int i = w + h * gridW;
        if (outputCovBuf[c * gridSize + i] >= threshold)
        {
          NvDsInferObjectDetectionInfo object;
          float rectX1f, rectY1f, rectX2f, rectY2f;

          rectX1f = (outputX1[w + h * gridW] - gcCentersX[w]) * -bboxNormX;
          rectY1f = (outputY1[w + h * gridW] - gcCentersY[h]) * -bboxNormY;
          rectX2f = (outputX2[w + h * gridW] + gcCentersX[w]) * bboxNormX;
          rectY2f = (outputY2[w + h * gridW] + gcCentersY[h]) * bboxNormY;

          object.classId = c;
          object.detectionConfidence = outputCovBuf[c * gridSize + i];

          /* Clip object box co-ordinates to network resolution */
          object.left = CLIP(rectX1f, 0, networkInfo.width - 1);
          object.top = CLIP(rectY1f, 0, networkInfo.height - 1);
          object.width = CLIP(rectX2f, 0, networkInfo.width - 1) -
                             object.left + 1;
          object.height = CLIP(rectY2f, 0, networkInfo.height - 1) -
                             object.top + 1;

          objectList.push_back(object);
        }
      }
    }
  }
  return true;
}

extern "C"
bool NvDsInferParseCustomTfSSD (std::vector<NvDsInferLayerInfo> const &outputLayersInfo,
    NvDsInferNetworkInfo  const &networkInfo,
    NvDsInferParseDetectionParams const &detectionParams,
    std::vector<NvDsInferObjectDetectionInfo> &objectList)
{
    auto layerFinder = [&outputLayersInfo](const std::string &name)
        -> const NvDsInferLayerInfo *{
        for (auto &layer : outputLayersInfo) {
            if (layer.dataType == FLOAT &&
              (layer.layerName && name == layer.layerName)) {
                return &layer;
            }
        }
        return nullptr;
    };

    const NvDsInferLayerInfo *numDetectionLayer = layerFinder("num_detections");
    const NvDsInferLayerInfo *scoreLayer = layerFinder("detection_scores");
    const NvDsInferLayerInfo *classLayer = layerFinder("detection_classes");
    const NvDsInferLayerInfo *boxLayer = layerFinder("detection_boxes");
    if (!numDetectionLayer || !scoreLayer || !classLayer || !boxLayer) {
        std::cerr << "ERROR: some layers missing or unsupported data types "
                  << "in output tensors" << std::endl;
        return false;
    }

    unsigned int numDetections = 0;
    if (numDetectionLayer->buffer) {
        numDetections = (int)((float*)numDetectionLayer->buffer)[0];
    }
    if (numDetections > classLayer->inferDims.d[0]) {
        numDetections = classLayer->inferDims.d[0];
    }
    numDetections = std::max<int>(0, numDetections);
    for (unsigned int i = 0; i < numDetections; ++i) {
        NvDsInferObjectDetectionInfo res;
        res.detectionConfidence = ((float*)scoreLayer->buffer)[i];
        res.classId = ((float*)classLayer->buffer)[i];
        if (res.classId >= detectionParams.perClassPreclusterThreshold.size() ||
            res.detectionConfidence <
            detectionParams.perClassPreclusterThreshold[res.classId]) {
            continue;
        }
        enum {y1, x1, y2, x2};
        float rectX1f, rectY1f, rectX2f, rectY2f;
        rectX1f = ((float*)boxLayer->buffer)[i *4 + x1] * networkInfo.width;
        rectY1f = ((float*)boxLayer->buffer)[i *4 + y1] * networkInfo.height;
        rectX2f = ((float*)boxLayer->buffer)[i *4 + x2] * networkInfo.width;;
        rectY2f = ((float*)boxLayer->buffer)[i *4 + y2] * networkInfo.height;
        rectX1f = CLIP(rectX1f, 0.0f, networkInfo.width - 1);
        rectX2f = CLIP(rectX2f, 0.0f, networkInfo.width - 1);
        rectY1f = CLIP(rectY1f, 0.0f, networkInfo.height - 1);
        rectY2f = CLIP(rectY2f, 0.0f, networkInfo.height - 1);
        if (rectX2f <= rectX1f || rectY2f <= rectY1f) {
            continue;
        }
        res.left = rectX1f;
        res.top = rectY1f;
        res.width = rectX2f - rectX1f;
        res.height = rectY2f - rectY1f;
        if (res.width && res.height) {
            objectList.emplace_back(res);
        }
    }

    return true;
}

/* Check that the custom function has been defined correctly */
CHECK_CUSTOM_PARSE_FUNC_PROTOTYPE(NvDsInferParseCustomResnet);
CHECK_CUSTOM_PARSE_FUNC_PROTOTYPE(NvDsInferParseCustomTfSSD);
