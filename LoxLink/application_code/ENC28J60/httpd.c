#include "httpd.h"
#include <stdlib.h>
#include <string.h>

static httpd_state_t httpd_pool[TCP_MAX_CONNECTIONS];

static void fill_buf(char **buf, const char *str) {
  int len = 0;
  char *p = *buf;
  while (*str)
    p[len++] = *(str++);
  *buf += len;
}

// get mime type from filename extension
static const char *const httpd_get_mime_type(const char *const url) {
  static const char *mime_type_table[][2] = {
    {"txt", "text/plain"},
    {"htm", "text/html"},
    {"html", "text/html"},
    {"js", "text/javascript"},
    {"css", "text/css"},
    {"gif", "image/gif"},
    {"png", "image/png"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
  };

  const char *ext = strrchr(url, '.');
  if (ext) {
    ext++;
    for (int i = 0; i < sizeof(mime_type_table) / sizeof(mime_type_table[0]); ++i) {
      if (!strcasecmp(ext, mime_type_table[i][0])) {
        return mime_type_table[i][1];
      }
    }
  }
  return 0;
}

// processing HTTP request
static void httpd_request(uint8_t id, httpd_state_t *st, const char *url) {
  if (!strcmp(url, "/")) // index file requested?
    url = HTTPD_INDEX_FILE;

  // defaults
  st->statuscode = 4;                     // status: 404 Not Found
  st->type = httpd_get_mime_type(".txt"); // type: text/plain
  st->data_mode = HTTPD_DATA_RAM;         // source: memory
  st->cursor = 0;                         // cursor: beginning
  st->numbytes = 0;                       // no data

  if (!memcmp(url, "/cgi-bin", 8)) {
    size_t stat = 0; //cgi_exec(id, url + 8, st->data.ram, &(st->data.callback));
    if (stat == 1 /*CGI_USE_CALLBACK*/) {
      st->statuscode = 2;
      st->data_mode = HTTPD_DATA_CALLBACK;
    } else if (stat != 0 /*CGI_NOT_FOUND*/) {
      st->statuscode = 2;
      st->numbytes = stat;
    }
  } else {
    if (0 /*!f_open(&(st->data.fs), url, FA_READ)*/) {
      st->statuscode = 2;                     // status: 200 OK
      st->type = httpd_get_mime_type(url);    // data type
      st->data_mode = HTTPD_DATA_FILE;        // source: file
      st->numbytes = 0 /*st->data.fs.fsize*/; // data length
    }
  }

  if (st->statuscode == 4) {                 // send 404 error page
    st->data_mode = HTTPD_DATA_CPOINTER;     // source: flash
    st->type = httpd_get_mime_type(".html"); // document type
    static const char http_not_found[] = "<h1>404 - Not Found</h1>";
    st->data.cPointer = http_not_found;        // document data
    st->numbytes = sizeof(http_not_found) - 1; // document length
  }
#ifdef WITH_TCP_REXMIT
  st->statuscode_saved = st->statuscode; // save state
  st->numbytes_saved = st->numbytes;
  st->cursor_saved = st->cursor;
#endif
}

// prepare HTTP reply header
static void httpd_header(httpd_state_t *st, char **buf) {
  if (st->statuscode == 2)
    fill_buf(buf, "HTTP/1.0 200 OK\r\n");
  else
    fill_buf(buf, "HTTP/1.0 404 Not Found\r\n");

  // Content-Type
  if (st->type) {
    fill_buf(buf, "Content-Type: ");
    fill_buf(buf, st->type);
    fill_buf(buf, "\r\n");
  }

  // Content-Length
  if (st->numbytes) {
    char str[16];
    ltoa(st->numbytes, str, 10);
    fill_buf(buf, "Content-Length: ");
    fill_buf(buf, str);
    fill_buf(buf, "\r\n");
  }

  // Server
  fill_buf(buf, "Server: " HTTPD_NAME "\r\n");

  // Connection: close
  fill_buf(buf, "Connection: close\r\n");

  // Header end
  fill_buf(buf, "\r\n");
}

// accepts incoming connections
uint8_t tcp_listen(uint8_t id, eth_frame_t *frame) {
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  tcp_packet_t *tcp = (tcp_packet_t *)(ip->data);

  // accept connections to port 80
  return tcp->to_port == HTTPD_PORT;
}

// upstream callback
void tcp_read(uint8_t id, eth_frame_t *frame, uint8_t re) {
  httpd_state_t *st = httpd_pool + id;
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  tcp_packet_t *tcp = (tcp_packet_t *)(ip->data);
  char *buf = (char *)(tcp->data);

  // Connection opened
  if (st->status == HTTPD_CLOSED) {
    st->status = HTTPD_INIT;
  } else if (st->status == HTTPD_WRITE_DATA) { // Sending data
#ifdef WITH_TCP_REXMIT
    if (re) {
      // load previous state
      st->statuscode = st->statuscode_saved;
      st->cursor = st->cursor_saved;
      st->numbytes = st->numbytes_saved;

      if (st->data_mode == HTTPD_DATA_FILE)
        f_lseek(&(st->data.fs), st->cursor);
    } else {
      // save state
      st->statuscode_saved = st->statuscode;
      st->cursor_saved = st->cursor;
      st->numbytes_saved = st->numbytes;
    }
#endif
    // Send bulk of packets
    for (int i = HTTPD_PACKET_BULK; i; --i) {
      size_t blocklen = HTTPD_MAX_BLOCK;
      char *bufptr = buf;

      // Put HTTP header to buffer
      if (st->statuscode != 0) {
        httpd_header(st, &bufptr);
        blocklen -= bufptr - buf;
        st->statuscode = 0;
      }

      // Send up to 512 bytes (-header)
      if (st->numbytes < blocklen)
        blocklen = st->numbytes;

      switch (st->data_mode) {
      // data from RAM
      case HTTPD_DATA_RAM:
        memcpy(bufptr, st->data.ram + st->cursor, blocklen);
        break;

      // data through callback
      case HTTPD_DATA_CALLBACK:
        blocklen = st->data.callback(id, bufptr);
        break;

      // data from progmem
      case HTTPD_DATA_CPOINTER:
        memcpy(bufptr, st->data.cPointer + st->cursor, blocklen);
        break;

        // data from file
      case HTTPD_DATA_FILE: {
        // align read to sector boundary
        uint16_t sectorbytes = 512 - (st->cursor & 0x1ff);
        if (blocklen > sectorbytes)
          blocklen = sectorbytes;

        // read data from file
        //f_read(&(st->data.fs), bufptr, blocklen, &blocklen);
        break;
      }
      }
      bufptr += blocklen;
      st->cursor += blocklen;
      st->numbytes -= blocklen;

      uint8_t options = 0;
      // Send packet
      if ((st->data_mode == HTTPD_DATA_CALLBACK) || (!st->numbytes)) {
        options = TCP_OPTION_CLOSE;
      } else if (i == 1) {
        options = TCP_OPTION_PUSH;
      }
      tcp_send(id, frame, bufptr - buf, options);

      if (options == TCP_OPTION_CLOSE)
        break;
    }
  }
}

// downstream callback
void tcp_write(uint8_t id, eth_frame_t *frame, uint16_t len) {
  const char *const http_header_end = "\r\n\r\n";
  httpd_state_t *st = httpd_pool + id;
  ip_packet_t *ip = (ip_packet_t *)(frame->data);
  tcp_packet_t *tcp = (tcp_packet_t *)(ip->data);
  char *request = (char *)tcp_get_data(tcp);
  request[len] = 0;

  // just connected?
  if (st->status == HTTPD_INIT) {
    // extract URL from request header
    char *url = request + 4;
    char *p;
    if ((!memcmp(request, "GET ", 4)) && ((p = strchr(url, ' ')))) {
      *(p++) = 0;

      // process URL request
      httpd_request(id, st, url);

      // skip other fields
      if (strstr(p, http_header_end))
        st->status = HTTPD_WRITE_DATA;
      else
        st->status = HTTPD_READ_HEADER;
    }
  } else if (st->status == HTTPD_READ_HEADER) { // receiving HTTP header?
    if (strstr(request, http_header_end))       // skip all fields
      st->status = HTTPD_WRITE_DATA;
  }
}

// connection closing handler
void tcp_closed(uint8_t id, uint8_t hard) {
  httpd_state_t *st = httpd_pool + id;

  //        if(st->data_mode == HTTPD_DATA_FILE)
  //                f_close(&(st->data.fs));
  st->data_mode = HTTPD_DATA_RAM;
  st->status = HTTPD_CLOSED;
}