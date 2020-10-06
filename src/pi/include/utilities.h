#ifndef __UTILITIES__H__
#define __UTILITIES__H__

#include <curl/curl.h>
#include <fstream>  	//--- used for CopyFile function
#include <map>
#include <vector>
#include <list>
#include <unordered_set>
#include <algorithm>
#include <regex>
#include <cctype> 		//--- used for tolower
#include <sys/stat.h>
#include <sys/types.h>	// --- DIR
#include <dirent.h>		// --- opendir, rmdir
#include <execinfo.h>   // --- backtrace defined here
#include <signal.h>     // --- signal interception
#include <openssl/sha.h>
#include <Magick++.h>
#include <codecvt>		// --- codecvt_utf8

#include "c_smsc.h"
#include "cmysql.h"
#include "cfiles.h"
#include "cuser.h"
#include "cexception.h"
#include "clog.h"
#include "localy.h"

using namespace std;

auto			crash_handler(int sig) -> void;
auto			wide_to_multibyte(std::wstring const& s) -> string;
auto			multibyte_to_wide(std::string const& s) -> wstring;
auto      		rtrim(string& str) -> string;
auto      		ltrim(string& str) -> string;
auto      		trim(string& str) -> string;
auto      		quoted(string src) -> string;
auto  			toLower(string src) -> string;
auto      		GetRandom(int len) -> string;
auto      		DeleteHTML(string src, bool removeBR = true) -> string;
auto      		RemoveQuotas(string src) -> string;
auto      		RemoveSpecialSymbols(wstring src) -> wstring;
auto      		RemoveSpecialSymbols(string src) -> string;
auto 			RemoveSpecialHTMLSymbols(const wstring &src) -> wstring;
auto      		RemoveSpecialHTMLSymbols(const string &src) -> string;
auto      		ReplaceDoubleQuoteToQuote(string src) -> string;
auto      		ReplaceCRtoHTML(string src) -> string;
auto      		CleanUPText(const string messageBody, bool removeBR = true) -> string;
auto      		RemoveAllNonAlphabetSymbols(const wstring &src) -> wstring;
auto      		RemoveAllNonAlphabetSymbols(const string &src) -> string;
auto			ConvertTextToHTML(const string &messageBody) -> string;
auto			ConvertHTMLToText(const string &src) -> string;
auto 			ConvertHTMLToText(const wstring &src) -> wstring;
auto 			CheckHTTPParam_Text(const string &srcText) -> string;
auto			CheckHTTPParam_Number(const string &srcText) -> string;
auto	 		CheckHTTPParam_Date(string srcText) -> string;
auto	 		CheckHTTPParam_Float(const string &srcText) -> string;
auto			CheckHTTPParam_Email(const string &srcText) -> string;
auto      		CheckIfFurtherThanNow(string occupationStart_cp1251)  -> string;
auto			GetDefaultActionFromUserType(const string &role, CMysql *) -> string;
auto			GetDefaultActionFromUserType(CUser *, CMysql *) -> string;
auto      		GetSecondsSinceY2k() -> double;
auto      		GetLocalFormattedTimestamp() -> string;
auto      		GetTimeDifferenceFromNow(const string timeAgo) -> double;
auto      		GetMinutesDeclension(const int value) -> string;
auto      		GetHoursDeclension(const int value) -> string;
auto      		GetDaysDeclension(const int value) -> string;
auto      		GetMonthsDeclension(const int value) -> string;
auto      		GetYearsDeclension(const int value) -> string;
auto      		GetHumanReadableTimeDifferenceFromNow (const string timeAgo) -> string;
auto      		SymbolReplace(const string where, const string src, const string dst) -> string;
auto      		SymbolReplace_KeepDigitsOnly(const string where) -> string;
auto         	qw(const string src, vector<string> &dst) -> int;
auto			join(const vector<string>& vec, string separator = ",") -> string;
auto			split(const string& s, const char& c) -> vector<string>;
auto      		UniqueUserIDInUserIDLine(string userIDLine) -> string; //-> decltype(static_cast<string>("123")
auto      		AutodetectSexByName(string name, CMysql *) -> string;
auto			GetPasswordNounsList(CMysql *) -> string;
auto			GetPasswordAdjectivesList(CMysql *) -> string;
auto			GetPasswordCharacteristicsList(CMysql *) -> string;
auto			isAllowed_NoSession_Action(string action) -> bool;


auto      		GetBaseUserInfoInJSONFormat(string dbQuery, CMysql *, CUser *) -> string;
auto      		GetGeoCountryListInJSONFormat(string dbQuery, CMysql *, CUser *) -> string;
string      	GetChatMessagesInJSONFormat(string dbQuery, CMysql *);
string 			GetBookListInJSONFormat(string dbQuery, CMysql *, bool includeReaders = false);
string 			GetComplainListInJSONFormat(string dbQuery, CMysql *, bool includeReaders = false);
string 			GetCertificationListInJSONFormat(string dbQuery, CMysql *, bool includeDevoted = false);
string 			GetCourseListInJSONFormat(string dbQuery, CMysql *, bool includeStudents = false);
string 			GetSkillListInJSONFormat(string user_id, CMysql *);
string 			GetUniversityListInJSONFormat(string dbQuery, CMysql *, bool includeStudents = false);
string 			GetSchoolListInJSONFormat(string dbQuery, CMysql *, bool includeStudents = false);
string      	GetUnreadChatMessagesInJSONFormat(CUser *, CMysql *);
string      	GetMessageImageList(string imageSetID, CMysql *);
string 			GetBookRatingList(string bookID, CMysql *);
string 			GetCourseRatingList(string courseID, CMysql *);
string      	GetMessageCommentsCount(string messageID, CMysql *);
string 			GetCompanyCommentsCount(string messageID, CMysql *);
string 			GetLanguageCommentsCount(string messageID, CMysql *);
string 			GetBookCommentsCount(string messageID, CMysql *);
string 			GetCertificateCommentsCount(string messageID, CMysql *);
string 			GetUniversityDegreeCommentsCount(string messageID, CMysql *);
string      	GetMessageSpam(string messageID, CMysql *);
string      	GetMessageSpamUser(string messageID, string userID, CMysql *);
string			GetLanguageIDByTitle(string title, CMysql *);
string			GetSkillIDByTitle(string title, CMysql *);
string			GetCompanyPositionIDByTitle(string title, CMysql *);
string 			GetOpenVacanciesInJSONFormat(string companyID, CMysql *, CUser * = NULL);
string			GetGeoLocalityIDByCityAndRegion(string regionName, string cityName, CMysql *);
bool        	AllowMessageInNewsFeed(CUser *me, const string messageOwnerID, const string messageAccessRights, vector<string> *messageFriendList);
bool        	isPersistenceRateLimited(string REMOTE_ADDR, CMysql *);
bool        	isFileExists(const std::string& name);
bool			isFilenameImage(string	filename);
bool			isFilenameVideo(string	filename);
void        	CopyFile(const string src, const string dst);
string      	GetCompanyDuplicates(CMysql *);
string      	GetPicturesWithEmptySet(CMysql *);
string      	GetPicturesWithUnknownMessage(CMysql *);
string      	GetPicturesWithUnknownUser(CMysql *);
string      	GetRecommendationAdverse(CMysql *);
string      	GetUserAvatarByUserID(string userID, CMysql *);
void        	RemoveMessageImages(string sqlWhereStatement, CMysql *);
void    		RemoveBookCover(string sqlWhereStatement, CMysql *);
bool    		RemoveSpecifiedCover(string itemID, string itemType, CMysql *);
bool        	CheckUserEmailExisting(string userNameToCheck, CMysql *);
vector<string>	GetUserTagsFromText(string srcMessage);
bool			RedirStdoutToFile(string fname);
bool			RedirStderrToFile(string fname);
bool 			AmIMessageOwner(string messageID, CUser *, CMysql *);
pair<string, string> GetMessageOwner(string messageID, CUser *, CMysql *);
string			GetUserSubscriptionsInJSONFormat(string sqlQuery, CMysql *);
string			SubscribeToCompany(string companyID, CUser *, CMysql *);
string			UnsubscribeFromCompany(string companyID, CUser *, CMysql *);
string			SubscribeToGroup(string groupID, CUser *, CMysql *);
string			UnsubscribeFromGroup(string groupID, CUser *, CMysql *);
bool 			isBotIP(string ip);
bool			isAdverseWordsHere(string text, CMysql *);
string			GetDefaultActionLoggedinUser(void);
auto			stod_noexcept(const string &) noexcept -> double;
auto			MaskSymbols(string src, int first_pos, int last_pos) -> string;
auto 			CutTrailingZeroes(string number) -> string;
auto			GetSiteThemesInJSONFormat(string sqlQuery, CMysql *, CUser *) -> string;

// --- Language functions
// ------- do NOT yse this function
// ------- it is used ONLY for admin part
auto 			GetLanguageListInJSONFormat(string dbQuery, CMysql *, bool includeStudents = false) -> string;
// ------- use this version, if possible
auto 			GetUserLanguageListInJSONFormat(string user_id, CMysql *) -> string;


// --- file system functions
auto			CleanupFilename(string filename) -> string;

// --- string counters
auto			GetNumberOfCntrls (const wstring &src) -> unsigned int;
auto			GetNumberOfCntrls (const  string &src) -> unsigned int;
auto			GetNumberOfSpaces (const wstring &src) -> unsigned int;
auto			GetNumberOfSpaces (const  string &src) -> unsigned int;
auto			GetNumberOfDigits (const wstring &src) -> unsigned int;
auto			GetNumberOfDigits (const  string &src) -> unsigned int;
auto			GetNumberOfLetters(const wstring &src) -> unsigned int;
auto			GetNumberOfLetters(const  string &src) -> unsigned int;

// --- SMS functions
auto			SendPhoneConfirmationCode(const string &country_code, const string &phone_number, const string &session, CMysql *db, CUser *user) -> string;
auto			CheckPhoneConfirmationCode(const string &confirmation_code, const string &session, CMysql *, CUser *) -> vector<pair<string, string>>;
auto			RemovePhoneConfirmationCodes(string sessid, CMysql *) -> string;

// --- DB functions
auto			GetValueFromDB(string sql, CMysql *) -> string;
auto			GetValuesFromDB(string sql, CMysql *) -> vector<string>;

// --- login functions
auto			GetCountryCodeAndPhoneNumberBySMSCode(const string &confirmation_code, const string &session, CMysql *) -> tuple<string, string, string>;

// --- helpdesk
auto			GetHelpDeskTicketsInJSONFormat(string sqlQuery, CMysql *db, CUser *user) -> string;
auto			GetHelpDeskTicketHistoryInJSONFormat(string sqlQuery, CMysql *db, CUser *user) -> string;
auto			GetHelpDeskTicketAttachInJSONFormat(string sqlQuery, CMysql *db, CUser *user) -> string;
auto			isHelpdeskTicketOwner(string ticket_id, string user_id, CMysql *db, CUser *user) -> bool;

// --- FAQ
auto			GetFAQInJSONFormat(string sqlQuery, CMysql *db, CUser *user) -> string;

// --- date functions
struct tm		GetTMObject(string date);
auto			operator <(const struct tm &tm_1, const struct tm &tm_2) -> bool;
auto			operator <=(const struct tm &tm_1, const struct tm &tm_2) -> bool;
auto			operator >(const struct tm &tm_1, const struct tm &tm_2) -> bool;
auto			operator >=(const struct tm &tm_1, const struct tm &tm_2) -> bool;
auto			operator ==(const struct tm &tm_1, const struct tm &tm_2) -> bool;
auto			PrintDate(const struct tm &_tm) -> string;
auto			PrintSQLDate(const struct tm &_tm) -> string;
auto			PrintDateTime(const struct tm &_tm) -> string;
auto			PrintTime(const struct tm &_tm, string format) -> string;

// --- function set for image upload/removal
auto 			GetSpecificData_GetNumberOfFolders(string itemType) -> int;
auto 			GetSpecificData_GetMaxFileSize(string itemType) -> int;
auto		 	GetSpecificData_GetMaxWidth(string itemType) -> unsigned int;
auto		 	GetSpecificData_GetMaxHeight(string itemType) -> unsigned int;
auto 			GetSpecificData_GetBaseDirectory(string itemType) -> string;
auto			GetSpecificData_GetFinalFileExtenstion(string itemType) -> string;
auto 			GetSpecificData_SelectQueryItemByID(string itemID, string itemType) -> string;
auto 			GetSpecificData_UpdateQueryItemByID(string itemID, string itemType, string folderID, string fileName) -> string;
auto 			GetSpecificData_GetDBCoverPhotoFolderString(string itemType) -> string;
auto 			GetSpecificData_GetDBCoverPhotoFilenameString(string itemType) -> string;
auto 			GetSpecificData_GetDataTypeByItemType(const string &itemType) -> string;
auto 			GetSpecificData_AllowedToChange(string itemID, string itemType, CMysql *, CUser *) -> string;

// --- UTF8 encoding/decoding
auto         	convert_utf8_to_windows1251(const char* utf8, char* windows1251, size_t n) -> int;
auto			utf8_to_cp1251(const string &) -> string;
auto 			convert_cp1251_to_utf8(const char *in, char *out, int size) -> bool;

// --- base64 encoding/decoding
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";
typedef unsigned char BYTE;

bool 			is_base64(BYTE c);
string 			base64_encode(BYTE const* buf, unsigned int bufLen) ;
string 			base64_decode(std::string const& encoded_string) ;

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

string		ParseGPSLongitude(const string longitudeStr);
string		ParseGPSLatitude(const string latitudeStr);
string		ParseGPSAltitude(const string altitudeStr);
string		ParseGPSSpeed(const string speedStr);

auto		GetDomain() -> string;

#endif
