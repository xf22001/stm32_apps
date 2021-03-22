#include <string.h>
#include <errno.h>

#include "lwip/netif.h"
#include "lwip/sockets.h"

#include "os_utils.h"
#include "ftpd/ftpd_file_rtt.h"
#include "poll_loop.h"
#include "mt_file.h"
#include "log.h"

#define FTP_PORT 21
#define FTP_SRV_ROOT "/"
#define FTP_MAX_CONNECTION 2
#define FTP_USER "rtt"
#define FTP_PASSWORD "demo"
#define FTP_WELCOME_MSG "220-= welcome on RT-Thread FTP server =-\r\n220 \r\n"
#define FTP_BUFFER_SIZE 1024


typedef struct {
	uint8_t ip1;
	uint8_t ip2;
	uint8_t ip3;
	uint8_t ip4;
} ip_addr_field_t;

typedef union {
	ip_addr_field_t s;
	uint32_t v;
} u_ip_addr_field_t;

typedef int (*session_data_callback_t)(void *ctx);

struct ftp_session {
	rt_bool_t is_anonymous;

	int sockfd;
	struct sockaddr_in remote;

	unsigned short pasv_port;

	int pasv_server_sockfd;
	rt_tick_t pasv_server_stamp;//rt_tick_get

	int data_sockfd;
	struct sockaddr_in data_remote;
	u_poll_mask_t data_poll_mask;
	rt_tick_t data_stamp;//rt_tick_get
	session_data_callback_t session_data_callback;

	FIL file;
	FIL *filefd;
	size_t offset;

	/* current directory */
	char currentdir[256];

	struct ftp_session *next;
};

static struct ftp_session *session_list = NULL;
unsigned short pasv_port = 10000;
static DIR dir;
static FILINFO fno;

static int ftp_process_request(struct ftp_session *session, char *buf);
static int ftp_get_filesize(char *filename);

static struct ftp_session *ftp_new_session()
{
	struct ftp_session *session;

	session = (struct ftp_session *)rt_malloc(sizeof(struct ftp_session));

	if(session == NULL) {
		return session;
	}

	session->is_anonymous = RT_TRUE;
	session->sockfd = -1;

	session->pasv_server_sockfd = -1;
	session->pasv_server_stamp = 0;

	session->data_sockfd = -1;
	session->session_data_callback = NULL;

	session->filefd = NULL;
	session->offset = 0;

	session->next = session_list;
	session_list = session;

	debug("new session:%p", session);

	return session;
}

static void ftp_close_session(struct ftp_session *session)
{
	struct ftp_session *list;

	if(session == NULL) {
		return;
	}

	if(session->sockfd != -1) {
		closesocket(session->sockfd);
	}

	if(session->pasv_server_sockfd != -1) {
		closesocket(session->pasv_server_sockfd);
	}

	if(session->data_sockfd != -1) {
		closesocket(session->data_sockfd);
	}

	if(session->filefd != NULL) {
		mt_f_close(session->filefd);
	}

	list = session_list;

	if(list == session) {
		session_list = list->next;
	} else {
		while (list != NULL) {
			if(list->next == session) {
				break;
			}

			list = list->next;
		}

		if(list != NULL) {
			list->next = session->next;
		} else {
			debug("list == NULL!!!!");
		}
	}

	debug("free session:%p", session);
	rt_free(session);
}

static int ftp_get_filesize(char *filename)
{
	FRESULT ret = mt_f_stat(filename, &fno);

	if(ret != FR_OK) {
		return -1;
	}

	return fno.fsize;
}

static rt_bool_t is_absolute_path(char *path)
{
#ifdef _WIN32

	if (path[0] == '\\' ||
	    (path[1] == ':' && path[2] == '\\')) {
		return RT_TRUE;
	}

#else

	if (path[0] == '/') {
		return RT_TRUE;
	}

#endif

	return RT_FALSE;
}

static void normalize_absolute_path(char *path)
{
	char *r = NULL;
	char *w = NULL;
	char *end = path + strlen(path);
	int part_flag = 0;

	r = path;

	while(r != end) {
		if(*r == '/') {
			*r = 0;
		}

		r++;
	}

	r = path;
	w = path;

	while(r != end) {
		if(*(r + 0) == 0) {//处理：'/'
			if(part_flag == 0) {//添加节号
				*w = '/';
				//debug("[%d] w:%c", w - path, *w);
				w++;
			} else {//已添加节号，不再添加
				//debug("[%d] process /", w - path);
			}

			if(((r + 3) <= end) && (*(r + 1) == '.') && (*(r + 2) == '.') && (*(r + 3) == 0)) {//处理'/../../' 回退一节
				char *last_part;

				w--;//刚刚添加了节号，退回到节号上
				*w = 0;//清除节号

				last_part = strrchr(path, '/');

				if(last_part != NULL) {//回退一节成功
					w = last_part;
					//debug("[%d] process /../", w - path);
					w++;
				} else {//回退一节失败
					*w = '/';//恢复节号
					//debug("[%d] process /../", w - path);
					w++;
				}

				r += 3;
			} else if(((r + 2) <= end) && (*(r + 1) == '.') && (*(r + 2) == 0)) {//处理'/././' 忽略无效节
				r += 2;
				//debug("[%d] process /./", w - path);
			} else {//处理节号'/'
				r++;
				//debug("[%d] process /", w - path);
			}

			part_flag = 1;
		} else {//不为'/'
			part_flag = 0;
			*w = *r;
			//debug("[%d] w:%c", w - path, *w);
			w++;
			r++;
		}
	}

	*w = 0;
}

static int build_full_path(struct ftp_session *session, char *path, char *new_path, size_t size)
{
	if (is_absolute_path(path) == RT_TRUE) {
		strcpy(new_path, path);
	} else {
		rt_sprintf(new_path, "%s/%s", session->currentdir, path);
	}

	//debug("new_path:%s", new_path);

	normalize_absolute_path(new_path);

	return 0;
}

static int ftpd_send(int s, const void *data, size_t size, int flags)
{
	int ret = -1;
	fd_set wfds;
	struct timeval tv;
	size_t sent = 0;

	while(sent < size) {
		FD_ZERO(&wfds);
		FD_SET(s, &wfds);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		ret = select(s + 1, NULL, &wfds, NULL, &tv);

		if(ret > 0) {
			if(FD_ISSET(s, &wfds)) {
				ret = send(s, data + sent, size - sent, flags);

				if(ret > 0) {
					sent += ret;
					ret = 0;
				} else {
					debug("%s", strerror(errno));
					ret = -1;
					break;
				}
			} else {
				debug("%s", strerror(errno));
				ret = -1;
			}
		} else {
			debug("%s", strerror(errno));
			ret = -1;
		}
	}

	if(ret == 0) {
		ret = sent;
	}

	return ret;
}

static void ftpd_thread_entry(void const *argument)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int sockfd, maxfd;
	struct sockaddr_in local;
	fd_set rfds;
	fd_set wfds;
	fd_set efds;
	struct ftp_session *session;
	rt_uint32_t addr_len = sizeof(struct sockaddr);

	local.sin_port = htons(FTP_PORT);
	local.sin_family = PF_INET;
	local.sin_addr.s_addr = INADDR_ANY;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0) {
		rt_kprintf("create socket failed\n");
		return;
	}

	bind(sockfd, (struct sockaddr *)&local, addr_len);
	listen(sockfd, FTP_MAX_CONNECTION);

	for(;;) {
		struct timeval tv;
		int ret;

		tv.tv_sec = 0;
		tv.tv_usec = 100 * 1000;

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&efds);

		FD_SET(sockfd, &rfds);

		/* get maximum fd */
		maxfd = sockfd;

		session = session_list;

		while (session != RT_NULL) {
			if (maxfd < session->sockfd) {
				maxfd = session->sockfd;
			}

			FD_SET(session->sockfd, &rfds);

			if(session->pasv_server_sockfd != -1) {
				if (maxfd < session->pasv_server_sockfd) {
					maxfd = session->pasv_server_sockfd;
				}

				FD_SET(session->pasv_server_sockfd, &rfds);
			}

			if(session->data_sockfd != -1) {
				if (maxfd < session->data_sockfd) {
					maxfd = session->data_sockfd;
				}

				if(session->data_poll_mask.s.poll_in == 1) {
					FD_SET(session->data_sockfd, &rfds);
				}

				if(session->data_poll_mask.s.poll_out == 1) {
					FD_SET(session->data_sockfd, &wfds);
				}

				if(session->data_poll_mask.s.poll_err == 1) {
					FD_SET(session->data_sockfd, &efds);
				}
			}

			session = session->next;
		}

		ret = select(maxfd + 1, &rfds, &wfds, &efds, &tv);

		if(ret == 0) {
			continue;
		} else if(ret < 0) {
			debug("ret:%d", ret);
			continue;
		}

		if(FD_ISSET(sockfd, &rfds)) {
			int session_sockfd;
			struct sockaddr_in session_remote;

			debug("process session accept");
			session_sockfd = accept(sockfd, (struct sockaddr *)&session_remote, &addr_len);

			if(session_sockfd == -1) {
				rt_kprintf("Error on accept()\nContinuing...\n");
				continue;
			} else {
				rt_kprintf("Got connection from %s\n", inet_ntoa(session_remote.sin_addr));
				ftpd_send(session_sockfd, FTP_WELCOME_MSG, strlen(FTP_WELCOME_MSG), 0);

				/* new session */
				session = ftp_new_session();

				if (session != NULL) {
					strcpy(session->currentdir, FTP_SRV_ROOT);
					session->sockfd = session_sockfd;
					session->remote = session_remote;
				}
			}
		} else {
			struct ftp_session *next;
			session = session_list;

			while (session != NULL) {
				next = session->next;

				if(FD_ISSET(session->sockfd, &rfds)) {
					int numbytes;
					numbytes = recv(session->sockfd, buffer, FTP_BUFFER_SIZE, 0);

					if(numbytes == 0 || numbytes == -1) {
						debug("Client %s disconnected", inet_ntoa(session->remote.sin_addr));
						//FD_CLR(session->sockfd, &rfds);
						ftp_close_session(session);
						break;
					} else {
						//debug("process session cmd");
						buffer[numbytes] = 0;

						if(ftp_process_request(session, buffer) == -1) {
							debug("Client %s disconnected", inet_ntoa(session->remote.sin_addr));
							ftp_close_session(session);
							break;
						}
					}
				}

				if(session->pasv_server_sockfd != -1) {
					if(ticks_duration(rt_tick_get(), session->pasv_server_stamp) >= rt_tick_from_millisecond(3000)) {
						closesocket(session->pasv_server_sockfd);
						session->pasv_server_sockfd = -1;
						rt_sprintf(buffer, "425 Can't open data connection.\r\n");
						ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
						debug("pasv_server_sockfd timeout, close...");
					} else {
						if(FD_ISSET(session->pasv_server_sockfd, &rfds)) {
							if(session->data_sockfd == -1) {
								session->data_sockfd = accept(session->pasv_server_sockfd, (struct sockaddr *)&session->data_remote, &addr_len);

								if(session->data_sockfd == -1) {
									rt_sprintf(buffer, "425 Can't open data connection.\r\n");
									ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
									debug("accept %s", strerror(errno));
								} else {
									debug("Got pasv connection from %s", inet_ntoa(session->data_remote.sin_addr));
									session->data_stamp = rt_tick_get();

									closesocket(session->pasv_server_sockfd);
									session->pasv_server_sockfd = -1;
									debug("Leave pasv mode...");
								}
							}
						}
					}

				}

				if(session->data_sockfd != -1) {
					if(ticks_duration(rt_tick_get(), session->data_stamp) >= rt_tick_from_millisecond(3000)) {
						if(session->filefd != NULL) {
							mt_f_close(session->filefd);
							session->filefd = NULL;
						}

						closesocket(session->data_sockfd);
						session->data_sockfd = -1;
						session->session_data_callback = NULL;
						debug("data_sockfd timeout! close...");
					} else {
						if(session->data_poll_mask.s.poll_in == 1) {
							if(FD_ISSET(session->data_sockfd, &rfds)) {
								if(session->session_data_callback != NULL) {
									//debug("process data_sockfd");
									if(session->session_data_callback(session) == -1) {
										ftp_close_session(session);
										break;
									}

									session->data_stamp = rt_tick_get();
								}
							}
						}

						if(session->data_poll_mask.s.poll_out == 1) {
							if(FD_ISSET(session->data_sockfd, &wfds)) {
								if(session->session_data_callback != NULL) {
									//debug("process data_sockfd");
									if(session->session_data_callback(session) == -1) {
										ftp_close_session(session);
										break;
									}

									session->data_stamp = rt_tick_get();
								}
							}
						}

						if(session->data_poll_mask.s.poll_err == 1) {
							if(FD_ISSET(session->data_sockfd, &efds)) {
								debug("poll err!");
							}
						}
					}
				}

				session = next;
			}
		}
	}

	rt_free(buffer);
}

static int str_begin_with(char *src, char *match)
{
	while (*match) {
		/* check source */
		if (*src == 0) {
			return -1;
		}

		if (*match != *src) {
			return -1;
		}

		match ++;
		src ++;
	}

	return 0;
}


static int list_callback(void *ctx)
{
	struct ftp_session *session = (struct ftp_session *)ctx;
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		debug("no memory");

		rt_sprintf(buffer, "500 Internal Error\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		return ret;
	}

	ret = mt_f_opendir(&dir, session->currentdir);

	if (ret != FR_OK) {
		debug("opendir ret %d", ret);

		rt_sprintf(buffer, "500 Internal Error\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		goto exit;
	}

	while (1) {
		int length = 0;
		ret = mt_f_readdir(&dir, &fno);

		if (ret != FR_OK) {
			break;
		}

		if(fno.fname[0] == 0) {
			break;
		}

		if (fno.fattrib & AM_DIR) {
			length = rt_sprintf(buffer, "drw-r--r-- 1 admin admin %d Jan 1 2000 %s\r\n", 0, fno.fname);
		} else {
			length = rt_sprintf(buffer, "-rw-r--r-- 1 admin admin %d Jan 1 2000 %s\r\n", (int)fno.fsize, fno.fname);
		}

		ftpd_send(session->data_sockfd, buffer, length, 0);
	}

	ret = 0;

	mt_f_closedir(&dir);

	rt_sprintf(buffer, "226 Transfert Complete.\r\n");
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

exit:
	closesocket(session->data_sockfd);
	session->data_sockfd = -1;
	session->session_data_callback = NULL;

	rt_free(buffer);

	return ret;
}

static int simple_list_callback(void *ctx)
{
	struct ftp_session *session = (struct ftp_session *)ctx;
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		rt_sprintf(buffer, "500 Internal Error\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		return ret;
	}

	ret = mt_f_opendir(&dir, session->currentdir);

	if (ret != FR_OK) {
		debug("opendir %s", strerror(errno));

		rt_sprintf(buffer, "500 Internal Error\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		goto exit;
	}

	while (1) {
		ret = mt_f_readdir(&dir, &fno);

		if (ret != FR_OK) {
			break;
		}

		if(fno.fname[0] == 0) {
			break;
		}

		rt_sprintf(buffer, "%s\r\n", fno.fname);
		ftpd_send(session->data_sockfd, buffer, strlen(buffer), 0);
	}

	mt_f_closedir(&dir);

	rt_sprintf(buffer, "226 Transfert Complete.\r\n");
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

exit:

	closesocket(session->data_sockfd);
	session->data_sockfd = -1;
	session->session_data_callback = NULL;

	rt_free(buffer);
	return 0;
}

static int retr_callback(void *ctx)
{
	struct ftp_session *session = (struct ftp_session *)ctx;
	int  numbytes;
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		rt_sprintf(buffer, "500 Internal Error\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		return ret;
	}

	ret = mt_f_read(session->filefd, buffer, FTP_BUFFER_SIZE, (UINT *)&numbytes);

	if(ret == FR_OK) {
		if(numbytes > 0) {
			ret = ftpd_send(session->data_sockfd, buffer, numbytes, 0);

			if(ret <= 0) {
				debug("abort retr!");

				mt_f_close(session->filefd);
				session->filefd = NULL;
				closesocket(session->data_sockfd);
				session->data_sockfd = -1;
				session->session_data_callback = NULL;
			} else {
				ret = 0;
			}
		} else if(numbytes == 0) {
			rt_sprintf(buffer, "226 Finished.\r\n");
			ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
			ret = 0;

			mt_f_close(session->filefd);
			session->filefd = NULL;
			closesocket(session->data_sockfd);
			session->data_sockfd = -1;
			session->session_data_callback = NULL;
		} else if(numbytes == -1) {
			debug("abort retr");

			mt_f_close(session->filefd);
			session->filefd = NULL;
			closesocket(session->data_sockfd);
			session->data_sockfd = -1;
			session->session_data_callback = NULL;
		}
	} else {
		debug("abort retr");

		mt_f_close(session->filefd);
		session->filefd = NULL;
		closesocket(session->data_sockfd);
		session->data_sockfd = -1;
		session->session_data_callback = NULL;
	}


	rt_free(buffer);

	return ret;
}

static int stor_callback(void *ctx)
{
	struct ftp_session *session = (struct ftp_session *)ctx;
	int  numbytes;
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		rt_sprintf(buffer, "500 Internal Error\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		return ret;
	}

	if((numbytes = recv(session->data_sockfd, buffer, FTP_BUFFER_SIZE, 0)) > 0) {
		ret = mt_f_write(session->filefd, buffer, numbytes, (UINT *)&numbytes);

		if(ret == FR_OK) {
			if(numbytes > 0) {
				ret = 0;
			} else {
				debug("abort stor!");

				mt_f_close(session->filefd);
				session->filefd = NULL;
				closesocket(session->data_sockfd);
				session->data_sockfd = -1;
				session->session_data_callback = NULL;
			}
		} else {
			debug("abort stor!");

			mt_f_close(session->filefd);
			session->filefd = NULL;
			closesocket(session->data_sockfd);
			session->data_sockfd = -1;
			session->session_data_callback = NULL;
		}
	} else if(numbytes == 0) {
		debug("finish stor!");
		rt_sprintf(buffer, "226 Finished.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;

		mt_f_close(session->filefd);
		session->filefd = NULL;
		closesocket(session->data_sockfd);
		session->data_sockfd = -1;
		session->session_data_callback = NULL;
	} else {
		debug("abort stor!");

		mt_f_close(session->filefd);
		session->filefd = NULL;
		closesocket(session->data_sockfd);
		session->data_sockfd = -1;
		session->session_data_callback = NULL;
	}

	rt_free(buffer);

	return ret;
}

static int do_user(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	debug("%s sent login \"%s\"", inet_ntoa(session->remote.sin_addr), parameter);

	if(buffer == NULL) {
		return ret;
	}

	// login correct
	if(strcmp(parameter, "anonymous") == 0) {
		session->is_anonymous = RT_TRUE;
		rt_sprintf(buffer, "331 Anonymous login OK SEND e-mail address for password.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;
	} else if (strcmp(parameter, FTP_USER) == 0) {
		session->is_anonymous = RT_FALSE;
		rt_sprintf(buffer, "331 Password required for %s\r\n", parameter);
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;
	} else {
		// incorrect login
		rt_sprintf(buffer, "530 Login incorrect. Bye.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	}

	rt_free(buffer);
	return ret;
}

static int do_pass(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	rt_kprintf("%s sent password \"%s\"\n", inet_ntoa(session->remote.sin_addr), parameter);

	if(buffer == NULL) {
		return ret;
	}

	if (strcmp(parameter, FTP_PASSWORD) == 0 || session->is_anonymous == RT_TRUE) {
		// password correct
		rt_sprintf(buffer, "230 User logged in\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;
	} else {
		// incorrect password
		rt_sprintf(buffer, "530 Login or Password incorrect. Bye!\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	}

	rt_free(buffer);

	return ret;
}

static int do_list(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "150 Opening Binary mode connection for file list.\r\n");
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	ret = 0;

	debug("start list_callback");
	session->data_poll_mask.v = 0;
	session->data_poll_mask.s.poll_out = 1;
	session->session_data_callback = list_callback;

	rt_free(buffer);

	return ret;
}

static int do_nlst(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "150 Opening Binary mode connection for file list.\r\n");
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	ret = 0;

	debug("start simple_list_callback");
	session->data_poll_mask.v = 0;
	session->data_poll_mask.s.poll_out = 1;
	session->session_data_callback = simple_list_callback;

	rt_free(buffer);

	return ret;
}

static int do_pwd(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "257 \"%s\" is current directory.\r\n", session->currentdir);
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	ret = 0;

	rt_free(buffer);

	return ret;
}

static int do_type(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if(strcmp(parameter, "I") == 0) {
		rt_sprintf(buffer, "200 Type set to binary.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	} else {
		rt_sprintf(buffer, "200 Type set to ascii.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	}

	ret = 0;

	rt_free(buffer);

	return ret;
}

static int do_pasv(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int dig1 = 0;
	int dig2 = 0;
	int optval = 1;
	struct sockaddr_in local;
	u_ip_addr_field_t u_ip_addr_field;
	int ret = -1;
	rt_uint32_t addr_len = sizeof(struct sockaddr_in);

	if(buffer == NULL) {
		return ret;
	}

	ret = 0;

	if(session->data_sockfd != -1) {
		debug("session->data_sockfd exist!");

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
		goto exit;
	}

	if(session->pasv_server_sockfd != -1) {
		debug("session->pasv_server_sockfd exist!");

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
		goto exit;
	}

	if(pasv_port < 10000) {
		pasv_port = 10000;
	}

	session->pasv_port = pasv_port;
	pasv_port++;

	local.sin_family = PF_INET;
	local.sin_port = htons(session->pasv_port);
	local.sin_addr.s_addr = INADDR_ANY;

	if((session->pasv_server_sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		debug("socket %s", strerror(errno));

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
		goto exit;
	}

	if(setsockopt(session->pasv_server_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
		debug("setsockopt %s", strerror(errno));

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		closesocket(session->pasv_server_sockfd);
		session->pasv_server_sockfd = -1;
		goto exit;
	}

	if(bind(session->pasv_server_sockfd, (struct sockaddr *)&local, addr_len) == -1) {
		debug("bind %s", strerror(errno));

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		closesocket(session->pasv_server_sockfd);
		session->pasv_server_sockfd = -1;
		goto exit;
	}

	if(listen(session->pasv_server_sockfd, 1) == -1) {
		debug("listen %s", strerror(errno));

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		closesocket(session->pasv_server_sockfd);
		session->pasv_server_sockfd = -1;
		goto exit;
	}

	session->pasv_server_stamp = rt_tick_get();

	dig1 = (int)(session->pasv_port / 256);
	dig2 = session->pasv_port % 256;
	u_ip_addr_field.v = netif_default->ip_addr.addr;

	rt_kprintf("Listening %d seconds @ port %d\n", 3, session->pasv_port);

	rt_sprintf(buffer, "227 Entering passive mode (%d,%d,%d,%d,%d,%d)\r\n", u_ip_addr_field.s.ip1, u_ip_addr_field.s.ip2, u_ip_addr_field.s.ip3, u_ip_addr_field.s.ip4, dig1, dig2);
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

exit:

	rt_free(buffer);

	return ret;
}

static int do_retr(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	char *filename = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;
	int file_size;

	if(buffer == NULL) {
		return ret;
	}

	if(filename == NULL) {
		rt_free(buffer);
		return ret;
	}

	//debug("session->currentdir:%s", session->currentdir);
	build_full_path(session, parameter, filename, FTP_BUFFER_SIZE);
	debug("filename:%s", filename);

	file_size = ftp_get_filesize(filename);

	if (file_size == -1) {
		debug("ftp_get_filesize error");
		rt_sprintf(buffer, "550 \"%s\" : not a regular file\r\n", filename);
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		goto exit;
	}

	ret = mt_f_open(&session->file, filename, FA_READ);

	if (ret != FR_OK) {
		debug("mt_f_open error");
		rt_sprintf(buffer, "550 \"%s\" : no access file\r\n", filename);
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		goto exit;
	}

	session->filefd = &session->file;

	if(session->offset > 0 && session->offset < file_size) {
		mt_f_lseek(session->filefd, session->offset);
		rt_sprintf(buffer, "150 Opening binary mode data connection for partial \"%s\" (%d/%d bytes).\r\n", filename, file_size - session->offset, file_size);
	} else {
		rt_sprintf(buffer, "150 Opening binary mode data connection for \"%s\" (%d bytes).\r\n", filename, file_size);
	}

	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

	ret = 0;

	debug("start retr_callback");
	session->data_poll_mask.v = 0;
	session->data_poll_mask.s.poll_out = 1;
	session->session_data_callback = retr_callback;

exit:
	rt_free(filename);
	rt_free(buffer);

	return ret;
}

static int do_stor(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	char *filename = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if(filename == NULL) {
		rt_free(buffer);
		return ret;
	}

	if(session->is_anonymous == RT_TRUE) {
		rt_sprintf(buffer, "550 Permission denied.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		debug("abort stor!");
		goto exit;
	}

	//debug("session->currentdir:%s", session->currentdir);
	build_full_path(session, parameter, filename, FTP_BUFFER_SIZE);
	debug("filename:%s", filename);

	ret = mt_f_open(&session->file, filename, FA_CREATE_ALWAYS | FA_WRITE);

	if(ret != FR_OK) {
		rt_sprintf(buffer, "550 Cannot open \"%s\" for writing.\r\n", filename);
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		goto exit;
	}

	session->filefd = &session->file;

	rt_sprintf(buffer, "150 Opening binary mode data connection for \"%s\".\r\n", filename);
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

	debug("start stor_callback");

	session->data_poll_mask.v = 0;
	session->data_poll_mask.s.poll_in = 1;
	session->session_data_callback = stor_callback;

	ret = 0;

exit:
	rt_free(filename);
	rt_free(buffer);

	return ret;
}

static int do_size(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	char *filename = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;
	int file_size;

	if(buffer == NULL) {
		return ret;
	}

	if(filename == NULL) {
		rt_free(buffer);
		return ret;
	}

	//debug("session->currentdir:%s", session->currentdir);
	build_full_path(session, parameter, filename, FTP_BUFFER_SIZE);
	debug("filename:%s", filename);

	file_size = ftp_get_filesize(filename);

	if( file_size == -1) {
		rt_sprintf(buffer, "550 \"%s\" : not a regular file\r\n", filename);
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	} else {
		rt_sprintf(buffer, "213 %d\r\n", file_size);
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	}

	ret = 0;

	rt_free(filename);
	rt_free(buffer);

	return ret;
}

static int do_mdtm(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "550 \"/\" : not a regular file\r\n");
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	ret = 0;
	rt_free(buffer);

	return ret;
}

static int do_syst(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "215 %s\r\n", "RT-Thread RTOS");
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	ret = 0;
	rt_free(buffer);

	return ret;
}

static int do_cwd(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	char *filename = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if(filename == NULL) {
		rt_free(buffer);
		return ret;
	}

	//debug("session->currentdir:%s", session->currentdir);
	build_full_path(session, parameter, filename, FTP_BUFFER_SIZE);
	debug("filename:%s", filename);

	rt_sprintf(buffer, "250 Changed to directory \"%s\"\r\n", filename);
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	strcpy(session->currentdir, filename);
	rt_kprintf("Changed to directory %s\n", filename);
	ret = 0;
	rt_free(filename);
	rt_free(buffer);

	return ret;
}

static int do_cdup(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	char *filename = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if(filename == NULL) {
		rt_free(buffer);
		return ret;
	}

	//debug("session->currentdir:%s", session->currentdir);
	build_full_path(session, "..", filename, FTP_BUFFER_SIZE);
	debug("filename:%s", filename);

	rt_sprintf(buffer, "250 Changed to directory \"%s\"\r\n", filename);
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	strcpy(session->currentdir, filename);
	rt_kprintf("Changed to directory %s\n", filename);

	ret = 0;
	rt_free(filename);
	rt_free(buffer);

	return ret;
}

static int do_port(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;
	int i;
	int portcom[6];
	char tmpip[100];
	rt_uint32_t addr_len = sizeof(struct sockaddr_in);

	if(buffer == NULL) {
		return ret;
	}

	if(session->data_sockfd != -1) {
		debug("session->data_sockfd exist!");

		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;
		goto exit;
	}

	i = 0;
	portcom[i++] = atoi(strtok(parameter, ".,;()"));

	for(; i < 6; i++) {
		portcom[i] = atoi(strtok(0, ".,;()"));
	}

	rt_sprintf(tmpip, "%d.%d.%d.%d", portcom[0], portcom[1], portcom[2], portcom[3]);

	if((session->data_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		debug("socket %s", strerror(errno));
		rt_sprintf(buffer, "425 Can't open data connection.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		goto exit;
	}

	session->data_remote.sin_addr.s_addr = inet_addr(tmpip);
	session->data_remote.sin_port = htons(portcom[4] * 256 + portcom[5]);
	session->data_remote.sin_family = PF_INET;

	if(connect(session->data_sockfd, (struct sockaddr *)&session->data_remote, addr_len) == -1) {
		// is it only local address?try using gloal ip addr
		session->data_remote.sin_addr = session->remote.sin_addr;

		if(connect(session->data_sockfd, (struct sockaddr *)&session->data_remote, addr_len) == -1) {
			debug("connect %s", strerror(errno));
			rt_sprintf(buffer, "425 Can't open data connection.\r\n");
			ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

			closesocket(session->data_sockfd);
			session->data_sockfd = -1;
			session->session_data_callback = NULL;

			ret = 0;
			goto exit;
		}
	}

	session->pasv_port = portcom[4] * 256 + portcom[5];
	rt_kprintf("Connected to Data(PORT) %s @ %d\n", tmpip, portcom[4] * 256 + portcom[5]);
	rt_sprintf(buffer, "200 Port Command Successful.\r\n");
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	session->data_stamp = rt_tick_get();

	ret = 0;
exit:
	rt_free(buffer);

	return ret;
}

static int do_rest(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if(atoi(parameter) >= 0) {
		session->offset = atoi(parameter);
		rt_sprintf(buffer, "350 Send RETR or STOR to start transfert.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	}

	ret = 0;

	rt_free(buffer);

	return ret;
}

static int do_mkd(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	char *filename = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if(filename == NULL) {
		rt_free(buffer);
		return ret;
	}


	if (session->is_anonymous == RT_TRUE) {
		rt_sprintf(buffer, "550 Permission denied.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;

		goto exit;
	}

	//debug("session->currentdir:%s", session->currentdir);
	build_full_path(session, parameter, filename, FTP_BUFFER_SIZE);
	debug("filename:%s", filename);

	if(mt_f_mkdir(filename) != FR_OK) {
		rt_sprintf(buffer, "550 File \"%s\" exists.\r\n", filename);
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	} else {
		rt_sprintf(buffer, "257 directory \"%s\" successfully created.\r\n", filename);
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	}

	ret = 0;

exit:
	rt_free(filename);
	rt_free(buffer);

	return ret;
}

static int do_dele(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	char *filename = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if(filename == NULL) {
		rt_free(buffer);
		return ret;
	}


	if (session->is_anonymous == RT_TRUE) {
		rt_sprintf(buffer, "550 Permission denied.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

		ret = 0;
		goto exit;
	}

	//debug("session->currentdir:%s", session->currentdir);
	build_full_path(session, parameter, filename, FTP_BUFFER_SIZE);
	debug("filename:%s", filename);

	if(mt_f_unlink(filename) == FR_OK) {
		rt_sprintf(buffer, "250 Successfully deleted file \"%s\".\r\n", filename);
	} else {
		rt_sprintf(buffer, "550 Not such file or directory: %s.\r\n", filename);
	}

	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

	ret = 0;

exit:
	rt_free(filename);
	rt_free(buffer);

	return ret;
}

static int do_rmd(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	char *filename = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	if(filename == NULL) {
		rt_free(buffer);
		return ret;
	}


	if (session->is_anonymous == RT_TRUE) {
		rt_sprintf(buffer, "550 Permission denied.\r\n");
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
		ret = 0;
		goto exit;
	}

	//debug("session->currentdir:%s", session->currentdir);
	build_full_path(session, parameter, filename, FTP_BUFFER_SIZE);
	debug("filename:%s", filename);

	if(mt_f_unlink(filename) != FR_OK) {
		rt_sprintf(buffer, "550 Directory \"%s\" doesn't exist.\r\n", filename);
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	} else {
		rt_sprintf(buffer, "257 directory \"%s\" successfully deleted.\r\n", filename);
		ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	}

	ret = 0;

exit:
	rt_free(filename);
	rt_free(buffer);

	return ret;
}

static int do_quit(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "221 Bye!\r\n");
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);

	rt_free(buffer);

	return ret;
}

static int do_un_imp(struct ftp_session *session, char *parameter)
{
	char *buffer = (char *)rt_malloc(FTP_BUFFER_SIZE);
	int ret = -1;

	if(buffer == NULL) {
		return ret;
	}

	rt_sprintf(buffer, "502 Not Implemented.\r\n");
	ftpd_send(session->sockfd, buffer, strlen(buffer), 0);
	ret = 0;

	rt_free(buffer);

	return ret;
}

static int ftp_process_request(struct ftp_session *session, char *buf)
{
	//int  numbytes;
	char *parameter_ptr, *ptr;
	int ret;

	/* remove \r\n */
	ptr = buf;

	while (*ptr) {
		if (*ptr == '\r' || *ptr == '\n') {
			*ptr = 0;
		}

		ptr ++;
	}

	/* get request parameter */
	parameter_ptr = strchr(buf, ' ');

	if(parameter_ptr != NULL) {
		parameter_ptr++;
	}

	// debug:
	rt_kprintf("%s requested: \"%s\"\n", inet_ntoa(session->remote.sin_addr), buf);

	//
	//-----------------------
	if(str_begin_with(buf, "USER") == 0) {
		ret = do_user(session, parameter_ptr);
	} else if(str_begin_with(buf, "PASS") == 0) {
		ret = do_pass(session, parameter_ptr);
	} else if(str_begin_with(buf, "LIST") == 0  ) {
		ret = do_list(session, parameter_ptr);
	} else if(str_begin_with(buf, "NLST") == 0 ) {
		ret = do_nlst(session, parameter_ptr);
	} else if(str_begin_with(buf, "PWD") == 0 || str_begin_with(buf, "XPWD") == 0) {
		ret = do_pwd(session, parameter_ptr);
	} else if(str_begin_with(buf, "TYPE") == 0) {
		ret = do_type(session, parameter_ptr);
	} else if(str_begin_with(buf, "PASV") == 0) {
		ret = do_pasv(session, parameter_ptr);
	} else if (str_begin_with(buf, "RETR") == 0) {
		ret = do_retr(session, parameter_ptr);
	} else if (str_begin_with(buf, "STOR") == 0) {
		ret = do_stor(session, parameter_ptr);
	} else if(str_begin_with(buf, "SIZE") == 0) {
		ret = do_size(session, parameter_ptr);
	} else if(str_begin_with(buf, "MDTM") == 0) {
		ret = do_mdtm(session, parameter_ptr);
	} else if(str_begin_with(buf, "SYST") == 0) {
		ret = do_syst(session, parameter_ptr);
	} else if(str_begin_with(buf, "CWD") == 0) {
		ret = do_cwd(session, parameter_ptr);
	} else if(str_begin_with(buf, "CDUP") == 0) {
		ret = do_cdup(session, parameter_ptr);
	} else if(str_begin_with(buf, "PORT") == 0) {
		ret = do_port(session, parameter_ptr);
	} else if(str_begin_with(buf, "REST") == 0) {
		ret = do_rest(session, parameter_ptr);
	} else if(str_begin_with(buf, "MKD") == 0) {
		ret = do_mkd(session, parameter_ptr);
	} else if(str_begin_with(buf, "DELE") == 0) {
		ret = do_dele(session, parameter_ptr);
	} else if(str_begin_with(buf, "RMD") == 0) {
		ret = do_rmd(session, parameter_ptr);
	} else if(str_begin_with(buf, "QUIT") == 0) {
		ret = do_quit(session, parameter_ptr);
	} else {
		ret = do_un_imp(session, parameter_ptr);
	}

	return ret;
}

void start_ftpd(void *argument)
{
	osThreadDef(ftpd, ftpd_thread_entry, osPriorityNormal, 0, 128 * 2 * 2);

	osThreadCreate(osThread(ftpd), argument);
}
