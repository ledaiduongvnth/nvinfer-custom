/**
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 */
#ifndef _NVDS_ANALYTICS_META_H_
#define _NVDS_ANALYTICS_META_H_

#include <gst/gst.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define NVDS_USER_FRAME_META_NVDSANALYTICS (nvds_get_user_meta_type((gchar*)"NVIDIA.DSANALYTICSFRAME.USER_META"))
#define NVDS_USER_OBJ_META_NVDSANALYTICS (nvds_get_user_meta_type((gchar*)"NVIDIA.DSANALYTICSOBJ.USER_META"))

/**
 */
typedef struct
{
  std::vector <std::string> roiStatus;
  std::vector <std::string> ocStatus;
  std::vector <std::string> lcStatus;
  std::string dirStatus;
  guint unique_id;
} NvDsAnalyticsObjInfo;

/**
 */
typedef struct
{
  std::unordered_map<std::string, bool> ocStatus;
  std::unordered_map<std::string, uint32_t> objInROIcnt;
  std::unordered_map<std::string, uint64_t> objLCCurrCnt;
  std::unordered_map<std::string, uint64_t> objLCCumCnt;
  guint unique_id;

} NvDsAnalyticsFrameMeta;

#ifdef __cplusplus
}
#endif

#endif
