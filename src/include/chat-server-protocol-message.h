#ifndef __CHAT_SERVER_PROTOCOL_MESSAGE__H__
#define __CHAT_SERVER_PROTOCOL_MESSAGE__H__

#include <unordered_map>
#include <list>
#include <memory>
#include <cstddef>
#include <fstream>
#include "ccgi.h"
#include "chat-server.h"
#include "clog.h"
#include "cmysql.h"
#include "chat-server-jsonparser.h"
#include "chat-server-presencecache.h"
#include "chat-server-requestratelimiter.h"

using namespace std;

extern string 		quoted(string src);
extern string 		trim(string& str);
extern int 			convert_utf8_to_windows1251(const char* utf8, char* windows1251, size_t n);
extern bool 		convert_cp1251_to_utf8(const char *in, char *out, int size);
extern string 		CleanUPText(const string messageBody);
extern double 		GetSecondsSinceY2k();
extern string 		GetLocalFormattedTimestamp();
extern string 		GetHumanReadableTimeDifferenceFromNow (const string timeAgo);
extern string 		GetUserListInJSONFormat(string dbQuery, CMysql *db, CUser *user);
extern string 		GetChatMessagesInJSONFormat(string dbQuery, CMysql *db);
extern string 		GetRandom(int len);
extern string		base64_decode(string const& encoded_string);
 
#endif 