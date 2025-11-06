/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __CAM_DT_BINDINGS_H__
#define __CAM_DT_BINDINGS_H__

/*
 * Wrapper for DT binding header with fallback to local copy.
 */

#if __has_include(<dt-bindings/camera/msm-camera.h>)
#include <dt-bindings/camera/msm-camera.h>
#else
#include "dt-bindings/msm-camera.h"
#endif

#endif