/***************************************************************************
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * Description:
 */
/**\file
 * \brief Audio description
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2014-03-04: create the document
 ***************************************************************************/


#ifndef _AM_AD_H
#define _AM_AD_H

#include "am_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief AD分析器句柄*/
typedef void* AM_AD_Handle_t;

/**\brief AD模块错误代码*/
enum AM_AD_ErrorCode
{
	AM_AD_ERROR_BASE=AM_ERROR_BASE(AM_MOD_AD),
	AM_AD_ERR_INVALID_PARAM,   /**< 参数无效*/
	AM_AD_ERR_INVALID_HANDLE,  /**< 句柄无效*/
	AM_AD_ERR_NOT_SUPPORTED,   /**< 不支持的操作*/
	AM_AD_ERR_OPEN_PES,        /**< 打开PES通道失败*/
	AM_AD_ERR_SET_BUFFER,      /**< 失置PES 缓冲区失败*/
	AM_AD_ERR_NO_MEM,                  /**< 空闲内存不足*/
	AM_AD_ERR_CANNOT_CREATE_THREAD,    /**< 无法创建线程*/
	AM_AD_ERR_END
};

/**\brief AD参数*/
typedef struct
{
	int dmx_id; /**< 使用Demux设备的ID*/
	int pid;    /**< AD 音频的PID*/
}AM_AD_Para_t;

typedef void (*AM_AD_Callback_t) (const uint8_t *data, int len, void *user_data);

/**\brief 创建AD解析句柄
 * \param[out] handle 返回创建的新句柄
 * \param[in] para AD解析参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_ad.h)
 */
extern AM_ErrorCode_t AM_AD_Create(AM_AD_Handle_t *handle, AM_AD_Para_t *para);

/**\brief 释放AD解析句柄
 * \param handle 要释放的句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_ad.h)
 */
extern AM_ErrorCode_t AM_AD_Destroy(AM_AD_Handle_t handle);

/**\brief 开始AD数据解析
 * \param handle AD句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_ad.h)
 */
extern AM_ErrorCode_t AM_AD_Start(AM_AD_Handle_t handle);

/**\brief 停止AD数据解析
 * \param handle AD句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_ad.h)
 */
extern AM_ErrorCode_t AM_AD_Stop(AM_AD_Handle_t handle);


/**\brief 设定AD音量
 * \param handle 要释放的句柄
 * \param vol 音量0~100
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_ad.h)
 */
extern AM_ErrorCode_t AM_AD_SetVolume(AM_AD_Handle_t handle, int vol);

extern AM_ErrorCode_t AM_AD_SetCallback(AM_AD_Handle_t handle, AM_AD_Callback_t cb, void *user);
#ifdef __cplusplus
}
#endif



#endif
