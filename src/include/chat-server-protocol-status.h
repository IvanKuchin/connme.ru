#ifndef __CHAT_SERVER_PROTOCOL_STATUS__H__
#define __CHAT_SERVER_PROTOCOL_STATUS__H__

#include <memory>
#include <time.h>
#include "chat-server.h"
#include "clog.h"
#include "cmysql.h"

using namespace std;

extern int convert_utf8_to_windows1251(const char* utf8, char* windows1251, size_t n);
extern bool convert_cp1251_to_utf8(const char *in, char *out, int size);

#endif