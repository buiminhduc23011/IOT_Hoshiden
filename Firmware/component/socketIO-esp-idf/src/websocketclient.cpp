/*==================================================================================================
*   Project              :  IOT NIDEC
*   Doccument            :  The WebSocket Handbook  (PDF)
*   FileName             :  wepsocketclient.cpp
*   File Description     :  Dinh nghia ham su dung SOCKETIO trong esp32
*
==================================================================================================*/
/*==================================================================================================
Revision History:
Modification     
    Author                  	Date D/M/Y     Description of Changes
----------------------------	----------     ------------------------------------------
    Do Xuan An              	06/03/2024     Tao file
----------------------------	----------     ------------------------------------------
==================================================================================================*/
/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include "websocketclient.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <string.h>
/*==================================================================================================
*                                     FILE VERSION CHECKS
==================================================================================================*/

/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/
/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/
/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/
/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/
/*==================================================================================================
*                                   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

static char TAG[] = "WSC";

#ifdef DEBUG
#define cl_ws_debug(fmt, args...)  ESP_LOGI(TAG, fmt, ## args);
#define cl_ws_info(fmt, args...)   ESP_LOGI(TAG, fmt, ## args);
#define cl_ws_error(fmt, args...)  ESP_LOGE(TAG, fmt, ## args);
#else
#define cl_ws_debug(fmt, args...) //ESP_LOGI(TAG, fmt, ## args);
#define cl_ws_info(fmt, args...) //ESP_LOGI(TAG, fmt, ## args);
#define cl_ws_error(fmt, args...)  ESP_LOGE(TAG, fmt, ## args);
#endif

#define WS_FIN  128
#define WS_MASK 128

#define directClose() esp_transport_close(m_tr)
#define directSend(data, len, timeout_ms) esp_transport_write(m_tr, data, len, timeout_ms)
#define directRecv(data, len, timeout_ms) esp_transport_read(m_tr, data, len, timeout_ms)
#define directPollRead(timeout_ms) esp_transport_poll_read(m_tr, timeout_ms)
#define directPollWrite(timeout_ms) esp_transport_poll_write(m_tr, timeout_ms)


uint32_t WebSocketClient::ToInt32(char* _string)
{
	uint32_t ret = 0;
	int length = strlen(_string);
	for(int i = 0; i < length ; i++)
    {
		ret += (_string[i] - 0x30) * pow(10,length-1-i);
	}
	return ret;
}
//------------------------------------------------------------------------------------------------------------------------------------------
WebSocketClient::WebSocketClient(const char* cptr_url, const char* cptr_token, int i16_pingInterval_ms, int i16_maxBufSize, uint8_t u8_pr, BaseType_t bu8_coreID)
{
	m_stackSize = 10000;
	m_priority  = u8_pr;
	m_coreId    = bu8_coreID;
	m_connected = false;
	m_ping_interval = i16_pingInterval_ms;
	m_url = strdup(cptr_url);
	m_token = NULL;
	if (cptr_token) m_token = strdup(cptr_token);
	m_maxBuf = i16_maxBufSize;
	m_maxBufC = i16_maxBufSize - 1;
	rx_buf = (char*)malloc(i16_maxBufSize);
	tx_buf = (char*)malloc(i16_maxBufSize);
	vSemaphoreCreateBinary(m_lock);
	m_sio_v = 4;
	m_reconnectInterval = 2000;
	m_connectTimeout = 2000;
	m_writeTimeout = 2000;
	m_readTimeout = 2000;
	timeoutPingInterval = 0;
	CountPong = 0;
	wsParseURL();
}

//------------------------------------------------------------------------------------------------------------------------------------------
WebSocketClient::~WebSocketClient()
{
	m_connected = false;
	if (m_url)   free(m_url);
	if (m_token) free(m_token);
	if (rx_buf)  free(rx_buf);
	if (tx_buf)  free(tx_buf);
	vSemaphoreDelete(m_lock);
	esp_transport_destroy(m_tr);
}


//------------------------------------------------------------------------------------------------------------------------------------------
void WebSocketClient::wsParseURL()
{
	char ch, * c = m_url, * port = NULL;
	m_port = 80;
	m_ssl = false;
	m_sio = false;
	m_path = "";
	m_host = "";

	/* Parse protocol */
	if (!strncmp(c, "wss://", 6)) {
		m_port = 443;
		m_ssl = true;
		c += 6;
		m_host = c;
	} else if (!strncmp(c, "ws://", 5)) {
		c += 5;
		m_host = c;
	} else if (!strncmp(c, "https://", 8)) {
		m_port = 443;
		m_ssl = true;
		m_sio = true;
		c += 8;
		m_host = c;
	} else if (!strncmp(c, "http://", 7)) {
		m_sio = true;
		c += 7;
		m_host = c;
	}
	while (*c != '\0') {
		ch = *c;
		if (ch == ':') {
			*c = '\0';
			port = c + 1;
		} else if (ch == '/') {
			*c = '\0';
			m_path = c + 1;
			break;
		}
		c++;
	}
	if (port) {
		m_port = atoi(port);
	}
	cl_ws_debug("URL parse (host = %s, path = %s, port = %d, ssl = %d, sio = %d )", m_host, m_path, m_port, m_ssl ? 1 : 0, m_sio ? 1 : 0);
	if (m_ssl) {
		m_tr = esp_transport_ssl_init();
	} else {
		m_tr = esp_transport_tcp_init();
	}
	esp_transport_set_default_port(m_tr, m_port);
}

//------------------------------------------------------------------------------------------------------------------------------------------
int WebSocketClient::wsConnect(int timeout_ms)
{
	int status, i, j, len, r, checked = 0;
	char* ch;
	m_connected = false;
	if (esp_transport_connect(m_tr, m_host, m_port, timeout_ms) < 0) {
		cl_ws_error("Khong th ket noi den may chu %s:%d", m_host, m_port);
		return 0;
	}
	{
		if (m_sio) {
			char sid[256];
			cl_ws_debug("ID (/%ssocket.io/?EIO=%d&transport=polling)", m_path, m_sio_v);
			r = snprintf(rx_buf, m_maxBufC, "GET /%ssocket.io/?EIO=%d&transport=polling HTTP/1.1\r\n", m_path, m_sio_v);
			if (m_port == 80) {
				r += snprintf(rx_buf + r, m_maxBufC - r, "Host: %s\r\n", m_host);
			} else {
				r += snprintf(rx_buf + r, m_maxBufC - r, "Host: %s:%d\r\n", m_host, m_port);
			}
			if (m_token) {
				r += snprintf(rx_buf + r, m_maxBufC - r, "%s\r\n", m_token);
			}
			r += snprintf(rx_buf + r, m_maxBufC - r, "User-Agent: WebSocket-Client\r\nConnection: \r\n\r\n\r\n");
			directSend(rx_buf, r, m_writeTimeout);
			for (i = 0; i < 2 || (i < m_maxBufC && rx_buf[i - 2] != '\r' && rx_buf[i - 1] != '\n'); ++i) { if (directRecv(rx_buf + i, 1, m_readTimeout) == 0) { directClose(); return 0; } }
			rx_buf[i] = 0;
			if (i == m_maxBufC) { cl_ws_error("ERROR: Trang thai khong on khi ket noi dong: %s", m_url); directClose(); return -1; }
			if (sscanf(rx_buf, "HTTP/1.1 %d", &status) != 1 || status != 200) { cl_ws_error("ERROR: Trang thai khong on khi ket noi %s: %s", m_url, rx_buf); directClose(); return 0; }
			len = m_maxBufC;
			while (1) {
				for (i = 0; i < 2 || (i < m_maxBufC && rx_buf[i - 2] != '\r' && rx_buf[i - 1] != '\n'); ++i) {
					if (directRecv(rx_buf + i, 1, m_readTimeout) == 0) {
						directClose();
						return 0;
					}
				}
				if (rx_buf[0] == '\r' && rx_buf[1] == '\n')  break;
				while ((i > 0) && ((rx_buf[i - 1] == '\r') || (rx_buf[i - 1] == '\n'))) i--;
				rx_buf[i] = '\0';
				if (!strncmp(rx_buf, "Content-Length:", 15)) {
					if (sscanf(rx_buf, "Content-Length:%d", &len) != 1) {
						len = m_maxBufC;
					} else {
						if (len > m_maxBufC) len = m_maxBufC;
					}
					cl_ws_debug("len = <%d>", len);
				}
				cl_ws_debug("header line = <%s>", rx_buf);
			}
			i = directRecv(rx_buf, len, m_readTimeout);
			if (i > 0) {
				rx_buf[i] = '\0';
				cl_ws_debug("JSON = <%s>", rx_buf);
				ch = strstr(rx_buf, "\"sid\":");
				if (ch) {
					ch += 6;
					j = 0;
					while (*ch != '\0') {
						if (*ch == ',') break;
						if (*ch == '}') break;
						if ((*ch != '"') && (*ch != ' '))
							sid[j++] = *ch;
						ch++;
					}
					sid[j] = '\0';
					cl_ws_debug("GOT sid = <%s>", sid);
				}

				ch = strstr(rx_buf, "\"pingInterval\":");
				if (ch) {
					timeoutPingInterval = 0;
					ch += 15;
					j = 0;
					while (*ch != '\0') {
						if (*ch == ',') break;
						if (*ch == '}') break;
						timeoutPingInterval = (timeoutPingInterval*10 + (*ch - 48));
						ch++;
					}
					ch[j] = 0;	
				}
			}
			cl_ws_debug("/%ssocket.io/?EIO=%d&transport=websocket&sid=%s)", m_path, m_sio_v, sid);
			r = snprintf(rx_buf, m_maxBufC, "GET /%ssocket.io/?EIO=4&transport=websocket&sid=%s HTTP/1.1\r\n", m_path, sid);
		} else {
			r = snprintf(rx_buf, m_maxBufC, "GET /%s HTTP/1.1\r\n", m_path);
		}
		if (m_port == 80) {
			r += snprintf(rx_buf + r, m_maxBufC - r, "Host: %s\r\n", m_host);
		} else {
			r += snprintf(rx_buf + r, m_maxBufC - r, "Host: %s:%d\r\n", m_host, m_port);
		}
		if (m_token) {
			r += snprintf(rx_buf + r, m_maxBufC - r, "%s\r\n", m_token);
		}
		r += snprintf(rx_buf + r, m_maxBufC - r, "User-Agent: WebSocket-Client\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\nSec-WebSocket-Version: 13\r\n\r\n");
		directSend(rx_buf, r, m_writeTimeout);
		for (i = 0; i < 2 || (i < m_maxBufC && rx_buf[i - 2] != '\r' && rx_buf[i - 1] != '\n'); ++i) { if (directRecv(rx_buf + i, 1, m_readTimeout) == 0) { directClose(); return 0; } }
		rx_buf[i] = 0;
		if (i == m_maxBufC) { cl_ws_error("ERROR: Khong the lay ket noi o : %s", m_url); directClose(); return -1; }
		if (sscanf(rx_buf, "HTTP/1.1 %d", &status) != 1 || status != 101) { cl_ws_error("ERROR: Trang thai khong on khi ket noi %s: %s", m_url, rx_buf); directClose(); return 0; }
		while (1) {
			for (i = 0; i < 2 || (i < m_maxBufC && rx_buf[i - 2] != '\r' && rx_buf[i - 1] != '\n'); ++i) {
				if (directRecv(rx_buf + i, 1, m_readTimeout) == 0) {
					directClose();
					return 0;
				}
			}
			if (rx_buf[0] == '\r' && rx_buf[1] == '\n')  break;
			while ((i > 0) && ((rx_buf[i - 1] == '\r') || (rx_buf[i - 1] == '\n'))) i--;
			rx_buf[i] = '\0';

			if (!strncmp(rx_buf, "Sec-WebSocket-Accept:", 21)) {
				if (strstr(rx_buf, "HSmrc0sMlYUkAGmm5OPpG2HaGWk=")) {
					checked = 1;
				} else {
					cl_ws_error("ERROR: Khong th lay duoc khoa (line: %s)", rx_buf);
					directClose();
					return -1;
				}
			}
			cl_ws_debug("line ư= <%s>", rx_buf);
		}
	}
	if (!checked) {
		cl_ws_error("ERROR: Khong the lay khoa!");
		directClose();
		return -1;
	}
	ws_ping_cnt = 0;
	ws_pong_cnt = 0;
	line_begin = 0;
	line_end = 0;
	ws_frame_size = 0;
	m_connected = true;
	if (m_ccb) m_ccb(this, true);
	return 1;
}

//------------------------------------------------------------------------------------------------------------------------------------------
int WebSocketClient::send(const char* cptr_msg, uint32_t u32_size, int i16_type)
{
	unsigned char* response = (unsigned char*)tx_buf;
	int idx_response, res = 0, allocated = 0;
	uint32_t i;
	uint8_t idx_header;
	uint32_t length;
	uint8_t masks[4];

	int poll_write;

	if ((poll_write = directPollWrite(m_writeTimeout)) <= 0) {
		return poll_write;
	}


	if (u32_size + 15 > m_maxBuf) {
		allocated = 1;
		response = (unsigned char*)malloc(u32_size + 16);
		if (!response) return 0;
	}

	if (xSemaphoreTake(m_lock, (TickType_t)1000) == pdFALSE) return 0;
	masks[0] = rand() & 0xff;
	masks[1] = rand() & 0xff;
	masks[2] = rand() & 0xff;
	masks[3] = rand() & 0xff;
	length = u32_size;
	if (length <= 125) {
		idx_header = 6;
		response[0] = (WS_FIN | i16_type);
		response[1] = (length & 0x7F) | WS_MASK;
		response[2] = masks[0];
		response[3] = masks[1];
		response[4] = masks[2];
		response[5] = masks[3];
	} else if (length >= 126 && length <= 65535) {
		idx_header = 8;
		response[0] = (WS_FIN | i16_type);
		response[1] = 126 | WS_MASK;
		response[2] = (length >> 8) & 255;
		response[3] = length & 255;
		response[4] = masks[0];
		response[5] = masks[1];
		response[6] = masks[2];
		response[7] = masks[3];
	} else {
		idx_header = 14;
		response[0] = (WS_FIN | i16_type);
		response[1] = 127 | WS_MASK;
		response[2] = 0;
		response[3] = 0;
		response[4] = 0;
		response[5] = 0;
		response[6] = (unsigned char)((length >> 24) & 255);
		response[7] = (unsigned char)((length >> 16) & 255);
		response[8] = (unsigned char)((length >> 8) & 255);
		response[9] = (unsigned char)(length & 255);
		response[10] = masks[0];
		response[11] = masks[1];
		response[12] = masks[2];
		response[13] = masks[3];
	}

	idx_response = idx_header;

	/* Add data bytes and apply mask. */
	for (i = 0; i < length; i++) {
		response[idx_response] = cptr_msg[i] ^ masks[i % 4];
		idx_response++;
	}
	response[idx_response] = '\0';

	res = (directSend((const char*)response, idx_response, m_writeTimeout) == idx_response) ? 1 : 0;
	/* Free allocated memory */
	if (allocated) free(response);

	xSemaphoreGive(m_lock);

	return ((int)res);
}


//------------------------------------------------------------------------------------------------------------------------------------------
int WebSocketClient::send2(const char* cptr_msg0, uint32_t u32_size0, const char* cptr_msg1, uint32_t u32_size1, int i16_type)
{
	unsigned char* response = (unsigned char*)tx_buf;
	int idx_response, res = 0, allocated = 0, poll_write;
	uint32_t i;
	uint8_t idx_header;
	uint32_t length;
	uint8_t masks[4];

	length = u32_size0 + u32_size1;

	if ((poll_write = directPollWrite(m_writeTimeout)) <= 0) {
		return poll_write;
	}


	if (length + 15 > m_maxBuf) {
		allocated = 1;
		response = (unsigned char*)malloc(length + 16);
		if (!response) return 0;
	}
	if (xSemaphoreTake(m_lock, (TickType_t)1000) == pdFALSE) return 0;
	masks[0] = rand() & 0xff;
	masks[1] = rand() & 0xff;
	masks[2] = rand() & 0xff;
	masks[3] = rand() & 0xff;
	if (length <= 125) {
		idx_header = 6;
		response[0] = (WS_FIN | i16_type);
		response[1] = (length & 0x7F) | WS_MASK;
		response[2] = masks[0];
		response[3] = masks[1];
		response[4] = masks[2];
		response[5] = masks[3];
	} else if (length >= 126 && length <= 65535) {
		idx_header = 8;
		response[0] = (WS_FIN | i16_type);
		response[1] = 126 | WS_MASK;
		response[2] = (length >> 8) & 255;
		response[3] = length & 255;
		response[4] = masks[0];
		response[5] = masks[1];
		response[6] = masks[2];
		response[7] = masks[3];
	} else {
		idx_header = 14;
		response[0] = (WS_FIN | i16_type);
		response[1] = 127 | WS_MASK;
		response[2] = 0;
		response[3] = 0;
		response[4] = 0;
		response[5] = 0;
		response[6] = (unsigned char)((length >> 24) & 255);
		response[7] = (unsigned char)((length >> 16) & 255);
		response[8] = (unsigned char)((length >> 8) & 255);
		response[9] = (unsigned char)(length & 255);
		response[10] = masks[0];
		response[11] = masks[1];
		response[12] = masks[2];
		response[13] = masks[3];
	}
	idx_response = idx_header;
	for (i = 0; i < u32_size0; i++) {
		response[idx_response] = cptr_msg0[i] ^ masks[i % 4];
		idx_response++;
	}
	for (i = 0; i < u32_size1; i++) {
		response[idx_response] = cptr_msg1[i] ^ masks[(u32_size0 + i) % 4];
		idx_response++;
	}
	response[idx_response] = '\0';
	res = (directSend((const char*)response, idx_response, m_writeTimeout) == idx_response) ? 1 : 0;
	if (allocated) free(response);
	xSemaphoreGive(m_lock);
	return ((int)res);
}

//------------------------------------------------------------------------------------------------------------------------------------------
static int ws_decode_pong(char* cptr_msg, int* res)
{
	int r = 0;
	char ch = cptr_msg[0];
	if ((ch < '0') || (ch > '9')) return 0;
	r = (int)(ch - '0');

	ch = cptr_msg[1];
	if ((ch < '0') || (ch > '9')) return 0;
	r *= 10;
	r += (int)(ch - '0');

	ch = cptr_msg[2];
	if ((ch < '0') || (ch > '9')) return 0;
	r *= 10;
	r += (int)(ch - '0');

	ch = cptr_msg[3];
	if ((ch < '0') || (ch > '9')) return 0;
	r *= 10;
	r += (int)(ch - '0');

	*res = r;
	return 1;
}


//------------------------------------------------------------------------------------------------------------------------------------------
int WebSocketClient::wsOnFrame()
{
	int cnt = ws_frame_size - ws_header_size;
	switch (ws_frame_type) {
		case WS_OP_CONT: {
			cl_ws_debug("Do dai khung (size = %d)", cnt);
			if (m_cb) {
				m_cb(this, ws_msg, cnt, WS_OP_CONT);
			}
		} break;
		case WS_OP_TXT: {
			cl_ws_debug("Do dai texxt (size = %d)", cnt);
			if (m_cb) {
				m_cb(this, ws_msg, cnt, WS_OP_TXT);
			}
			if (m_on.size()) {
				char* k = ws_msg, *x;
				int len = cnt;
				bool lev = false;			
				while ((len > 0) && ((*k == '[') || (*k == '"') || (*k == ' '))) { if (*k == '"') lev = true; k++;len--; }
				x = k + 1;len--;
				while ((len > 0) && (*x != '"') && (*x != ',') && ((lev) || (*x != ' '))) { x++; len--; }
				std::string key(k, (int)(x - k));
				cl_ws_debug("key (%s)", key.c_str());
				auto itr = m_on.find(key);
				if ((len > 1) && (itr != m_on.end())) {
					x++;len--;
					while ((len > 0) && ((*x == ',') || (*x == ' '))) { x++; len--; }
					while ((len > 0) && ((x[len - 1] == ']') || (x[len - 1] == ' '))) { x[len - 1] = '\0'; len--; }
					if (len > 0) {
						if (itr != m_on.end()) {
							itr->second(this, x, len);
						}
					}
				}
			}
		} break;
		case WS_OP_BIN: {
			cl_ws_debug("Do dai khung Bin (size = %d)", cnt);
			if (m_cb) {
				m_cb(this, ws_msg, cnt, WS_OP_BIN);
			}
		} break;
		case WS_OP_CLOSE: {
			cl_ws_debug("Khung Dong ket noi");
			return 0;
		} break;
		case WS_OP_PING: {
			cl_ws_debug("Khung Ping (size = %d)", cnt);
			send(ws_msg, cnt, WS_OP_PONG);
		} break;
		case WS_OP_PONG: {
			ESP_LOGI("AnDX","Khung PONG");
			if (cnt == 4) {
				if (ws_decode_pong(ws_msg, &ws_pong_cnt) == 1) {
					ESP_LOGI("AnDX","Khung PONG (size = %d, cnt = %d)", cnt, ws_pong_cnt);
					if (ws_ping_cnt != ws_pong_cnt) {
						cl_ws_error("Deo doc duoc !");
						return 0;
					}
				}
			}
		} break;
		default: break;
	}
	return 1;
}

//------------------------------------------------------------------------------------------------------------------------------------------
int WebSocketClient::wsFeedFrame()
{
	uint8_t opcode;
	int i, cur_byte, cnt;

	cl_ws_debug("WS (total = %d)", line_end);

	if (ws_frame_size == 0) {
		unsigned char* b = (unsigned char*)rx_buf;
		ws_header_size = 2;
		if (line_end < 2) return 0;
		cur_byte = *b++;
		ws_is_fin = (cur_byte & 0xFF) >> 7;
		opcode = (cur_byte & 0x0F);
		ws_frame_type = opcode;
		if (cur_byte & 0x70) {
			cl_ws_debug("Su dung RSV trong khi deo duoc khai bao");
			return -1;
		}
		cur_byte = *b++;
		cnt = cur_byte & 0x7F;
		ws_is_mask = (cur_byte & 0xFF) >> 7;
		if (ws_is_mask) {
			cl_ws_debug("(opcode = %d)", opcode);
			return -1;
		}
		if (cnt == 126) {
			ws_header_size += 2;
			if (line_end < ws_header_size) return 0;
			cur_byte = *b++;
			cnt = (((uint64_t)cur_byte) << 8);
			cur_byte = *b++;
			cnt |= cur_byte;
		} else if (cnt == 127) {
			cl_ws_debug("Khung qua dai");
			return -1;
		}
		ws_frame_size = ws_header_size + cnt;
		cl_ws_debug("Header (size = %d, opcode = %d, fin = %d, header = %d)", ws_frame_size, opcode, ws_is_fin, ws_header_size);
	}
	if (ws_frame_size > 0) {
		unsigned char* b = (unsigned char*)&rx_buf[ws_header_size];
		if (line_end < ws_frame_size) return 0;
		cnt = ws_frame_size - ws_header_size;
		ws_msg = (char*)b;
		if (wsOnFrame() == 0) return -1;
		if (line_end > ws_frame_size) {
			int left = line_end - ws_frame_size;
			cl_ws_debug("Move buffer (left = %d)", left);
			for (i = 0; i < left; ++i) {
				rx_buf[i] = rx_buf[ws_frame_size + i];
			}
			line_end = left;
		} else {
			line_end = 0;
		}
		ws_frame_size = 0;
		return cnt;
	}
	return 0;
}

//------------------------------------------------------------------------------------------------------------------------------------------
int WebSocketClient::wsSendPing()
{
	if (m_connected) {
		char b[16];
		if (ws_ping_cnt != ws_pong_cnt)
		{
			return 0;
		}
		ws_ping_cnt++;
		/* Send ping */
		snprintf(b, 15, "%04d", ws_ping_cnt);
		cl_ws_debug("Gui PING (%d)", ws_ping_cnt);
		send(b, 4, WS_OP_PING);
		if(ws_ping_cnt >= 100)ws_ping_cnt = 1;
	}
	return 1;
}


#define MAX_MEMORY_BUFF (1024)

//------------------------------------------------------------------------------------------------------------------------------------------
void WebSocketClient::run()
{
	int idx;

	line_begin = 0;
	line_end = 0;
	ws_frame_size = 0;

	cl_ws_debug("Bat dau chay");
	while (true) {
		if (!m_connected) {
			if (m_ccb) m_ccb(this, false);
			while (!m_connected) {
				line_begin = 0;
				line_end = 0;
				ws_frame_size = 0;
				vTaskDelay(m_reconnectInterval / portTICK_PERIOD_MS);
				this->wsConnect(m_connectTimeout);
			}
		}
		if (timeoutPingInterval) {
			idx = directPollRead(timeoutPingInterval + 1000);
			if (idx == 0) {
				if (!wsSendPing()) {
					cl_ws_error("Khong nhan duoc PONG tu may chu");
					directClose();
					m_connected = false;
				}
				continue;
			} else if (idx < 0) {
				cl_ws_error("Xoa socket");
				directClose();
				m_connected = false;
				continue;
			}
		}
		idx = directRecv(&rx_buf[line_end], (MAX_MEMORY_BUFF - line_end), -1);
		if (idx <= 0) {
			cl_ws_debug("Xoa socket");
			directClose();
			m_connected = false;
			continue;
		}
		line_end += idx;
		while (idx > 0) {
			idx = wsFeedFrame();
		}
		if (idx < 0) {
			cl_ws_debug("Xoa socket");
			directClose();
			m_connected = false;
			continue;
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------------------
static void WebSocketClientRunTask(void* arg) 
{
	WebSocketClient* p = (WebSocketClient*) arg;
	p->run();
	p->stop();
}

//------------------------------------------------------------------------------------------------------------------------------------------
void WebSocketClient::stop() 
{
	if (m_handle == nullptr) return;
	::vTaskDelete(m_handle);
	m_handle = NULL;
}

//------------------------------------------------------------------------------------------------------------------------------------------
void WebSocketClient::start() 
{
	::xTaskCreatePinnedToCore(&WebSocketClientRunTask, "WebSocketClient", m_stackSize, this, m_priority, &m_handle, m_coreId);
}


//======================================END FILE===================================================/
