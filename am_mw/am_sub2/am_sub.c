#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif


#include "includes.h"
#include "dvb_sub.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "pthread.h"
#include "memwatch.h"
#include "semaphore.h"
#include <am_sub2.h>
#include <am_misc.h>
#include <am_time.h>
#include <am_debug.h>

typedef struct
{
	INT32U             handle;
	AM_SUB2_Para_t     para;
	AM_Bool_t          running;
	pthread_mutex_t    lock;
	pthread_cond_t     cond;
	pthread_t          thread;
	AM_SUB2_Picture_t *pic;
}AM_SUB2_Parser_t;

static void sub2_check(AM_SUB2_Parser_t *parser)
{
	AM_SUB2_Picture_t *old = parser->pic;
	AM_SUB2_Picture_t *npic;
	uint64_t pts;
	int64_t diff;
	AM_Bool_t loop;

	do
	{
		loop = AM_TRUE;

		if(parser->pic)
		{
			npic = parser->pic->p_next;
		}
		else
		{
			npic = dvbsub_get_display_set(parser->handle);
		}

		if(npic)
		{
			pts = parser->para.get_pts(parser, npic->pts);
			diff = npic->pts - pts;

			if(diff < 0)
			{
				dvbsub_remove_display_picture(parser->handle, parser->pic);
				parser->pic = npic;
			}
			else
			{
				loop = AM_FALSE;
			}
		}

		if(parser->pic)
		{
			pts = parser->para.get_pts(parser, parser->pic->pts);
			diff = pts - parser->pic->pts;
			
			if((diff > parser->pic->timeout * 90000ll) || pts == 0llu)
			{
				dvbsub_remove_display_picture(parser->handle, parser->pic);
				parser->pic = NULL;
			}
		}
	}
	while(npic && !parser->pic && loop);
	
	if(parser->running && (old != parser->pic))
	{
		parser->para.show(parser, parser->pic);
	}
}

static void* sub2_thread(void *arg)
{
	AM_SUB2_Parser_t *parser = (AM_SUB2_Parser_t*)arg;

	pthread_mutex_lock(&parser->lock);

	while(parser->running)
	{
		if(parser->running){
			struct timespec ts;
			int timeout = 20;

			AM_TIME_GetTimeSpecTimeout(timeout, &ts);
			pthread_cond_timedwait(&parser->cond, &parser->lock, &ts);
		}

		sub2_check(parser);
	}

	pthread_mutex_unlock(&parser->lock);

	return NULL;
}

/**\brief 创建subtitle解析句柄
 * \param[out] handle 返回创建的新句柄
 * \param[in] para 解析参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub2.h)
 */
AM_ErrorCode_t AM_SUB2_Create(AM_SUB2_Handle_t *handle, AM_SUB2_Para_t *para)
{
	AM_SUB2_Parser_t *parser;

	if(!handle || !para)
	{
		return AM_SUB2_ERR_INVALID_PARAM;
	}

	parser = (AM_SUB2_Parser_t*)malloc(sizeof(AM_SUB2_Parser_t));
	if(!parser)
	{
		return AM_SUB2_ERR_NO_MEM;
	}

	memset(parser, 0, sizeof(AM_SUB2_Parser_t));

	dvbsub_decoder_create(para->composition_id, para->ancillary_id, NULL, &parser->handle);

	pthread_mutex_init(&parser->lock, NULL);
	pthread_cond_init(&parser->cond, NULL);

	parser->para = *para;

	*handle = parser;
	return AM_SUCCESS;
}

/**\brief 释放subtitle解析句柄
 * \param handle 要释放的句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub2.h)
 */
AM_ErrorCode_t AM_SUB2_Destroy(AM_SUB2_Handle_t handle)
{
	AM_SUB2_Parser_t *parser;

	if(!handle)
	{
		return AM_SUB2_ERR_INVALID_HANDLE;
	}

	parser = (AM_SUB2_Parser_t*)handle;

	AM_SUB2_Stop(handle);

	dvbsub_decoder_destroy(parser->handle);

	pthread_mutex_destroy(&parser->lock);
	pthread_cond_destroy(&parser->cond);

	free(parser);

	return AM_SUCCESS;
}

/**\brief 取得用户定义数据
 * \param handle 句柄
 * \return 用户定义数据
 */
void*
AM_SUB2_GetUserData(AM_SUB2_Handle_t handle)
{
	AM_SUB2_Parser_t *parser;

	parser = (AM_SUB2_Parser_t*)handle;
	if(!parser)
	{
		return NULL;
	}

	return parser->para.user_data;
}

/**\brief 分析subtitle数据
 * \param handle 句柄
 * \param[in] buf PES数据缓冲区
 * \param size 缓冲区内数据大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub2.h)
 */
AM_ErrorCode_t AM_SUB2_Decode(AM_SUB2_Handle_t handle, uint8_t *buf, int size)
{
	AM_SUB2_Parser_t *parser;

	if(!handle)
	{
		return AM_SUB2_ERR_INVALID_HANDLE;
	}

	if(!buf || !size)
	{
		return AM_SUB2_ERR_INVALID_PARAM;
	}

	parser = (AM_SUB2_Parser_t*)handle;

	pthread_mutex_lock(&parser->lock);
	dvbsub_parse_pes_packet(parser->handle, buf, size);
	sub2_check(parser);
	pthread_mutex_unlock(&parser->lock);

	return AM_SUCCESS;
}

/**\brief 开始subtitle显示
 * \param handle 句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub2.h)
 */
AM_ErrorCode_t AM_SUB2_Start(AM_SUB2_Handle_t handle)
{
	AM_SUB2_Parser_t *parser;
	AM_ErrorCode_t ret = AM_SUCCESS;

	if(!handle)
	{
		return AM_SUB2_ERR_INVALID_HANDLE;
	}

	parser = (AM_SUB2_Parser_t*)handle;

	pthread_mutex_lock(&parser->lock);

	if(!parser->running)
	{
		parser->running = AM_TRUE;
		if(pthread_create(&parser->thread, NULL, sub2_thread, parser))
		{
			parser->running = AM_FALSE;
			ret = AM_SUB2_ERR_CANNOT_CREATE_THREAD;
		}
	}

	pthread_mutex_unlock(&parser->lock);

	return ret;
}

/**\brief 停止subtitle显示
 * \param handle 句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub2.h)
 */
AM_ErrorCode_t AM_SUB2_Stop(AM_SUB2_Handle_t handle)
{
	AM_SUB2_Parser_t *parser;
	pthread_t th;
	AM_Bool_t wait = AM_FALSE;

	if(!handle)
	{
		return AM_SUB2_ERR_INVALID_HANDLE;
	}

	parser = (AM_SUB2_Parser_t*)handle;

	pthread_mutex_lock(&parser->lock);

	if(parser->running)
	{
		parser->running = AM_FALSE;
		pthread_cond_signal(&parser->cond);
		th = parser->thread;
		wait = AM_TRUE;
	}

	pthread_mutex_unlock(&parser->lock);

	if(wait){
		pthread_join(th, NULL);
	}

	return AM_SUCCESS;
}

