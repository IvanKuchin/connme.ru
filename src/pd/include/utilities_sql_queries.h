#ifndef __UTILITIES_SQL_QUERIES__H__
#define __UTILITIES_SQL_QUERIES__H__

#include <string>

inline auto Get_UserIDByImageID_sqlquery(const string &id)
{
	return (
		"SELECT `userId` FROM `feed` WHERE `actionTypeId`=\"11\" AND `actionId` IN ("
			"SELECT `id` FROM `feed_message` WHERE `imageSetID` IN ("
				"SELECT `setID` FROM `feed_images` WHERE `id` IN (" + id + ")"
			")"
		") "
		);
}

inline auto Get_UserIDByGroupID_sqlquery(const string &id)
{
	return (
				"SELECT `user_id` FROM `users_subscriptions` WHERE `entity_id` IN (" + id + ") and `entity_type`=\"group\""
		);
}

inline auto Get_OwnerUserIDByGroupID_sqlquery(const string &id)
{
	return (
				"SELECT `owner_id` FROM `groups` WHERE `id` IN (" + id + ")"
		);
}

inline auto Get_Groups_UserSubscribedTo_sqlquery(const string &id)
{
	return (
				"SELECT `entity_id` FROM `users_subscriptions` WHERE `user_id` IN (" + id + ") AND `entity_type`=\"group\""
		);
}

inline auto Get_SetIDByImageID_sqlquery(const string &id)
{
	return (
				"SELECT `setID` FROM `feed_images` WHERE `id` IN (" + id + ")"
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
				"SELECT `order` FROM `feed_images` WHERE `id` IN (" + id + ")"
		);
}

inline auto Get_MaxOrderByImageID_sqlquery(const string &id)
{
	return (
			"SELECT MAX(`order`) FROM `feed_images` WHERE `setID` IN ("
				"SELECT `setID` FROM `feed_images` WHERE `id` IN (" + id + ")"
			")"
		);
}

inline auto Get_UserRibbonsIDByUserID_sqlquery(const string &id)
{
	return (
				"SELECT `ribbon_id` FROM `users_ribbons` WHERE `user_id` IN (" + id + ")"
		);
}


#endif
