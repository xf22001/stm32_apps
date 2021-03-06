

/*================================================================
 *
 *
 *   文件名称：ftp_client.c
 *   创 建 者：肖飞
 *   创建日期：2020年09月15日 星期二 09时32分10秒
 *   修改日期：2021年05月27日 星期四 10时36分32秒
 *   描    述：
 *
 *================================================================*/
#include "ftp_client.h"

#include <string.h>

#include "sal_hook.h"

#include "log.h"

#define FTP_SESSION_TIMEOUT (30 * 1000)

static ftp_client_info_t *ftp_client_info = NULL;

char *get_ftp_client_action_des(ftp_client_action_t action)
{
	char *des = "unknow";

	switch(action) {
			add_des_case(FTP_CLIENT_ACTION_IDLE);
			add_des_case(FTP_CLIENT_ACTION_DOWNLOAD);
			add_des_case(FTP_CLIENT_ACTION_UPLOAD);

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
			add_des_case(FTP_CLIENT_CMD_STATE_CONNECTED);
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
			add_des_case(FTP_CLIENT_DATA_STATE_CONNECT);
			add_des_case(FTP_CLIENT_DATA_STATE_CONNECT_CONFIRM);
			add_des_case(FTP_CLIENT_DATA_STATE_CONNECTED);
			add_des_case(FTP_CLIENT_DATA_STATE_DISCONNECT);
			add_des_case(FTP_CLIENT_DATA_STATE_SUSPEND);

		default: {
		}
		break;
	}

	return des;
}

static ftp_client_action_t get_ftp_client_action(ftp_client_info_t *ftp_client_info)
{
	return ftp_client_info->action;
}

static void set_ftp_client_action(ftp_client_info_t *ftp_client_info, ftp_client_action_t action)
{
	debug("%s -> %s", get_ftp_client_action_des(ftp_client_info->action), get_ftp_client_action_des(action));
	ftp_client_info->action = action;
}

static ftp_client_cmd_state_t get_ftp_client_cmd_state(ftp_client_info_t *ftp_client_info)
{
	return ftp_client_info->cmd.state;
}

static void set_ftp_client_cmd_state(ftp_client_info_t *ftp_client_info, ftp_client_cmd_state_t state)
{
	debug("%s -> %s", get_ftp_client_cmd_state_des(ftp_client_info->cmd.state), get_ftp_client_cmd_state_des(state));
	ftp_client_info->cmd.state = state;
}

static ftp_client_cmd_state_t get_ftp_client_cmd_request_state(ftp_client_info_t *ftp_client_info)
{
	return ftp_client_info->cmd.request_state;
}

static void set_ftp_client_cmd_request_state(ftp_client_info_t *ftp_client_info, ftp_client_cmd_state_t state)
{
	//debug("%s -> %s", get_ftp_client_cmd_state_des(ftp_client_info->cmd.request_state), get_ftp_client_cmd_state_des(state));
	ftp_client_info->cmd.request_state = state;
}

static ftp_client_data_state_t get_ftp_client_data_state(ftp_client_info_t *ftp_client_info)
{
	return ftp_client_info->data.state;
}

static void set_ftp_client_data_state(ftp_client_info_t *ftp_client_info, ftp_client_data_state_t state)
{
	debug("%s -> %s", get_ftp_client_data_state_des(ftp_client_info->data.state), get_ftp_client_data_state_des(state));
	ftp_client_info->data.state = state;
}

static ftp_client_data_state_t get_ftp_client_data_request_state(ftp_client_info_t *ftp_client_info)
{
	return ftp_client_info->data.request_state;
}

static void set_ftp_client_data_request_state(ftp_client_info_t *ftp_client_info, ftp_client_data_state_t state)
{
	//debug("%s -> %s", get_ftp_client_data_state_des(ftp_client_info->data.request_state), get_ftp_client_data_state_des(state));
	ftp_client_info->data.request_state = state;
}

static void set_ftp_client_server_info(ftp_client_info_t *ftp_client_info, const char *host, const char *port, const char *path, const char *user, const char *password)
{
	mutex_lock(ftp_client_info->ftp_server_path.mutex);

	snprintf(ftp_client_info->ftp_server_path.host, sizeof(ftp_client_info->ftp_server_path.host), "%s", host);
	snprintf(ftp_client_info->ftp_server_path.port, sizeof(ftp_client_info->ftp_server_path.port), "%s", port);
	snprintf(ftp_client_info->ftp_server_path.path, sizeof(ftp_client_info->ftp_server_path.path), "%s", path);
	snprintf(ftp_client_info->ftp_server_path.user, sizeof(ftp_client_info->ftp_server_path.user), "%s", user);
	snprintf(ftp_client_info->ftp_server_path.password, sizeof(ftp_client_info->ftp_server_path.password), "%s", password);

	mutex_unlock(ftp_client_info->ftp_server_path.mutex);
}

static void get_ftp_client_cmd_addr_info(ftp_client_info_t *ftp_client_info)
{
	int ret;
	struct list_head *list_head;

	list_head = &ftp_client_info->cmd.addr_info.socket_addr_info_list;
	ftp_client_info->cmd.addr_info.socket_addr_info = NULL;

	mutex_lock(ftp_client_info->ftp_server_path.mutex);

	ret = update_addr_info_list(list_head,
	                            ftp_client_info->ftp_server_path.host,
	                            ftp_client_info->ftp_server_path.port,
	                            SOCK_STREAM,
	                            IPPROTO_TCP);

	mutex_unlock(ftp_client_info->ftp_server_path.mutex);

	if(ret == 0) {
		ftp_client_info->cmd.addr_info.socket_addr_info = get_next_socket_addr_info(list_head, ftp_client_info->cmd.addr_info.socket_addr_info);
	}
}

int request_ftp_client_download(const char *host, const char *port, const char *path, const char *user, const char *password, ftp_client_action_t action, void *fn_ctx, ftp_download_callback_t callback)
{
	int ret = -1;

	if(ftp_client_info == NULL) {
		return ret;
	}

	if(get_ftp_client_action(ftp_client_info) != FTP_CLIENT_ACTION_IDLE) {
		return ret;
	}

	set_ftp_client_action(ftp_client_info, action);

	ftp_client_info->ftp_data_callback_item.fn = callback;
	ftp_client_info->ftp_data_callback_item.fn_ctx = fn_ctx;
	OS_ASSERT(register_callback(ftp_client_info->ftp_data_event_chain, &ftp_client_info->ftp_data_callback_item) == 0);

	set_ftp_client_server_info(ftp_client_info, host, port, path, user, password);
	set_ftp_client_cmd_request_state(ftp_client_info, FTP_CLIENT_CMD_STATE_CONNECT);
	ret = 0;

	return ret;
}

static int ftp_client_connect(socket_addr_info_t *socket_addr_info, int *sock_fd)
{
	int ret = -1;

	ret = socket_nonblock_connect(socket_addr_info, sock_fd);
	debug("ret:%d", ret);

	return ret;
}

static int ftp_client_close(int *sock_fd)
{
	int ret = -1;

	if(*sock_fd != -1) {
		debug("close socket %d", *sock_fd);
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
		if(poll_wait_write_available(fd, 100) == 0) {
			ret = send(fd, buffer + sent, len - sent, 0);

			if(ret >= 0) {
				_hexdump("sent", buffer + sent, len - sent);
				sent += ret;
				ret = 0;
			} else {
				debug("send error %d(%s)!", ret, strerror(errno));
				break;
			}
		} else {
		}
	}

	return ret;
}

static void ftp_client_cmd_download(void *ctx)
{
	int ret;
	poll_ctx_t *poll_ctx_cmd = (poll_ctx_t *)ctx;
	ftp_client_info_t *ftp_client_info = (ftp_client_info_t *)poll_ctx_cmd->priv;
	uint32_t ticks = osKernelSysTick();

	switch(ftp_client_info->cmd.action_state) {
		case 0: {
			ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "REST 0\r\n");
			ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

			if(ret != 0) {
				debug("");
				set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
			} else {
				ftp_client_info->cmd.action_state = 1;
			}

			ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "SIZE %s\r\n", ftp_client_info->ftp_server_path.path);
			ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

			if(ret != 0) {
				debug("");
				set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
			} else {
				ftp_client_info->cmd.action_state = 1;
			}
		}
		break;

		case 1: {
			int response_code = atoi(ftp_client_info->cmd.rx_buffer);

			if(response_code == 213) {
				int code;
				int size;

				ret = sscanf(ftp_client_info->cmd.rx_buffer, "%d %d", &code, &size);

				if(ret == 2) {
					ftp_client_info->file_size = size;
					debug("ftp_client_info->file_size:%d", ftp_client_info->file_size);

					if(ftp_client_info->ftp_server_path.rest_enable == 1) {
						ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "REST %lu\r\n", ftp_client_info->data_size);
						ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

						if(ret != 0) {
							debug("");
							set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
						} else {
							ftp_client_info->cmd.action_state = 2;
						}
					}

					ftp_client_info->data_stamp = ticks;

					ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "RETR %s\r\n", ftp_client_info->ftp_server_path.path);
					ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

					if(ret != 0) {
						debug("");
						set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
					} else {
						ftp_client_info->cmd.action_state = 2;
					}
				} else {
					debug("");
					set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
				}
			} else if(response_code == 350) {
				ftp_client_info->ftp_server_path.rest_enable = 1;
			}
		}
		break;

		case 2: {
			int response_code = atoi(ftp_client_info->cmd.rx_buffer);

			if(response_code == 350) {
				ftp_client_info->ftp_server_path.rest_enable = 1;
				debug("start download ...");
				set_ftp_client_data_request_state(ftp_client_info, FTP_CLIENT_DATA_STATE_CONNECT);
				ftp_client_info->cmd.action_state = 3;
			} else if(response_code == 150) {
				debug("start download ...");
				set_ftp_client_data_request_state(ftp_client_info, FTP_CLIENT_DATA_STATE_CONNECT);
				ftp_client_info->cmd.action_state = 3;
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void ftp_client_cmd_upload(void *ctx)
{
	int ret;
	poll_ctx_t *poll_ctx_cmd = (poll_ctx_t *)ctx;
	ftp_client_info_t *ftp_client_info = (ftp_client_info_t *)poll_ctx_cmd->priv;
	uint32_t ticks = osKernelSysTick();

	switch(ftp_client_info->cmd.action_state) {
		case 0: {
			ftp_client_info->data_stamp = ticks;
			ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "STOR %s\r\n", ftp_client_info->ftp_server_path.path);
			ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

			if(ret != 0) {
				debug("");
				set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
			} else {
				ftp_client_info->cmd.action_state = 1;
			}
		}
		break;

		case 1: {
			int response_code = atoi(ftp_client_info->cmd.rx_buffer);

			if(response_code == 150) {
				debug("start upload ...");
				ftp_client_info->data_ready = 1;
				set_ftp_client_data_request_state(ftp_client_info, FTP_CLIENT_DATA_STATE_CONNECT);
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void ftp_client_cmd_pasv(void *ctx)
{
	int ret;
	poll_ctx_t *poll_ctx_cmd = (poll_ctx_t *)ctx;
	ftp_client_info_t *ftp_client_info = (ftp_client_info_t *)poll_ctx_cmd->priv;

	switch(ftp_client_info->cmd.action_state) {
		case 0: {
			ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "PASV\r\n");
			ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

			if(ret != 0) {
				debug("");
				set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
			} else {
				ftp_client_info->cmd.action_state = 1;
			}
		}
		break;

		case 1: {
			int response_code = atoi(ftp_client_info->cmd.rx_buffer);

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

						ftp_client_info->data.addr_info.socket_addr_info = NULL;
						list_head = &ftp_client_info->data.addr_info.socket_addr_info_list;
						ret = update_addr_info_list(list_head,
						                            ip,
						                            port,
						                            SOCK_STREAM,
						                            IPPROTO_TCP);

						if(ret == 0) {
							ftp_client_info->data.addr_info.socket_addr_info = get_next_socket_addr_info(list_head, ftp_client_info->data.addr_info.socket_addr_info);
							ftp_client_info->cmd.action_state = 2;

							switch(get_ftp_client_action(ftp_client_info)) {
								case FTP_CLIENT_ACTION_DOWNLOAD: {
									ftp_client_info->cmd.action_state = 0;
									ftp_client_info->data_size = 0;
									ftp_client_info->data.tx_size = 0;
									ftp_client_info->data.rx_size = 0;
									ftp_client_info->ftp_server_path.rest_enable = 0;
									ftp_client_info->ftp_server_path.retry = 0;
									ftp_client_info->cmd.handler = ftp_client_cmd_download;
									ftp_client_info->cmd.handler(ctx);
								}
								break;

								case FTP_CLIENT_ACTION_UPLOAD: {
									ftp_client_info->cmd.action_state = 0;
									ftp_client_info->data_size = 0;
									ftp_client_info->data.tx_size = 0;
									ftp_client_info->data.rx_size = 0;
									ftp_client_info->data_ready = 0;
									ftp_client_info->file_size = 0xffffffff;
									ftp_client_info->cmd.handler = ftp_client_cmd_upload;
									ftp_client_info->cmd.handler(ctx);
								}
								break;

								default: {
								}
								break;
							}
						} else {
							debug("");
							set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
						}

					} else {
						debug("");
						set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
					}

					if(ip != NULL) {
						os_free(ip);
					}

					if(port != NULL) {
						os_free(port);
					}
				} else {
					debug("");
					set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
				}
			}
		}
		break;

		default: {
		}
		break;
	}
}


static void ftp_client_cmd_login(void *ctx)
{
	int ret;
	poll_ctx_t *poll_ctx_cmd = (poll_ctx_t *)ctx;
	ftp_client_info_t *ftp_client_info = (ftp_client_info_t *)poll_ctx_cmd->priv;

	switch(ftp_client_info->cmd.action_state) {
		case 0: {
			int response_code = atoi(ftp_client_info->cmd.rx_buffer);

			if(response_code == 220) {//send user
				ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "USER %s\r\n", ftp_client_info->ftp_server_path.user);
				ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

				if(ret != 0) {
					debug("");
					set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
				} else {
					ftp_client_info->cmd.action_state = 1;
				}
			}
		}
		break;

		case 1: {
			int response_code = atoi(ftp_client_info->cmd.rx_buffer);

			if(response_code == 331) {//send passwd
				ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "PASS %s\r\n", ftp_client_info->ftp_server_path.password);
				ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

				if(ret != 0) {
					debug("");
					set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
				} else {
					ftp_client_info->cmd.action_state = 2;
				}
			} else if(response_code == 230) {
				ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "TYPE I\r\n");
				ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

				if(ret != 0) {
					debug("");
					set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
				} else {
					ftp_client_info->cmd.action_state = 3;
				}
			}
		}
		break;

		case 2: {
			int response_code = atoi(ftp_client_info->cmd.rx_buffer);

			if(response_code == 230) {//Set to binary mode
				ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "TYPE I\r\n");
				ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

				if(ret != 0) {
					debug("");
					set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
				} else {
					ftp_client_info->cmd.action_state = 3;
				}
			}
		}
		break;

		case 3: {
			int response_code = atoi(ftp_client_info->cmd.rx_buffer);

			if(response_code == 200) {//ok
				switch(get_ftp_client_action(ftp_client_info)) {
					case FTP_CLIENT_ACTION_DOWNLOAD:
					case FTP_CLIENT_ACTION_UPLOAD: {
						ftp_client_info->cmd.action_state = 0;
						ftp_client_info->cmd.handler = ftp_client_cmd_pasv;
						ftp_client_info->cmd.handler(ctx);
					}
					break;

					default: {
					}
					break;
				}
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void ftp_client_cmd_handler(void *ctx)
{
	poll_ctx_t *poll_ctx_cmd = (poll_ctx_t *)ctx;
	ftp_client_info_t *ftp_client_info = (ftp_client_info_t *)poll_ctx_cmd->priv;
	uint32_t ticks = osKernelSysTick();
	int ret;

	//debug("poll_ctx_cmd->poll_fd.fd:%d", poll_ctx_cmd->poll_fd.fd);
	//debug("poll_ctx_cmd->poll_fd.status.s.poll_in:%d", poll_ctx_cmd->poll_fd.status.s.poll_in);
	//debug("poll_ctx_cmd->poll_fd.status.s.poll_out:%d", poll_ctx_cmd->poll_fd.status.s.poll_out);
	//debug("poll_ctx_cmd->poll_fd.status.s.poll_err:%d", poll_ctx_cmd->poll_fd.status.s.poll_err);

	switch(get_ftp_client_cmd_state(ftp_client_info)) {
		case FTP_CLIENT_CMD_STATE_CONNECT_CONFIRM: {
			if(socket_nonblock_connect_confirm(poll_ctx_cmd->poll_fd.fd) == 0) {
				poll_ctx_cmd->poll_fd.config.v = 0;
				poll_ctx_cmd->poll_fd.config.s.poll_in = 1;

				ftp_client_info->cmd.action_state = 0;
				ftp_client_info->cmd.handler = ftp_client_cmd_login;

				set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_CONNECTED);
			} else {
				debug("");
				set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
			}
		}
		break;

		case FTP_CLIENT_CMD_STATE_CONNECTED: {
			ret = recv(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.rx_buffer + ftp_client_info->cmd.rx_size, sizeof(ftp_client_info->cmd.rx_buffer) - ftp_client_info->cmd.rx_size - 1, 0);

			if(ret > 0) {
				char *find;

				ftp_client_info->cmd.rx_size += ret;
				ftp_client_info->cmd.rx_buffer[ftp_client_info->cmd.rx_size] = 0;

				//debug("ftp_client_info->cmd.rx_size:%d", ftp_client_info->cmd.rx_size);
				_hexdump("cmd recv", ftp_client_info->cmd.rx_buffer, ftp_client_info->cmd.rx_size);

				while(ftp_client_info->cmd.rx_size != 0) {
					//_hexdump("process", ftp_client_info->cmd.rx_buffer, ftp_client_info->cmd.rx_size);
					ftp_client_info->cmd.handler(ctx);
					ftp_client_info->stamp = ticks;

					find = strchr(ftp_client_info->cmd.rx_buffer, '\n');

					if(find != NULL) {
						find++;
						ftp_client_info->cmd.rx_size -= (find - ftp_client_info->cmd.rx_buffer);

						//debug("ftp_client_info->cmd.rx_size:%d", ftp_client_info->cmd.rx_size);

						if(ftp_client_info->cmd.rx_size != 0) {
							memcpy(ftp_client_info->cmd.rx_buffer, find, ftp_client_info->cmd.rx_size);
							ftp_client_info->cmd.rx_buffer[ftp_client_info->cmd.rx_size] = 0;
						} else {
							find = NULL;
						}
					} else {
						ftp_client_info->cmd.rx_size = 0;
					}

				}
			} else {
				debug("recv error %d, (%s)", ret, strerror(errno));
				set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
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

	switch(get_ftp_client_cmd_state(ftp_client_info)) {
		case FTP_CLIENT_CMD_STATE_IDLE: {
			switch(get_ftp_client_cmd_request_state(ftp_client_info)) {
				case FTP_CLIENT_CMD_STATE_CONNECT: {
					set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_CONNECT);
				}
				break;

				default: {
				}
				break;
			}

			set_ftp_client_cmd_request_state(ftp_client_info, FTP_CLIENT_CMD_STATE_IDLE);
		}
		break;

		case FTP_CLIENT_CMD_STATE_CONNECT: {
			get_ftp_client_cmd_addr_info(ftp_client_info);
			ret = ftp_client_connect(ftp_client_info->cmd.addr_info.socket_addr_info, &ftp_client_info->cmd.sock_fd);

			if(ret == 0) {
				poll_ctx_cmd->poll_fd.fd = ftp_client_info->cmd.sock_fd;

				poll_ctx_cmd->poll_fd.config.v = 0;
				poll_ctx_cmd->poll_fd.config.s.poll_out = 1;
				poll_ctx_cmd->poll_fd.config.s.poll_err = 1;

				poll_ctx_cmd->poll_fd.available = 1;

				ftp_client_info->stamp = ticks;
				set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_CONNECT_CONFIRM);
			} else {
				set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
			}
		}
		break;

		case FTP_CLIENT_CMD_STATE_CONNECT_CONFIRM: {
			if(ticks_duration(ticks, ftp_client_info->stamp) >= FTP_SESSION_TIMEOUT) {
				switch(get_ftp_client_action(ftp_client_info)) {
					case FTP_CLIENT_ACTION_DOWNLOAD: {
						if(ftp_client_info->ftp_server_path.rest_enable == 1) {
							ftp_client_info->ftp_server_path.retry++;

							if(ftp_client_info->ftp_server_path.retry >= 10) {
								debug("FTP_CLIENT_CMD_STATE_CONNECT_CONFIRM timeout!");
								set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
							} else {
								ftp_client_info->stamp = ticks;
								ret = snprintf(ftp_client_info->cmd.tx_buffer, sizeof(ftp_client_info->cmd.tx_buffer), "REST %lu\r\n", ftp_client_info->data_size);
								ret = ftp_client_send_data(poll_ctx_cmd->poll_fd.fd, ftp_client_info->cmd.tx_buffer, ret);

								if(ret != 0) {
									debug("");
									set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
								}
							}
						} else {
							debug("FTP_CLIENT_CMD_STATE_CONNECT_CONFIRM timeout!");
							set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
						}
					}
					break;

					case FTP_CLIENT_ACTION_UPLOAD: {
						debug("FTP_CLIENT_CMD_STATE_CONNECT_CONFIRM timeout!");
						set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
					}
					break;

					default: {
						debug("FTP_CLIENT_CMD_STATE_CONNECT_CONFIRM timeout!");
						set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
					}
					break;
				}
			}
		}
		break;

		case FTP_CLIENT_CMD_STATE_CONNECTED: {
			if(ticks_duration(ticks, ftp_client_info->stamp) >= FTP_SESSION_TIMEOUT) {
				debug("FTP_CLIENT_CMD_STATE_CONNECTED timeout!");
				set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
			} else {
				switch(get_ftp_client_cmd_request_state(ftp_client_info)) {
					case FTP_CLIENT_CMD_STATE_DISCONNECT: {
						debug("");
						set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
					}
					break;

					default: {
					}
					break;
				}

				set_ftp_client_cmd_request_state(ftp_client_info, FTP_CLIENT_CMD_STATE_IDLE);
			}
		}
		break;

		case FTP_CLIENT_CMD_STATE_DISCONNECT: {
			poll_ctx_cmd->poll_fd.available = 0;
			ftp_client_close(&poll_ctx_cmd->poll_fd.fd);
			set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_IDLE);

			if(get_ftp_client_data_state(ftp_client_info) != FTP_CLIENT_DATA_STATE_IDLE) {
				set_ftp_client_data_request_state(ftp_client_info, FTP_CLIENT_DATA_STATE_DISCONNECT);
			} else {
				OS_ASSERT(remove_callback(ftp_client_info->ftp_data_event_chain, &ftp_client_info->ftp_data_callback_item) == 0);
				set_ftp_client_action(ftp_client_info, FTP_CLIENT_ACTION_IDLE);
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void ftp_client_data_download(void *ctx)
{
	poll_ctx_t *poll_ctx_data = (poll_ctx_t *)ctx;
	ftp_client_info_t *ftp_client_info = (ftp_client_info_t *)poll_ctx_data->priv;
	uint32_t ticks = osKernelSysTick();

	switch(ftp_client_info->data.action_state) {
		case 0: {
			do_callback_chain(ftp_client_info->ftp_data_event_chain, ftp_client_info);
			ftp_client_info->data_size += ftp_client_info->data.rx_size;

			if((ticks_duration(ticks, ftp_client_info->debug_stamp) >= 1 * 1000) ||
			   (ftp_client_info->file_size == ftp_client_info->data_size)) {
				uint32_t duration = (ticks - ftp_client_info->data_stamp);

				float percent = (ftp_client_info->data_size * 1.0 / ftp_client_info->file_size) * 100;
				float speed;

				ftp_client_info->debug_stamp = ticks;

				if(duration == 0) {
					duration = 1;
				}

				duration = duration * 1000;

				speed = ftp_client_info->data_size * 1.0 / duration;

				debug("downloading %s %d/%d(%.2f%%), %.6f Mbyte/S",
				      ftp_client_info->ftp_server_path.path,
				      ftp_client_info->data_size,
				      ftp_client_info->file_size,
				      percent,
				      speed);
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void ftp_client_data_upload(void *ctx)
{
	poll_ctx_t *poll_ctx_data = (poll_ctx_t *)ctx;
	ftp_client_info_t *ftp_client_info = (ftp_client_info_t *)poll_ctx_data->priv;
	uint32_t ticks = osKernelSysTick();

	switch(ftp_client_info->data.action_state) {
		case 0: {
			if(ftp_client_info->data_ready == 1) {
				ftp_client_info->data.action_state = 1;
			}
		}
		break;

		case 1: {
			do_callback_chain(ftp_client_info->ftp_data_event_chain, ftp_client_info);

			if(ftp_client_info->data.tx_size + ftp_client_info->data_size > ftp_client_info->file_size) {
				ftp_client_info->data.tx_size = ftp_client_info->file_size - ftp_client_info->data_size;
			}

			ftp_client_info->data_size += ftp_client_info->data.tx_size;

			if((ticks_duration(ticks, ftp_client_info->debug_stamp) >= 1 * 1000) ||
			   (ftp_client_info->file_size == ftp_client_info->data_size)) {
				uint32_t duration = (ticks - ftp_client_info->data_stamp);

				float percent = (ftp_client_info->data_size * 1.0 / ftp_client_info->file_size) * 100;
				float speed;

				ftp_client_info->debug_stamp = ticks;

				if(duration == 0) {
					duration = 1;
				}

				duration = duration * 1000;

				speed = ftp_client_info->data_size * 1.0 / duration;

				debug("uploading %s %d/%d(%.2f%%), %.6f Mbyte/S",
				      ftp_client_info->ftp_server_path.path,
				      ftp_client_info->data_size,
				      ftp_client_info->file_size,
				      percent,
				      speed);
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void ftp_client_data_handler(void *ctx)
{
	int ret;
	poll_ctx_t *poll_ctx_data = (poll_ctx_t *)ctx;
	ftp_client_info_t *ftp_client_info = (ftp_client_info_t *)poll_ctx_data->priv;
	uint32_t ticks = osKernelSysTick();

	//debug("poll_ctx_data->poll_fd.fd:%d", poll_ctx_data->poll_fd.fd);
	//debug("poll_ctx_data->poll_fd.status.s.poll_in:%d", poll_ctx_data->poll_fd.status.s.poll_in);
	//debug("poll_ctx_data->poll_fd.status.s.poll_out:%d", poll_ctx_data->poll_fd.status.s.poll_out);
	//debug("poll_ctx_data->poll_fd.status.s.poll_err:%d", poll_ctx_data->poll_fd.status.s.poll_err);

	switch(get_ftp_client_data_state(ftp_client_info)) {
		case FTP_CLIENT_DATA_STATE_CONNECT_CONFIRM: {
			if(socket_nonblock_connect_confirm(poll_ctx_data->poll_fd.fd) == 0) {
				switch(get_ftp_client_action(ftp_client_info)) {
					case FTP_CLIENT_ACTION_DOWNLOAD: {
						poll_ctx_data->poll_fd.config.v = 0;
						poll_ctx_data->poll_fd.config.s.poll_in = 1;
						poll_ctx_data->poll_fd.config.s.poll_err = 1;

						ftp_client_info->data.action_state = 0;
						ftp_client_info->data.handler = ftp_client_data_download;
					}
					break;

					case FTP_CLIENT_ACTION_UPLOAD: {
						poll_ctx_data->poll_fd.config.v = 0;
						poll_ctx_data->poll_fd.config.s.poll_out = 1;
						poll_ctx_data->poll_fd.config.s.poll_err = 1;
						ftp_client_info->data.action_state = 0;
						ftp_client_info->data.handler = ftp_client_data_upload;
					}
					break;

					default: {
					}
					break;
				}

				set_ftp_client_data_state(ftp_client_info, FTP_CLIENT_DATA_STATE_CONNECTED);
			} else {
				set_ftp_client_data_state(ftp_client_info, FTP_CLIENT_DATA_STATE_DISCONNECT);
			}
		}
		break;

		case FTP_CLIENT_DATA_STATE_CONNECTED: {
			switch(get_ftp_client_action(ftp_client_info)) {
				case FTP_CLIENT_ACTION_DOWNLOAD: {
					ret = recv(poll_ctx_data->poll_fd.fd, ftp_client_info->data.rx_buffer, sizeof(ftp_client_info->data.rx_buffer), 0);

					if(ret > 0) {
						ftp_client_info->data.rx_size = ret;
						//_hexdump("data recv", ftp_client_info->data.rx_buffer, ftp_client_info->data.rx_size);
						ftp_client_info->data.handler(ctx);
						ftp_client_info->stamp = ticks;
					} else {
						debug("ret:%d", ret);

						if(ret < 0) {
							debug("recv error %d, (%s)", ret, strerror(errno));
						}

						set_ftp_client_data_state(ftp_client_info, FTP_CLIENT_DATA_STATE_DISCONNECT);
					}
				}
				break;

				case FTP_CLIENT_ACTION_UPLOAD: {
					ftp_client_info->data.handler(ctx);

					//_hexdump("data send", ftp_client_info->data.tx_buffer, ftp_client_info->data.tx_size);

					if(ftp_client_info->data.tx_size > 0) {
						ret = send(poll_ctx_data->poll_fd.fd, ftp_client_info->data.tx_buffer, ftp_client_info->data.tx_size, 0);

						if(ret > 0) {
							ftp_client_info->stamp = ticks;
						} else {
							if(ret < 0) {
								debug("send error %d, (%s)", ret, strerror(errno));
							}

							set_ftp_client_data_state(ftp_client_info, FTP_CLIENT_DATA_STATE_DISCONNECT);
						}
					} else {
						if(ftp_client_info->file_size == ftp_client_info->data_size) {
							debug("");
							set_ftp_client_data_state(ftp_client_info, FTP_CLIENT_DATA_STATE_DISCONNECT);
						}
					}

				}
				break;

				default: {
				}
				break;
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

	switch(get_ftp_client_data_state(ftp_client_info)) {
		case FTP_CLIENT_DATA_STATE_IDLE: {
			switch(get_ftp_client_data_request_state(ftp_client_info)) {
				case FTP_CLIENT_DATA_STATE_CONNECT: {
					set_ftp_client_data_state(ftp_client_info, FTP_CLIENT_DATA_STATE_CONNECT);
				}
				break;

				default: {
				}
				break;
			}

			set_ftp_client_data_request_state(ftp_client_info, FTP_CLIENT_DATA_STATE_IDLE);
		}
		break;

		case FTP_CLIENT_DATA_STATE_CONNECT: {
			ret = ftp_client_connect(ftp_client_info->data.addr_info.socket_addr_info, &ftp_client_info->data.sock_fd);

			if(ret == 0) {
				poll_ctx_data->poll_fd.fd = ftp_client_info->data.sock_fd;

				poll_ctx_data->poll_fd.config.v = 0;
				poll_ctx_data->poll_fd.config.s.poll_out = 1;
				poll_ctx_data->poll_fd.config.s.poll_err = 1;

				poll_ctx_data->poll_fd.available = 1;

				ftp_client_info->stamp = ticks;
				set_ftp_client_data_state(ftp_client_info, FTP_CLIENT_DATA_STATE_CONNECT_CONFIRM);
			} else {
				set_ftp_client_data_state(ftp_client_info, FTP_CLIENT_DATA_STATE_DISCONNECT);
			}
		}
		break;

		case FTP_CLIENT_DATA_STATE_CONNECT_CONFIRM: {
			if(ticks_duration(ticks, ftp_client_info->stamp) >= FTP_SESSION_TIMEOUT) {
				debug("FTP_CLIENT_DATA_STATE_CONNECT_CONFIRM timeout!");
				set_ftp_client_data_state(ftp_client_info, FTP_CLIENT_DATA_STATE_DISCONNECT);
			}
		}
		break;

		case FTP_CLIENT_DATA_STATE_CONNECTED: {
			switch(get_ftp_client_data_request_state(ftp_client_info)) {
				case FTP_CLIENT_DATA_STATE_DISCONNECT: {
					set_ftp_client_data_state(ftp_client_info, FTP_CLIENT_DATA_STATE_DISCONNECT);
				}
				break;

				default: {
				}
				break;
			}

			set_ftp_client_data_request_state(ftp_client_info, FTP_CLIENT_DATA_STATE_IDLE);
		}
		break;


		case FTP_CLIENT_DATA_STATE_DISCONNECT: {
			poll_ctx_data->poll_fd.available = 0;
			ftp_client_close(&poll_ctx_data->poll_fd.fd);

			set_ftp_client_data_state(ftp_client_info, FTP_CLIENT_DATA_STATE_IDLE);

			if(get_ftp_client_cmd_state(ftp_client_info) != FTP_CLIENT_CMD_STATE_IDLE) {
				set_ftp_client_cmd_request_state(ftp_client_info, FTP_CLIENT_CMD_STATE_DISCONNECT);
			} else {
				OS_ASSERT(remove_callback(ftp_client_info->ftp_data_event_chain, &ftp_client_info->ftp_data_callback_item) == 0);
				set_ftp_client_action(ftp_client_info, FTP_CLIENT_ACTION_IDLE);
			}
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

	OS_ASSERT(ftp_client_info == NULL);
	ftp_client_info = (ftp_client_info_t *)os_calloc(1, sizeof(ftp_client_info_t));
	OS_ASSERT(ftp_client_info != NULL);

	set_ftp_client_action(ftp_client_info, FTP_CLIENT_ACTION_IDLE);
	set_ftp_client_cmd_state(ftp_client_info, FTP_CLIENT_CMD_STATE_IDLE);
	INIT_LIST_HEAD(&ftp_client_info->cmd.addr_info.socket_addr_info_list);
	set_ftp_client_data_state(ftp_client_info, FTP_CLIENT_DATA_STATE_IDLE);
	INIT_LIST_HEAD(&ftp_client_info->data.addr_info.socket_addr_info_list);
	ftp_client_info->cmd.sock_fd = -1;
	ftp_client_info->data.sock_fd = -1;
	ftp_client_info->ftp_server_path.mutex = mutex_create();
	ftp_client_info->ftp_data_event_chain = alloc_callback_chain();

	//add ftp client cmd poll ctx
	poll_ctx_cmd = alloc_poll_ctx();

	poll_ctx_cmd->priv = ftp_client_info;
	poll_ctx_cmd->name = "ftp_client_cmd";
	poll_ctx_cmd->poll_handler = ftp_client_cmd_handler;
	poll_ctx_cmd->poll_periodic = ftp_client_cmd_periodic;

	add_poll_loop_ctx_item(poll_loop, poll_ctx_cmd);


	//add ftp client data poll ctx
	poll_ctx_data = alloc_poll_ctx();
	OS_ASSERT(poll_ctx_data != NULL);

	poll_ctx_data->priv = ftp_client_info;
	poll_ctx_data->name = "ftp_client_data";
	poll_ctx_data->poll_handler = ftp_client_data_handler;
	poll_ctx_data->poll_periodic = ftp_client_data_periodic;

	add_poll_loop_ctx_item(poll_loop, poll_ctx_data);
}
