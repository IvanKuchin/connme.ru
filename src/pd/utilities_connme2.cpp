#include "utilities_connme.h"

auto GetUsersID_BySearchString(const string &lookForKey, bool include_myself, CMysql *db, CUser *user) -> vector<string>
{
	MESSAGE_DEBUG("", "", "start(" + lookForKey + ")");

	vector<string>	result, searchWords;

	if(qw(lookForKey, searchWords))
	{
		// --- single search word can be name, surname, company name 
		if(searchWords.size() == 1)		
		{
			MESSAGE_DEBUG("", "", "single word search");

			// --- Looking through name, last name
			auto query = 
					"SELECT `users`.`id` FROM `users`"
					"LEFT JOIN `users_company` on `users_company`.`user_id` = `users`.`id` "
					"LEFT JOIN `company` on `company`.`id`=`users_company`.`company_id` "
					"WHERE "
					"("
						" `users`.`isActivated`='Y' AND `users`.`isblocked`='N' "
						+ (include_myself ? "" : " AND `users`.`id`!=\"" + user->GetID() + "\" ") +
					") "
					" AND "
					"("
						"("
							"`users`.`name` LIKE \"%" 	+ lookForKey + "%\" or `users`.`nameLast` LIKE \"%" 	+ lookForKey + "%\""
						")"
						" OR "
						"("
							"`users_company`.`current_company`='1' AND `company`.`name` LIKE \"%" + lookForKey + "%\""
						")"
					")";

			result = GetValuesFromDB(query, db);
		}
		if(searchWords.size() == 2)
		{
			MESSAGE_DEBUG("", "", "two words search");

			auto query = 
					"SELECT `users`.`id` FROM `users` "
					" LEFT JOIN `users_company` on `users_company`.`user_id` = `users`.`id` "
					" LEFT JOIN `company` on `company`.`id`=`users_company`.`company_id` "
					" WHERE "
					"("
						" `users`.`isActivated`='Y' AND `users`.`isblocked`='N' "
						+ (include_myself ? "" : " AND `users`.`id`!=\"" + user->GetID() + "\" ") +
					") "
					" AND "
					"("
						"("
							" `users_company`.`current_company`='1' "
							" AND "
							"( "
							" 	`company`.`name` like \"%" 	+ searchWords[0] + "%\" or "
							"	`company`.`name` like \"%" 	+ searchWords[1] + "%\" "
							")"
							" AND "
							"( "
							" 	`users`.`name` like \"%" 		+ searchWords[0] + "%\" or "
							" 	`users`.`name` like \"%" 		+ searchWords[1] + "%\" or "
							" 	`users`.`nameLast` like \"%" 	+ searchWords[0] + "%\" or "
							" 	`users`.`nameLast` like \"%" 	+ searchWords[1] + "%\" "
							" )"
						")"
						" OR "
						"("
							" ( "
							" 	`users`.`name` like \"%" 		+ searchWords[1] + "%\" AND "
							" 	`users`.`nameLast` like \"%" 	+ searchWords[0] + "%\" "
							" ) "
							" or "
							" ( "
							" 	`users`.`name` like \"%" 		+ searchWords[0] + "%\" AND "
							" 	`users`.`nameLast` like \"%" 	+ searchWords[1] + "%\" "
							" ) "
						")"
					")";

			result = GetValuesFromDB(query, db);
		}
		if(searchWords.size() == 3)
		{
			MESSAGE_DEBUG("", "", "three words search");

			auto	query = 
					"SELECT `users`.`id` FROM `users` "
					" LEFT JOIN `users_company` on `users_company`.`user_id` = `users`.`id` "
					" LEFT JOIN `company` on `company`.`id`=`users_company`.`company_id` "
					" WHERE "
					"("
						" `users`.`isActivated`='Y' AND `users`.`isblocked`='N' "
						+ (include_myself ? "" : " AND `users`.`id`!=\"" + user->GetID() + "\" ") +
					") "
					" AND "
					"("
						"("
							"`users_company`.`current_company`='1' and "
							"( "
							"	`company`.`name` like \"%" 		+ searchWords[0] + "%\" or "
							"	`company`.`name` like \"%" 		+ searchWords[1] + "%\" or "
							"	`company`.`name` like \"%" 		+ searchWords[2] + "%\" "
							")"
							" AND "
							"( "
							"	`users`.`name` like \"%" 		+ searchWords[0] + "%\" or "
							"	`users`.`name` like \"%" 		+ searchWords[1] + "%\" or "
							"	`users`.`name` like \"%" 		+ searchWords[2] + "%\" or "
							"	`users`.`nameLast` like \"%" 	+ searchWords[0] + "%\" or "
							"	`users`.`nameLast` like \"%" 	+ searchWords[1] + "%\" or "
							"	`users`.`nameLast` like \"%" 	+ searchWords[2] + "%\""
							")"
						")"
						" OR "
						"("
							"("
							"	`users`.`name` like \"%" 		+ searchWords[1] + "%\" and "
							"	`users`.`nameLast` like \"%" 	+ searchWords[0] + "%\" "
							")"
							" OR "
							"("
							"	`users`.`name` like \"%" 		+ searchWords[0] + "%\" and "
							"	`users`.`nameLast` like \"%" 	+ searchWords[1] + "%\" "
							")"
							" OR "
							"("
							"	`users`.`name` like \"%" 		+ searchWords[2] + "%\" and "
							"	`users`.`nameLast` like \"%" 	+ searchWords[0] + "%\" "
							")"
							" OR "
							"("
							"	`users`.`name` like \"%" 		+ searchWords[0] + "%\" and "
							"	`users`.`nameLast` like \"%" 	+ searchWords[2] + "%\" "
							")"
							" OR "
							"("
							"	`users`.`name` like \"%" 		+ searchWords[1] + "%\" and "
							"	`users`.`nameLast` like \"%" 	+ searchWords[2] + "%\" "
							")"
							" OR "
							"("
							"	`users`.`name` like \"%" 		+ searchWords[2] + "%\" and "
							"	`users`.`nameLast` like \"%" 	+ searchWords[1] + "%\" "
							")"
						")"
					")";

			result = GetValuesFromDB(query, db);
		}
	}
	else
	{
		MESSAGE_DEBUG("", "", "search request is empty");
	}

	MESSAGE_DEBUG("", "", "finish");

	return result;
}

auto GetUserListInJSONFormat_BySearchString(const string &lookForKey, bool include_myself, CMysql *db, CUser *user) -> string
{
	MESSAGE_DEBUG("", "", "start(" + lookForKey + ")");

	auto			users_id	= GetUsersID_BySearchString(lookForKey, include_myself, db, user);
	auto			result		= (users_id.size() ? GetUserListInJSONFormat("SELECT * FROM `users` WHERE `id` IN (" + join(users_id, ",") + ");", db, user) : "");

	MESSAGE_DEBUG("", "", "finish");

	return result;
}

