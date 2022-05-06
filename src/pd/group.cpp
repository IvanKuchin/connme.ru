#include "group.h"

int main()
{
	CStatistics		appStat;  // --- CStatistics must be a first statement to measure end2end param's
	CCgi			indexPage(EXTERNAL_TEMPLATE);
	CUser			user;
	c_config		config(CONFIG_DIR);
	string			action, partnerID;
	CMysql			db;
	struct timeval	tv;

	MESSAGE_DEBUG("", "", __FILE__);

	signal(SIGSEGV, crash_handler); 

	gettimeofday(&tv, NULL);
	srand(tv.tv_sec * tv.tv_usec * 100000);    /* Flawfinder: ignore */

	try
	{

		indexPage.ParseURL();

		if(!indexPage.SetProdTemplate("index.htmlt"))
		{
			MESSAGE_ERROR("", "", "template file was missing");
			throw CException("Template file was missing");
		}

		if(db.Connect(&config) < 0)
		{
			MESSAGE_ERROR("", "", "Can not connect to mysql database");
			throw CException("MySql connection");
		}

		indexPage.SetDB(&db);

		action = CheckHTTPParam_Text(indexPage.GetVarsHandler()->Get("action"));

		MESSAGE_DEBUG("", "", "action taken from HTTP is " + action);

		// ------------ generate common parts
		{
			if(RegisterInitialVariables(&indexPage, &db, &user))
			{
			}
			else
			{
				MESSAGE_ERROR("", "", "RegisterInitialVariables failed, throwing exception");
				throw CExceptionHTML("environment variable error");
			}

			action = GenerateSession(action, &config, &indexPage, &db, &user);
		}
		// ------------ end generate common parts

		MESSAGE_DEBUG("", "", "pre-condition if(action == \"" + action + "\")");

		if(action == "getGroupWall")
		{
			ostringstream	ost;
			string			id = "", link = "";

			MESSAGE_DEBUG("", action, "start");

/*
			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				MESSAGE_DEBUG("", action, "re-login required");

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str());
			}
*/
			id = CheckHTTPParam_Number(indexPage.GetVarsHandler()->Get("id"));
			link = CheckHTTPParam_Text(indexPage.GetVarsHandler()->Get("link"));

			if(!id.length())
			{
				if(link.length())
				{
					if(db.Query("SELECT `id` FROM `groups` WHERE `link`=\"" + link + "\";"))
					{
						id = db.Get(0, "id");
					}
					else
					{
						MESSAGE_ERROR("", action, "group.link(" + link + ") not found");
					}
				}
				else
				{
					MESSAGE_ERROR("", action, "id and link are empty");

				}
			}

			if(!link.length())
			{
				if(id.length())
				{
					if(db.Query("SELECT `link` FROM `groups` WHERE `id`=\"" + id + "\";"))
					{
						link = db.Get(0, "link");
					}
					else
					{
						MESSAGE_ERROR("", action, "group.id(" + id + ") not found");
					}
				}
				else
				{
					MESSAGE_ERROR("", action, "id and link are empty");

				}
			}

			if(id.length()) indexPage.RegisterVariableForce("id", id);
			if(link.length()) indexPage.RegisterVariableForce("link", link);

			if(!indexPage.SetProdTemplate("view_group_profile.htmlt"))
			{
				MESSAGE_ERROR("", action, "template file getGroupWall.htmlt was missing");
				throw CException("Template file was missing");
			}
		}

	    if(action == "AJAX_getGroupWall")
	    {
			auto			success_message = ""s;
			auto			error_message = ""s;
	        auto            currPage = 0, newsOnSinglePage = 0;
	        auto			result = ""s;

	        auto			strNewsOnSinglePage = CheckHTTPParam_Number(indexPage.GetVarsHandler()->Get("NewsOnSinglePage"));
	        auto			strPageToGet        = CheckHTTPParam_Number(indexPage.GetVarsHandler()->Get("page"));
	        auto			groupLink			= CheckHTTPParam_Text(indexPage.GetVarsHandler()->Get("link"));
	        auto			groupID				= CheckHTTPParam_Number(indexPage.GetVarsHandler()->Get("id"));

	        if(strPageToGet.empty()) strPageToGet = "0";

			MESSAGE_DEBUG("", action, "page " + strPageToGet + " requested");

	        try{
	            currPage = stoi(strPageToGet);
	        } catch(...) {
	            currPage = 0;
	        }

	        try{
	            newsOnSinglePage = stoi(strNewsOnSinglePage);
	        } catch(...) {
	            newsOnSinglePage = 30;
	        }

	/*
	        if(user.GetLogin() == "Guest")
	        {
	            ostringstream   ost;

                MESSAGE_DEBUG("", action, "re-login required");

	            ost.str("");
	            ost << "/?rand=" << GetRandom(10);
	            indexPage.Redirect(ost.str());
	        }
	*/
			if(groupLink.length())
			{
				auto	isBlocked = GetValueFromDB("SELECT `isBlocked` FROM `groups` WHERE `link`=\"" + groupLink + "\";", &db);

				if(isBlocked == "N")
				{
					result = GetNewsFeedInJSONFormat(" ((`feed`.`dstType`=\"group\") AND `feed`.`dstID` IN (SELECT `id` FROM `groups` WHERE `link`=\"" + groupLink + "\" AND `isBlocked`=\"N\")) ", currPage, newsOnSinglePage, &user, &db);
				}
				else if(isBlocked == "Y")
				{
					MESSAGE_DEBUG("", "", "group.link(" + groupLink + ") blocked");
				}
				else
				{
					MESSAGE_ERROR("", action, "group.link(" + groupLink + ") not found");
				}
			}
			else if(groupID.length())
			{
				auto	isBlocked = GetValueFromDB("SELECT `isBlocked` FROM `groups` WHERE `id`=\"" + groupID + "\";", &db);

				if(isBlocked == "N")
				{
					result = GetNewsFeedInJSONFormat(" ((`feed`.`dstType`=\"group\") AND `feed`.`dstID` IN (SELECT `id` FROM `groups` WHERE `id`=\"" + groupID + "\" AND `isBlocked`=\"N\")) ", currPage, newsOnSinglePage, &user, &db);
				}
				else if(isBlocked == "Y")
				{
					MESSAGE_DEBUG("", "", "group.id(" + groupID + ") blocked");
				}
				else
				{
					MESSAGE_ERROR("", action, "group.id(" + groupID + ") not found");
				}
			}
			else
			{
                MESSAGE_ERROR("", action, "mandatory parameter missed");
			}

			success_message = "\"feed\":[" + result + "]";

			AJAX_ResponseTemplate(&indexPage, success_message, error_message);

			MESSAGE_DEBUG("", action, "finish");
	    }

		if(action == "groups_i_own_list")
		{
			ostringstream	ost;
			string			strPageToGet, strFriendsOnSinglePage;

			MESSAGE_DEBUG("", action, "start");

			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				MESSAGE_DEBUG("", action, " re-login required");

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str());
			}
			else
			{
				indexPage.RegisterVariableForce("title_head", "Мои группы");

				strFriendsOnSinglePage	= indexPage.GetVarsHandler()->Get("FriendsOnSinglePage");
				strPageToGet 			= indexPage.GetVarsHandler()->Get("page");
				if(strPageToGet.empty()) strPageToGet = "0";
				MESSAGE_DEBUG("", action, "page " + strPageToGet + " requested");

				indexPage.RegisterVariableForce("myFirstName", user.GetName());
				indexPage.RegisterVariableForce("myLastName", user.GetNameLast());


				if(!indexPage.SetProdTemplate("groups_i_own_list.htmlt"))
				{
					MESSAGE_ERROR("", action, "can't find template my_network.htmlt");
					throw CExceptionHTML("user not activated");
				}
			}


			MESSAGE_DEBUG("", action, "finish");
		}

		if(action == "AJAX_getGroupProfile")
		{
			ostringstream   ostResult;

			MESSAGE_DEBUG("", action, "start");

			ostResult.str("");
			if(user.GetLogin() == "Guest")
			{
				MESSAGE_DEBUG("", action, "re-login required");

				ostResult << "{\"result\":\"error\",\"description\":\"сессия закончилась, необходимо вновь зайти на сайт\"}";
			}
			else
			{
				string		groupID;

				groupID = CheckHTTPParam_Number(indexPage.GetVarsHandler()->Get("id"));

				if(groupID.length())
				{
					ostringstream		ost;

					ost.str("");
					ost << "SELECT * FROM `groups` WHERE `id`=\"" << groupID << "\" AND (`isBlocked`='N' OR `owner_id`=\"" << user.GetID() << "\");";

					ostResult	<< "{"
								<< "\"result\":\"success\","
								<< "\"groups\":[" << GetGroupListInJSONFormat(ost.str(), &db, &user) << "]"
								<< "}";
				}
				else
				{
					MESSAGE_ERROR("", action, "in groupID [" + groupID + "]");

					ostResult << "{\"result\":\"error\",\"description\":\"ERROR in groupID\",\"groups\":[]}";
				}
			}



			indexPage.RegisterVariableForce("result", ostResult.str());

			if(!indexPage.SetProdTemplate("json_response.htmlt"))
			{
				MESSAGE_ERROR("", action, "template file json_response.htmlt was missing");
				throw CException("Template file json_response.htmlt was missing");
			}  // if(!indexPage.SetProdTemplate("AJAX_precreateNewOpenVacancy.htmlt"))

			MESSAGE_DEBUG("", action, "start");
		}

		if((action == "AJAX_blockGroup") || (action == "AJAX_unblockGroup"))
		{
			ostringstream   ostResult;

			MESSAGE_DEBUG("", action, "start");

			ostResult.str("");
			if(user.GetLogin() == "Guest")
			{
				MESSAGE_DEBUG("", action, "re-login required");

				ostResult << "{\"result\":\"error\",\"description\":\"re-login required\"}";
			}
			else
			{
				string		groupID;

				groupID = CheckHTTPParam_Number(indexPage.GetVarsHandler()->Get("id"));

				if(groupID.length())
				{
					if(db.Query("SELECT `id` FROM `groups` WHERE `id`=\"" + groupID + "\" AND `owner_id`=\"" + user.GetID() + "\";"))
					{
						string	dbQuery;

						if(action == "AJAX_blockGroup")
							dbQuery = "UPDATE `groups` SET `isBlocked`=\"Y\" WHERE `id`=\"" + groupID + "\";";
						if(action == "AJAX_unblockGroup")
							dbQuery = "UPDATE `groups` SET `isBlocked`=\"N\" WHERE `id`=\"" + groupID + "\";";

						db.Query(dbQuery);
						if(!db.isError())
						{
							ostResult	<< "{"
										<< "\"result\":\"success\","
										<< "\"groups\":[" << GetGroupListInJSONFormat("SELECT * FROM `groups` WHERE `id`=\"" + groupID + "\";", &db, &user) << "]"
										<< "}";
						}
						else
						{

							MESSAGE_ERROR("", action, ":ERROR: updating DB");

							ostResult.str("");
							ostResult << "{";
							ostResult << "\"result\" : \"error\",";
							ostResult << "\"description\" : \"ошибка БД\"";
							ostResult << "}";
						}
					}
					else
					{
						MESSAGE_ERROR("", action, ":ERROR: groupID [" + groupID + "] doesn't belongs to you");

						ostResult << "{\"result\":\"error\",\"description\":\"вы не можете редактировать группу\",\"groups\":[]}";
					}

				}
				else
				{
					MESSAGE_ERROR("", action, "in groupID [" + groupID + "]");

					ostResult << "{\"result\":\"error\",\"description\":\"ERROR in groupID\",\"groups\":[]}";
				}
			}



			indexPage.RegisterVariableForce("result", ostResult.str());

			if(!indexPage.SetProdTemplate("json_response.htmlt"))
			{
				MESSAGE_ERROR("", action, "template file json_response.htmlt was missing");
				throw CException("Template file json_response.htmlt was missing");
			}  // if(!indexPage.SetProdTemplate("AJAX_precreateNewOpenVacancy.htmlt"))

			MESSAGE_DEBUG("", action, "start");
		}

		if(action == "AJAX_getGroupProfileAndUser")
		{
			ostringstream   ostResult;

			MESSAGE_DEBUG("", action, "start");

			ostResult.str("");
/*
			if(user.GetLogin() == "Guest")
			{
				MESSAGE_DEBUG("", action, "re-login required");

				ostResult << "{\"result\":\"error\",\"description\":\"сессия закончилась, необходимо вновь зайти на сайт\"}";
			}
			else
*/
			{
				string			groupID, groupLink, strNewsOnSinglePage, strPageToGet;

				groupLink 			= CheckHTTPParam_Text(indexPage.GetVarsHandler()->Get("link"));
				groupID 			= CheckHTTPParam_Text(indexPage.GetVarsHandler()->Get("id"));

				if(groupLink.length() || groupID.length())
				{
					ostringstream		ost;

					ost.str("");

					if(groupLink.length())
						ost << "SELECT * FROM `groups` WHERE `link`=\"" << groupLink << "\" and `isBlocked`='N';";
					if(groupID.length())
						ost << "SELECT * FROM `groups` WHERE `id`=\"" << groupID << "\" and `isBlocked`='N';";

					ostResult	<< "{"
								<< "\"result\":\"success\","
								<< "\"groups\":[" << GetGroupListInJSONFormat(ost.str(), &db, &user) << "],"
								<< "\"users\":[" << GetUserListInJSONFormat("SELECT * FROM `users` WHERE `id`=\"" + user.GetID() + "\" AND `isblocked`=\"N\";", &db, &user) << "]"
								<< "}";
				}
				else
				{
					MESSAGE_ERROR("", action, "in groupLink [" + groupLink + "]");

					ostResult << "{\"result\":\"error\",\"description\":\"ERROR in groupLink\",\"groups\":[]}";
				}
			}



			indexPage.RegisterVariableForce("result", ostResult.str());

			if(!indexPage.SetProdTemplate("json_response.htmlt"))
			{
				MESSAGE_ERROR("", action, "template file json_response.htmlt was missing");
				throw CException("Template file json_response.htmlt was missing");
			}  // if(!indexPage.SetProdTemplate("AJAX_precreateNewOpenVacancy.htmlt"))

			MESSAGE_DEBUG("", action, "start");
		}

		// --- AJAX friend list for autocomplete
		if((action == "AJAX_getFindGroupsListAutocomplete") || (action == "AJAX_getFindGroupsList") || (action == "AJAX_getFindGroupByID") || (action == "AJAX_getMyGroupsList"))
		{
			ostringstream	ost, ostFinal;
			string			sessid, lookForKey, groupsList;
			vector<string>	searchWords;

			MESSAGE_DEBUG("", action, "start");

			// --- Initialization
			ostFinal.str("");

			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				MESSAGE_DEBUG("", action, " re-login required");

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str());
			}
			else
			{
				lookForKey = CheckHTTPParam_Text(indexPage.GetVarsHandler()->Get("lookForKey"));

				if( (lookForKey.length() >= 3) || ((action == "AJAX_getFindGroupByID") && lookForKey.length())  || (action == "AJAX_getMyGroupsList")) 
				{
					ostringstream	ost;

					// --- Looking through group name
					ost.str("");
					if(action == "AJAX_getFindGroupByID")
						ost << "SELECT * FROM `groups` WHERE (`isBlocked`='N' OR `owner_id`=\"" << user.GetID() << "\") AND (`id`=\"" << lookForKey << "\");";
					else if (action == "AJAX_getMyGroupsList")
						ost << "SELECT * FROM `groups` WHERE (`owner_id`=\"" << user.GetID() << "\") OR ((`isBlocked`='N') AND `id` IN (SELECT `entity_id` FROM `users_subscriptions` WHERE `entity_type`=\"group\" AND `user_id`=\"" + user.GetID() + "\"));";
					else
						ost << "SELECT * FROM `groups` WHERE (`isBlocked`='N' OR `owner_id`=\"" << user.GetID() << "\") AND ((`title` like \"%" << lookForKey << "%\") OR (`description` like \"%" << lookForKey << "%\"));";

					ostFinal << "{\"status\":\"success\",\"groups\":[" << GetGroupListInJSONFormat(ost.str(), &db, &user) << "]}";

				}
				else
				{
					MESSAGE_DEBUG("", action, " searching key is empty");
					ostFinal << "{\"status\":\"error\",\"description\":\"searching key is empty or less then 3\", \"groups\":[]}";
				}

				indexPage.RegisterVariableForce("result", ostFinal.str());

				if(!indexPage.SetProdTemplate("json_response.htmlt"))
				{
					MESSAGE_ERROR("", action, "template file json_response.htmlt was missing");
					throw CException("Template file was missing");
				}
			}

			MESSAGE_DEBUG("", action, "final response [length = " + to_string(ostFinal.str().length()) + "]");
		}

		if((action == "AJAX_SubscribeOnGroup") || (action == "AJAX_UnsubscribeFromGroup"))
		{
			ostringstream   ostResult;
			MESSAGE_DEBUG("", action, "start");

			ostResult.str("");
			if(user.GetLogin() == "Guest")
			{
				ostringstream   ost;

				MESSAGE_DEBUG("", action, "re-login required");

				ostResult << "{\"result\":\"error\",\"description\":\"сессия закончилась, необходимо вновь зайти на сайт\"}";
			}
			else
			{
				string		  groupID = CheckHTTPParam_Number(indexPage.GetVarsHandler()->Get("id"));

				ostResult	<< "{" << (action == "AJAX_SubscribeOnGroup" ? SubscribeToGroup(groupID, &user, &db) : UnsubscribeFromGroup(groupID, &user, &db)) << ",";
				ostResult	<< "\"subscriptions\":[" << GetUserSubscriptionsInJSONFormat("SELECT * FROM `users_subscriptions` WHERE `user_id`=\"" + user.GetID() + "\";", &db) << "]";
				ostResult	<< "}";
			}

			indexPage.RegisterVariableForce("result", ostResult.str());
			if(!indexPage.SetProdTemplate("json_response.htmlt"))
			{
				MESSAGE_ERROR("", action, "can't find template json_response.htmlt");
				throw CExceptionHTML("user not activated");
			}

			MESSAGE_DEBUG("", action, "finish");
		}
		if(action == "edit_group")
		{
			ostringstream	ost;

			MESSAGE_DEBUG("", action, "start");

			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				MESSAGE_DEBUG("", action, " re-login required");

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str());
			}

			indexPage.RegisterVariableForce("title", "Редактирование группы");

			if(!indexPage.SetProdTemplate("edit_group.htmlt"))
			{
				MESSAGE_ERROR("", action, "template file edit_group.htmlt was missing");
				throw CException("Template file edit_group.htmlt was missing");
			}  // if(!indexPage.SetProdTemplate("edit_group.htmlt"))

			MESSAGE_DEBUG("", action, "end");
		} 	// if(action == "edit_group")

		if(action == "createnewgroup")
		{
			ostringstream	ost;

			MESSAGE_DEBUG("", action, "start");

			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				MESSAGE_DEBUG("", action, " re-login required");

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str());
			}

			indexPage.RegisterVariableForce("title", "Новая группа");

			if(!indexPage.SetProdTemplate("create_group.htmlt"))
			{
				MESSAGE_ERROR("", action, "template file createnewgroup.htmlt was missing");
				throw CException("Template file createnewgroup.htmlt was missing");
			}  // if(!indexPage.SetProdTemplate("createnewgroup.htmlt"))

			MESSAGE_DEBUG("", action, "end");
		} 	// if(action == "createnewgroup")

		if(action == "AJAX_updateGroupTitle")
		{
			string			value, id;
			ostringstream	ostFinal;

			MESSAGE_DEBUG("", action, "start");

			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				MESSAGE_DEBUG("", action, " re-login required");

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str());
			}

			value = CheckHTTPParam_Text(indexPage.GetVarsHandler()->Get("value"));
			id = CheckHTTPParam_Number(indexPage.GetVarsHandler()->Get("groupid"));
			ostFinal.str("");

			if(value.length() && id.length()) 
			{
				if(db.Query("SELECT `id` FROM `groups` WHERE `id`=\"" + id + "\" AND `owner_id`=\"" + user.GetID() + "\";"))
				{
					db.Query("UPDATE `groups` SET `title`=\"" + value + "\"  WHERE `id`=\"" + id + "\";");

					if(!db.isError())
					{
						ostFinal.str("");
						ostFinal << "{";
						ostFinal << "\"result\" : \"success\",";
						ostFinal << "\"description\" : \"\"";
						ostFinal << "}";
					}
					else
					{

						{
							MESSAGE_ERROR("", action, ":ERROR: updating DB");
						}

						ostFinal.str("");
						ostFinal << "{";
						ostFinal << "\"result\" : \"error\",";
						ostFinal << "\"description\" : \"error updating DB\"";
						ostFinal << "}";
					}
				}
				else
				{
					{
						MESSAGE_ERROR("", action, ":ERROR: user.id(" + user.GetID() + ") is not a group(" + id + ") owner");
					}

					ostFinal.str("");
					ostFinal << "{";
					ostFinal << "\"result\" : \"error\",";
					ostFinal << "\"description\" : \"вы не можете изменить данные группы\"";
					ostFinal << "}";
				}
			}
			else
			{
				ostringstream	ost;
				{
					MESSAGE_ERROR("", action, ": id(" + id + ") or value(" + value + ") is empty");
				}

				ostFinal.str("");
				ostFinal << "{";
				ostFinal << "\"result\" : \"error\",";
				ostFinal << "\"description\" : \"пустые параметры id или value\"";
				ostFinal << "}";
			}

			indexPage.RegisterVariableForce("result", ostFinal.str());

			if(!indexPage.SetProdTemplate("json_response.htmlt"))
			{
				MESSAGE_ERROR("", action, "template file json_response.htmlt was missing");
				throw CException("Template file was missing");
			}

			MESSAGE_DEBUG("", action, "end");

		}

		if(action == "AJAX_updateGroupDescription")
		{
			string			value, id;
			ostringstream	ostFinal;

			MESSAGE_DEBUG("", action, "start");

			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				MESSAGE_DEBUG("", action, " re-login required");

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str());
			}

			value = CheckHTTPParam_Text(indexPage.GetVarsHandler()->Get("description"));
			id = CheckHTTPParam_Number(indexPage.GetVarsHandler()->Get("groupid"));
			ostFinal.str("");

			if(value.length() && id.length()) 
			{
				if(db.Query("SELECT `id` FROM `groups` WHERE `id`=\"" + id + "\" AND `owner_id`=\"" + user.GetID() + "\";"))
				{
					db.Query("UPDATE `groups` SET `description`=\"" + value + "\"  WHERE `id`=\"" + id + "\";");

					if(!db.isError())
					{
						ostFinal.str("");
						ostFinal << "{";
						ostFinal << "\"result\" : \"success\",";
						ostFinal << "\"description\" : \"\"";
						ostFinal << "}";
					}
					else
					{

						{
							MESSAGE_ERROR("", action, ":ERROR: updating DB");
						}

						ostFinal.str("");
						ostFinal << "{";
						ostFinal << "\"result\" : \"error\",";
						ostFinal << "\"description\" : \"error updating DB\"";
						ostFinal << "}";
					}
				}
				else
				{
					{
						MESSAGE_ERROR("", action, ":ERROR: user.id(" + user.GetID() + ") is not a group(" + id + ") owner");
					}

					ostFinal.str("");
					ostFinal << "{";
					ostFinal << "\"result\" : \"error\",";
					ostFinal << "\"description\" : \"вы не можете изменить данные группы\"";
					ostFinal << "}";
				}
			}
			else
			{
				ostringstream	ost;
				{
					MESSAGE_ERROR("", action, ": id(" + id + ") or value(" + value + ") is empty");
				}

				ostFinal.str("");
				ostFinal << "{";
				ostFinal << "\"result\" : \"error\",";
				ostFinal << "\"description\" : \"пустые параметры id или value\"";
				ostFinal << "}";
			}

			indexPage.RegisterVariableForce("result", ostFinal.str());

			if(!indexPage.SetProdTemplate("json_response.htmlt"))
			{
				MESSAGE_ERROR("", action, "template file json_response.htmlt was missing");
				throw CException("Template file was missing");
			}

			MESSAGE_DEBUG("", action, "end");

		}

		if(action == "AJAX_createGroup")
		{
			string			title, link, description;
			long int		id;
			ostringstream	ostFinal;

			MESSAGE_DEBUG("", action, "start");

			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				MESSAGE_DEBUG("", action, " re-login required");

				ostFinal.str("");
				ostFinal << "{";
				ostFinal << "\"result\" : \"error\",";
				ostFinal << "\"link\" : \"/autologin?rand=" << GetRandom(10) << "\",";
				ostFinal << "\"description\" : \"re-login required\"";
				ostFinal << "}";
			}
			else
			{
				title = CheckHTTPParam_Text(indexPage.GetVarsHandler()->Get("title"));
				link = toLower(CheckHTTPParam_Text(indexPage.GetVarsHandler()->Get("link")));
				link = link.substr(0, 64);
				description = CheckHTTPParam_Text(indexPage.GetVarsHandler()->Get("description"));

				ostFinal.str("");
				if(title.length() == 0) 
				{
					ostringstream	ost;
					{
						MESSAGE_ERROR("", action, ": title(" + title + ") is empty");
					}

					ostFinal.str("");
					ostFinal << "{";
					ostFinal << "\"result\" : \"error\",";
					ostFinal << "\"description\" : \"Группе необходимо название\"";
					ostFinal << "}";
				}
				else if(link.length() && (link.length() < 12))
				{
					// --- if link is empty it became equal to group id
					// --- maximum length group.id is 11
					// --- to avoid overlapping between group.links, it must be longer than 11 
					MESSAGE_DEBUG("", action, "link[" + link + "] too short");

					ostFinal.str("");
					ostFinal << "{";
					ostFinal << "\"result\" : \"error\",";
					ostFinal << "\"link\" : \"" + link + "\",";
					ostFinal << "\"description\" : \"ссылка должна быть длиннее 10 символов\"";
					ostFinal << "}";
				}
				else if(link.find_first_not_of("abcdefghijklmnopqrstuvwxyz_1234567890") != string::npos)
				{
					MESSAGE_DEBUG("", action, "link[" + link + "] contains prohibited symbols");

					ostFinal.str("");
					ostFinal << "{";
					ostFinal << "\"result\" : \"error\",";
					ostFinal << "\"link\" : \"" + link + "\",";
					ostFinal << "\"description\" : \"ссылка можеть содержать только английские буквы или цифры\"";
					ostFinal << "}";
				}
				else if(title.length() && db.Query("SELECT `id` FROM `groups` WHERE `title`=\"" + title + "\";"))
				{
					MESSAGE_DEBUG("", action, "group[" + title + "] already exists");
					ostFinal.str("");
					ostFinal << "{";
					ostFinal << "\"result\" : \"error\",";
					ostFinal << "\"description\" : \"группа с таким названием уже существует\"";
					ostFinal << "}";
				}
				else if(link.length() && db.Query("SELECT `id` FROM `groups` WHERE `link`=\"" + link + "\";"))
				{
					MESSAGE_DEBUG("", action, "link[" + link + "] already exists");
					ostFinal.str("");
					ostFinal << "{";
					ostFinal << "\"result\" : \"error\",";
					ostFinal << "\"description\" : \"такая ссылка уже существует\"";
					ostFinal << "}";
				}
				else
				{
					id = db.InsertQuery("INSERT INTO `groups` SET `link`=\"" + link + "\","
																"`title`=\"" + title + "\","
																"`description`=\"" + description + "\","
																"`owner_id`=\"" + user.GetID() + "\","
																"`eventTimestampCreation`=UNIX_TIMESTAMP()"
																";");

					if(id)
					{
						if(!link.length()) 
						{
							db.Query("UPDATE `groups` SET `link`=\"" + to_string(id) + "\" WHERE `id`=\"" + to_string(id) + "\";");
							if(db.isError())
							{
								MESSAGE_ERROR("", action, ":ERROR: updating `group` table");
							}
						}

						ostFinal.str("");
						ostFinal << "{";
						ostFinal << "\"result\" : \"success\",";
						ostFinal << "\"groups\" : [" << GetGroupListInJSONFormat("SELECT * FROM `groups` WHERE `id`=\"" + to_string(id) + "\";", &db, &user) << "],";
						ostFinal << "\"description\" : \"\"";
						ostFinal << "}";

						db.InsertQuery("INSERT INTO `users_subscriptions` SET `user_id`=\"" + user.GetID() + "\","
																			"`entity_type`=\"group\","
																			"`entity_id`=\"" + to_string(id) + "\","
																			"`eventTimestamp`=UNIX_TIMESTAMP()"
																			";");

						// --- insert notification into feed
						if(db.Query("SELECT `id` FROM `feed` WHERE `userId`=\"" + user.GetID() + "\" AND `srcType`=\"user\" AND `actionTypeId`=\"65\" AND `actionId`=\"" + to_string(id) + "\";"))
						{
							MESSAGE_DEBUG("", action, "[groupID: " + to_string(id) + ", userID: " + user.GetID() + "]");
						}
						else
						{
							if(db.InsertQuery("INSERT INTO `feed` (`userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"" + user.GetID() + "\", \"65\", \"" + to_string(id) + "\", NOW())"))
							{
							}
							else
							{
								MESSAGE_ERROR("", action, "inserting into `feed` table");
							}
						}
					}
					else
					{

						{
							MESSAGE_ERROR("", action, ":ERROR: inserting into `group` table");
						}

						ostFinal.str("");
						ostFinal << "{";
						ostFinal << "\"result\" : \"error\",";
						ostFinal << "\"description\" : \"ошибка БД\"";
						ostFinal << "}";
					}
				}
			}

			indexPage.RegisterVariableForce("result", ostFinal.str());

			if(!indexPage.SetProdTemplate("json_response.htmlt"))
			{
				MESSAGE_ERROR("", action, "template file json_response.htmlt was missing");
				throw CException("Template file was missing");
			}

			MESSAGE_DEBUG("", action, "end");

		}


		if(action == "AJAX_updateGroupLink")
		{
			string			value, id;
			ostringstream	ostFinal;

			MESSAGE_DEBUG("", action, "start");

			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				MESSAGE_DEBUG("", action, " re-login required");

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str());
			}

			value = toLower(CheckHTTPParam_Text(indexPage.GetVarsHandler()->Get("value")));
			value = value.substr(0, 64);
			id = CheckHTTPParam_Number(indexPage.GetVarsHandler()->Get("groupid"));
			ostFinal.str("");

			if(value.length() && id.length()) 
			{
				if(db.Query("SELECT `link` FROM `groups` WHERE `id`=\"" + id + "\" AND `owner_id`=\"" + user.GetID() + "\";"))
				{
					string	originalLink = db.Get(0, "link");

					if(value.length() < 10)
					{
						MESSAGE_DEBUG("", action, "link[" + value + "] is too short, must be longer than 10 symbols");

						ostFinal.str("");
						ostFinal << "{";
						ostFinal << "\"result\" : \"error\",";
						ostFinal << "\"link\" : \"" + originalLink + "\",";
						ostFinal << "\"description\" : \"ссылка должна быть минимум 10 символов\"";
						ostFinal << "}";
					}
					else if(value.find_first_not_of("abcdefghijklmnopqrstuvwxyz_-1234567890") != string::npos)
					{
						MESSAGE_DEBUG("", action, "link[" + value + "] contains prohibited symbols");

						ostFinal.str("");
						ostFinal << "{";
						ostFinal << "\"result\" : \"error\",";
						ostFinal << "\"link\" : \"" + originalLink + "\",";
						ostFinal << "\"description\" : \"ссылка можеть содержать только английские буквы или цифры\"";
						ostFinal << "}";
					}
					else if(db.Query("SELECT `id` FROM `groups` WHERE `link`=\"" + value + "\";"))
					{
						MESSAGE_DEBUG("", action, "link[" + value + "] contains prohibited symbols");

						ostFinal.str("");
						ostFinal << "{";
						ostFinal << "\"result\" : \"error\",";
						ostFinal << "\"link\" : \"" + originalLink + "\",";
						ostFinal << "\"description\" : \"группа(" + value + ") уже существует\"";
						ostFinal << "}";
					}
					else
					{
						db.Query("UPDATE `groups` SET `link`=\"" + value + "\"  WHERE `id`=\"" + id + "\";");

						if(!db.isError())
						{
							ostFinal.str("");
							ostFinal << "{";
							ostFinal << "\"result\" : \"success\",";
							ostFinal << "\"description\" : \"\"";
							ostFinal << "}";
						}
						else
						{

							MESSAGE_ERROR("", action, "updating DB");

							ostFinal.str("");
							ostFinal << "{";
							ostFinal << "\"result\" : \"error\",";
							ostFinal << "\"link\" : \"" + originalLink + "\",";
							ostFinal << "\"description\" : \"ошибка БД\"";
							ostFinal << "}";
						}
					}
				}
				else
				{
					MESSAGE_ERROR("", action, "user.id(" + user.GetID() + ") is not a group(" + id + ") owner");

					ostFinal.str("");
					ostFinal << "{";
					ostFinal << "\"result\" : \"error\",";
					ostFinal << "\"description\" : \"вы не можете изменить данные группы\"";
					ostFinal << "}";
				}
			}
			else
			{
				MESSAGE_ERROR("", action, "id(" + id + ") or value(" + value + ") is empty");

				ostFinal.str("");
				ostFinal << "{";
				ostFinal << "\"result\" : \"error\",";
				ostFinal << "\"description\" : \"пустые параметры id или value\"";
				ostFinal << "}";
			}

			indexPage.RegisterVariableForce("result", ostFinal.str());

			if(!indexPage.SetProdTemplate("json_response.htmlt"))
			{
				MESSAGE_ERROR("", action, "template file json_response.htmlt was missing");
				throw CException("Template file was missing");
			}

			MESSAGE_DEBUG("", action, "end");

		}

		if(action == "AJAX_deleteGroup")
		{
			MESSAGE_DEBUG("", action, "start");

			auto	success_message = ""s;
			auto	error_message = ""s;

			if(user.GetLogin() == "Guest")
			{
				error_message = "re-login required";
				MESSAGE_DEBUG("", action, error_message);
			}
			else
			{
				auto	groupID = CheckHTTPParam_Number(indexPage.GetVarsHandler()->Get("id"));

				if(AmIGroupOwner(groupID, &db, &user))
				{
					// --- delete group messages
					auto	messageIDs = GetValuesFromDB("SELECT `actionId` FROM `feed` WHERE `actionTypeId` IN (11, 12) AND `dstType`=\"group\" AND `dstID`=\"" + groupID + "\";", &db);

					error_message = DeleteMessageByID(join(messageIDs), &db, &user);
					if(error_message.empty())
					{
						// --- delete group metadata
						error_message = DeleteGroupByID(groupID, &db, &user, &config);
						if(error_message.empty())
						{
						}
						else
						{
							MESSAGE_ERROR("", action, error_message);	
						}
					}
					else
					{
						MESSAGE_ERROR("", action, error_message);	
					}
				}
				else
				{
					error_message = gettext("you are not authorized");
					MESSAGE_ERROR("", action, error_message);
				}
			}

			AJAX_ResponseTemplate(&indexPage, success_message, error_message);

			MESSAGE_DEBUG("", action, "start");
		}

		MESSAGE_DEBUG("", action, " end (action's == \"" + action + "\") condition");

		indexPage.OutTemplate();

	}
	catch(CExceptionHTML &c)
	{
		c.SetLanguage(indexPage.GetLanguage());
		c.SetDB(&db);

		MESSAGE_DEBUG("", action, "catch CExceptionHTML: DEBUG exception reason: [" + c.GetReason() + "]");

		if(!indexPage.SetProdTemplate(c.GetTemplate()))
		{
			MESSAGE_ERROR("", "", "template (" + c.GetTemplate() + ") not found");
			return(-1);
		}

		indexPage.RegisterVariable("content", c.GetReason());
		indexPage.OutTemplate();

		return(-1);
	}
	catch(CException &c)
	{
		MESSAGE_ERROR("", action, "catch CException: exception: ERROR  " + c.GetReason());

		if(!indexPage.SetProdTemplate("error.htmlt"))
		{
			MESSAGE_ERROR("", "", "template not found");
			return(-1);
		}

		indexPage.RegisterVariable("content", c.GetReason());
		indexPage.OutTemplate();

		return(-1);
	}
	catch(exception& e)
	{
		MESSAGE_ERROR("", action, "catch(exception& e): catch standard exception: ERROR  " + e.what());

		if(!indexPage.SetProdTemplate("error.htmlt"))
		{
			MESSAGE_ERROR("", "", "template not found");
			return(-1);
		}
		
		indexPage.RegisterVariable("content", e.what());
		indexPage.OutTemplate();

		return(-1);
	}

	return(0);
}
