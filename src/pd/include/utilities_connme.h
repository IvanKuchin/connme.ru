#ifndef __UTILITIES_CONNME__H__
#define __UTILITIES_CONNME__H__

#include "c_cache_obj.h"
#include "utilities.h"
#include "utilities_sql_queries.h"


// --- this function returns lots of data, be careful with it
auto 	     	GetUserListInJSONFormat(string dbQuery, CMysql *, CUser *) -> string;
// --- use short version of previos function, where possible
auto 	     	GetUsersNameAvatarInJSONFormat(string dbQuery, CMysql *, CUser *) -> string;

auto    	  	GetCompanyListInJSONFormat(string dbQuery, CMysql *, CUser *, bool quickSearch = true, bool includeEmployedUsersList = false) -> string;
auto 			GetNewsFeedInJSONFormat(string whereStatement, int currPage, int newsOnSinglePage, CUser *, CMysql *) -> string;
auto      		GetMessageLikesUsersList(string messageID, CUser *, CMysql *) -> string;
auto 			GetBookLikesUsersList(string usersBookID, CUser *, CMysql *) -> string;
auto 			GetLanguageLikesUsersList(string usersLanguageID, CUser *, CMysql *) -> string;
auto 			GetCompanyLikesUsersList(string usersCompanyID, CUser *, CMysql *) -> string;
auto 			GetCertificationLikesUsersList(string usersCertificationID, CUser *, CMysql *) -> string;
auto 			GetCourseLikesUsersList(string usersCourseID, CUser *, CMysql *) -> string;
auto 			GetUniversityDegreeLikesUsersList(string messageID, CUser *, CMysql *) -> string;
auto 			GetBookRatingUsersList(string bookID, CUser *, CMysql *) -> string;
auto 	     	GetUserNotificationSpecificDataByType(unsigned long typeID, unsigned long actionID, CMysql *, CUser *) -> string;
auto    	  	GetUserNotificationInJSONFormat(string sqlRequest, CMysql *, CUser *) -> string;
auto 			GetCandidatesListAppliedToVacancyInJSONFormat(string dbQuery, CMysql *) -> string;

auto			RotateImage(string filename, int degree) -> string;
auto			FlipImageVertical(string filename) -> string;
auto			FlipImageHorizontal(string filename) -> string;
auto			RenameImageInDB(string image_id, CMysql *) -> string;

auto			GetUserListInJSONFormat_BySearchString(const string &lookForKey, bool include_myself, CMysql *, CUser *) -> string;
auto			GetUsersID_BySearchString(const string &lookForKey, bool include_myself, CMysql *db, CUser *user) -> vector<string>;

auto      		GetGroupListInJSONFormat(string dbQuery, CMysql *, CUser *) -> string;

#endif
