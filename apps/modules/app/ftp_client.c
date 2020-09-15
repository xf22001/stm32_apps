

/*================================================================
 *
 *
 *   文件名称：ftp_client.c
 *   创 建 者：肖飞
 *   创建日期：2020年09月15日 星期二 09时32分10秒
 *   修改日期：2020年09月15日 星期二 17时40分35秒
 *   描    述：
 *
 *================================================================*/
#include "ftp_client.h"
#include <string.h>

#include "log.h"

static ftp_client_info_t *ftp_client_info = NULL;

char *get_ftp_client_state_des(ftp_client_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(FTP_CLIENT_STATE_IDLE);
			add_des_case(FTP_CLIENT_STATE_CONNECTED);

		default: {
		}
		break;
	}

	return des;
}

char *get_ftp_client_cmd_state_des(ftp_client_cmd_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(FTP_CLIENT_CMD_STATE_IDLE);
			add_des_case(FTP_CLIENT_CMD_STATE_CONNECT);
			add_des_case(FTP_CLIENT_CMD_STATE_CONNECT_CONFIRM);
			add_des_case(FTP_CLIENT_CMD_STATE_DISCONNECT);
			add_des_case(FTP_CLIENT_CMD_STATE_SUSPEND);

		default: {
		}
		break;
	}

	return des;
}

char *get_ftp_client_data_state_des(ftp_client_data_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(FTP_CLIENT_DATA_STATE_IDLE);
			add_des_case(FTP_CLIENT_DATA_STATE_SUSPEND);

		default: {
		}
		break;
	}

	return des;
}

static void set_ftp_client_server_info(ftp_client_info_t *ftp_client_info, const char *host, const char *port, const char *path, const char *user, const char *password)
{
	osStatus status;

	status = osMutexWait(ftp_client_info->ftp_server_path.mutex, osWaitForever);

	if(status != osOK) {
	}

	snprintf(ftp_client_info->ftp_server_path.host, sizeof(ftp_client_info->ftp_server_path.host), "%s", host);
	snprintf(ftp_client_info->ftp_server_path.port, sizeof(ftp_client_info->ftp_server_path.port), "%s", port);
	snprintf(ftp_client_info->ftp_server_path.path, sizeof(ftp_client_info->ftp_server_path.path), "%s", path);
	snprintf(ftp_client_info->ftp_server_path.user, sizeof(ftp_client_info->ftp_server_path.user), "%s", user);
	snprintf(ftp_client_info->ftp_server_path.password, sizeof(ftp_client_info->ftp_server_path.password), "%s", password);

	status = osMutexRelease(ftp_client_info->ftp_server_path.mutex);

	if(status != osOK) {
	}
}

static void get_ftp_client_cmd_addr_info(ftp_client_info_t *ftp_client_info)
{
	int ret;
	osStatus status;
	struct list_head *list_head;

	list_head = &ftp_client_info->cmd.addr_info.socket_addr_info_list;
	ftp_client_info->cmd.addr_info.socket_addr_info = NULL;

	status = osMutexWait(ftp_client_info->ftp_server_path.mutex, osWaitForever);

	if(status != osOK) {
	}

	ret = update_addr_info_list(list_head,
	                            ftp_client_info->ftp_server_path.host,
	                            ftp_client_info->ftp_server_path.port,
	                            SOCK_STREAM,
	                            IPPROTO_TCP);

	status = osMutexRelease(ftp_client_info->ftp_server_path.mutex);

	if(status != osOK) {
	}

	if(ret == 0) {
		ftp_client_info->cmd.addr_info.socket_addr_info = get_next_socket_addr_info(list_head, ftp_client_info->cmd.addr_info.socket_addr_info);
	}
}

int request_ftp_client_connect(const char *host, const char *port, const char *path, const char *user, const char *password)
{
	int ret = -1;

	if(ftp_client_info == NULL) {
		return ret;
	}

	set_ftp_client_server_info(ftp_client_info, host, port, path, user, password);

	ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_CONNECT;
	debug("ftp_client_info %p\n", ftp_client_info);

	return ret;
}

static int ftp_client_connect(socket_addr_info_t *socket_addr_info, int *sock_fd)
{
	int ret = -1;
	int flags = 0;

	*sock_fd = -1;

	if(socket_addr_info == NULL) {
		debug("\n");
		return ret;
	}

	*sock_fd = socket(socket_addr_info->ai_family, socket_addr_info->ai_socktype, socket_addr_info->ai_protocol);

	if(*sock_fd == -1) {
		debug("\n");
		return ret;
	}

	debug("create socket %d\n", *sock_fd);

	flags = fcntl(*sock_fd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(*sock_fd, F_SETFL, flags);

	ret = connect(*sock_fd, (struct sockaddr *)&socket_addr_info->addr, socket_addr_info->addr_size);

	if(ret != 0) {
		if(errno != EINPROGRESS) {
			debug("close socket %d(%d)\n", *sock_fd, errno);
			close(*sock_fd);
			*sock_fd = -1;
		} else {
			ret = 0;
		}
	}

	return ret;
}

static int ftp_client_close(int *sock_fd)
{
	int ret = -1;

	if(*sock_fd != -1) {
		debug("close socket %d\n", *sock_fd);
		close(*sock_fd);
		ret = 0;
	}

	*sock_fd = -1;

	return ret;
}

static int ftp_client_send_data(int fd, char *buffer, size_t len)
{
	int ret = -1;
	int sent;

	for(sent = 0; sent < len;) {
		if(poll_loop_wait_send(fd, 100) == 0) {
			ret = send(fd, buffer + sent, len - sent, 0);

			if(ret >= 0) {
				_hexdump("sent", buffer + sent, len - sent);
				sent += ret;
				ret = 0;
			} else {
				debug("send error %d(%s)!\n", ret, strerror(errno));
				break;
			}
		} else {
		}
	}

	return ret;
}

static void ftp_client_cmd_handler(void *ctx)
{
	poll_ctx_t *poll_ctx_cmd = (poll_ctx_t *)ctx;
	ftp_client_info_t *ftp_client_info = (ftp_client_info_t *)poll_ctx_cmd->priv;
	int ret;

	switch(ftp_client_info->state) {
		case FTP_CLIENT_STATE_CONNECTED: {
			ret = recv(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.rx_buffer, sizeof(ftp_client_info->cmd.rx_buffer), 0);

			if(ret > 0) {
				ftp_client_info->cmd.rx_size = ret;
				_hexdump("recv", ftp_client_info->cmd.rx_buffer, ftp_client_info->cmd.rx_size);
			} else {
				debug("recv error %d, (%s)\n", ret, strerror(errno));
				ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
			}
		}
		break;

		default: {
		}
		break;
	}

	switch(ftp_client_info->cmd.state) {
		case FTP_CLIENT_CMD_STATE_CONNECT_CONFIRM: {
			if(socket_connect_confirm(poll_ctx_cmd->poll_fd.fd) == 0) {
				poll_ctx_cmd->poll_fd.config.s.poll_out = 0;
				poll_ctx_cmd->poll_fd.config.s.poll_in = 1;
				ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_WAIT_HELLO;
				ftp_client_info->state = FTP_CLIENT_STATE_CONNECTED;
			} else {
				ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void ftp_client_cmd_periodic(void *ctx)
{
	int ret;
	poll_ctx_t *poll_ctx_cmd = (poll_ctx_t *)ctx;
	ftp_client_info_t *ftp_client_info = (ftp_client_info_t *)poll_ctx_cmd->priv;
	uint32_t ticks = osKernelSysTick();

	switch(ftp_client_info->state) {
		default: {
		}
		break;
	}

	switch(ftp_client_info->cmd.state) {
		case FTP_CLIENT_CMD_STATE_CONNECT: {
			debug("FTP_CLIENT_CMD_STATE_CONNECT\n");
			get_ftp_client_cmd_addr_info(ftp_client_info);
			ret = ftp_client_connect(ftp_client_info->cmd.addr_info.socket_addr_info, &ftp_client_info->cmd.sock_fd);

			if(ret == 0) {
				poll_ctx_cmd->poll_fd.fd = ftp_client_info->cmd.sock_fd;

				poll_ctx_cmd->poll_fd.config.v = 0;
				poll_ctx_cmd->poll_fd.config.s.poll_out = 1;
				poll_ctx_cmd->poll_fd.config.s.poll_err = 1;

				poll_ctx_cmd->poll_fd.available = 1;

				ftp_client_info->cmd.stamp = ticks;
				ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_CONNECT_CONFIRM;
			} else {
				ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
			}
		}
		break;

		case FTP_CLIENT_CMD_STATE_CONNECT_CONFIRM: {
			if(ticks - ftp_client_info->cmd.stamp >= 3000) {
				debug("FTP_CLIENT_CMD_STATE_CONNECT_CONFIRM timeout!\n");
				ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
			}
		}
		break;

		case FTP_CLIENT_CMD_STATE_WAIT_HELLO: {
			if(ticks - ftp_client_info->cmd.stamp >= 3000) {
				debug("FTP_CLIENT_CMD_STATE_WAIT_HELLO timeout!\n");
				ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
			} else {
				if(ftp_client_info->cmd.rx_size > 0) {
					int response_code = atoi(ftp_client_info->cmd.rx_buffer);

					ftp_client_info->cmd.rx_size = 0;

					if(response_code == 220) {//send user
						ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "USER %s\r\n", ftp_client_info->ftp_server_path.user);

						_hexdump("cmd sent", ftp_client_info->cmd.tx_buffer, ret);
						ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

						if(ret != 0) {
							ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
						} else {
							ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_WAIT_USER_RESPONSE;
						}
					}

				}
			}
		}
		break;

		case FTP_CLIENT_CMD_STATE_WAIT_USER_RESPONSE: {
			if(ticks - ftp_client_info->cmd.stamp >= 3000) {
				debug("FTP_CLIENT_CMD_STATE_WAIT_USER_RESPONSE timeout!\n");
				ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
			} else {
				if(ftp_client_info->cmd.rx_size > 0) {
					int response_code = atoi(ftp_client_info->cmd.rx_buffer);

					ftp_client_info->cmd.rx_size = 0;

					if(response_code == 331) {//send passwd
						ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "PASS %s\r\n", ftp_client_info->ftp_server_path.password);

						_hexdump("cmd sent", ftp_client_info->cmd.tx_buffer, ret);
						ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

						if(ret != 0) {
							ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
						} else {
							ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_WAIT_PASSWORD_RESPONSE;
						}
					}
				}
			}
		}
		break;

		case FTP_CLIENT_CMD_STATE_WAIT_PASSWORD_RESPONSE: {
			if(ticks - ftp_client_info->cmd.stamp >= 3000) {
				debug("FTP_CLIENT_CMD_STATE_WAIT_PASSWORD_RESPONSE timeout!\n");
				ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
			} else {
				if(ftp_client_info->cmd.rx_size > 0) {
					int response_code = atoi(ftp_client_info->cmd.rx_buffer);

					ftp_client_info->cmd.rx_size = 0;

					if(response_code == 230) {//Set to binary mode
						ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "TYPE I\r\n");

						_hexdump("cmd sent", ftp_client_info->cmd.tx_buffer, ret);
						ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

						if(ret != 0) {
							ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
						} else {
							ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_WAIT_BINARY_MODE_RESPONSE;
						}
					}
				}
			}
		}
		break;

		case FTP_CLIENT_CMD_STATE_WAIT_BINARY_MODE_RESPONSE: {
			if(ticks - ftp_client_info->cmd.stamp >= 3000) {
				debug("FTP_CLIENT_CMD_STATE_WAIT_BINARY_MODE_RESPONSE timeout!\n");
				ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
			} else {
				if(ftp_client_info->cmd.rx_size > 0) {
					int response_code = atoi(ftp_client_info->cmd.rx_buffer);

					ftp_client_info->cmd.rx_size = 0;

					if(response_code == 200) {//ok
						ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "PASV\r\n");

						_hexdump("cmd sent", ftp_client_info->cmd.tx_buffer, ret);
						ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

						if(ret != 0) {
							ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
						} else {
							ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_WAIT_BINARY_MODE_RESPONSE;
						}

						ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_PASV_RESPONSE;
					}
				}
			}
		}
		break;

		case FTP_CLIENT_CMD_STATE_PASV_RESPONSE: {
			if(ticks - ftp_client_info->cmd.stamp >= 3000) {
				debug("FTP_CLIENT_CMD_STATE_PASV_RESPONSE timeout!\n");
				ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
			} else {
				if(ftp_client_info->cmd.rx_size > 0) {
					int response_code = atoi(ftp_client_info->cmd.rx_buffer);

					ftp_client_info->cmd.rx_size = 0;

					if(response_code == 227) {//ok
						int a;
						int b;
						int c;
						int d;
						int pa;
						int pb;
						char *find = strrchr(ftp_client_info->cmd.rx_buffer, '(');
						char *ip;
						char *port;

						ret = sscanf(find, "(%d,%d,%d,%d,%d,%d)", &a, &b, &c, &d, &pa, &pb);

						if(ret == 6) {
							struct list_head *list_head;
							ip = (char *)os_alloc(64);
							port = (char *)os_alloc(16);

							if((ip != NULL) && (port != NULL)) {
								snprintf(ip, 64, "%d.%d.%d.%d", a, b, c, d);
								snprintf(port, 16, "%d", pa * 256 + pb);

								list_head = &ftp_client_info->data.addr_info.socket_addr_info_list;
								ret = update_addr_info_list(list_head,
								                            ip,
								                            port,
								                            SOCK_STREAM,
								                            IPPROTO_TCP);

								if(ret == 0) {
									ftp_client_info->data.addr_info.socket_addr_info = get_next_socket_addr_info(list_head, ftp_client_info->data.addr_info.socket_addr_info);
									ftp_client_info->data.state = FTP_CLIENT_DATA_STATE_CONNECT;
									ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DATA;
								}

							}

							if(ip != NULL) {
								os_free(ip);
							}

							if(port != NULL) {
								os_free(port);
							}
						} else {
							ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_DISCONNECT;
						}
					}
				}
			}
		}
		break;

		case FTP_CLIENT_CMD_STATE_DISCONNECT: {
			poll_ctx_cmd->poll_fd.available = 0;
			ftp_client_close(&poll_ctx_cmd->poll_fd.fd);
			ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_IDLE;
			ftp_client_info->state = FTP_CLIENT_STATE_IDLE;
		}
		break;

		default: {
		}
		break;
	}
}

static void ftp_client_data_handler(void *ctx)
{
	poll_ctx_t *poll_ctx_data = (poll_ctx_t *)ctx;
	ftp_client_info_t *ftp_client_info = (ftp_client_info_t *)poll_ctx_data->priv;

	switch(ftp_client_info->state) {
		default: {
		}
		break;
	}

	switch(ftp_client_info->data.state) {
		case FTP_CLIENT_DATA_STATE_CONNECT_CONFIRM: {
			if(socket_connect_confirm(poll_ctx_data->poll_fd.fd) == 0) {
				poll_ctx_data->poll_fd.config.s.poll_out = 0;
				poll_ctx_data->poll_fd.config.s.poll_in = 1;
				ftp_client_info->data.state = FTP_CLIENT_DATA_STATE_DISCONNECT;
			} else {
				ftp_client_info->data.state = FTP_CLIENT_DATA_STATE_DISCONNECT;
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void ftp_client_data_periodic(void *ctx)
{
	int ret;
	poll_ctx_t *poll_ctx_data = (poll_ctx_t *)ctx;
	ftp_client_info_t *ftp_client_info = (ftp_client_info_t *)poll_ctx_data->priv;
	uint32_t ticks = osKernelSysTick();

	switch(ftp_client_info->state) {
		default: {
		}
		break;
	}

	switch(ftp_client_info->data.state) {
		case FTP_CLIENT_DATA_STATE_CONNECT: {
			ret = ftp_client_connect(ftp_client_info->data.addr_info.socket_addr_info, &ftp_client_info->data.sock_fd);

			if(ret == 0) {
				poll_ctx_data->poll_fd.fd = ftp_client_info->data.sock_fd;

				poll_ctx_data->poll_fd.config.v = 0;
				poll_ctx_data->poll_fd.config.s.poll_out = 1;
				poll_ctx_data->poll_fd.config.s.poll_err = 1;

				poll_ctx_data->poll_fd.available = 1;

				ftp_client_info->data.stamp = ticks;
				ftp_client_info->data.state = FTP_CLIENT_DATA_STATE_CONNECT_CONFIRM;
			} else {
				ftp_client_info->data.state = FTP_CLIENT_DATA_STATE_DISCONNECT;
			}
		}
		break;

		case FTP_CLIENT_DATA_STATE_CONNECT_CONFIRM: {
			if(ticks - ftp_client_info->data.stamp >= 3000) {
				debug("FTP_CLIENT_DATA_STATE_CONNECT_CONFIRM timeout!\n");
				ftp_client_info->data.state = FTP_CLIENT_DATA_STATE_DISCONNECT;
			}
		}
		break;

		case FTP_CLIENT_DATA_STATE_DISCONNECT: {
			poll_ctx_data->poll_fd.available = 0;
			ftp_client_close(&poll_ctx_data->poll_fd.fd);

			ftp_client_info->data.state = FTP_CLIENT_CMD_STATE_IDLE;
			ftp_client_info->state = FTP_CLIENT_STATE_IDLE;
		}
		break;

		default: {
		}
		break;
	}
}

void ftp_client_add_poll_loop(poll_loop_t *poll_loop)
{
	poll_ctx_t *poll_ctx_cmd;
	poll_ctx_t *poll_ctx_data;

	if(ftp_client_info != NULL) {
		debug("\n");
		app_panic();
	}

	ftp_client_info = (ftp_client_info_t *)os_alloc(sizeof(ftp_client_info_t));

	if(ftp_client_info == NULL) {
		debug("\n");
		app_panic();
	}

	memset(ftp_client_info, 0, sizeof(ftp_client_info_t));

	ftp_client_info->state = FTP_CLIENT_STATE_IDLE;
	ftp_client_info->cmd.state = FTP_CLIENT_CMD_STATE_IDLE;
	INIT_LIST_HEAD(&ftp_client_info->cmd.addr_info.socket_addr_info_list);
	ftp_client_info->data.state = FTP_CLIENT_DATA_STATE_IDLE;
	INIT_LIST_HEAD(&ftp_client_info->data.addr_info.socket_addr_info_list);
	ftp_client_info->cmd.sock_fd = -1;
	ftp_client_info->data.sock_fd = -1;

	{
		osMutexDef(mutex);
		ftp_client_info->ftp_server_path.mutex = osMutexCreate(osMutex(mutex));

		if(ftp_client_info->ftp_server_path.mutex == NULL) {
			app_panic();
		}
	}

	//add ftp client cmd poll ctx
	poll_ctx_cmd = alloc_poll_ctx();

	if(poll_ctx_cmd == NULL) {
		debug("\n");
		app_panic();
	}

	poll_ctx_cmd->priv = ftp_client_info;
	poll_ctx_cmd->name = "ftp_client_cmd";
	poll_ctx_cmd->poll_handler = ftp_client_cmd_handler;
	poll_ctx_cmd->poll_periodic = ftp_client_cmd_periodic;

	add_poll_loop_ctx_item(poll_loop, poll_ctx_cmd);


	//add ftp client data poll ctx
	poll_ctx_data = alloc_poll_ctx();

	if(poll_ctx_data == NULL) {
		debug("\n");
		app_panic();
	}

	poll_ctx_data->priv = ftp_client_info;
	poll_ctx_data->name = "ftp_client_data";
	poll_ctx_data->poll_handler = ftp_client_data_handler;
	poll_ctx_data->poll_periodic = ftp_client_data_periodic;

	add_poll_loop_ctx_item(poll_loop, poll_ctx_data);
}
