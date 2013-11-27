#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief Misc functions
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-05: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 1

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <am_types.h>
#include <am_debug.h>
#include <am_mem.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static functions
 ***************************************************************************/

#ifdef ANDROID

# define SUN_LEN(ptr) ((size_t) (((struct sockaddr_un*) 0)->sun_path) + strlen ((ptr)->sun_path) )

#endif
static AM_ErrorCode_t try_write(int fd, const char *buf, int len)
{
	const char *ptr = buf;
	int left = len;
	int ret;
	
	while(left)
	{
		ret = write(fd, ptr, left);
		if(ret==-1)
		{
			if(errno!=EINTR)
				return AM_FAILURE;
			ret = 0;
		}
		
		ptr += ret;
		left-= ret;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t try_read(int fd, char *buf, int len)
{
	char *ptr = buf;
	int left = len;
	int ret;
	
	while(left)
	{
		ret = read(fd, ptr, left);
		if(ret==-1)
		{
			if(errno!=EINTR)
				return AM_FAILURE;
			ret = 0;
		}
		
		ptr += ret;
		left-= ret;
	}
	
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 向一个文件打印字符串
 * \param[in] name 文件名
 * \param[in] cmd 向文件打印的字符串
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_FileEcho(const char *name, const char *cmd)
{
	int fd, len, ret;
	
	assert(name && cmd);
	
	fd = open(name, O_WRONLY);
	if(fd==-1)
	{
		AM_DEBUG(1, "cannot open file \"%s\"", name);
		return AM_FAILURE;
	}
	
	len = strlen(cmd);
	
	ret = write(fd, cmd, len);
	if(ret!=len)
	{
		AM_DEBUG(1, "write failed file:\"%s\" cmd:\"%s\" error:\"%s\"", name, cmd, strerror(errno));
		close(fd);
		return AM_FAILURE;
	}

	close(fd);
	
	return AM_SUCCESS;
}

/**\brief 读取一个文件中的字符串
 * \param[in] name 文件名
 * \param[out] buf 存放字符串的缓冲区
 * \param len 缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_FileRead(const char *name, char *buf, int len)
{
	FILE *fp;
	char *ret;
	
	assert(name && buf);
	
	fp = fopen(name, "r");
	if(!fp)
	{
		AM_DEBUG(1, "cannot open file \"%s\"", name);
		return AM_FAILURE;
	}
	
	ret = fgets(buf, len, fp);
	if(!ret)
	{
		AM_DEBUG(1, "read the file:\"%s\" error:\"%s\" failed", name, strerror(errno));
	}
	
	fclose(fp);
	
	return ret ? AM_SUCCESS : AM_FAILURE;
}

/**\brief 创建本地socket服务
 * \param[in] name 服务名称
 * \param[out] fd 返回服务器socket
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_LocalServer(const char *name, int *fd)
{
	struct sockaddr_un addr;
	int s, ret;
	
	assert(name && fd);
	
	s = socket(AF_LOCAL, SOCK_STREAM, 0);
	if(s==-1)
	{
		AM_DEBUG(1, "cannot create local socket \"%s\"", strerror(errno));
		return AM_FAILURE;
	}
	
	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path, name, sizeof(addr.sun_path)-1);

	ret = bind(s, (struct sockaddr*)&addr, SUN_LEN(&addr));
	if(ret==-1)
	{
		AM_DEBUG(1, "bind to \"%s\" failed \"%s\"", name, strerror(errno));
		close(s);
		return AM_FAILURE;
	}

	ret = listen(s, 5);
	if(ret==-1)
	{
		AM_DEBUG(1, "listen failed \"%s\" (%s)", name, strerror(errno));
		close(s);
		return AM_FAILURE;
	}

	*fd = s;
	return AM_SUCCESS;
        
}

/**\brief 通过本地socket连接到服务
 * \param[in] name 服务名称
 * \param[out] fd 返回socket
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_LocalConnect(const char *name, int *fd)
{
	struct sockaddr_un addr;
	int s, ret;
	
	assert(name && fd);
	
	s = socket(AF_LOCAL, SOCK_STREAM, 0);
	if(s==-1)
	{
		AM_DEBUG(1, "cannot create local socket \"%s\"", strerror(errno));
		return AM_FAILURE;
	}
	
	addr.sun_family = AF_LOCAL;
        strncpy(addr.sun_path, name, sizeof(addr.sun_path)-1);
        
        ret = connect(s, (struct sockaddr*)&addr, SUN_LEN(&addr));
        if(ret==-1)
        {
        	AM_DEBUG(1, "connect to \"%s\" failed \"%s\"", name, strerror(errno));
        	close(s);
		return AM_FAILURE;
        }
        
        *fd = s;
        return AM_SUCCESS;
}

/**\brief 通过本地socket发送命令
 * \param fd socket
 * \param[in] cmd 命令字符串
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_LocalSendCmd(int fd, const char *cmd)
{
	AM_ErrorCode_t ret;
	int len;
	
	assert(cmd);
	
	len = strlen(cmd)+1;
	
	ret = try_write(fd, (const char*)&len, sizeof(int));
	if(ret!=AM_SUCCESS)
	{
		AM_DEBUG(1, "write local socket failed");
		return ret;
	}
	
	ret = try_write(fd, cmd, len);
	if(ret!=AM_SUCCESS)
	{
		AM_DEBUG(1, "write local socket failed");
		return ret;
	}
	
	AM_DEBUG(2, "write cmd: %s", cmd);
	
	return AM_SUCCESS;
}

/**\brief 通过本地socket接收响应字符串
 * \param fd socket
 * \param[out] buf 缓冲区
 * \param len 缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_LocalGetResp(int fd, char *buf, int len)
{
	AM_ErrorCode_t ret;
	int bytes;
	
	assert(buf);
	
	ret = try_read(fd, (char*)&bytes, sizeof(int));
	if(ret!=AM_SUCCESS)
	{
		AM_DEBUG(1, "read local socket failed");
		return ret;
	}
	
	if(len<bytes)
	{
		AM_DEBUG(1, "respond buffer is too small");
		return AM_FAILURE;
	}
	
	ret = try_read(fd, buf, bytes);
	if(ret!=AM_SUCCESS)
	{
		AM_DEBUG(1, "read local socket failed");
		return ret;
	}
	
	return AM_SUCCESS;
}

/* UTF8 utilities */

/* This parses a UTF8 string one character at a time. It is passed a pointer
 * to the string and the length of the string. It sets 'value' to the value of
 * the current character. It returns the number of characters read or a
 * negative error code:
 * -1 = string too short
 * -2 = illegal character
 * -3 = subsequent characters not of the form 10xxxxxx
 * -4 = character encoded incorrectly (not minimal length).
 */

int UTF8_getc(const unsigned char *str, int len, unsigned long *val)
{
	const unsigned char *p;
	unsigned long value;
	int ret;
	if(len <= 0) return 0;
	p = str;

	/* Check syntax and work out the encoded value (if correct) */
	if((*p & 0x80) == 0) {
		value = *p++ & 0x7f;
		ret = 1;
	} else if((*p & 0xe0) == 0xc0) {
		if(len < 2) return -1;
		if((p[1] & 0xc0) != 0x80) return -3;
		value = (*p++ & 0x1f) << 6;
		value |= *p++ & 0x3f;
		if(value < 0x80) return -4;
		ret = 2;
	} else if((*p & 0xf0) == 0xe0) {
		if(len < 3) return -1;
		if( ((p[1] & 0xc0) != 0x80)
		   || ((p[2] & 0xc0) != 0x80) ) return -3;
		value = (*p++ & 0xf) << 12;
		value |= (*p++ & 0x3f) << 6;
		value |= *p++ & 0x3f;
		if(value < 0x800) return -4;
		ret = 3;
	} else if((*p & 0xf8) == 0xf0) {
		if(len < 4) return -1;
		if( ((p[1] & 0xc0) != 0x80)
		   || ((p[2] & 0xc0) != 0x80) 
		   || ((p[3] & 0xc0) != 0x80) ) return -3;
		value = ((unsigned long)(*p++ & 0x7)) << 18;
		value |= (*p++ & 0x3f) << 12;
		value |= (*p++ & 0x3f) << 6;
		value |= *p++ & 0x3f;
		if(value < 0x10000) return -4;
		ret = 4;
	} else if((*p & 0xfc) == 0xf8) {
		if(len < 5) return -1;
		if( ((p[1] & 0xc0) != 0x80)
		   || ((p[2] & 0xc0) != 0x80) 
		   || ((p[3] & 0xc0) != 0x80) 
		   || ((p[4] & 0xc0) != 0x80) ) return -3;
		value = ((unsigned long)(*p++ & 0x3)) << 24;
		value |= ((unsigned long)(*p++ & 0x3f)) << 18;
		value |= ((unsigned long)(*p++ & 0x3f)) << 12;
		value |= (*p++ & 0x3f) << 6;
		value |= *p++ & 0x3f;
		if(value < 0x200000) return -4;
		ret = 5;
	} else if((*p & 0xfe) == 0xfc) {
		if(len < 6) return -1;
		if( ((p[1] & 0xc0) != 0x80)
		   || ((p[2] & 0xc0) != 0x80) 
		   || ((p[3] & 0xc0) != 0x80) 
		   || ((p[4] & 0xc0) != 0x80) 
		   || ((p[5] & 0xc0) != 0x80) ) return -3;
		value = ((unsigned long)(*p++ & 0x1)) << 30;
		value |= ((unsigned long)(*p++ & 0x3f)) << 24;
		value |= ((unsigned long)(*p++ & 0x3f)) << 18;
		value |= ((unsigned long)(*p++ & 0x3f)) << 12;
		value |= (*p++ & 0x3f) << 6;
		value |= *p++ & 0x3f;
		if(value < 0x4000000) return -4;
		ret = 6;
	} else return -2;
	*val = value;
	return ret;
}

static int in_utf8(unsigned long value, void *arg)
{
	int *nchar;
	nchar = arg;
	(*nchar) += value;
	return 1;
}

/* This function traverses a string and passes the value of each character
 * to an optional function along with a void * argument.
 */

static int traverse_string(const unsigned char *p, int len,
		 int (*rfunc)(unsigned long value, void *in), void *arg,char *dest, int *dest_len)
{
	unsigned long value;
	int ret;
	while(len) {

		ret = UTF8_getc(p, len, &value);
		if(ret < 0) 
		{
			len -- ;
			p++;
			continue;
		}//return -1;
		else
		{        
			int *pos = arg;

                    if((*pos + ret) > *dest_len)
                        break;            
			
			char *tp = dest+(*pos);
			memcpy(tp,p,ret);
			len -= ret;
			p += ret;			
		}
		if(rfunc) {
			ret = rfunc(ret, arg);
			if(ret <= 0) return ret;
		}
	}
	return 1;
}


/**\brief 跳过无效UTF8字符
 * \param[in] src 源字符缓冲区
 * \param src_len 源字符缓冲区大小
 * \param[out] dest 目标字符缓冲区
 * \param[in] dest_len 目标字符缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_Check_UTF8(const char *src, int src_len, char *dest, int *dest_len)
{
	assert(src);
	assert(dest);
	assert(dest_len);

	int ret;
	int nchar;
	nchar = 0;
	/* This counts the characters and does utf8 syntax checking */
	ret = traverse_string(src, src_len, in_utf8, &nchar,dest, dest_len);

	*dest_len = nchar;	
	return AM_SUCCESS;	
}


