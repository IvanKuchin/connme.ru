#include <sstream>
#include <stdexcept>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <exception>

#include "localy.h"
#include "ccgi.h"
#include "cvars.h"
#include "clog.h"
#include "cmysql.h"
#include "cexception.h"
#include "cactivator.h"
#include "cforum.h"
#include "cmenu.h"
#include "cuser.h"
#include "cmail.h"
#include "ccatalog.h"
#include "cstatistics.h"
#include "utilities.h"

int main()
{
    CStatistics			appStat;  // --- CStatistics must be firts statement to measure end2end param's
    CCgi				indexPage(EXTERNAL_TEMPLATE);
    CUser				user;
    string				action, partnerID;
    CMysql				db;
    struct timeval		tv;
    map<string,string>	mapResult;


    gettimeofday(&tv, NULL);
    srand(tv.tv_sec * tv.tv_usec * 100000);

    try
    {

		indexPage.ParseURL();

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file was missing");
			throw CException("Template file was missing");
		}

		if(db.Connect(DB_NAME, DB_LOGIN, DB_PASSWORD) < 0)
		{
			CLog	log;

			log.Write(ERROR, "Can not connect to mysql database");
			throw CExceptionHTML("MySql connection");
		}

		indexPage.SetDB(&db);

#ifdef MYSQL_4
	db.Query("set names cp1251");
#endif

		action = indexPage.GetVarsHandler()->Get("action");
		{
			CLog	log;

			log.Write(DEBUG, "main(): action = ", action);
		}

	// ------------ generate common parts
		{
			ostringstream			query, ost1, ost2;
			string				partNum;
			map<string, string>		menuHeader;
			map<string, string>::iterator	iterMenuHeader;
			string				content;
			Menu				m;

			indexPage.RegisterVariableForce("rand", GetRandom(10));
			indexPage.RegisterVariableForce("random", GetRandom(10));
			indexPage.RegisterVariableForce("style", "style.css");

	//------- Generate session
			{
				string			lng, sessidHTTP;
				ostringstream	ost;

				mapResult["session"] = "false";
				mapResult["user"] = "false";

				sessidHTTP = indexPage.SessID_Get_FromHTTP();
				if(sessidHTTP.length() < 5) {
					{
						CLog	log;
						log.Write(DEBUG, "main(): session checks: session cookie is not exists / expired, UA must be redirected to main page.");					
					}
				} 
				else 
				{
					{
						CLog	log;
						log.Write(DEBUG, "main(): session checks: HTTP session exists");
					}

					if(indexPage.SessID_Load_FromDB(sessidHTTP)) 
					{
						{
							CLog	log;
							log.Write(DEBUG, "main(): session checks: DB session loaded");
						}

						if(indexPage.SessID_CheckConsistency()) 
						{

							mapResult["session"] = "true";

							indexPage.RegisterVariableForce("loginUser", "");

							if(indexPage.SessID_Get_UserFromDB() != "Guest") 
							{

								user.SetDB(&db);

								if(user.GetFromDBbyEmailNoPassword(indexPage.SessID_Get_UserFromDB()))
								{

									indexPage.RegisterVariableForce("loginUser", indexPage.SessID_Get_UserFromDB());
									mapResult["user"] = "true";

									{
										CLog	log;
										ostringstream	ost;

										ost << "int main(void): session checks: user [" << user.GetLogin() << "] logged in";
										log.Write(DEBUG, ost.str());
									}								
								}
								else
								{
									// --- enforce to close session
									mapResult["user"] = "false";
									action = "logout";

									{
										CLog	log;
										ostringstream	ost;

										ost << "int main(void): ERROR user [" << indexPage.SessID_Get_UserFromDB() << "] session exists on client device, but not in the DB. Change the [action = logout].";
										log.Write(ERROR, ost.str());
									}
								}
							}
							else
							{
								{
									CLog	log;
									log.Write(DEBUG, "main(): session checks: session assigned to Guest user");
								}

							}
						}
						else 
						{
							CLog	log;
							log.Write(ERROR, "main(): ERROR session consistency check failed");
						}
					}
					else 
					{
						ostringstream	ost;

						{
							CLog	log;
							log.Write(DEBUG, "main(): cookie session and DB session is not equal. Need to recreate session");
						}

/*						if(!indexPage.Cookie_Expire()) 
						{
							CLog	log;
							log.Write(ERROR, "main(): Error in session expiration");			
						} // --- if(!indexPage.Cookie_Expire())

						// --- redirect URL
						ost.str("");
						ost << "/?rand=" << GetRandom(10);

						indexPage.Redirect(ost.str().c_str());
*/
					} // --- if(indexPage.SessID_Load_FromDB(sessidHTTP)) 
				} // --- if(sessidHTTP.length() < 5)




			}
	//------- End generate session

		}
	// ------------ end generate common parts


		if(action == "EchoRequest")
		{
			{
				CLog	log;

				log.Write(DEBUG, "main(): action == EchoRequest: start");
			}
			mapResult["type"] = "EchoResponse";

			if(user.GetLogin() != "Guest")
			{
				user.UpdatePresence();
			}
		}

		if(action == "GetUserRequestNotifications")
		{
			ostringstream	ost, result, ostUserNotifications;
			int				numberOfFriendshipRequests;

			{
				CLog	log;

				log.Write(DEBUG, "main(): action == GetUserRequestNotifications: start");
			}

			result.str("");

			ost.str("");
			ost << "SELECT * FROM  `users_friends` \
					WHERE `users_friends`.`userID`='" << user.GetID() << "' and `users_friends`.`state`='requested';";
			numberOfFriendshipRequests = db.Query(ost.str());
			// if(numberOfFriendshipRequests > 0)
			// {
				result << "[";
				for(int i = 0; i < numberOfFriendshipRequests; i++)
				{
					if(i > 0) result << ",";

					result << "{";
					result << "\"id\" : \"" << db.Get(i, "id") << "\",";
					result << "\"userID\" : \"" << db.Get(i, "userID") << "\",";
					result << "\"friendID\" : \"" << db.Get(i, "friendID") << "\",";
					result << "\"typeID\" : \"friendshipRequest\",";
					result << "\"state\" : \"" << db.Get(i, "state") << "\",";
					result << "\"date\" : \"" << db.Get(i, "date") << "\"";
					result << "}";
				}
				result << "]";
			// }

			{
				// ostUserNotifications.str("");
				// ostUserNotifications << "[";

				ost.str("");
				ost << "SELECT `users_notification`.`eventTimestamp` as `feed_eventTimestamp`, `users_notification`.`actionId` as `feed_actionId` , `users_notification`.`actionTypeId` as `feed_actionTypeId`, \
					`users_notification`.`id` as `users_notification_id`, `action_types`.`title` as `action_types_title`, \
					`users`.`id` as `user_id`, `users`.`name` as `user_name`, `users`.`nameLast` as `user_nameLast`, `users`.`email` as `user_email`, \
					`action_category`.`title` as `action_category_title`, `action_category`.`id` as `action_category_id` \
					FROM `users_notification` \
					INNER JOIN  `action_types` 		ON `users_notification`.`actionTypeId`=`action_types`.`id` \
					INNER JOIN  `action_category` 	ON `action_types`.`categoryID`=`action_category`.`id` \
					INNER JOIN  `users` 			ON `users_notification`.`userId`=`users`.`id` \
					WHERE `users_notification`.`userId`=\"" << user.GetID() << "\" AND `action_types`.`isShowNotification`='1' AND `users_notification`.`notificationStatus`='unread' \
					ORDER BY  `users_notification`.`eventTimestamp` DESC LIMIT 0 , 21";



/*				affected = db.Query(ost.str());
				if(affected)
				{
					class DBUserNotification
					{
						public:
							string		notificationID;
							string		feed_eventTimestamp;
							string		feed_actionId;
							string		feed_actionTypeId;
							string		action_types_title;
							string		user_id;
							string		user_name;
							string		user_nameLast;
							string		user_email;
							string		action_category_title;
							string		action_category_id;
					};

					vector<DBUserNotification>	dbResult;

					for(int i = 0; i < affected; ++i)
					{
						DBUserNotification		tmpObj;

						tmpObj.notificationID = db.Get(i, "users_notification_id");
						tmpObj.feed_actionTypeId = db.Get(i, "feed_actionTypeId");
						tmpObj.action_types_title = db.Get(i, "action_types_title");
						tmpObj.feed_eventTimestamp = db.Get(i, "feed_eventTimestamp");
						tmpObj.user_id = db.Get(i, "user_id");
						tmpObj.user_name = db.Get(i, "user_name");
						tmpObj.user_nameLast = db.Get(i, "user_nameLast");
						tmpObj.user_email = db.Get(i, "user_email");
						tmpObj.action_category_title = db.Get(i, "action_category_title");
						tmpObj.action_category_id = db.Get(i, "action_category_id");
						tmpObj.feed_actionId = db.Get(i, "feed_actionId");

						dbResult.push_back(tmpObj);
					}


					for(auto &it: dbResult)
					{
						string		userNotificationEnrichment = "";

						if(ostUserNotifications.str().length() > 20) ostUserNotifications << ",";
						ostUserNotifications << "{";

						// --- common part
						ostUserNotifications << "\"notificationID\":\"" << it.notificationID << "\",";
						ostUserNotifications << "\"notificationTypeID\":\"" << it.feed_actionTypeId << "\",";
						ostUserNotifications << "\"notificationTypeTitle\":\"" << it.action_types_title << "\",";
						ostUserNotifications << "\"notificationCategoryID\":\"" << it.action_category_id << "\",";
						ostUserNotifications << "\"notificationCategoryTitle\":\"" << it.action_category_title << "\",";
						ostUserNotifications << "\"notificationEventTimestamp\":\"" << it.feed_eventTimestamp << "\",";
						ostUserNotifications << "\"notificationOwnerUserID\":\"" << it.user_id << "\",";
						ostUserNotifications << "\"notificationOwnerUserName\":\"" << it.user_name << "\",";
						ostUserNotifications << "\"notificationOwnerUserNameLast\":\"" << it.user_nameLast << "\",";
						ostUserNotifications << "\"notificationActionID\":\"" << it.feed_actionId << "\",";
						ostUserNotifications << "\"notificationStatus\":\"" << "unread" << "\"";

						userNotificationEnrichment = GetUserNotificationSpecificDataByType(atol(it.feed_actionTypeId.c_str()), atol(it.feed_actionId.c_str()), &db);
						if(userNotificationEnrichment.length()) ostUserNotifications << "," << userNotificationEnrichment;

						ostUserNotifications << "}";
					}
				}
				ostUserNotifications << "]";
*/
				mapResult["userNotificationsArray"] = GetUserNotificationInJSONFormat(ost.str(), &db);
			}

			mapResult["friendshipNotificationsArray"] = result.str();
			mapResult["type"] = "UnreadUserNotifications";


			{
				CLog	log;

				log.Write(DEBUG, "main(): action == GetUserRequestNotifications: end (# of friendship notifications = ", to_string(numberOfFriendshipRequests), ")");
			}
		}

		// --- JSON get user info
		if(action == "GetUserInfo")
		{
			ostringstream	ost, ostFinal;
			string			sessid, userID, userList;
			char			*convertBuffer;

			{
				CLog	log;
				log.Write(DEBUG, "int main(void):action == GetUserInfo: start");
			}

			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(DEBUG, "int main()::GetUserInfo: re-login required");
				}

			}
			else
			{
				userID = indexPage.GetVarsHandler()->Get("userID");

				convertBuffer = new char[1024];
				memset(convertBuffer, 0, 1024);
				convert_utf8_to_windows1251(userID.c_str(), convertBuffer, 1024);
				userID = convertBuffer;
				trim(userID);

				delete[] convertBuffer;

				// --- Clean-up the text
				userID = ReplaceDoubleQuoteToQuote(userID);
				userID = DeleteHTML(userID);
				userID = SymbolReplace(userID, "\r\n", "");
				userID = SymbolReplace(userID, "\r", "");
				userID = SymbolReplace(userID, "\n", "");

				ost << "select * from `users` where `isActivated`='Y' and `isblocked`='N' and `id` in (" << userID << ") ;";

				userList = GetUserListInJSONFormat(ost.str(), &db, &user);

				ostFinal.str("");
				ostFinal << "[" << std::endl << userList << std::endl << "]" << std::endl;


				mapResult["userArray"] = ostFinal.str();
				mapResult["type"] = "UserInfo";
			} // --- if(user.GetLogin() == "Guest")

			{
				CLog	log;
				log.Write(DEBUG, "int main(void):action == GetUserInfo: end");
			}
		}

		// --- JSON get chat status
		if(action == "GetNavMenuChatStatus")
		{
			ostringstream	ost, ostFinal, friendsSqlQuery;
			string			sessid, lookForKey, userList, messageList;
			int				affected;

			{
				CLog	log;
				log.Write(DEBUG, "int main(void):action == GetNavMenuChatStatus: start");
			}

			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(DEBUG, "int main()::GetNavMenuChatStatus: re-login required");
				}

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str().c_str());
			}

			friendsSqlQuery.str("");
			ost.str("");
			ost << "select `fromID` from `chat_messages` where `toID`='" << user.GetID() << "' and (`messageStatus`='unread' or `messageStatus`='sent' or `messageStatus`='delivered');";
			affected = db.Query(ost.str());
			if(affected)
			{
				string	tempUserIDList {""};

				for(int i = 0; i < affected; i++)
				{
					tempUserIDList += (tempUserIDList.length() ? "," : "");
					tempUserIDList += db.Get(i, "fromID");
				}

				friendsSqlQuery << "select * from `users` where `isActivated`='Y' and `isblocked`='N' and `id` IN (";
				friendsSqlQuery << UniqueUserIDInUserIDLine(tempUserIDList);
				friendsSqlQuery << ");";

				{
					CLog	log;
					log.Write(DEBUG, "int main(void):action == GetNavMenuChatStatus: query for JSON prepared [", friendsSqlQuery.str(), "]");
				}

				userList = GetUserListInJSONFormat(friendsSqlQuery.str(), &db, &user);
				messageList = GetUreadChatMessagesInJSONFormat(&user, &db);
			}

			userList = "[" + userList + "]";
			messageList = "[" + messageList + "]";
			mapResult["userArray"] = userList;
			mapResult["unreadMessagesArray"] = messageList;
			mapResult["type"] = "ChatStatus";

			{
				CLog	log;
				log.Write(DEBUG, "int main(void):action == GetNavMenuChatStatus: end");
			}
		}


		// --- JSON get user info
		if(action == "CheckSessionPersistence")
		{
			ostringstream	ost, ostFinal;
			string			sessidPersistence, userID, userList;

			sessidPersistence = indexPage.GetVarsHandler()->Get("sessid");

			{
				CLog	log;
				log.Write(DEBUG, "int main(void):action == CheckSessionPersistence: start (persistence sessid", sessidPersistence, ")");
			}

			mapResult["result"] = "error";
			mapResult["sessionPersistence"] = "false";
			mapResult["userPersistence"] = "false";

			if(!isPersistenceRateLimited(getenv("REMOTE_ADDR"), &db))
			{
				if(sessidPersistence.length() < 5) 
				{
					{
						CLog	log;
						log.Write(DEBUG, "int main(void):action == CheckSessionPersistence: session checks: session cookie is not exists / expired, UA must be redirected to main page.");
					}
				} 
				else 
				{
					{
						CLog	log;
						log.Write(DEBUG, "int main(void):action == CheckSessionPersistence: session checks: HTTP session exists");
					}

					if(indexPage.SessID_Load_FromDB(sessidPersistence)) 
					{
						{
							CLog	log;
							log.Write(DEBUG, "int main(void):action == CheckSessionPersistence: session checks: DB session loaded");
						}

						if(indexPage.SessID_CheckConsistency()) 
						{

							{
								CLog	log;
								log.Write(DEBUG, "int main(void):action == CheckSessionPersistence: session checks: HTTP session consistent with DB session");
							}

							mapResult["sessionPersistence"] = "true";

							// if(indexPage.SessID_Update_HTTP_DB()) 
							{
								indexPage.RegisterVariableForce("loginUser", "");

								if(indexPage.SessID_Get_UserFromDB() != "Guest") 
								{
									{
										CLog	log;
										log.Write(DEBUG, "main(): session checks: session active for registered user");
									}

									user.SetDB(&db);
									user.GetFromDBbyEmail(indexPage.SessID_Get_UserFromDB());
									indexPage.RegisterVariableForce("loginUser", indexPage.SessID_Get_UserFromDB());
									mapResult["userPersistence"] = "true";
									mapResult["result"] = "success";

									{
										CLog	log;
										ostringstream	ost;

										ost << "int main(void):action == CheckSessionPersistence: session checks: user [" << user.GetLogin() << "] saved in persistence";
										log.Write(DEBUG, ost.str());
									}
								}
							}
						}
						else 
						{
							CLog	log;
							log.Write(ERROR, "int main(void):action == CheckSessionPersistence: ERROR session consistency check failed");

						}
					}
					else 
					{
						ostringstream	ost;

						{
							CLog	log;
							log.Write(DEBUG, "int main(void):action == CheckSessionPersistence: persisted session not found in DB. Need to recreate session");
						}

	/*						if(!indexPage.Cookie_Expire()) 
						{
							CLog	log;
							log.Write(ERROR, "main(): Error in session expiration");			
						} // --- if(!indexPage.Cookie_Expire())

						// --- redirect URL
						ost.str("");
						ost << "/?rand=" << GetRandom(10);

						indexPage.Redirect(ost.str().c_str());
	*/
					} // --- if(indexPage.SessID_Load_FromDB(sessidHTTP)) 
				} // --- if(sessidHTTP.length() < 5)
			} // --- if(isRateLimited)
			else
			{
				{
					CLog	log;
					log.Write(DEBUG, "int main(void):action == CheckSessionPersistence: BruteForce detected REMOTE_ADDR [", getenv("REMOTE_ADDR"), "]");
				}
			}

			{
				CLog	log;
				log.Write(DEBUG, "int main(void):action == CheckSessionPersistence: end");
			}
		} // --- if(action == "CheckSessionPersistence")


		if(action == "logout")
		{
			ostringstream	ost;
			string		sessid;

			{
				CLog	log;
				log.Write(DEBUG, "main: action == logout: start");
			}

			sessid = indexPage.GetCookie("sessid");
			if(sessid.length() > 0)
			{
				ostringstream	ost;
				ost.str("");
				ost << "update sessions set `user`='Guest', `expire`='1' where `id`='" << sessid << "'";
				db.Query(ost.str());

				if(!indexPage.Cookie_Expire()) {
					CLog	log;
					log.Write(ERROR, "main(): Error in session expiration");			
				} // --- if(!indexPage.Cookie_Expire())

			}

			{
				CLog	log;
				log.Write(DEBUG, "main: action == logout: end");
			}
		}


		// --- Build the response
		{ // --- Just for definition of ost
			ostringstream	ost;

			ost.str("");
			ost << "{" << endl;
			for(map<string, string>::iterator it = mapResult.begin(); it != mapResult.end(); it++)
			{
				if((it->first).find("Array") != string::npos)
				{
					ost << "\"" << it->first << "\" : " << it->second;
				}
				else if((it->first).find("Object") != string::npos)
				{
					ost << "\"" << it->first << "\" : " << it->second;
				}
				else
				{
					ost << "\"" << it->first << "\" : \"" << it->second << "\"";
				}
				if(next(it) != mapResult.end()) ost << ",	";
				ost << endl;
			}
			ost << "}" << endl;

			indexPage.RegisterVariableForce("result", ost.str());
		}


		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == AJAX_getNewsFeed: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))

		indexPage.OutTemplate();

    }
    catch(CExceptionHTML &c)
    {
		CLog	log;

		c.SetLanguage(indexPage.GetLanguage());
		c.SetDB(&db);

		log.Write(DEBUG, "catch CExceptionHTML: DEBUG exception reason: [", c.GetReason(), "]");

		if(!indexPage.SetTemplate(c.GetTemplate()))
		{
			return(-1);
		}
		indexPage.RegisterVariable("content", c.GetReason());
		indexPage.OutTemplate();
		return(0);
    }
    catch(CException &c)
    {
    	CLog 	log;

		if(!indexPage.SetTemplateFile("templates/error.htmlt"))
		{
			return(-1);
		}

		log.Write(ERROR, "catch CException: exception: ERROR  ", c.GetReason());

		indexPage.RegisterVariable("content", c.GetReason());
		indexPage.OutTemplate();
		return(-1);
    }
    catch(exception& e)
    {
    	CLog 	log;
		log.Write(PANIC, "catch standard exception: ERROR  ", e.what());

		if(!indexPage.SetTemplateFile("templates/error.htmlt"))
		{
			return(-1);
		}
		indexPage.RegisterVariable("content", e.what());
		indexPage.OutTemplate();
		return(-1);
    }

    return(0);
}

