#ifndef __UTILITIES_CONNME__H__
#define __UTILITIES_CONNME__H__

#include "c_cache_obj.h"
#include "utilities.h"

string      	GetUserListInJSONFormat(string dbQuery, CMysql *, CUser *);
string      	GetCompanyListInJSONFormat(string dbQuery, CMysql *, CUser *, bool quickSearch = true, bool includeEmployedUsersList = false);
string 			GetNewsFeedInJSONFormat(string whereStatement, int currPage, int newsOnSinglePage, CUser *, CMysql *);
string      	GetMessageLikesUsersList(string messageID, CUser *, CMysql *);
string 			GetBookLikesUsersList(string usersBookID, CUser *, CMysql *);
string 			GetLanguageLikesUsersList(string usersLanguageID, CUser *, CMysql *);
string 			GetCompanyLikesUsersList(string usersCompanyID, CUser *, CMysql *);
string 			GetCertificationLikesUsersList(string usersCertificationID, CUser *, CMysql *);
string 			GetCourseLikesUsersList(string usersCourseID, CUser *, CMysql *);
string 			GetUniversityDegreeLikesUsersList(string messageID, CUser *, CMysql *);
string 			GetBookRatingUsersList(string bookID, CUser *, CMysql *);
string      	GetUserNotificationSpecificDataByType(unsigned long typeID, unsigned long actionID, CMysql *, CUser *);
string      	GetUserNotificationInJSONFormat(string sqlRequest, CMysql *, CUser *);
string 			GetCandidatesListAppliedToVacancyInJSONFormat(string dbQuery, CMysql *);

auto			RotateImage(string filename, int degree) -> string;
auto			FlipImageVertical(string filename) -> string;
auto			FlipImageHorizontal(string filename) -> string;
auto			RenameImageInDB(string image_id, CMysql *) -> string;

#endif
