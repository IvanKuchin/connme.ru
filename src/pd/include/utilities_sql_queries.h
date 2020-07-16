#ifndef __UTILITIES_SQL_QUERIES__H__
#define __UTILITIES_SQL_QUERIES__H__

#include <string>

inline auto Get_UserIDByImageID_sqlquery(const string &id)
{
	return (
		"SELECT `userId` FROM `feed` WHERE `actionTypeId`=\"11\" AND `actionId` IN ("
			"SELECT `id` FROM `feed_message` WHERE `imageSetID` IN ("
				"SELECT `setID` FROM `feed_images` WHERE `id` IN (" + quoted(id) + ")"
			")"
		") "
		);
}

inline auto Get_SetIDByImageID_sqlquery(const string &id)
{
	return (
				"SELECT `setID` FROM `feed_images` WHERE `id` IN (" + quoted(id) + ")"
		);
}

inline auto Get_MessageIDByImageID_sqlquery(const string &id)
{
	return (
				"SELECT `id` FROM `feed_message` WHERE `imageSetID` IN (" + Get_SetIDByImageID_sqlquery(id) + ")"
		);
}

inline auto Get_OrderByImageID_sqlquery(const string &id)
{
	return (
				"SELECT `order` FROM `feed_images` WHERE `id` IN (" + quoted(id) + ")"
		);
}

inline auto Get_MaxOrderByImageID_sqlquery(const string &id)
{
	return (
			"SELECT MAX(`order`) FROM `feed_images` WHERE `setID` IN ("
				"SELECT `setID` FROM `feed_images` WHERE `id` IN (" + quoted(id) + ")"
			")"
		);
}

#endif
