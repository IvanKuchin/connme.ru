#ifndef __CHAT_SERVER_PROTOCOL_MESSAGE__H__
#define __CHAT_SERVER_PROTOCOL_MESSAGE__H__

#include <unordered_map>
#include <list>
#include <memory>
#include <cstddef>
#include <fstream>
#include <Magick++.h>

#include "ccgi.h"
#include "chat-server.h"
#include "clog.h"
#include "cmysql.h"
#include "utilities_connme.h"
#include "chat-server-jsonparser.h"
#include "chat-server-presencecache.h"
#include "chat-server-requestratelimiter.h"

using namespace std;

// TODO: 2delete @ Jun 1, 2017
/*
extern string 		quoted(string src);
extern string 		trim(string& str);
extern int 			convert_utf8_to_windows1251(const char* utf8, char* windows1251, size_t n);
extern bool 		convert_cp1251_to_utf8(const char *in, char *out, int size);
extern string		CleanUPText(const string messageBody, bool removeBR = true);
extern double 		GetSecondsSinceY2k();
extern string 		GetLocalFormattedTimestamp();
extern string 		GetHumanReadableTimeDifferenceFromNow (const string timeAgo);
extern string 		GetUserListInJSONFormat(string dbQuery, CMysql *db, CUser *user);
extern string 		GetChatMessagesInJSONFormat(string dbQuery, CMysql *db);
extern string 		GetRandom(int len);
extern string		base64_decode(string const& encoded_string);
extern void 		CopyFile(const string src, const string dst);
*/

#define	MESSAGE_TYPE_TEXT	"text"
#define MESSAGE_TYPE_IMAGE	"image"

#define	KEEP_BR				false
#define	REMOVE_BR			true	
 
// TODO: 2delete @ Jun 1, 2017
/*
struct ExifInfo 
{
	string  DateTime;
	string  GPSAltitude;
	string  GPSLatitude;
	string  GPSLongitude;
	string  GPSSpeed;
	string  Model;
	string  Authors;
	string  ApertureValue;
	string  BrightnessValue;
	string  ColorSpace;
	string  ComponentsConfiguration;
	string  Compression;
	string  DateTimeDigitized;
	string  DateTimeOriginal;
	string  ExifImageLength;
	string  ExifImageWidth;
	string  ExifOffset;
	string  ExifVersion;
	string  ExposureBiasValue;
	string  ExposureMode;
	string  ExposureProgram;
	string  ExposureTime;
	string  Flash;
	string  FlashPixVersion;
	string  FNumber;
	string  FocalLength;
	string  FocalLengthIn35mmFilm;
	string  GPSDateStamp;
	string  GPSDestBearing;
	string  GPSDestBearingRef;
	string  GPSImgDirection;
	string  GPSImgDirectionRef;
	string  GPSInfo;
	string  GPSTimeStamp;
	string  ISOSpeedRatings;
	string  JPEGInterchangeFormat;
	string  JPEGInterchangeFormatLength;
	string  Make;
	string  MeteringMode;
	string  Orientation;
	string  ResolutionUnit;
	string  SceneCaptureType;
	string  SceneType;
	string  SensingMethod;
	string  ShutterSpeedValue;
	string  Software;
	string  SubjectArea;
	string  SubSecTimeDigitized;
	string  SubSecTimeOriginal;
	string  WhiteBalance;
	string  XResolution;
	string  YCbCrPositioning;
	string  YResolution;
};
*/
#endif 