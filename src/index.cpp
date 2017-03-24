#include <sstream>
// #include <stdexcept>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <Magick++.h>

#include "localy.h"
#include "ccgi.h"
#include "cvars.h"
#include "clog.h"
#include "cmysql.h"
#include "cexception.h"
#include "cactivator.h"
// #include "cmenu.h"
#include "cuser.h"
#include "cmail.h"
#include "cstatistics.h"
#include "utilities.h"

// --- Generating image with text "randStr"
// --- Input: randStr - text needs to be written on the image
// --- Output: path to the file
string GenerateImage(string randStr)
{
	string	fileName = "dessin.gif", fileFont, fileResultFull, fileResult;
	string	annotateFlag = "yes";

	fileName = IMAGE_DIRECTORY;
	fileName += "pages/login/dessin.gif";

	if(!fileName.empty())
	{
		if(annotateFlag == "yes")
		{
			string				fileNameWater;
			Magick::Image		imageMaster, imageDest;
			// ImageInfo	*image1_info, *anotherInfo;
			// char	        geometry[128];
			ostringstream 	ost;

			Magick::InitializeMagick(NULL);

			try 
			{
				bool 		fileFlagExist;

				imageMaster.read(fileName);
				imageDest = imageMaster;
				imageDest.fontPointsize(14);
				imageDest.addNoise(Magick::GaussianNoise);
				imageDest.addNoise(Magick::GaussianNoise);
				ost.str("");
				ost << "+" << 1 + (int)(rand()/(RAND_MAX + 1.0) * 45) << "+" << 13 + (int)(rand()/(RAND_MAX + 1.0) * 10);
				imageDest.annotate(randStr, Magick::Geometry(ost.str()));

			
				fileFlagExist = true;	
				do {
					{
						CLog	log;
						log.Write(DEBUG, "GenerateImage: checking captha file existance");
					}
					fileResult = "_";
					fileResult += GetRandom(10);
					fileResult += ".gif";
					fileResultFull = IMAGE_CAPTCHA_DIRECTORY;
					fileResultFull += fileResult;
					int fh = open(fileResultFull.c_str(), O_RDONLY);
					if(fh < 0) 
					{
						fileFlagExist = false;
						{
							CLog	log;
							log.Write(DEBUG, "GenerateImage: trying file ", fileResultFull, " -> can be used for writing");
						}
					}
					else 
					{ 
						close(fh); 
						{
							CLog	log;
							log.Write(DEBUG, "GenerateImage: trying file ", fileResultFull, " -> can't be used, needs another one");
						}
					}
				} while(fileFlagExist == true);


				{
					CLog	log;
					log.Write(DEBUG, "GenerateImage: write captcha-image to ", fileResultFull);
				}
				imageDest.write(fileResultFull);

			}
			catch(Magick::Exception &error_)
			{
				CLog			log;
				ostringstream	ost;

				ost.str("");
				ost << "GenerateImage(" << randStr << "): ERROR: Caught exception: " << error_.what();
				log.Write(ERROR, ost.str());

				fileResult = "";		
			}
		}
	}
	return fileResult;
}

int main()
{
    CStatistics		appStat;  // --- CStatistics must be firts statement to measure end2end param's
    CCgi			indexPage(EXTERNAL_TEMPLATE);
    CUser			user;
    string			action, partnerID;
    CMysql			db;
    struct timeval	tv;


    gettimeofday(&tv, NULL);
    srand(tv.tv_sec * tv.tv_usec * 100000);

    try
    {

	indexPage.ParseURL();
	indexPage.AddCookie("lng", "ru", "", "", "/");

	if(!indexPage.SetTemplate("index.htmlt"))
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
		ostringstream		query, ost1, ost2;
		string				partNum;
		string				content;
		// Menu				m;

		indexPage.RegisterVariableForce("rand", GetRandom(10));
		indexPage.RegisterVariableForce("random", GetRandom(10));
		indexPage.RegisterVariableForce("style", "style.css");

		indexPage.RegisterVariableForce("REMOTE_ADDR", getenv("REMOTE_ADDR"));
		indexPage.RegisterVariableForce("SERVER_NAME", getenv("SERVER_NAME"));



//------- Cleanup data
		{
			ostringstream	ost;

			ost.str("");
			ost << "delete from captcha where `timestamp`<=(NOW()-INTERVAL " << SESSION_LEN << " MINUTE)";
			db.Query(ost.str());
		}

//------- Generate session
		{
			string			lng, sessidHTTP;
			ostringstream	ost;


			sessidHTTP = indexPage.SessID_Get_FromHTTP();
			if(sessidHTTP.length() < 5) {
				{
					CLog	log;
					log.Write(DEBUG, "main(): session cookie is not exist, generating new session.");
				}
				sessidHTTP = indexPage.SessID_Create_HTTP_DB();
				if(sessidHTTP.length() < 5) {
					CLog	log;
					log.Write(ERROR, "main(): error in generating session ID");
					throw CExceptionHTML("session can't be created");
				}
			} 
			else {
				if(indexPage.SessID_Load_FromDB(sessidHTTP)) 
				{
					if(indexPage.SessID_CheckConsistency()) 
					{
						if(indexPage.SessID_Update_HTTP_DB()) 
						{
							indexPage.RegisterVariableForce("loginUser", "");
							indexPage.RegisterVariableForce("loginUserID", "");

							if(indexPage.SessID_Get_UserFromDB() != "Guest") {
								user.SetDB(&db);
								// --- place 2change user from user.email to user.id 
								if(user.GetFromDBbyEmail(indexPage.SessID_Get_UserFromDB()))
								{
									ostringstream	ost1;
									string			avatarPath;

									indexPage.RegisterVariableForce("loginUser", indexPage.SessID_Get_UserFromDB());
									indexPage.RegisterVariableForce("loginUserID", user.GetID());
									indexPage.RegisterVariableForce("myFirstName", user.GetName());
									indexPage.RegisterVariableForce("myLastName", user.GetNameLast());

									// --- Get user avatars
									ost1.str("");
									ost1 << "select * from `users_avatars` where `userid`='" << user.GetID() << "' and `isActive`='1';";
									avatarPath = "empty";
									if(db.Query(ost1.str()))
									{
										ost1.str("");
										ost1 << "/images/avatars/avatars" << db.Get(0, "folder") << "/" << db.Get(0, "filename");
										avatarPath = ost1.str();
									}
									indexPage.RegisterVariableForce("myUserAvatar", avatarPath);


									{
										CLog	log;
										ostringstream	ost;

										ost << "int main(void): user [" << user.GetLogin() << "] logged in";
										log.Write(DEBUG, ost.str());
									}
								}
								else
								{
									// --- enforce to close session
									action = "logout";

									{
										CLog	log;
										ostringstream	ost;

										ost << "int main(void): ERROR user [" << indexPage.SessID_Get_UserFromDB() << "] session exists on client device, but not in the DB. Change the [action = logout].";
										log.Write(ERROR, ost.str());
									}

								}
							}
						

						}
						else
						{
							CLog	log;
							log.Write(ERROR, "main(): ERROR update session in HTTP or DB failed");
						}
					}
					else {
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

					ost.str("");
					ost << "/?rand=" << GetRandom(10);

					if(!indexPage.Cookie_Expire()) {
						CLog	log;
						log.Write(ERROR, "main(): Error in session expiration");			
					} // --- if(!indexPage.Cookie_Expire())
					indexPage.Redirect(ost.str().c_str());
				} // --- if(indexPage.SessID_Load_FromDB(sessidHTTP)) 
			} // --- if(sessidHTTP.length() < 5)




		}
//------- End generate session


//------- Register html META-tags
		{
			int	affected;

			affected = db.Query("select `value` from `settings_default` where `setting`=\"keywords_head\"");
			if(affected > 0)
			{
				indexPage.RegisterVariable("keywords_head", db.Get(0, "value"));
			}
			else
			{
				CLog	log;
				log.Write(ERROR, "int main(void): ERROR getting keywords_head from settings_default");
			}

			affected = db.Query("select `value` from `settings_default` where `setting`=\"title_head\"");
			if(affected > 0)
			{
				indexPage.RegisterVariable("title_head", db.Get(0, "value"));
			}
			else
			{
				CLog	log;
				log.Write(ERROR, "int main(void): ERROR getting title_head from settings_default");
			}

			affected = db.Query("select `value` from `settings_default` where `setting`=\"description_head\"");
			if(affected > 0)
			{
				indexPage.RegisterVariable("description_head", db.Get(0, "value"));
			}
			else
			{
				CLog	log;
				log.Write(ERROR, "int main(void): ERROR getting description_head from settings_default");
			}

			affected = db.Query("select `value` from `settings_default` where `setting`=\"NewsOnSinglePage\"");
			if(affected > 0)
			{
				indexPage.RegisterVariable("NewsOnSinglePage", db.Get(0, "value"));
			}
			else
			{
				ostringstream	ost;
				CLog			log;
				log.Write(ERROR, "int main(void): ERROR getting NewsOnSinglePage from settings_default");

				ost.str("");
				ost << NEWS_ON_SINGLE_PAGE;
				indexPage.RegisterVariable("NewsOnSinglePage", ost.str());
			}

			affected = db.Query("select `value` from `settings_default` where `setting`=\"FriendsOnSinglePage\"");
			if(affected > 0)
			{
				indexPage.RegisterVariable("FriendsOnSinglePage", db.Get(0, "value"));
			}
			else
			{
				ostringstream	ost;
				CLog			log;
				log.Write(ERROR, "int main(void): ERROR getting FriendsOnSinglePage from settings_default");

				ost.str("");
				ost << FRIENDS_ON_SINGLE_PAGE;
				indexPage.RegisterVariable("FriendsOnSinglePage", ost.str());
			}

			if((!action.length()) and (user.GetLogin().length()) and (user.GetLogin() != "Guest"))
			{
				int		affected;

				affected = db.Query("select `value` from `settings_default` where `setting`=\"'logged in user' action\"");
				if(affected > 0)
				{
					action = db.Get(0, "value");
				}
				else
				{
					{
						CLog	log;

						log.Write(ERROR, "int main(void): ERROR getting \"'logged-in user' action\" from settings_default");
					}

					action = FALLBACK_LOGGEDIN_USER_DEFAULT_ACTION;
				}

				{
					CLog	log;
					log.Write(DEBUG, "int main(void): META-registration: action has been overriden to 'logged-in user' default action [action = ", action, "]");
				}

			}
			else if((!action.length()))
			{
				int		affected;

				affected = db.Query("select `value` from `settings_default` where `setting`=\"'guest user' action\"");
				if(affected > 0)
				{
					action = db.Get(0, "value");
				}
				else
				{
					{
						CLog	log;

						log.Write(ERROR, "int main(void): ERROR getting \"'guest user' action\" from settings_default");
					}

					action = FALLBACK_GUEST_USER_DEFAULT_ACTION;
				}

				{
					CLog	log;
					log.Write(DEBUG, "int main(void): META-registration: action has been overriden to 'guest user' default action [action = ", action, "]");
				}
			}
		}
//------- End register html META-tags

	}
// ------------ end generate common parts

// ------------ forum parts
	// {
	// 	string	forum, thID, parentID, firstTh, page, nextTh, prevTh, postMes, submitForumMes, content;
	// 	string	msg, name, email, topic, url, url_name, securityCode;
	// 	char	query[300];
	// 	int	affected;

	// 	forum = indexPage.GetVarsHandler()->Get("forum");
	// 	thID = indexPage.GetVarsHandler()->Get("thid");
	// 	firstTh = indexPage.GetVarsHandler()->Get("firstth");
	// 	page = indexPage.GetVarsHandler()->Get("page");
	// 	nextTh = indexPage.GetVarsHandler()->Get("nextth");
	// 	prevTh = indexPage.GetVarsHandler()->Get("prevth");

	// 	postMes = indexPage.GetVarsHandler()->Get("postmes");
	// 	submitForumMes = indexPage.GetVarsHandler()->Get("submitforummes");

	// 	parentID = indexPage.GetVarsHandler()->Get("pid");
	// 	name = indexPage.GetVarsHandler()->Get("name");
	// 	email = indexPage.GetVarsHandler()->Get("email");
	// 	topic = indexPage.GetVarsHandler()->Get("topic");
	// 	msg = indexPage.GetVarsHandler()->Get("msg");
	// 	url = indexPage.GetVarsHandler()->Get("url");
	// 	url_name = indexPage.GetVarsHandler()->Get("url_name");
	// 	securityCode = indexPage.GetVarsHandler()->Get("securityStr");


	// 	if(submitForumMes.length() > 0)
	// 	{
	// 		ostringstream   ost;

	// 		msg = DeleteHTML(msg);
	// 		name = DeleteHTML(name);
	// 		email = DeleteHTML(email);
	// 		topic = DeleteHTML(topic);
	// 		url = DeleteHTML(url);
	// 		url_name = DeleteHTML(url_name);
	// 		parentID = DeleteHTML(parentID);

	// 		if(topic.length() == 0) topic = "RE:";

	// 		ost.str("");
	// 		ost << "select * from sessions_rand where `session`='" << indexPage.GetCookie("sessid") << "'";
	// 		affected = db.Query(ost.str());
	// 		if(affected > 0)
	// 		{
	// 			string		securityStr;
	// 			securityStr = db.Get(0, "rand");
	// 			if(securityStr == securityCode)
	// 			{
	// 				ost.str("");
	// 				ost << "INSERT INTO `forum` (`parentID`, `name`, `email`, `topic`, `msg`, `time`, `ip`, `getmail`, `url`, `url_name`) VALUES (";
	// 				ost << " '" << parentID << "', \"" << RemoveQuotas(name) << "\", \"" << RemoveQuotas(email) << "\", \"" << RemoveQuotas(topic) << "\", \"" << RemoveQuotas(msg) << "\", '" << time(NULL) << "',";
	// 				ost << " '" << getenv("REMOTE_ADDR") << "', '', \"" << RemoveQuotas(url) << "\", \"" << RemoveQuotas(url_name) << "\");";
		
	// 				db.Query(ost.str().c_str());
	// 			}
	// 			ost.str("");
	// 			ost << "delete from sessions_rand where `session`='" << indexPage.GetCookie("sessid") << "'";
	// 			db.Query(ost.str());
	// 		}
	// 		forum = "y";
	// 	}
	// 	if(forum.length() > 0)
	// 	{

	// 	{
	// 		Catalog				m;
	// 		string				catID;

	// 		catID = indexPage.GetVarsHandler()->Get("cat");
	// 		if(catID.empty()) catID = "411";
	// 		m.SetDB(&db);
	// 		m.Load();

	// 		GenerateAndRegisterCatalogV(catID, &m, &db, &indexPage);
	// 	}

	// 		CForum  forumInst(&db);

	// 		if(thID.length() > 0)
	// 		{
	// 			if(firstTh.length() == 0) firstTh = "0";
	// 			forumInst.SetFirstMessage(atoi(firstTh.c_str()));
	// 			content = forumInst.GetTextMessage(atoi(thID.c_str()));
	// 		}
	// 		else
	// 		{
	// 			if(firstTh.length() == 0)
	// 				firstTh = "0";
	// 			forumInst.SetCurrentPage(atoi(page.c_str()));
	// 			content = forumInst.GetTextForum(atoi(firstTh.c_str()), THREADS_PER_PAGE);
	// 		}

	// 		indexPage.RegisterVariable("title", "‘орум");
	// 		indexPage.RegisterVariable("content", content.c_str());
	// 	}
	// 	if(postMes.length() > 0)
	// 	{
	// 		string				securityStr, sessionStr;
	// 		ostringstream			ost;
	// 	{
	// 		Catalog				m;
	// 		string				catID;

	// 		catID = indexPage.GetVarsHandler()->Get("cat");
	// 		if(catID.empty()) catID = "411";
	// 		m.SetDB(&db);
	// 		m.Load();

	// 		GenerateAndRegisterCatalogV(catID, &m, &db, &indexPage);
	// 	}
	// 		memset(query, 0, sizeof(query));
	// 		snprintf(query, sizeof(query) - 2, "select * from forum where rootID=%s", parentID.c_str());

	// 		affected = db.Query(query);
	// 		{
	// 			CLog    log;
	// 			char    tmp[100];

	// 			memset(tmp, 0, sizeof(tmp));
	// 			sprintf(tmp, "affected %d rows", affected);
	// 			log.Write(DEBUG, tmp);
	// 		}
	// 		if(affected <= 0)
	// 		{
	// 			indexPage.RegisterVariable("parentID", "0");
	// 		}
	// 		else
	// 		{
	// 			string      topic;

	// 			topic = "RE: ";
	// 			topic += db.Get(0, "topic");

	// 			indexPage.RegisterVariable("parentID", db.Get(0, "rootID"));
	// 			indexPage.RegisterVariable("topic", topic.c_str());
	// 		}

	// 		securityStr = GetRandom(4);
	// 		sessionStr = indexPage.GetCookie("sessid");

	// 		ost.str("");
	// 		ost << "delete from sessions_rand where `session`='" << sessionStr << "'";
	// 		db.Query(ost.str());

	// 		ost.str("");
	// 		ost << "insert into sessions_rand set `session`='" << sessionStr << "', `rand`='" << securityStr << "', `date`=NOW()";
	// 		db.Query(ost.str());

	// 		indexPage.RegisterVariableForce("securityStr", GenerateImage(securityStr));

	// 		if(!indexPage.SetTemplate("forumpost.htmlt"))
	// 		{
	// 			CLog        log;

	// 			log.Write(ERROR, "template file forumpost.htmlt was missing");
	// 			throw CException("Template file was missing");
	// 		}
	// 	}
	// }
// ------------ end forum parts

	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "int main(void): start (action's == \"" << action << "\") condition";
		log.Write(DEBUG, ost.str());
	}


	if(action == "setlang")
	{
		string		lng;

		lng = indexPage.GetVarsHandler()->Get("lng");
		indexPage.AddCookie("lng", lng, "", "", "/");

		if(!indexPage.SetTemplate("index.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "int main(void): action == setlang: ERROR: template file index.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	// --- add to "home screen" on iPhone.
	// --- this functionality has not been tested
	if(action == "add_to_home_screen")
	{
		string		lng;

		if(!indexPage.SetTemplate("add_to_home_screen.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "int main(void): action == add_to_home_screen: ERROR: template file index.htmlt was missing");
			throw CException("Template file was missing");
		}
	}


	// --- JSON part has started


	// --- JSON news feed
	if(action == "AJAX_getNewsFeed")
	{
		ostringstream	ost;
		int				affected;
		CMysql			db1;
		string			strPageToGet, strNewsOnSinglePage, strFriendList;
		vector<int>		vectorFriendList;

		strNewsOnSinglePage	= indexPage.GetVarsHandler()->Get("NewsOnSinglePage");
		strPageToGet 		= indexPage.GetVarsHandler()->Get("page");
		if(strPageToGet.empty()) strPageToGet = "0";
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_getNewsFeed: page ", strPageToGet, " requested");
		}

		if(db1.Connect(DB_NAME, DB_LOGIN, DB_PASSWORD) < 0)
		{
			CLog	log;
	
			log.Write(ERROR, "int main(void): action == AJAX_getNewsFeed: Can not connect to mysql database");
			return(1);
		}

#ifdef MYSQL_4
		db1.Query("set names cp1251");
#endif

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == AJAX_getNewsFeed: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		ost.str("");
		ost << "select `users_friends`.`friendID` \
				from `users_friends` \
				left join `users` on `users`.`id`=`users_friends`.`friendID` \
				where `users_friends`.`userID`='" << user.GetID() << "' and `users_friends`.`state`='confirmed' and `users`.`isactivated`='Y' and `users`.`isblocked`='N';";

		affected = db.Query(ost.str());
		for(int i = 0; i < affected; i++)
		{
			vectorFriendList.push_back(atoi(db.Get(i, "friendID")));
		}

		ost.str("");
		for(vector<int>::iterator it = vectorFriendList.begin(); it != vectorFriendList.end(); ++it)
		{
			if(it != vectorFriendList.begin()) ost << ",";
			ost << *it;
		}
		strFriendList = ost.str();
		if(strFriendList.length() > 0) strFriendList += ",";
		strFriendList += user.GetID();

		ost.str("");
		ost << "SELECT `feed`.`id` as `feed_id`, `feed`.`eventTimestamp` as `feed_eventTimestamp`, `feed`.`actionId` as `feed_actionId` , `feed`.`actionTypeId` as `feed_actionTypeId` , \
				`action_types`.`title` as `action_types_title`, \
				`users`.`id` as `user_id`, `users`.`name` as `user_name`, `users`.`nameLast` as `user_nameLast`, `users`.`email` as `user_email`, \
				`action_category`.`title` as `action_category_title` \
				FROM `feed` \
				INNER JOIN  `action_types` 		ON `feed`.`actionTypeId`=`action_types`.`id` \
				INNER JOIN  `action_category` 	ON `action_types`.`categoryID`=`action_category`.`id` \
				INNER JOIN  `users` 			ON `feed`.`userId`=`users`.`id` \
				WHERE `feed`.`userId` in (" << strFriendList << ") and `action_types`.`isShowFeed`='1' \
				ORDER BY  `feed`.`eventTimestamp` DESC LIMIT " << stoi(strPageToGet, nullptr, 10) * stoi(strNewsOnSinglePage, nullptr, 10) << " , " << stoi(strNewsOnSinglePage, nullptr, 10);

		affected = db.Query(ost.str());

		ost.str("");
		ost << "[";
		for(int i = 0; i < affected; i++)
		{
			ostringstream	ost1;
			string			avatarPath;
			string			feedID = db.Get(i, "feed_id");
			string			feedActionTypeId = db.Get(i, "feed_actionTypeId");
			string			feedActionId = db.Get(i, "feed_actionId");
			string			feedMessageOwner = db.Get(i, "user_id");
			string			feedMessageTimestamp = db.Get(i, "feed_eventTimestamp");

			ost1.str("");
			ost1 << "select * from `users_avatars` where `userid`='" << db.Get(i, "user_id") << "' and `isActive`='1';";
			avatarPath = "empty";
			if(db1.Query(ost1.str()))
			{
				ost1.str("");
				ost1 << "/images/avatars/avatars" << db1.Get(0, "folder") << "/" << db1.Get(0, "filename");
				avatarPath = ost1.str();
			}

			if(feedActionTypeId == "11") 
			{
				// --- 11 - message
				
				ost1.str("");
				ost1 << "select * from `feed_message` where `id`='" << feedActionId << "';";
				if(db1.Query(ost1.str()))
				{
					string	messageId = db1.Get(0, "id");
					string	messageTitle = db1.Get(0, "title");
					string	messageLink = db1.Get(0, "link");
					string	messageMessage = db1.Get(0, "message");
					string	messageImageSetID = db1.Get(0, "imageSetID");
					string	messageAccessRights = db1.Get(0, "access");
					string	messageImageList = 				GetMessageImageList(messageImageSetID, &db1);
					string	messageParamLikesUserList = 	GetMessageLikesUsersList(messageId, &user, &db1);
					string	messageParamCommentsCount =		GetMessageCommentsCount(messageId, &db1);
					string	messageParamSpam = 				GetMessageSpam(messageId, &db1);
					string	messageParamSpamMe = 			GetMessageSpamUser(messageId, user.GetID(), &db1);

					if(AllowMessageInNewsFeed(&user, feedMessageOwner, messageAccessRights, &vectorFriendList))
					{
						if(ost.str().length() > 10) ost << "," << std::endl;

						ost << "{";
						ost << "\"avatar\":\"" 				<< avatarPath 										<< "\"," << std::endl;
						ost << "\"userID\":\""				<< db.Get(i, "user_id")								<< "\"," << std::endl;
						ost << "\"userName\":\""			<< db.Get(i, "user_name")							<< "\"," << std::endl;
						ost << "\"userLastName\":\""		<< db.Get(i, "user_nameLast")						<< "\"," << std::endl;
						ost << "\"actionCategoryTitle\":\""	<< db.Get(i, "action_category_title")				<< "\"," << std::endl;
						ost << "\"actionTypesTitle\":\""	<< db.Get(i, "action_types_title")					<< "\"," << std::endl;
						ost << "\"actionTypesId\":\""		<< db.Get(i, "feed_actionTypeId")					<< "\"," << std::endl;

						ost << "\"messageId\":\""			<< messageId								<< "\"," << std::endl;
						ost << "\"messageTitle\":\""		<< messageTitle								<< "\"," << std::endl;
						ost << "\"messageLink\":\""			<< messageLink								<< "\"," << std::endl;
						ost << "\"messageMessage\":\""		<< messageMessage							<< "\"," << std::endl;
						ost << "\"messageImageSetID\":\""	<< messageImageSetID						<< "\"," << std::endl;
						ost << "\"messageImageList\":["		<< messageImageList							<< "]," << std::endl;
						ost << "\"messageLikesUserList\":["	<< messageParamLikesUserList				<< "]," << std::endl;
						ost << "\"messageCommentsCount\":\""<< messageParamCommentsCount				<< "\"," << std::endl;
						ost << "\"messageSpam\":\""			<< messageParamSpam							<< "\"," << std::endl;
						ost << "\"messageSpamMe\":\""		<< messageParamSpamMe						<< "\"," << std::endl;
						ost << "\"messageAccessRights\":\""	<< messageAccessRights						<< "\"," << std::endl;

						ost << "\"eventTimestamp\":\""		<< db.Get(i, "feed_eventTimestamp")											<< "\"," << std::endl;
						// ost << "\"eventTimestampDelta\":\""	<< GetHumanReadableTimeDifferenceFromNow(db.Get(i,"feed_eventTimestamp"))	<< "\"" << std::endl;
						ost << "\"eventTimestampDelta\":\""	<< GetTimeDifferenceFromNow(db.Get(i,"feed_eventTimestamp"))	<< "\"" << std::endl;

						ost << "}" << std::endl;

						// if(i < (affected - 1)) ost << "," << std::endl;
					} // --- Message Access Rights onot allow to post it into feed
				}
				else
				{
					CLog	log;
					log.Write(ERROR, "int main(void): action == AJAX_getNewsFeed: ERROR can't get message [", db.Get(i, "feed_actionId"), "] from feed_message");
				} // --- Message in news feed not found
			}
			else if(feedActionTypeId == "19")
			{
				// --- 19 - comment written
				
				ost1.str("");
				ost1 << "select * from `feed_message` where `id`='" << feedActionId << "';";
				if(db1.Query(ost1.str()))
				{
					string	messageId = db1.Get(0, "id");
					string	messageTitle = db1.Get(0, "title");
					string	messageLink = db1.Get(0, "link");
					string	messageMessage = db1.Get(0, "message");
					string	messageImageSetID = db1.Get(0, "imageSetID");
					string	messageAccessRights = db1.Get(0, "access");
					string	messageImageList = 				GetMessageImageList(messageImageSetID, &db1);
					string	messageParamLikesUserList = 	GetMessageLikesUsersList(messageId, &user, &db1);
					string	messageParamCommentsCount =		GetMessageCommentsCount(messageId, &db1);
					string	messageParamSpam = 				GetMessageSpam(messageId, &db1);
					string	messageParamSpamMe = 			GetMessageSpamUser(messageId, user.GetID(), &db1);
					string	messageOwnerID;
					string	messageOwnerName;
					string	messageOwnerNameLast;
					string	messageOwnerAvatar;
					string	messageEventTimestamp;

					ost1.str("");
					ost1 << "select * from `feed` where `actionTypeId`='11' and `actionId`='" << messageId << "';";
					if(db1.Query(ost1.str()))
					{
						messageOwnerID = db1.Get(0, "userId");
						messageEventTimestamp = db1.Get(0, "eventTimestamp");

						ost1.str("");
						ost1 << "select * from `users` where `id`='" << messageOwnerID << "' and `isactivated`='Y' and `isblocked`='N';";
						if(db1.Query(ost1.str()))
						{
							messageOwnerName = db1.Get(0, "name");
							messageOwnerNameLast = db1.Get(0, "nameLast");

							if(AllowMessageInNewsFeed(&user, messageOwnerID, messageAccessRights, &vectorFriendList))
							{
								if(ost.str().length() > 10) ost << "," << std::endl;

								ost << "{";
								ost << "\"userID\":\""				<< messageOwnerID						<< "\"," << std::endl;
								ost << "\"userName\":\""			<< messageOwnerName						<< "\"," << std::endl;
								ost << "\"userLastName\":\""		<< messageOwnerNameLast					<< "\"," << std::endl;
								ost << "\"actionCategoryTitle\":\""	<< db.Get(i, "action_category_title")	<< "\"," << std::endl;
								ost << "\"actionTypesTitle\":\""	<< db.Get(i, "action_types_title")		<< "\"," << std::endl;
								ost << "\"actionTypesId\":\""		<< db.Get(i, "feed_actionTypeId")		<< "\"," << std::endl;

								ost << "\"messageId\":\""			<< messageId							<< "\"," << std::endl;
								ost << "\"messageTitle\":\""		<< messageTitle							<< "\"," << std::endl;
								ost << "\"messageLink\":\""			<< messageLink							<< "\"," << std::endl;
								ost << "\"messageMessage\":\""		<< messageMessage						<< "\"," << std::endl;
								ost << "\"messageImageSetID\":\""	<< messageImageSetID					<< "\"," << std::endl;
								ost << "\"messageImageList\":["		<< messageImageList						<< "]," << std::endl;
								ost << "\"messageLikesUserList\":["	<< messageParamLikesUserList			<< "]," << std::endl;
								ost << "\"messageCommentsCount\":\""<< messageParamCommentsCount			<< "\"," << std::endl;
								ost << "\"messageSpam\":\""			<< messageParamSpam						<< "\"," << std::endl;
								ost << "\"messageSpamMe\":\""		<< messageParamSpamMe					<< "\"," << std::endl;
								ost << "\"messageAccessRights\":\""	<< messageAccessRights					<< "\"," << std::endl;

								ost << "\"commentUserID\":\""		<< db.Get(i, "user_id")					<< "\"," << std::endl;
								ost << "\"commentUserName\":\""		<< db.Get(i, "user_name")				<< "\"," << std::endl;
								ost << "\"commentUserLastName\":\""	<< db.Get(i, "user_nameLast")			<< "\"," << std::endl;
								ost << "\"commentUserAvatar\":\""	<< avatarPath							<< "\"," << std::endl;
								ost << "\"commentTimestamp\":\""	<< db.Get(i, "feed_eventTimestamp")		<< "\"," << std::endl;
								{
									// --- comment
									ostringstream	ostTemp;
									string			avatarPath = "empty";
									
									ostTemp.str("");
									ostTemp << "select * from `users_avatars` where `userid`='" << messageOwnerID << "' and `isActive`='1';";
									if(db1.Query(ostTemp.str()))
									{
										ostTemp.str("");
										ostTemp << "/images/avatars/avatars" << db1.Get(0, "folder") << "/" << db1.Get(0, "filename");
										avatarPath = ostTemp.str();
									}
									ost << "\"avatar\":\""	<< avatarPath							<< "\"," << std::endl;
								}

								ost << "\"eventTimestamp\":\""		<< db.Get(i, "feed_eventTimestamp")											<< "\"," << std::endl;
								// ost << "\"eventTimestampDelta\":\""	<< GetHumanReadableTimeDifferenceFromNow(db.Get(i,"feed_eventTimestamp"))	<< "\"" << std::endl;
								ost << "\"eventTimestampDelta\":\""	<< GetTimeDifferenceFromNow(db.Get(i,"feed_eventTimestamp"))	<< "\"" << std::endl;
								ost << "}" << std::endl;

								// if(i < (affected - 1)) ost << "," << std::endl;
							} // --- Message Access Rights onot allow to post it into feed
							else
							{
								CLog	log;
								log.Write(DEBUG, "int main(void): action == AJAX_getNewsFeed: actionTypeID=19: DEBUG message [", messageId,"] is not allowed in news_feed by access rights permission");
							}
						}
						else
						{
							CLog	log;
							log.Write(ERROR, "int main(void): action == AJAX_getNewsFeed: actionTypeID=19: ERROR can't find userID in `users` table  (userID[", messageOwnerID, "])");
						}
					}
					else
					{
						CLog	log;
						log.Write(ERROR, "int main(void): action == AJAX_getNewsFeed: actionTypeID=19: ERROR can't find commented message in news_feed (messageID[", messageId, "])");
					}

				}
				else
				{
					CLog	log;
					log.Write(ERROR, "int main(void): action == AJAX_getNewsFeed: ERROR can't get message [", db.Get(i, "feed_actionId"), "] from feed_message");
				} // --- Message in news feed not found
			}
			else if((feedActionTypeId == "14") || (feedActionTypeId == "15") || (feedActionTypeId == "16")) 
			{
				// --- 14 friendship established
				// --- 15 friendship broken
				// --- 16 friendship request sent

				string	friendID = feedActionId;

				ost1.str("");
				ost1 << "SELECT `users`.`name` as `users_name`, `users`.`nameLast` as `users_nameLast` FROM `users` WHERE `id`=\"" << friendID << "\" and `isblocked`='N';";
				if(db1.Query(ost1.str()))
				{
					string	friendAvatar = "empty";
					string	friendName;
					string	friendNameLast;
					string	friendCompanyName;
					string	friendCompanyID;
					string	friendUsersCompanyPositionTitle;

					friendName = db1.Get(0, "users_name");
					friendNameLast = db1.Get(0, "users_nameLast");

					ost1.str("");
					ost1 << "select * from `users_avatars` where `userid`='" << friendID << "' and `isActive`='1';";
					if(db1.Query(ost1.str()))
					{
						ost1.str("");
						ost1 << "/images/avatars/avatars" << db1.Get(0, "folder") << "/" << db1.Get(0, "filename");
						friendAvatar = ost1.str();
					}

					ost1.str("");
					ost1 << "SELECT `users_company_position`.`title` as `users_company_position_title`,  \
							`company`.`id` as `company_id`, `company`.`name` as `company_name` \
							FROM `users_company` \
							LEFT JOIN  `users_company_position` ON `users_company_position`.`id`=`users_company`.`position_title_id` \
							LEFT JOIN  `company` 				ON `company`.`id`=`users_company`.`company_id` \
							WHERE `users_company`.`user_id`=\"" << friendID << "\" and `users_company`.`current_company`='1' \
							ORDER BY  `users_company`.`occupation_start` DESC ";
					if(db1.Query(ost1.str()))
					{
						friendCompanyName = db1.Get(0, "company_name");
						friendCompanyID = db1.Get(0, "company_id");
						friendUsersCompanyPositionTitle = db1.Get(0, "users_company_position_title");
					}
					else
					{
						CLog	log;
						log.Write(DEBUG, "int main(void): action == AJAX_getNewsFeed: can't get information [", db.Get(i, "feed_actionId"), "] about his/her employment");
					} // --- Message in news feed not found

					{
						if(ost.str().length() > 10) ost << "," << std::endl;

						ost << "{";
						ost << "\"avatar\":\"" 				<< avatarPath 										<< "\"," << std::endl;
						ost << "\"userID\":\""				<< db.Get(i, "user_id")								<< "\"," << std::endl;
						ost << "\"userName\":\""			<< db.Get(i, "user_name")							<< "\"," << std::endl;
						ost << "\"userLastName\":\""		<< db.Get(i, "user_nameLast")						<< "\"," << std::endl;
						ost << "\"actionCategoryTitle\":\""	<< db.Get(i, "action_category_title")				<< "\"," << std::endl;
						ost << "\"actionTypesTitle\":\""	<< db.Get(i, "action_types_title")					<< "\"," << std::endl;
						ost << "\"actionTypesId\":\""		<< db.Get(i, "feed_actionTypeId")					<< "\"," << std::endl;

						ost << "\"friendAvatar\":\""		<< friendAvatar										<< "\"," << std::endl;
						ost << "\"friendID\":\""			<< friendID											<< "\"," << std::endl;
						ost << "\"friendName\":\""			<< friendName										<< "\"," << std::endl;
						ost << "\"friendNameLast\":\""		<< friendNameLast									<< "\"," << std::endl;
						ost << "\"friendCompanyID\":\""		<< friendCompanyID									<< "\"," << std::endl;
						ost << "\"friendCompanyName\":\""	<< friendCompanyName								<< "\"," << std::endl;
						ost << "\"friendUsersCompanyPositionTitle\":\""	<< friendUsersCompanyPositionTitle		<< "\"," << std::endl;

						ost << "\"eventTimestamp\":\""		<< db.Get(i, "feed_eventTimestamp")											<< "\"," << std::endl;
						// ost << "\"eventTimestampDelta\":\""	<< GetHumanReadableTimeDifferenceFromNow(db.Get(i,"feed_eventTimestamp"))	<< "\"" << std::endl;
						ost << "\"eventTimestampDelta\":\""	<< GetTimeDifferenceFromNow(db.Get(i,"feed_eventTimestamp"))	<< "\"" << std::endl;
						ost << "}" << std::endl;

						// if(i < (affected - 1)) ost << "," << std::endl;
					} // --- Message Access Rights onot allow to post it into feed
				}
				else
				{
					CLog	log;
					log.Write(ERROR, "int main(void): action == AJAX_getNewsFeed: ERROR user [", friendID, "] not found or blocked");
				}
			}
			else if((feedActionTypeId == "41")) 
			{
				// --- 41 skill added

				string	users_skillID = feedActionId;

				ost1.str("");
				ost1 << "SELECT * FROM `users_skill` WHERE `id`=\"" << users_skillID << "\";";
				if(db1.Query(ost1.str()))
				{
					string	skillID = db1.Get(0, "skill_id");

					ost1.str("");
					ost1 << "select * from `skill` where `id`=\"" << skillID << "\";";
					if(db1.Query(ost1.str()))
					{
						string		skillTitle = db1.Get(0, "title");
						
						if(ost.str().length() > 10) ost << "," << std::endl;

						ost << "{";
						ost << "\"avatar\":\"" 				<< avatarPath 										<< "\"," << std::endl;
						ost << "\"userID\":\""				<< db.Get(i, "user_id")								<< "\"," << std::endl;
						ost << "\"userName\":\""			<< db.Get(i, "user_name")							<< "\"," << std::endl;
						ost << "\"userLastName\":\""		<< db.Get(i, "user_nameLast")						<< "\"," << std::endl;
						ost << "\"actionCategoryTitle\":\""	<< db.Get(i, "action_category_title")				<< "\"," << std::endl;
						ost << "\"actionTypesTitle\":\""	<< db.Get(i, "action_types_title")					<< "\"," << std::endl;
						ost << "\"actionTypesId\":\""		<< db.Get(i, "feed_actionTypeId")					<< "\"," << std::endl;

						ost << "\"skillID\":\""				<< skillID											<< "\"," << std::endl;
						ost << "\"skillTitle\":\""			<< skillTitle										<< "\"," << std::endl;

						ost << "\"eventTimestamp\":\""		<< db.Get(i, "feed_eventTimestamp")								<< "\"," << std::endl;
						// ost << "\"eventTimestampDelta\":\""	<< GetHumanReadableTimeDifferenceFromNow(db.Get(i,"feed_eventTimestamp"))	<< "\"" << std::endl;
						ost << "\"eventTimestampDelta\":\""	<< GetTimeDifferenceFromNow(db.Get(i,"feed_eventTimestamp"))	<< "\"" << std::endl;
						ost << "}" << std::endl;

					}
					else
					{
						CLog	log;
						log.Write(ERROR, "int main(void): action == AJAX_getNewsFeed: ERROR skill_id [", skillID, "] not found");
					}

				}
				else
				{
					CLog	log;
					log.Write(ERROR, "int main(void): action == AJAX_getNewsFeed: ERROR users_skill_id [", users_skillID, "] not found");
				}
			}
			else
			{

				if(ost.str().length() > 10) ost << "," << std::endl;

				ost << "{";
				ost << "\"avatar\":\"" 				<< avatarPath 																<< "\"," << std::endl;
				ost << "\"userID\":\""				<< db.Get(i, "user_id")														<< "\"," << std::endl;
				ost << "\"userName\":\""			<< db.Get(i, "user_name")													<< "\"," << std::endl;
				ost << "\"userLastName\":\""		<< db.Get(i, "user_nameLast")												<< "\"," << std::endl;
				ost << "\"actionCategoryTitle\":\""	<< db.Get(i, "action_category_title")										<< "\"," << std::endl;
				ost << "\"actionTypesTitle\":\""	<< db.Get(i, "action_types_title")											<< "\"," << std::endl;
				ost << "\"actionTypesId\":\""		<< db.Get(i, "feed_actionTypeId")											<< "\"," << std::endl;
				ost << "\"eventTimestamp\":\""		<< db.Get(i, "feed_eventTimestamp")											<< "\"," << std::endl;
				// ost << "\"eventTimestampDelta\":\""	<< GetHumanReadableTimeDifferenceFromNow(db.Get(i,"feed_eventTimestamp"))	<< "\"" << std::endl;
				ost << "\"eventTimestampDelta\":\""	<< GetTimeDifferenceFromNow(db.Get(i,"feed_eventTimestamp"))	<< "\"" << std::endl;
				ost << "}" << std::endl;
				
				// if(i < (affected - 1)) ost << "," << std::endl;
			}


		}
		ost << "]";

		indexPage.RegisterVariableForce("result", ost.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == AJAX_getNewsFeed: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("json_response.htmlt"))
	}

	// --- AJAX delete message from news feed
	if(action == "AJAX_deleteNewsFeedMessage")
	{
		ostringstream	ost, ostFinal;
		CMysql			db1;
		string			messageID;

		messageID	= indexPage.GetVarsHandler()->Get("messageID");

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_deleteNewsFeedMessage: start [", messageID, "]");
		}


		if(messageID.empty())
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_deleteNewsFeedMessage: messageID is not defined");

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"messageID is empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}
		else
		{
			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(DEBUG, "int main(void): action == AJAX_deleteNewsFeedMessage: re-login required");
				}

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str().c_str());
			}


			ost.str("");
			ost << "select `userId` from `feed` where `userId`='" << user.GetID() << "' and `actionTypeId`='11' and `actionId`='" << messageID << "';";
			if(db.Query(ost.str()))
			{
				ost.str("");
				ost << "delete from `feed` where `userId`='" << user.GetID() << "' and `actionTypeId`='11' and `actionId`='" << messageID << "';";
				db.Query(ost.str());

				ost.str("");
				ost << "select * from `feed_message` where `id`='" << messageID << "';";
				if(db.Query(ost.str()))
				{
					string		imageSetID = db.Get(0, "imageSetID");
					// int			affected;

					ost.str("");
					ost << "`setID`='" << imageSetID << "' and `userID`='" << user.GetID() << "'";
					RemoveMessageImages(ost.str(), &db);

				} // --- if ("select * from `feed_message` where `id`='" << messageID << "';";)

				ost.str("");
				ost << "delete from `feed_message` where `id`='" << messageID << "';";
				db.Query(ost.str());

				// --- removing likes / dislikes and notifications
				ost.str("");
				ost << "delete from `users_notification` where `actionTypeId`='50' and `actionId`='" << messageID << "';";
				db.Query(ost.str());
				ost.str("");
				ost << "delete from `users_notification` where `actionTypeId`='49' and `actionId` in (select `id` from `feed_message_params` where `messageID`='" << messageID << "');";
				db.Query(ost.str());
				ost.str("");
				ost << "delete from `feed_message_params` where `messageID`='" << messageID << "';";
				db.Query(ost.str());

				// --- removing comments and notifications
				ost.str("");
				ost << "delete from `users_notification` where `actionTypeId`='19' and `actionId` in (select `id` from `feed_message_comment` where `messageID`='" << messageID << "');";
				db.Query(ost.str());
				ost.str("");
				ost << "delete from `feed_message_comment` where `messageID`='" << messageID << "';";
				db.Query(ost.str());

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"success\"," << std::endl;
				ostFinal << "\"description\" : \"\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
			else
			{
				CLog	log;
				log.Write(ERROR, "int main(void): action == AJAX_deleteNewsFeedMessage: ERROR message is not belongs to you");
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_deleteNewsFeedMessage: end (messageID ", messageID, ")");
		}

	}

	// --- AJAX delete comment from message
	if(action == "AJAX_deleteNewsFeedComment")
	{
		ostringstream	ost, ostFinal;
		CMysql			db1;
		string			commentID;

		commentID	= indexPage.GetVarsHandler()->Get("commentID");

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_deleteNewsFeedComment: start [", commentID, "]");
		}


		if(commentID.empty())
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_deleteNewsFeedComment: commentID is not defined");

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"commentID is empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}
		else
		{
			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(DEBUG, "int main(void): action == AJAX_deleteNewsFeedComment: re-login required");
				}

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str().c_str());
			}

			ost.str("");
			ost << "delete from `feed_message_comment` where `id`='" << commentID << "' and `userID`='" << user.GetID() << "';";
			db.Query(ost.str());

			ost.str("");
			ost << "delete from `users_notification` where `actionId`='" << commentID << "' and `actionTypeId`='19';";
			db.Query(ost.str());

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"success\"," << std::endl;
			ostFinal << "\"description\" : \"\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	// --- AJAX delete comment from message
	if(action == "AJAX_removeCompanyExperience")
	{
		ostringstream	ost, ostFinal;
		CMysql			db1;
		string			expID;

		expID	= indexPage.GetVarsHandler()->Get("id");

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeCompanyExperience: start");
		}


		if(expID.empty())
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_removeCompanyExperience: expID is not defined");

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"expID is empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}
		else
		{
			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(DEBUG, "int main(void): action == AJAX_removeCompanyExperience: re-login required");
				}

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str().c_str());
			}


			ost.str("");
			ost << "delete from `users_company` where `id`='" << expID << "' and `user_id`='" << user.GetID() << "';";
			db.Query(ost.str());

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"success\"," << std::endl;
			ostFinal << "\"description\" : \"\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeCompanyExperience: end");
		}
	}

	// --- AJAX remove certification path
	if(action == "AJAX_removeCertificationEntry")
	{
		ostringstream	ost, ostFinal;
		CMysql			db1;
		string			certificationID;

		certificationID	= indexPage.GetVarsHandler()->Get("id");

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeCertificationEntry: start");
		}


		if(certificationID.empty())
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_removeCertificationEntry: certificationID is not defined");

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"certificationID is empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}
		else
		{
			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(DEBUG, "int main(void): action == AJAX_removeCertificationEntry: re-login required");
				}

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str().c_str());
			}


			ost.str("");
			ost << "delete from `users_certifications` where `id`='" << certificationID << "' and `user_id`='" << user.GetID() << "';";
			db.Query(ost.str());

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"success\"," << std::endl;
			ostFinal << "\"description\" : \"\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeCertificationEntry: end");
		}
	}

	// --- AJAX remove course path
	if(action == "AJAX_removeCourseEntry")
	{
		ostringstream	ost, ostFinal;
		CMysql			db1;
		string			courseID;

		courseID	= indexPage.GetVarsHandler()->Get("id");

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeCourseEntry: start");
		}


		if(courseID.empty())
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_removeCourseEntry: courseID is not defined");

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"courseID is empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}
		else
		{
			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(DEBUG, "int main(void): action == AJAX_removeCourseEntry: re-login required");
				}

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str().c_str());
			}


			ost.str("");
			ost << "delete from `users_courses` where `id`='" << courseID << "' and `user_id`='" << user.GetID() << "';";
			db.Query(ost.str());

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"success\"," << std::endl;
			ostFinal << "\"description\" : \"\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeCourseEntry: end");
		}
	}

	// --- AJAX remove school path
	if(action == "AJAX_removeSchoolEntry")
	{
		ostringstream	ost, ostFinal;
		CMysql			db1;
		string			schoolID;

		schoolID	= indexPage.GetVarsHandler()->Get("id");

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeSchoolEntry: start");
		}


		if(schoolID.empty())
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_removeSchoolEntry: schoolID is not defined");

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"schoolID is empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}
		else
		{
			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(DEBUG, "int main(void): action == AJAX_removeSchoolEntry: re-login required");
				}

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str().c_str());
			}


			ost.str("");
			ost << "delete from `users_school` where `id`='" << schoolID << "' and `user_id`='" << user.GetID() << "';";
			db.Query(ost.str());

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"success\"," << std::endl;
			ostFinal << "\"description\" : \"\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeSchoolEntry: end");
		}
	}

	// --- AJAX remove University path
	if(action == "AJAX_removeUniversityEntry")
	{
		ostringstream	ost, ostFinal;
		CMysql			db1;
		string			universityID;

		universityID	= indexPage.GetVarsHandler()->Get("id");

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeUniversityEntry: start");
		}


		if(universityID.empty())
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_removeUniversityEntry: universityID is not defined");

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"universityID is empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}
		else
		{
			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(DEBUG, "int main(void): action == AJAX_removeUniversityEntry: re-login required");
				}

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str().c_str());
			}


			ost.str("");
			ost << "delete from `users_university` where `id`='" << universityID << "' and `user_id`='" << user.GetID() << "';";
			db.Query(ost.str());

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"success\"," << std::endl;
			ostFinal << "\"description\" : \"\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeUniversityEntry: end");
		}
	}

	// --- AJAX remove Language path
	if(action == "AJAX_removeLanguageEntry")
	{
		ostringstream	ost, ostFinal;
		CMysql			db1;
		string			languageID;

		languageID	= indexPage.GetVarsHandler()->Get("id");

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeLanguageEntry: start");
		}


		if(languageID.empty())
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_removeLanguageEntry: languageID is not defined");

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"languageID is empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}
		else
		{
			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(DEBUG, "int main(void): action == AJAX_removeLanguageEntry: re-login required");
				}

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str().c_str());
			}


			ost.str("");
			ost << "delete from `users_language` where `id`='" << languageID << "' and `user_id`='" << user.GetID() << "';";
			db.Query(ost.str());

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"success\"," << std::endl;
			ostFinal << "\"description\" : \"\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeLanguageEntry: end");
		}
	}

	// --- AJAX remove skill path
	if(action == "AJAX_removeCompanyFounder")
	{
		ostringstream	ost, ostFinal;
		CMysql			db1;
		string			companyFounderID;

		companyFounderID	= indexPage.GetVarsHandler()->Get("id");

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeCompanyFounder: start");
		}


		if(companyFounderID.empty())
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_removeCompanyFounder: companyFounderID is not defined");

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"companyFounderID is empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}
		else
		{
			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(DEBUG, "int main(void): action == AJAX_removeCompanyFounder: re-login required");
				}

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str().c_str());
			}


			ost.str("");
			ost << "select * from `company_founder` where `id`='" << companyFounderID << "';";
			if(db.Query(ost.str()))
			{			
				string		companyID = db.Get(0, "companyID");

				ost.str("");
				ost << "select * from `company` where `id`='" << companyID << "' and admin_userID=\"" << user.GetID() << "\";";
				if(db.Query(ost.str()))
				{			

					ost.str("");
					ost << "delete from `company_founder` where `id`='" << companyFounderID << "';";
					db.Query(ost.str());
					ost.str("");
					ost << "delete from `feed` where `actionTypeId`=\"1001\" and `actionId`='" << companyFounderID << "';";
					db.Query(ost.str());

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"," << std::endl;
					ostFinal << "\"description\" : \"\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{
					CLog	log;
					log.Write(DEBUG, "int main(void): action == AJAX_removeCompanyFounder: company is not belongs to you or empty");

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"company is not belongs to you or empty\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
			}
			else
			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == AJAX_removeCompanyFounder: can't found companyFounderID");

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"companyFounderID not found\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeCompanyFounder: end");
		}
	}

	// --- AJAX remove skill path
	if(action == "AJAX_removeSkillEntry")
	{
		ostringstream	ost, ostFinal;
		CMysql			db1;
		string			skillID;

		skillID	= indexPage.GetVarsHandler()->Get("id");

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeSkillEntry: start");
		}


		if(skillID.empty())
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_removeSkillEntry: skillID is not defined");

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"skillID is empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}
		else
		{
			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(DEBUG, "int main(void): action == AJAX_removeSkillEntry: re-login required");
				}

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str().c_str());
			}


			ost.str("");
			ost << "select * from `users_skill` where `id`='" << skillID << "' and `user_id`='" << user.GetID() << "';";
			if(db.Query(ost.str()))
			{			
				ost.str("");
				ost << "delete from `users_skill` where `id`='" << skillID << "' and `user_id`='" << user.GetID() << "';";
				db.Query(ost.str());
				ost.str("");
				ost << "delete from `users_notification` where `actionTypeId`=\"44\" and `actionId`='" << skillID << "';";
				db.Query(ost.str());

				ost.str("");
				ost << "delete from `users_notification` where `actionTypeId`=\"43\" and `actionId` in (select `id` from `skill_confirmed` where `users_skill_id`='" << skillID << "');";
				db.Query(ost.str());
				ost.str("");
				ost << "delete from `skill_confirmed` where `users_skill_id`='" << skillID << "';";
				db.Query(ost.str());



				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"success\"," << std::endl;
				ostFinal << "\"description\" : \"\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
			else
			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == AJAX_removeSkillEntry: skillID is not belongs to you or empty");

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"skillID is empty\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeSkillEntry: end");
		}
	}


	// --- AJAX remove recommendation entry
	if(action == "AJAX_removeRecommendationEntry")
	{
		ostringstream	ost, ostFinal;
		CMysql			db1;
		string			recommendationID;

		recommendationID	= indexPage.GetVarsHandler()->Get("id");

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeRecommendationEntry: start");
		}


		if(recommendationID.empty())
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_removeRecommendationEntry: recommendationID is not defined");

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"recommendationID is empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}
		else
		{
			if(user.GetLogin() == "Guest")
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(DEBUG, "int main(void): action == AJAX_removeRecommendationEntry: re-login required");
				}

				ost.str("");
				ost << "/?rand=" << GetRandom(10);
				indexPage.Redirect(ost.str().c_str());
			}


			ost.str("");
			ost << "select * from `users_recommendation` where `id`='" << recommendationID << "' and (`recommending_userID`='" << user.GetID() << "' or `recommended_userID`='" << user.GetID() << "');";
			if(db.Query(ost.str()))
			{			
				string	recommended_userID = db.Get(0, "recommended_userID");
				string	recommending_userID = db.Get(0, "recommending_userID");

				ost.str("");
				ost << "delete from `users_recommendation` where `id`='" << recommendationID << "' and (`recommending_userID`='" << user.GetID() << "' or `recommended_userID`='" << user.GetID() << "');";
				db.Query(ost.str());

				// --- delete from users_notification
				// --- this keeps user notification consistent
				// --- otherwise notification related to insert recommendation  will looks empty, without friend name
				// --- because of "friend name" keeps in users_recommendation table 
				ost.str("");
				ost << "delete from `users_notification` where (`actionTypeId`='45' or `actionTypeId`='48') and `actionId`=\"" << recommendationID << "\";";
				db.Query(ost.str());

				// --- Update live feed
				ost.str("");
				if(user.GetID() == recommended_userID)
				{
					ost << "insert into `users_notification` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << recommending_userID << "\", \"46\", \"" << recommended_userID << "\", TIMESTAMPDIFF(second, \"1970-01-01\", NOW()))";
				}
				else
				{
					ost << "insert into `users_notification` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << recommended_userID << "\", \"47\", \"" << recommending_userID << "\", TIMESTAMPDIFF(second, \"1970-01-01\", NOW()))";
				}
				if(db.InsertQuery(ost.str()))
				{
					ostFinal.str("");
					ostFinal << "{ \"result\":\"success\", \"description\":\"\" }";
				}
				else
				{
					ostFinal.str("");
					ostFinal << "{ \"result\":\"error\", \"description\":\"ERROR inserting into DB `feed`\" }";

					{
						CLog	log;

						ost.str("");
						ost << "int main(void): action == AJAX_removeRecommendationEntry: ERROR inserting into `feed` (" << ost.str() << ")";
						log.Write(ERROR, ost.str());
					}
				}
			}
			else
			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == AJAX_removeRecommendationEntry: recommendationID is not belongs to you or empty");

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"recommendationID is empty or not belongs to you\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_removeRecommendationEntry: end");
		}
	}

	// --- AJAX job titeles
	if(action == "AJAX_getJobTitles") 
	{
		ostringstream	ost;
		string		sessid;
		int			affected;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_getJobTitles: start");
		}

		ost.str("");
		ost << "select * from `users_company_position`;";
		if((affected = db.Query(ost.str())) > 0)
		{
			ost.str("");
			for(int i = 0; i < affected; i++) 
			{
//				ost << "{ \"label\": \"" << db.Get(i, "title") << "\", \"category\": \"" << db.Get(i, "area") << "\" }";
				ost << "\"" << db.Get(i, "title") << "\"";
				if(i < (affected-1)) ost << ", ";
			}
		}
		else
		{
			CLog	log;

			log.Write(ERROR, "int main(void): action == AJAX_getJobTitles: ERROR table users_company_position is empty");
			ost.str("");
		}

		indexPage.RegisterVariableForce("result", ost.str());

		if(!indexPage.SetTemplate("ajax_getJobTitles.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file ajax_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	// --- JSON company names
	if(action == "AJAX_getCompanyName") 
	{
		ostringstream	ost;
		string		sessid;
		int			affected;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_getCompanyName: start");
		}

		ost.str("");
		ost << "select * from `company`;";
		if((affected = db.Query(ost.str())) > 0)
		{
			ost.str("");
			for(int i = 0; i < affected; i++) 
			{
//				ost << "{ \"label\": \"" << db.Get(i, "title") << "\", \"category\": \"" << db.Get(i, "area") << "\" }";
				ost << "\"" << db.Get(i, "name") << "\"";
				if(i < (affected-1)) ost << ", ";
			}
		}
		else
		{
			CLog	log;

			log.Write(ERROR, "int main(void): action == AJAX_getCompanyName: ERROR table compnay is empty");
			ost.str("");
		}

		indexPage.RegisterVariableForce("result", ost.str());

		if(!indexPage.SetTemplate("ajax_getJobTitles.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file ajax_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	// --- AJAX get certification vendors
	if(action == "AJAX_getCertificationTracks") 
	{
		ostringstream	ost;
		string		sessid;
		int			affected;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_getCertificationTracks: start");
		}

		ost.str("");
		ost << "select * from `certification_tracks`;";
		if((affected = db.Query(ost.str())) > 0)
		{
			ost.str("");
			for(int i = 0; i < affected; i++) 
			{
//				ost << "{ \"label\": \"" << db.Get(i, "title") << "\", \"category\": \"" << db.Get(i, "area") << "\" }";
				ost << "\"" << db.Get(i, "title") << "\"";
				if(i < (affected-1)) ost << ", ";
			}
		}
		else
		{
			CLog	log;

			log.Write(ERROR, "int main(void): action == AJAX_getCertificationTracks: ERROR table users_company_position is empty");
			ost.str("");
		}

		indexPage.RegisterVariableForce("result", ost.str());

		if(!indexPage.SetTemplate("ajax_getJobTitles.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file ajax_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	// --- AJAX get data for profile
	if(action == "AJAX_getDataForProfile") 
	{
		ostringstream	ost;
		string		sessid;
		int			affected;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_getDataForProfile: start");
		}

		ost.str("");
		ost << "select * from `geo_country`;";
		if((affected = db.Query(ost.str())) > 0)
		{
			ost.str("");
			for(int i = 0; i < affected; i++) 
			{
//				ost << "{ \"label\": \"" << db.Get(i, "title") << "\", \"category\": \"" << db.Get(i, "area") << "\" }";
				ost << "{\"id\":\"" << db.Get(i, "id") << "\",\"title\":\"" << db.Get(i, "title") << "\"}";
				if(i < (affected-1)) ost << ", ";
			}
		}
		else
		{
			CLog	log;

			log.Write(ERROR, "int main(void): action == AJAX_getDataForProfile: ERROR table geo_country is empty");
			ost.str("");
		}

		indexPage.RegisterVariableForce("geo_country", ost.str());

		ost.str("");
		ost << "select * from `geo_region`;";
		if((affected = db.Query(ost.str())) > 0)
		{
			ost.str("");
			for(int i = 0; i < affected; i++) 
			{
//				ost << "{ \"label\": \"" << db.Get(i, "title") << "\", \"category\": \"" << db.Get(i, "area") << "\" }";
				ost << "{\"id\":\"" << db.Get(i, "id") << "\",\"geo_country_id\":\"" << db.Get(i, "geo_country_id") << "\",\"title\":\"" << db.Get(i, "title") << "\"}";
				if(i < (affected-1)) ost << ", ";
			}
		}
		else
		{
			CLog	log;

			log.Write(ERROR, "int main(void): action == AJAX_getDataForProfile: ERROR table geo_region is empty");
			ost.str("");
		}

		indexPage.RegisterVariableForce("geo_region", ost.str());

		ost.str("");
		ost << "select * from `geo_locality`;";
		if((affected = db.Query(ost.str())) > 0)
		{
			ost.str("");
			for(int i = 0; i < affected; i++) 
			{
//				ost << "{ \"label\": \"" << db.Get(i, "title") << "\", \"category\": \"" << db.Get(i, "area") << "\" }";
				ost << "{\"id\":\"" << db.Get(i, "id") << "\",\"geo_region_id\":\"" << db.Get(i, "geo_region_id") << "\",\"title\":\"" << db.Get(i, "title") << "\"}";
				if(i < (affected-1)) ost << ", ";
			}
		}
		else
		{
			CLog	log;

			log.Write(ERROR, "int main(void): action == AJAX_getDataForProfile: ERROR table geo_locality is empty");
			ost.str("");
		}

		indexPage.RegisterVariableForce("geo_locality", ost.str());

		ost.str("");
		ost << "select * from `university`;";
		if((affected = db.Query(ost.str())) > 0)
		{
			ost.str("");
			for(int i = 0; i < affected; i++) 
			{
//				ost << "{ \"label\": \"" << db.Get(i, "title") << "\", \"category\": \"" << db.Get(i, "area") << "\" }";
				ost << "{\"id\":\"" << db.Get(i, "id") << "\",\"geo_region_id\":\"" << db.Get(i, "geo_region_id") << "\",\"title\":\"" << db.Get(i, "title") << "\"}";
				if(i < (affected-1)) ost << ", ";
			}
		}
		else
		{
			CLog	log;

			log.Write(ERROR, "int main(void): action == AJAX_getDataForProfile: ERROR table university is empty");
			ost.str("");
		}

		indexPage.RegisterVariableForce("university", ost.str());

		ost.str("");
		ost << "select * from `school`;";
		if((affected = db.Query(ost.str())) > 0)
		{
			ost.str("");
			for(int i = 0; i < affected; i++) 
			{
				ost << "{\"id\":\"" << db.Get(i, "id") << "\",\"geo_locality_id\":\"" << db.Get(i, "geo_locality_id") << "\",\"title\":\"" << db.Get(i, "title") << "\"}";
				if(i < (affected-1)) ost << ", ";
			}
		}
		else
		{
			CLog	log;

			log.Write(ERROR, "int main(void): action == AJAX_getDataForProfile: ERROR table school is empty");
			ost.str("");
		}

		indexPage.RegisterVariableForce("school", ost.str());

		ost.str("");
		ost << "select * from `language`;";
		if((affected = db.Query(ost.str())) > 0)
		{
			ost.str("");
			for(int i = 0; i < affected; i++) 
			{
				ost << "{\"id\":\"" << db.Get(i, "id") << "\",\"title\":\"" << db.Get(i, "title") << "\"}";
				if(i < (affected-1)) ost << ", ";
			}
		}
		else
		{
			CLog	log;

			log.Write(ERROR, "int main(void): action == AJAX_getDataForProfile: ERROR table language is empty");
			ost.str("");
		}

		indexPage.RegisterVariableForce("language", ost.str());

		ost.str("");
		ost << "select * from `skill`;";
		if((affected = db.Query(ost.str())) > 0)
		{
			ost.str("");
			for(int i = 0; i < affected; i++) 
			{
				ost << "{\"id\":\"" << db.Get(i, "id") << "\",\"title\":\"" << db.Get(i, "title") << "\"}";
				if(i < (affected-1)) ost << ", ";
			}
		}
		else
		{
			CLog	log;

			log.Write(ERROR, "int main(void): action == AJAX_getDataForProfile: ERROR table skill is empty");
			ost.str("");
		}

		indexPage.RegisterVariableForce("skill", ost.str());

		if(!indexPage.SetTemplate("ajax_getDataForProfile.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file ajax_getDataForProfile.htmlt was missing");
			throw CException("Template file was missing");
		}
	}



	// --- AJAX delete preview avatar
	if(action == "AJAX_deleteAvatar") 
	{
		ostringstream	result;
		ostringstream	ost;
		string		sessid, avatarID;
		int			affected;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void)::action == AJAX_deleteAvatar: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_deleteAvatar: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		avatarID = indexPage.GetVarsHandler()->Get("id");

		ost << "select * from `users_avatars` where `id`=\"" << avatarID << "\";";
		affected = db.Query(ost.str());
		if((affected = db.Query(ost.str())) > 0)
		{
			if(db.Get(0, "userid") == user.GetID())
			{
				string filename;

				filename += IMAGE_AVATAR_DIRECTORY;
				filename += "/avatars";
				filename += db.Get(0, "folder");
				filename += "/";
				filename += db.Get(0, "filename");

				{
					CLog	log;

					ost.str("");
					ost << "int main(void): action == AJAX_deleteAvatar: removing avatar [id=" << avatarID << "] belongs to user " << user.GetLogin() << " [filename=" << filename << "]";
					log.Write(DEBUG, ost.str());
				}

				ost.str("");
				ost << "delete from `users_avatars` where `id`=\"" << avatarID << "\";";
				db.Query(ost.str());

				if(isFileExists(filename))
				{
					unlink(filename.c_str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"9\", \"0\", NOW())";
					if(db.InsertQuery(ost.str()))
					{
						result.str("");
						result << "{ \"result\":\"success\", \"description\":\"\" }";
					}
					else
					{
						result.str("");
						result << "{ \"result\":\"error\", \"description\":\"ERROR inserting into DB `feed`\" }";

						{
							CLog	log;

							ost.str("");
							ost << "int main(void): action == AJAX_deleteAvatar: ERROR inserting into `feed` (" << ost.str() << ")";
							log.Write(ERROR, ost.str());
						}
					}
				}
				else
				{
					result.str("");
					result << "{ \"result\":\"error\", \"description\":\"ERROR file is not exists\" }";

					{
						CLog	log;

						ost.str("");
						ost << "int main(void): action == AJAX_deleteAvatar: ERROR file is not exists  [filename=" << filename << "] belongs to user " << user.GetLogin();
						log.Write(ERROR, ost.str());
					}
				}
			}
			else
			{
				result.str("");
				result << "{ \"result\":\"error\", \"description\":\"avatar do not belongs to you\" }";
					
				{
					CLog	log;

					ost.str("");
					ost << "int main(void): action == AJAX_deleteAvatar: ERROR avatar [id=" << avatarID << "] do not belongs to user " << user.GetLogin();
					log.Write(ERROR, ost.str());
				}
			}
		}
		else
		{
			result.str("");
			result << "{ \"result\":\"error\", \"description\":\"there is no avatar\" }";

			{
				CLog	log;

				ost.str("");
				log.Write(DEBUG, "int main(void): action == AJAX_deleteAvatar: there is no avatar [id=", avatarID, "]");
			}
		}

		indexPage.RegisterVariableForce("result", result.str());

		if(!indexPage.SetTemplate("ajax_getJobTitles.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file ajax_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	// --- AJAX change in friendship status
	if(action == "AJAX_setFindFriend_FriendshipStatus") 
	{
		ostringstream	ost, result;
		string			sessid, friendID, currentFriendshipStatus, requestedFriendshipStatus;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void)::action == AJAX_setFindFriend_FriendshipStatus: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_setFindFriend_FriendshipStatus: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		friendID = indexPage.GetVarsHandler()->Get("friendID");
		requestedFriendshipStatus = indexPage.GetVarsHandler()->Get("status");
		result.str("");

		if((friendID.length() == 0) || (requestedFriendshipStatus.length() == 0))
		{
			ostringstream	ost2;
			ost2.str("");
			ost2 << "int main()::AJAX_setFindFriend_FriendshipStatus: ERROR: friendID [" << friendID << "] or status [" << requestedFriendshipStatus << "] is empty";

			result.str("");
			result << "{ \"result\":\"error\", \"description\":\"" << ost2.str() << "\" }";

			{
				CLog	log;
				log.Write(ERROR, ost2.str());
			}
		}
		else
		{
			int				affected;

			ost.str("");
			ost << "select * from `users_friends` where `userID`='" << user.GetID() << "' and `friendID`='" << friendID << "';";
			affected = db.Query(ost.str());
			if(affected)
			{
				currentFriendshipStatus = db.Get(0, "state");
			}
			else
			{
				currentFriendshipStatus = "empty";
			}
			if((requestedFriendshipStatus == "requested") || (requestedFriendshipStatus == "requesting")) 
			{
				if((currentFriendshipStatus == "empty") || (currentFriendshipStatus == "ignored"))
				{
					ost.str("");
					ost << "START TRANSACTION;";
					ost << "insert into `users_friends` (`userID`, `friendID`, `state`, `date`) \
							VALUES (\
							\"" << user.GetID() << "\", \
							\"" << friendID << "\", \
							\"requesting\", \
							NOW() \
							);";
					ost << "insert into `users_friends` (`userID`, `friendID`, `state`, `date`) \
							VALUES (\
							\"" << friendID << "\", \
							\"" << user.GetID() << "\", \
							\"requested\", \
							NOW() \
							);";
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) \
							VALUES(\
							\"\", \
							\"" << user.GetID() << "\", \
							\"16\", \
							\"" << friendID << "\",  \
							NOW());";
					ost << "COMMIT;";
					db.Query(ost.str());

					result.str("");
					result << "{ \"result\":\"ok\", \"description\":\"\" }";

				}
				else
				{
					ostringstream	ost2;
					ost2.str("");
					ost2 << "int main()::AJAX_setFindFriend_FriendshipStatus: FSM ERROR: can't move from " << currentFriendshipStatus << " to " << requestedFriendshipStatus << " ";

					result.str("");
					result << "{ \"result\":\"error\", \"description\":\"" << ost2.str() << "\" }";

					{
						CLog	log;
						log.Write(ERROR, ost2.str());
					} // CLog
				}
			}
			else if (requestedFriendshipStatus == "disconnect")
			{

				if((currentFriendshipStatus == "requested") || (currentFriendshipStatus == "requesting") || (currentFriendshipStatus == "confirmed") || (currentFriendshipStatus == "blocked"))
				{
					ost.str("");
					ost << "delete from `users_friends` where  \
							(`userID`=\"" << user.GetID() << "\" and `friendID`=\"" << friendID << "\" ) \
							OR \
							(`friendID`=\"" << user.GetID() << "\" and `userID`=\"" << friendID << "\" );";
					db.Query(ost.str());

					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"15\", \"" << friendID << "\", NOW())";
					db.Query(ost.str());

					result.str("");
					result << "{ \"result\":\"ok\", \"description\":\"\" }";
				}
				else
				{
					ostringstream	ost2;
					ost2.str("");
					ost2 << "int main()::AJAX_setFindFriend_FriendshipStatus: FSM ERROR: can't move from " << currentFriendshipStatus << " to " << requestedFriendshipStatus << " ";

					result.str("");
					result << "{ \"result\":\"error\", \"description\":\"" << ost2.str() << "\" }";

					{
						CLog	log;
						log.Write(ERROR, ost2.str());
					} // CLog
				}

			}
			else if(requestedFriendshipStatus == "confirm")
			{
				if((currentFriendshipStatus == "requested") || (currentFriendshipStatus == "requesting") || (currentFriendshipStatus == "confirmed") || (currentFriendshipStatus == "blocked"))
				{
					ost.str("");
					ost << "update `users_friends` set `state`='confirmed', `date`=NOW() where  \
							(`userID`=\"" << user.GetID() << "\" and `friendID`=\"" << friendID << "\") \
							OR \
							(`friendID`=\"" << user.GetID() << "\" and `userID`=\"" << friendID << "\" );";
					db.Query(ost.str());

					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"14\", \"" << friendID << "\", NOW())";
					db.Query(ost.str());
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << friendID << "\", \"14\", \"" << user.GetID() << "\", NOW())";
					db.Query(ost.str());

					result.str("");
					result << "{ \"result\":\"ok\", \"description\":\"\" }";
				}
				else
				{
					ostringstream	ost2;
					ost2.str("");
					ost2 << "int main()::AJAX_setFindFriend_FriendshipStatus: FSM ERROR: can't move from " << currentFriendshipStatus << " to " << requestedFriendshipStatus << " ";

					result.str("");
					result << "{ \"result\":\"error\", \"description\":\"" << ost2.str() << "\" }";

					{
						CLog	log;
						log.Write(ERROR, ost2.str());
					} // CLog
				}
			}
			else
			{
				ostringstream	ost2;
				ost2.str("");
				ost2 << "int main()::AJAX_setFindFriend_FriendshipStatus: ERROR: requested friendship status is unknown [" << requestedFriendshipStatus << "] friendID [" << friendID << "]";

				result.str("");
				result << "{ \"result\":\"error\", \"description\":\"" << ost2.str() << "\" }";

				{
					CLog	log;
					log.Write(ERROR, ost2.str());
				} // CLog
			} // Check friendship status
		} // if((friendID.length() == 0) || (status.length() == 0))

		indexPage.RegisterVariableForce("result", result.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file ajax_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	// --- AJAX block user account
	if(action == "AJAX_block_user_account") 
	{
		ostringstream	ost, result;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void)::action == AJAX_block_user_account: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_block_user_account: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		ost.str("");
		ost << "update `users` set `isblocked`='Y' where `id`='" << user.GetID() << "';";
		db.Query(ost.str());
		ost.str("");
		ost << "select `isblocked` from `users` where `id`='" << user.GetID() << "';";
		if(db.Query(ost.str()))
		{
			string	blockStatus = db.Get(0, "isblocked");

			if(blockStatus == "Y")
			{

				// --- update news feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"17\", \"0\", NOW());";
				db.Query(ost.str());
	
				result.str("");
				result << "{ \"result\":\"success\", \"description\":\"\" }";
			}
			else
			{
				ostringstream	ost2;
				ost2.str("");
				ost2 << "int main()::AJAX_block_user_account: ERROR userID [" << user.GetID() << "] can't block";

				result.str("");
				result << "{ \"result\":\"error\", \"description\":\"userID [" << user.GetID() << "] can't block\" }";

				{
					CLog	log;
					log.Write(ERROR, ost2.str());
				} // CLog
			}
		}
		else
		{
			ostringstream	ost2;
			ost2.str("");
			ost2 << "int main()::AJAX_block_user_account: ERROR userID [" << user.GetID() << "] has not been found";

			result.str("");
			result << "{ \"result\":\"error\", \"description\":\"userID [" << user.GetID() << "] has not been found\" }";

			{
				CLog	log;
				log.Write(ERROR, ost2.str());
			} // CLog
		}

		indexPage.RegisterVariableForce("result", result.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file ajax_response.htmlt was missing");
			throw CException("Template file was missing");
		}

	}

	// --- AJAX enable user account
	if(action == "AJAX_enable_user_account") 
	{
		ostringstream	ost, result;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void)::action == AJAX_enable_user_account: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_enable_user_account: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		ost.str("");
		ost << "update `users` set `isblocked`='N' where `id`='" << user.GetID() << "';";
		db.Query(ost.str());
		ost.str("");
		ost << "select `isblocked` from `users` where `id`='" << user.GetID() << "';";
		if(db.Query(ost.str()))
		{
			string	blockStatus = db.Get(0, "isblocked");

			if(blockStatus == "N")
			{

				// --- update news feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"18\", \"0\", NOW());";
				db.Query(ost.str());
	
				result.str("");
				result << "{ \"result\":\"success\", \"description\":\"\" }";
			}
			else
			{
				ostringstream	ost2;
				ost2.str("");
				ost2 << "int main()::AJAX_enable_user_account: ERROR userID [" << user.GetID() << "] can't enable";

				result.str("");
				result << "{ \"result\":\"error\", \"description\":\"userID [" << user.GetID() << "] can't enable\" }";

				{
					CLog	log;
					log.Write(ERROR, ost2.str());
				} // CLog
			}
		}
		else
		{
			ostringstream	ost2;
			ost2.str("");
			ost2 << "int main()::AJAX_enable_user_account: ERROR userID [" << user.GetID() << "] has not been found";

			result.str("");
			result << "{ \"result\":\"error\", \"description\":\"userID [" << user.GetID() << "] has not been found\" }";

			{
				CLog	log;
				log.Write(ERROR, ost2.str());
			} // CLog
		}

		indexPage.RegisterVariableForce("result", result.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file ajax_response.htmlt was missing");
			throw CException("Template file was missing");
		}

	}


	// --- JSON like handler
	if(action == "JSON_ClickLikeHandler")
	{
		ostringstream	ost, ostFinal;
		string			sessid, messageId, userList = "";
		char			*convertBuffer = new char[1024];
		int				affected;
		bool			isLiked;
		unsigned long	feed_message_params_id = 0;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_ClickLikeHandler: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::JSON_ClickLikeHandler: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		messageId = indexPage.GetVarsHandler()->Get("messageId");

		memset(convertBuffer, 0, 1024);
		convert_utf8_to_windows1251(messageId.c_str(), convertBuffer, 1024);
		messageId = convertBuffer;
		trim(messageId);

		delete[] convertBuffer;

		// --- Clean-up the text
		messageId = ReplaceDoubleQuoteToQuote(messageId);
		messageId = DeleteHTML(messageId);
		messageId = SymbolReplace(messageId, "\r\n", "");
		messageId = SymbolReplace(messageId, "\r", "");
		messageId = SymbolReplace(messageId, "\n", "");

		ost.str("");
		ost << "select * from `feed_message_params` where `parameter`='like' and `messageID`='" << messageId << "' and `userID`=\"" << user.GetID() << "\";";
		affected = db.Query(ost.str());
		if(affected)
		{
			string	feed_message_params_id_to_remove = db.Get(0, "id");

			isLiked = false;
			// --- remove "my like"
			ost.str("");
			ost << "delete from `feed_message_params` where `parameter`='like' and `messageID`='" << messageId << "' and `userID`=\"" << user.GetID() << "\";";
			db.Query(ost.str());

			// --- remove it from users_notifications
			ost.str("");
			ost << "delete from `users_notification` where `actionTypeId`='49' and `actionId`='" << feed_message_params_id_to_remove << "';";
			db.Query(ost.str());
			
		}
		else
		{
			isLiked = true;
			// --- add "my like"
			ost.str("");
			ost << "insert into  `feed_message_params` (`id`, `parameter`, `messageID`, `userID`) VALUES (NULL, 'like', '" << messageId << "', '" << user.GetID() << "');";
			feed_message_params_id = db.InsertQuery(ost.str());
		}

		ost.str("");
		ost << "select * from `feed_message_params` where `parameter`='like' and `messageID`='" << messageId << "';";
		affected = db.Query(ost.str());
		ost.str("");
		ost << "select * from `users` where `id` in (";
		if(affected)
		{
			for(int i = 0; i < affected; i++)
			{
				if(i > 0) ost << ",";
				ost << db.Get(i, "userID");
			}
		}
		ost << ");";
		userList = GetUserListInJSONFormat(ost.str(), &db, &user);

		ostFinal.str("");
		ostFinal << "{" << std::endl;
		ostFinal << "\"result\" : \"success\"," << std::endl;
		ostFinal << "\"description\" : \"\"," << std::endl;
		ostFinal << "\"messageLikesUserList\" : [" << std::endl << userList << std::endl << "]" << std::endl;
		ostFinal << "}" << std::endl;

		ost.str("");
		ost << "select * from `feed` where `actionTypeId`=\"11\" and `actionId`=\"" << messageId << "\";";
		if(db.Query(ost.str()))
		{
			string		messageUserOwnerID = db.Get(0, "userId");

			ost.str("");
			if(isLiked)
			{
				ost << "insert into `users_notification` (`userId`, `actionTypeId`, `actionId`, `eventTimestamp`) VALUES (\"" << messageUserOwnerID << "\", \"49\", \"" << feed_message_params_id << "\", TIMESTAMPDIFF(second, \"1970-01-01\", NOW()));";
			}
			else
			{
				ost << "insert into `users_notification` (`userId`, `actionTypeId`, `actionId`, `eventTimestamp`) VALUES (\"" << messageUserOwnerID << "\", \"50\", \"" << messageId << "\", TIMESTAMPDIFF(second, \"1970-01-01\", NOW()));";
			}
			if(db.InsertQuery(ost.str()))
			{

			}
			else
			{
				CLog	log;
				log.Write(ERROR, "int main(void):action == JSON_ClickLikeHandler: ERROR inserting into users_notification table");
			}
			
		}
		else
		{
			CLog	log;
			log.Write(ERROR, "int main(void):action == JSON_ClickLikeHandler: ERROR messageID [", messageId, "] can't be found");
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}
		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_ClickLikeHandler: end");
		}
	}


	// --- JSON returns shake hands in view profile
	// ---		common friends
	// ---		common companies
	if(action == "JSON_getShakeHands")
	{
		ostringstream	ost, ostFinal;
		string			sessid, lookForKey;
		string			user1, user2, handshakeUserStatus = "";
		string			user1Data, user2Data, hopUserList;
		char			*convertBuffer = new char[1024];
		CMysql			db1;
		vector<int>		vectorFriendList1, vectorFriendList2, vectorFriendList3;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_getShakeHands: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::JSON_getShakeHands: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		user1 = indexPage.GetVarsHandler()->Get("user1");
		user2 = indexPage.GetVarsHandler()->Get("user2");

		memset(convertBuffer, 0, 1024);
		convert_utf8_to_windows1251(user1.c_str(), convertBuffer, 1024);
		user1 = convertBuffer;
		trim(user1);

		delete[] convertBuffer;

		// --- Clean-up the text
		user1 = ReplaceDoubleQuoteToQuote(user1);
		user1 = DeleteHTML(user1);
		user1 = SymbolReplace(user1, "\r\n", "");
		user1 = SymbolReplace(user1, "\r", "");
		user1 = SymbolReplace(user1, "\n", "");
		user2 = ReplaceDoubleQuoteToQuote(user2);
		user2 = DeleteHTML(user2);
		user2 = SymbolReplace(user2, "\r\n", "");
		user2 = SymbolReplace(user2, "\r", "");
		user2 = SymbolReplace(user2, "\n", "");


		if(user1 != user2)
		{
			int				affected;

			ost.str("");
			ost << "SELECT `users_friends`.`friendID` from `users_friends` \
					RIGHT JOIN `users` on `users_friends`.`friendID` = `users`.`id` \
					WHERE `users_friends`.`userID`='" << user1 << "' and `users`.`isblocked`='N' and  `users`.`isactivated`='Y' and `users_friends`.`state`='confirmed';";
			affected = db.Query(ost.str());
			if(affected)
			{
				for(int i = 0; i < affected; i++)
				{
					vectorFriendList1.push_back(atoi(db.Get(i, "friendID")));

					if(atoi(user2.c_str()) == atoi(db.Get(i, "friendID")))
					{
						handshakeUserStatus = "directFriends";
					}
				}


				if(handshakeUserStatus == "")
				{

					ost.str("");
					ost << "SELECT `users_friends`.`friendID` from `users_friends` \
							RIGHT JOIN `users` on `users_friends`.`friendID` = `users`.`id` \
							WHERE `users_friends`.`userID`='" << user2 << "' and `users`.`isblocked`='N' and  `users`.`isactivated`='Y' and `users_friends`.`state`='confirmed';";
					affected = db.Query(ost.str());
					if(affected)
					{
						for(int i = 0; i < affected; i++)
						{
							vectorFriendList2.push_back(atoi(db.Get(i, "friendID")));
						}
					}

					for(vector<int>::iterator it1 = vectorFriendList1.begin(); it1 != vectorFriendList1.end(); ++it1)
					{
						for(vector<int>::iterator it2 = vectorFriendList2.begin(); it2 != vectorFriendList2.end(); ++it2)
						{
							if(*it1 == *it2)
							{
								vectorFriendList3.push_back(*it1);
							}
						}

					}

					if(vectorFriendList3.size() > 0)
					{
						handshakeUserStatus = "1hop";
				
						ost.str("");
						ost << "select * from `users` where `isActivated`='Y' and `isblocked`='N' and `id` in (";
						for(vector<int>::iterator it = vectorFriendList3.begin(); it != vectorFriendList3.end(); ++it)
						{
							if(it != vectorFriendList3.begin())	ost << ",";
							ost << *it;
						}
						ost << ");";
						hopUserList = GetUserListInJSONFormat(ost.str(), &db, &user);
					}
					else
					{
						handshakeUserStatus = "noConnection";
					}
				}
			}
			else
			{
				handshakeUserStatus = "noConnection";
			}
		}
		else
		{
			handshakeUserStatus = "sameUser";
		}

		ost.str("");
		ost << "select * from `users` where `isActivated`='Y' and `isblocked`='N' and `id`='" << user1 << "'";
		user1Data = GetUserListInJSONFormat(ost.str(), &db, &user);

		ost.str("");
		ost << "select * from `users` where `isActivated`='Y' and `isblocked`='N' and `id`='" << user2 << "'";
		user2Data = GetUserListInJSONFormat(ost.str(), &db, &user);


		ostFinal.str("");
		ostFinal << "{" << std::endl;
		ostFinal << "\"result\" : \"success\"," << std::endl;
		ostFinal << "\"description\" : \"\"," << std::endl;
		ostFinal << "\"handshakeUserStatus\" : \"" << handshakeUserStatus << "\"," << std::endl;
		ostFinal << "\"handshakeUsers\" : [" << std::endl << hopUserList << std::endl << "]," << std::endl;
		ostFinal << "\"user1\" : " << std::endl << user1Data << std::endl << "," << std::endl;
		ostFinal << "\"user2\" : " << std::endl << user2Data << std::endl << "" << std::endl;
		ostFinal << "}" << std::endl;

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_getShakeHands: finish");
		}
	}

	// --- JSON FindFriend by ID
	if(action == "JSON_getFindFriendByID")
	{
		ostringstream	ost, ostFinal;
		string			sessid, lookForKey, userList;
		char			*convertBuffer;
		CMysql			db1;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_getFindFriendByID: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::JSON_getFindFriendByID: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		if(db1.Connect(DB_NAME, DB_LOGIN, DB_PASSWORD) < 0)
		{
			CLog	log;
	
			log.Write(ERROR, "Can not connect to mysql database");
			return(1);
		}

#ifdef MYSQL_4
		db1.Query("set names cp1251");
#endif

		lookForKey = indexPage.GetVarsHandler()->Get("lookForKey");

		convertBuffer = new char[1024];
		memset(convertBuffer, 0, 1024);
		convert_utf8_to_windows1251(lookForKey.c_str(), convertBuffer, 1024);
		lookForKey = convertBuffer;
		trim(lookForKey);

		delete[] convertBuffer;

		// --- Clean-up the text
		lookForKey = ReplaceDoubleQuoteToQuote(lookForKey);
		lookForKey = DeleteHTML(lookForKey);
		lookForKey = SymbolReplace(lookForKey, "\r\n", "");
		lookForKey = SymbolReplace(lookForKey, "\r", "");
		lookForKey = SymbolReplace(lookForKey, "\n", "");

		ost << "select * from `users` where `isActivated`='Y' and `isblocked`='N' and `id`=\"" << lookForKey << "\" ;";

		userList = GetUserListInJSONFormat(ost.str(), &db, &user);

		ostFinal.str("");
		ostFinal << "[" << std::endl << userList << std::endl << "]" << std::endl;

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_getFindFriendByID: finish");
		}
	}

	// --- JSON friend list for autocomplete
	if((action == "JSON_getFindFriendsListAutocomplete") || (action == "JSON_getFindFriendsList"))
	{
		ostringstream	ost, ostFinal;
		string			sessid, lookForKey, userList;
		char			*convertBuffer = new char[1024];
		vector<string>	searchWords;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == JSON_getFindFriendsListAutocomplete: start");
		}

		// --- Initialization
		ostFinal.str("");

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::JSON_getFindFriendsListAutocomplete: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		lookForKey = indexPage.GetVarsHandler()->Get("lookForKey");


		memset(convertBuffer, 0, 1024);
		convert_utf8_to_windows1251(lookForKey.c_str(), convertBuffer, 1024);
		lookForKey = convertBuffer;
		trim(lookForKey);

		delete[] convertBuffer;

		// --- Clean-up the text
		lookForKey = ReplaceDoubleQuoteToQuote(lookForKey);
		lookForKey = DeleteHTML(lookForKey);
		lookForKey = SymbolReplace(lookForKey, "\r\n", "");
		lookForKey = SymbolReplace(lookForKey, "\r", "");
		lookForKey = SymbolReplace(lookForKey, "\n", "");
		if(qw(lookForKey, searchWords))
		{

			ostFinal << "[";
			{
				// --- Logging
				CLog			log;
				ostringstream	ost;
				int				i = 0;

				ost.str("");
				ost << "int main(void): action == JSON_getFindFriendsListAutocomplete: qw4(" << lookForKey << ")" << endl;
				for(vector<string>::iterator it = searchWords.begin(); it != searchWords.end(); ++it, i++)
				{
					ost << "int main(void): action == JSON_getFindFriendsListAutocomplete: parsed value at index " << i << " [" << *it << "]" << endl;
				}
				log.Write(DEBUG, ost.str());
			}

			// --- single search word can be name, surname, company name 
			if(searchWords.size() == 1)		
			{
				{
					CLog log;
					log.Write(DEBUG, "int main(void): action == JSON_getFindFriendsListAutocomplete: single word search");
				}

				// --- Looking through name, surname
				ost.str("");
				ost << "select * from `users` where `isActivated`='Y' and `isblocked`='N' and `id`!=\"" << user.GetID() << "\" and ( \
						`name` like \"%" 		<< lookForKey << "%\" or \
						`nameLast` like \"%" 	<< lookForKey << "%\" \
						);";

				userList = GetUserListInJSONFormat(ost.str(), &db, &user);
				if(userList.length() > 0) 
				{
					// --- comma required only if previous text is exists
					if(ostFinal.str().length() > 10) ostFinal << ","; 
					ostFinal  << std::endl << userList; 
				}

				// --- Looking through company title
				ost.str("");
				ost << "select * from `users` \
						left join `users_company` on `users_company`.`user_id` = `users`.`id` \
						left join `company` on `company`.`id`=`users_company`.`company_id` \
						where \
						`users`.`isActivated`='Y' and `users`.`isblocked`='N' and `users`.`id`!=\"" << user.GetID() << "\" and \
						`users_company`.`current_company`='1' and \
						`company`.`name` like \"%" 	<< lookForKey << "%\";";

				userList = GetUserListInJSONFormat(ost.str(), &db, &user);
				if(userList.length() > 0) 
				{
					// --- comma required only if previous text is exists
					if(ostFinal.str().length() > 10) ostFinal << ","; 
					ostFinal  << std::endl << userList; 
				}

			}

			// --- two words searching through DB 
			if(searchWords.size() == 2)
			{
				{
					CLog log;
					log.Write(DEBUG, "int main(void): action == JSON_getFindFriendsListAutocomplete: two words search");
				}

				// --- Looking through user name,surname and company title
				ost.str("");
				ost << "select * from `users` \
						left join `users_company` on `users_company`.`user_id` = `users`.`id` \
						left join `company` on `company`.`id`=`users_company`.`company_id` \
						where \
						`users`.`isActivated`='Y' and `users`.`isblocked`='N' and `users`.`id`!=\"" << user.GetID() << "\" and \
						`users_company`.`current_company`='1' and \
						( \
							`company`.`name` like \"%" 	<< searchWords[0] << "%\" or \
							`company`.`name` like \"%" 	<< searchWords[1] << "%\" \
						) and ( \
							`users`.`name` like \"%" 		<< searchWords[0] << "%\" or \
							`users`.`name` like \"%" 		<< searchWords[1] << "%\" or \
							`users`.`nameLast` like \"%" 	<< searchWords[0] << "%\" or \
							`users`.`nameLast` like \"%" 	<< searchWords[1] << "%\" \
						);";

				userList = GetUserListInJSONFormat(ost.str(), &db, &user);
				if(userList.length() > 0) 
				{
					// --- comma required only if previous text is exists
					if(ostFinal.str().length() > 10) ostFinal << ","; 
					ostFinal  << std::endl << userList; 
				}
				else
				{
					// --- here code will be run only if multiwork search was not sucessfull on previous step
					// --- earlier: user _and_ company is not success
					// --- here: user _or_ company
					{
						CLog log;
						log.Write(DEBUG, "int main(void): action == JSON_getFindFriendsListAutocomplete: (user _and_ company) has fail, try (user _without_ company) ");
					}

					ost.str("");
					ost << "select * from `users` where `isActivated`='Y' and `isblocked`='N' and `id`!=\"" << user.GetID() << "\" and ( \
							( \
								`users`.`name` like \"%" 		<< searchWords[1] << "%\" and \
								`users`.`nameLast` like \"%" 	<< searchWords[0] << "%\" \
							) \
							or \
							( \
								`users`.`name` like \"%" 		<< searchWords[0] << "%\" and \
								`users`.`nameLast` like \"%" 	<< searchWords[1] << "%\" \
							) \
							);";

					userList = GetUserListInJSONFormat(ost.str(), &db, &user);
					if(userList.length() > 0) 
					{
						// --- comma required only if previous text is exists
						if(ostFinal.str().length() > 10) ostFinal << ","; 
						ostFinal  << std::endl << userList; 
					}
				}

			}

			// --- three words searching through DB 
			if(searchWords.size() == 3)
			{
				{
					CLog log;
					log.Write(DEBUG, "int main(void): action == JSON_getFindFriendsListAutocomplete: three words search");
				}

				// --- Looking through user name,surname and company title
				ost.str("");
				ost << "select * from `users` \
						left join `users_company` on `users_company`.`user_id` = `users`.`id` \
						left join `company` on `company`.`id`=`users_company`.`company_id` \
						where \
						`users`.`isActivated`='Y' and `users`.`isblocked`='N' and `users`.`id`!=\"" << user.GetID() << "\" and \
						`users_company`.`current_company`='1' and \
						( \
							`company`.`name` like \"%" 		<< searchWords[0] << "%\" or \
							`company`.`name` like \"%" 		<< searchWords[1] << "%\" or \
							`company`.`name` like \"%" 		<< searchWords[2] << "%\" \
						) and ( \
							`users`.`name` like \"%" 		<< searchWords[0] << "%\" or \
							`users`.`name` like \"%" 		<< searchWords[1] << "%\" or \
							`users`.`name` like \"%" 		<< searchWords[2] << "%\" or \
							`users`.`nameLast` like \"%" 	<< searchWords[0] << "%\" or \
							`users`.`nameLast` like \"%" 	<< searchWords[1] << "%\" or \
							`users`.`nameLast` like \"%" 	<< searchWords[2] << "%\" \
						);";

				userList = GetUserListInJSONFormat(ost.str(), &db, &user);
				if(userList.length() > 0) 
				{
					// --- comma required only if previous text is exists
					if(ostFinal.str().length() > 10) ostFinal << ","; 
					ostFinal  << std::endl << userList; 
				}
				else
				{
					// --- here code will be run only if multiwork search was not sucessfull on previous step
					// --- earlier: user _and_ company is not success
					// --- here: user _or_ company
					{
						CLog log;
						log.Write(DEBUG, "int main(void): action == JSON_getFindFriendsListAutocomplete: (user _and_ company) has fail, try (user _without_ company) ");
					}

					ost.str("");
					ost << "select * from `users` where `isActivated`='Y' and `isblocked`='N' and `id`!=\"" << user.GetID() << "\" and ( \
							( \
								`users`.`name` like \"%" 		<< searchWords[1] << "%\" and \
								`users`.`nameLast` like \"%" 	<< searchWords[0] << "%\" \
							) \
							or \
							( \
								`users`.`name` like \"%" 		<< searchWords[0] << "%\" and \
								`users`.`nameLast` like \"%" 	<< searchWords[1] << "%\" \
							) \
							or \
							( \
								`users`.`name` like \"%" 		<< searchWords[2] << "%\" and \
								`users`.`nameLast` like \"%" 	<< searchWords[0] << "%\" \
							) \
							or \
							( \
								`users`.`name` like \"%" 		<< searchWords[0] << "%\" and \
								`users`.`nameLast` like \"%" 	<< searchWords[2] << "%\" \
							) \
							or \
							( \
								`users`.`name` like \"%" 		<< searchWords[1] << "%\" and \
								`users`.`nameLast` like \"%" 	<< searchWords[2] << "%\" \
							) \
							or \
							( \
								`users`.`name` like \"%" 		<< searchWords[2] << "%\" and \
								`users`.`nameLast` like \"%" 	<< searchWords[1] << "%\" \
							) \
							);";

					userList = GetUserListInJSONFormat(ost.str(), &db, &user);
					if(userList.length() > 0) 
					{
						// --- comma required only if previous text is exists
						if(ostFinal.str().length() > 10) ostFinal << ","; 
						ostFinal  << std::endl << userList; 
					}
				}

			}

			ostFinal << std::endl << "]";
		}

		{
			CLog	log;

			log.Write(DEBUG, "int main(void):action == JSON_getFindFriendsListAutocomplete: final response [", ostFinal.str(), "]");
		}


		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	// --- JSON friend list for autocomplete
	if((action == "JSON_getFindCompaniesListAutocomplete") || (action == "JSON_getFindCompaniesList") || (action == "JSON_getFindCompanyByID") || (action == "JSON_getMyCompaniesList"))
	{
		ostringstream	ost, ostFinal;
		string			sessid, lookForKey, companiesList;
		vector<string>	searchWords;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == JSON_getFindCompaniesListAutocomplete: start");
		}

		// --- Initialization
		ostFinal.str("");

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::JSON_getFindCompaniesListAutocomplete: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		lookForKey = indexPage.GetVarsHandler()->Get("lookForKey");

		if( (lookForKey.length() >= 3) || ((action == "JSON_getFindCompanyByID") && lookForKey.length())  || (action == "JSON_getMyCompaniesList")) 
		{
			ostringstream	ost;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(lookForKey.c_str(), convertBuffer, sizeof(convertBuffer));
			lookForKey = ConvertTextToHTML(convertBuffer);

			// --- Looking through company name
			ost.str("");
			if(action == "JSON_getFindCompanyByID")
				ost << "select * from `company` where `isBlocked`='N' and (`id`=\"" << lookForKey << "\");";
			else if (action == "JSON_getMyCompaniesList")
				ost << "select * from `company` where `isBlocked`='N' and (`admin_userID`=\"" << user.GetID() << "\");";
			else
				ost << "select * from `company` where `isBlocked`='N' and (`name` like \"%" << lookForKey << "%\");";

			// companiesList = GetCompanyListInJSONFormat(ost.str(), &db, &user);
			ostFinal << "{\"status\":\"success\",\"companies\":[" << GetCompanyListInJSONFormat(ost.str(), &db, &user) << "]}";

		}
		else
		{
			{
				CLog	log;
				log.Write(DEBUG, "int main()::JSON_getFindCompaniesListAutocomplete: searching key is empty");
			}
			ostFinal << "{\"status\":\"error\",\"description\":\"searching key is empty or less then 3\", \"companies\":[]}";
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;

			log.Write(DEBUG, "int main(void):action == JSON_getFindCompaniesListAutocomplete: final response [", ostFinal.str(), "]");
		}
	}

	if(action == "AJAX_addEditCompanyAddCompanyFounder")
	{
		ostringstream	ostResult;
		{
			CLog	log;
			log.Write(DEBUG, "int main(): action == AJAX_addEditCompanyAddCompanyFounder: start");
		}

		ostResult.str("");
		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(): action == AJAX_addEditCompanyAddCompanyFounder: re-login required");
			}

			ostResult << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";
		}
		else
		{
			string			userName = indexPage.GetVarsHandler()->Get("userName");
			string			userID = indexPage.GetVarsHandler()->Get("userid");
			string			companyID = indexPage.GetVarsHandler()->Get("companyid");
			char			*convertBuffer = new char[ 1024 * 1024];
			ostringstream	ost;

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(companyID.c_str(), convertBuffer, 1024*1024);
			companyID = ConvertTextToHTML(convertBuffer);

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(userName.c_str(), convertBuffer, 1024*1024);
			userName = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(userID.c_str(), convertBuffer, 1024*1024);
			userID = ConvertTextToHTML(convertBuffer);

			if(companyID.length() && (userName.length() || userID.length()))
			{

				// --- check if company editor is admin
				ost.str("");
				ost << "select * from `company` where `id`=\"" << companyID << "\" and `admin_userID`=\"" << user.GetID() << "\";";
				if(db.Query(ost.str()))
				{
					bool				isDuplicate = false;

					ost.str("");
					ost << "select * from `company_founder` where (`companyID`=\"" << companyID << "\") and (`founder_name` LIKE \"%" << userName << "%\" ";
					if(userID.length() && (userID != "0")) ost << " or `founder_userID`=\"" << userID << "\" ";
					ost << ");";
					if(db.Query(ost.str())) isDuplicate = true;

					if(!isDuplicate)
					{
						unsigned long	newFounderID;

						ost.str("");
						if(userID.length()) ost << "insert into `company_founder` (`companyID`, `founder_userID`) VALUES (\"" << companyID << "\", \"" << userID << "\");";
						else ost << "insert into `company_founder` (`companyID`, `founder_name`) VALUES (\"" << companyID << "\", \"" << userName << "\");";

						newFounderID = db.InsertQuery(ost.str());
						if(newFounderID)
						{
							// --- Update live feed
							ost.str("");
							ost << "insert into `feed` (`title`, `userID`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << companyID << "\", \"1001\", \"" << newFounderID << "\", NOW())";
							db.Query(ost.str());

							ostResult << "{\"result\": \"success\", \"id\": \"" << newFounderID << "\"}";

						}
						else
						{
							{
								CLog	log;
								log.Write(DEBUG, "int main(): action == AJAX_addEditCompanyAddCompanyFounder: ERROR: insertion into company_founder");
							}

							ostResult << "{\"result\": \"error\", \"desription\": \"ERROR: insertion into company_founder\"}";
						}
					}
					else
					{
						{
							CLog	log;
							log.Write(DEBUG, "int main(): action == AJAX_addEditCompanyAddCompanyFounder: founder already exists");
						}

						ostResult << "{\"result\": \"error\", \"desription\": \"founder already exists\"}";
					}
				}
				else
				{
					{
						CLog	log;
						log.Write(ERROR, "int main(): action == AJAX_addEditCompanyAddCompanyFounder: ERROR: company doesn't belongs to you");
					}

					ostResult << "{\"result\": \"error\", \"desription\": \"founder already exists\"}";
				}
			}
			else
			{
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::AJAX_addEditCompanyAddCompanyFounder: ERROR: mandatory parameter missed or empty in HTML request [userName, userID]";
					log.Write(ERROR, ost.str());
				}

				ostResult << "{\"result\": \"error\", \"description\": \"any mandatory parameter missed\"}";
			}

		}

		indexPage.RegisterVariableForce("result", ostResult.str());
		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(): action == AJAX_addEditCompanyAddCompanyFounder: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))

		{
			CLog	log;
			log.Write(DEBUG, "int main(): action == AJAX_addEditCompanyAddCompanyFounder: finish");
		}
	}


	// --- JSON get list of my friends
	if((action == "JSON_getMyNetworkFriendList") || (action == "JSON_getWhoWatchedONMeList"))
	{
		ostringstream	ost, ostFinal, friendsSqlQuery;
		string			sessid, lookForKey, userList = "";
		// char			*convertBuffer = new char[1024];
		CMysql			db1;
		int				affected;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_getMyNetworkFriendList: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::JSON_getMyNetworkFriendList: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		if(db1.Connect(DB_NAME, DB_LOGIN, DB_PASSWORD) < 0)
		{
			CLog	log;
	
			log.Write(ERROR, "Can not connect to mysql database");
			return(1);
		}

#ifdef MYSQL_4
		db1.Query("set names cp1251");
#endif

		friendsSqlQuery.str("");
		ost.str("");
		if(action == "JSON_getMyNetworkFriendList")
			ost << "select `friendID` from `users_friends` where `userID`='" << user.GetID() << "';";
		if(action == "JSON_getWhoWatchedONMeList")
			ost << "select `watching_userID` as `friendID` from `users_watched` where `watched_userID`='" << user.GetID() << "';";
		affected = db.Query(ost.str());
		if(affected)
		{
			friendsSqlQuery << "select * from `users` where `isActivated`='Y' and `isblocked`='N' and `id` IN (";
			for(int i = 0; i < affected; i++)
			{
				friendsSqlQuery << (i > 0 ? ", " : "") << db.Get(i, "friendID");
			}
			friendsSqlQuery << ");";

			{
				CLog	log;
				log.Write(DEBUG, "int main(void):action == JSON_getMyNetworkFriendList: query for JSON prepared [", friendsSqlQuery.str(), "]");
			}

			userList = GetUserListInJSONFormat(friendsSqlQuery.str(), &db, &user);
		}


		ostFinal.str("");
		ostFinal << "[" << std::endl << userList << std::endl << "]" << std::endl;

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	if(action == "JSON_chatGetInitialData")
	{
		ostringstream	ost, ostFinal, friendsSqlQuery, chatMessageQuery;
		string			sessid, lookForKey, userArray = "", messageArray = "";

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_chatGetInitialData: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::JSON_chatGetInitialData: re-login required");
			}

			ostFinal << "result: fail";
		}
		else
		{
			int				affected;

			friendsSqlQuery.str("");
			ost.str("");
			ost << "select `friendID` from `users_friends` where `userID`='" << user.GetID() << "';";
			affected = db.Query(ost.str());
			if(affected)
			{
				friendsSqlQuery << "select * from `users` where `isActivated`='Y' and `isblocked`='N' and `id` IN (";
				for(int i = 0; i < affected; i++)
				{
					friendsSqlQuery << (i > 0 ? ", " : "") << db.Get(i, "friendID");
				}
				friendsSqlQuery << ");";

				{
					CLog	log;
					log.Write(DEBUG, "int main(void):action == JSON_chatGetInitialData: query for JSON prepared [", friendsSqlQuery.str(), "]");
				}
				userArray = GetUserListInJSONFormat(friendsSqlQuery.str(), &db, &user);

				chatMessageQuery.str("");
				chatMessageQuery << "select * from `chat_messages` where `toID`='" << user.GetID() << "' or `fromID`='" << user.GetID() << "';";
				messageArray = GetChatMessagesInJSONFormat(chatMessageQuery.str(), &db);
			}


			ostFinal.str("");
			ostFinal << "\"result\": \"success\"," << std::endl;
			ostFinal << "\"userArray\": [" << userArray << "]," << std::endl;
			ostFinal << "\"messageArray\": [" << messageArray << "]";
		}


		indexPage.RegisterVariableForce("result", "{" + ostFinal.str() + "}");

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_chatGetInitialData: end");
		}
	}


	// --- JSON avatar list
	if(action == "JSON_getAvatarList") 
	{
		ostringstream	ost;
		string		sessid;
		int			affected;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_getAvatarList: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main():action == JSON_getAvatarList: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		ost << "select * from `users_avatars` where `userid`=\"" << user.GetID() << "\";";
		affected = db.Query(ost.str());
		if(affected > 0)
		{
			ost.str("");
			for(int i = 0; i < affected; i++) 
			{
				ost << "{ \"folder\": \"" << db.Get(i, "folder") << "\", \"filename\": \"" << db.Get(i, "filename") << "\", \"isActive\": \"" << db.Get(i, "isActive") << "\", \"avatarID\": \"" << db.Get(i, "id") << "\" }";
				if(i < (affected-1)) ost << ", ";
			}
		}
		else
		{
			CLog	log;

			ost.str("");
			log.Write(DEBUG, "int main(void): action == JSON_getAvatarList: there are no avatars for user ", user.GetLogin());
		}

		indexPage.RegisterVariableForce("result", ost.str());

		if(!indexPage.SetTemplate("ajax_getJobTitles.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file ajax_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	// --- JSON get user CV
	if(action == "JSON_getUserCV") 
	{
		ostringstream	ost, ostResult;
		string		sessid;
		int			affected;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_getUserCV: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main():action == JSON_getUserCV: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		ost.str("");
		ostResult.str("");
		ost << "select * from `users` where `id`=\"" << user.GetID() << "\";";
		affected = db.Query(ost.str());
		if(affected > 0)
		{
			ostResult << "{ \"result\": \"success\", \"cv\": \"" << db.Get(0, "cv") << "\" }";
		}
		else
		{
			ostResult << "{ \"result\": \"fail\", \"description\": \"error receiving CV from DB\" }";
			{
				CLog	log;

				ost.str("");
				log.Write(DEBUG, "int main(void): action == JSON_getUserCV: error receiving CV for user ", user.GetLogin());
			}
		}

		indexPage.RegisterVariableForce("result", ostResult.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	// --- JSON get comments list on message
	if(action == "JSON_getCommentsOnMessage")
	{
		ostringstream	ost, ostList;
		string			messageID;
		char			*convertBuffer = new char[1024];

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_getCommentsOnMessage: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::JSON_getCommentsOnMessage: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		messageID = indexPage.GetVarsHandler()->Get("messageID");


		memset(convertBuffer, 0, 1024);
		convert_utf8_to_windows1251(messageID.c_str(), convertBuffer, 1024);
		messageID = convertBuffer;
		trim(messageID);

		delete[] convertBuffer;

		// --- Clean-up the text
		messageID = ReplaceDoubleQuoteToQuote(messageID);
		messageID = DeleteHTML(messageID);
		messageID = SymbolReplace(messageID, "\r\n", "");
		messageID = SymbolReplace(messageID, "\r", "");
		messageID = SymbolReplace(messageID, "\n", "");

		if(messageID != "")
		{
			int				affected;

			ostList.str("");
			ost.str("");
			ost << "select \
`feed_message_comment`.`id` as `feed_message_comment_id`, \
`feed_message_comment`.`messageID` as `feed_message_comment_messageID`, \
`feed_message_comment`.`userID` as `feed_message_comment_userID`, \
`feed_message_comment`.`comment` as `feed_message_comment_comment`, \
`feed_message_comment`.`eventTimestamp` as `feed_message_comment_eventTimestamp`, \
`users`.`id` as `users_id`, \
`users`.`name` as `users_name`, \
`users`.`nameLast` as `users_nameLast` \
from `feed_message_comment` \
inner join `users` on `feed_message_comment`.`userID`=`users`.`id` \
where `messageID`=\"" << messageID << "\" and `users`.`isactivated`='Y' and `users`.`isblocked`='N'";

			affected = db.Query(ost.str());
			if(affected)
			{
				ostList.str("");
				for(int i = 0; i < affected; i++)
				{
					if(i > 0) ostList << "," << std::endl;
					ostList << "{" << std::endl;
					ostList << "\"id\" : \"" << db.Get(i, "feed_message_comment_id") << "\"," << std::endl; 
					ostList << "\"messageID\" : \"" << db.Get(i, "feed_message_comment_messageID") << "\"," << std::endl; 
					ostList << "\"user\" : {";
					ostList << 		"\"userID\" : \"" << db.Get(i, "users_id") << "\","; 
					ostList << 		"\"name\" : \"" << db.Get(i, "users_name") << "\","; 
					ostList << 		"\"nameLast\" : \"" << db.Get(i, "users_nameLast") << "\""; 
					ostList << "}," << std::endl;
					ostList << "\"comment\" : \"" << db.Get(i, "feed_message_comment_comment") << "\"," << std::endl; 
					ostList << "\"eventTimestamp\" : \"" << db.Get(i, "feed_message_comment_eventTimestamp") << "\"," << std::endl; 
					ostList << "\"eventTimestampDelta\":\""	<< GetHumanReadableTimeDifferenceFromNow(db.Get(i,"feed_message_comment_eventTimestamp")) << "\"" << std::endl;
					ostList << "}";
				}
			}

			ost.str("");
			ost << "{" << std::endl;
			ost << "\"result\": \"success\",";
			ost << "\"commentsList\": [" << ostList.str() << "]" << std::endl;
			ost << "}" << std::endl;
		}
		else
		{
			// --- Empty messageID
			ost.str("");
			ost << "{";
			ost << "\"result\": \"error\",";
			ost << "\"description\": \"can't get comments list due to messageID is empty\"";
			ost << "}";

			CLog	log;
			log.Write(ERROR, "int main(void): action == JSON_getCommentsOnMessage: ERROR can't get comments list due to messageID is empty");
		} // --- if(messageID != "")

		indexPage.RegisterVariableForce("result", ost.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == JSON_getCommentsOnMessage: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("template page missing");
		} // if(!indexPage.SetTemplate("json_response.htmlt"))
	}

	if(action == "AJAX_chatPostMessage")
	{
		ostringstream	ost, ostFinal;
		string			sessid, message = "", toID = "";

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_chatPostMessage: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_chatPostMessage: re-login required");
			}

			ostFinal << "result: fail";
		}
		else
		{
			char			*convertBuffer = new char[ 1024 * 1024];

			message = indexPage.GetVarsHandler()->Get("message");
			toID = indexPage.GetVarsHandler()->Get("toID");
			

			// --- clean-up message parameter
			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(message.c_str(), convertBuffer, 1024*1024);
			message = convertBuffer;
			delete[] convertBuffer;

			message = ReplaceDoubleQuoteToQuote(message);
			message = RemoveSpecialSymbols(message);
			message = DeleteHTML(message);
			message = SymbolReplace(message, "\r\n", "<br>");
			message = SymbolReplace(message, "\r", "");
			message = SymbolReplace(message, "\n", "<br>");
			trim(message);

			// --- clean-up recipient parameter
			toID = ReplaceDoubleQuoteToQuote(toID);
			toID = DeleteHTML(toID);
			trim(toID);

			{
				CLog	log;
				log.Write(DEBUG, "int main(void):action == AJAX_chatPostMessage: message [", message, "]");
			}

			if(message.length() && toID.length())
			{
				int				affected;
				
				ost.str("");
				ost << "insert into `chat_messages` (`message`, `fromType`, `fromID`, `toType`, `toID`, `messageStatus`, `eventTimestamp`) \
						VALUES (\
						\"" << message << "\", \
						\"fromUser\", \
						\"" << user.GetID() << "\", \
						\"toUser\", \
						\"" << toID << "\", \
						\"unread\", \
						NOW() \
						);";

				db.Query(ost.str());

				ost.str("");
				ost << "select * from `chat_messages` where `fromID`='" << user.GetID() << "' and `toID`='" << toID << "' and `messageStatus`='unread' ORDER BY `id` DESC LIMIT 0,1;";
				affected = db.Query(ost.str());
				if(affected)
				{
					string		tempMessage = db.Get(0, "message");

					if(tempMessage == message)
					{
						ostringstream	chatMessageQuery;
						string			messageObj;

						chatMessageQuery.str("");
						chatMessageQuery << "select * from `chat_messages` where `id`='" << db.Get(0, "id") << "';";
						messageObj = GetChatMessagesInJSONFormat(chatMessageQuery.str(), &db);

						if(messageObj.length())
						{
							ostFinal.str("");
							ostFinal << "\"result\": \"success\"," << std::endl;
							ostFinal << "\"messageObj\": " << messageObj << std::endl;

						}
						else
						{
							{
								CLog			log;
								ostringstream	ost;

								ost.str("");
								ost << "int main(void):action == AJAX_chatPostMessage: ERROR in getting chat message by ID from DB (" << db.Get(0, "id") << ")";
								log.Write(ERROR, ost.str());
							}
							ostFinal.str("");
							ostFinal << "\"result\": \"error\"," << std::endl;
							ostFinal << "\"description\": \"проблема с базой данных (сообщите администрации)\"" << std::endl;
						}
					}

					else
					{
						{
							CLog			log;
							ostringstream	ost;

							ost.str("");
							ost << "int main(void):action == AJAX_chatPostMessage: ERROR can't find message after inserting into DB (original message: [" << message << "], DB message: [" << tempMessage << "])";
							log.Write(ERROR, ost.str());
						}
						ostFinal.str("");
						ostFinal << "\"result\": \"error\"," << std::endl;
						ostFinal << "\"description\": \"проблема с базой данных (сообщите администрации)\"" << std::endl;
					}

				}
				else
				{
					{
						CLog	log;
						log.Write(ERROR, "int main(void):action == AJAX_chatPostMessage: ERROR can't find message after inserting into DB");
					}
					ostFinal.str("");
					ostFinal << "\"result\": \"error\"," << std::endl;
					ostFinal << "\"description\": \"проблема с базой данных (сообщите администрации)\"" << std::endl;
				}


			}
			else
			{
				{
					CLog	log;
					log.Write(DEBUG, "int main(void):action == AJAX_chatPostMessage: message_body or message_recipient is empty");
				}
				ostFinal.str("");
				ostFinal << "\"result\": \"error\"," << std::endl;
				ostFinal << "\"description\": \"сообщение должно содержать текст\"" << std::endl;
			}





		}


		indexPage.RegisterVariableForce("result", "{" + ostFinal.str() + "}");

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_chatPostMessage: end");
		}
	}

	if(action == "AJAX_chatMarkMessageReadByMessageID")
	{
		ostringstream	ost, ostFinal;
		string			messageID = "";

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_chatMarkMessageReadByMessageID: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_chatMarkMessageReadByMessageID: re-login required");
			}

			ostFinal << "result: fail";
		}
		else
		{
			char			*convertBuffer = new char[ 1024 * 1024];

			messageID = indexPage.GetVarsHandler()->Get("messageid");

			// --- clean-up message parameter
			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(messageID.c_str(), convertBuffer, 1024*1024);
			messageID = convertBuffer;
			delete[] convertBuffer;

			messageID = ReplaceDoubleQuoteToQuote(messageID);
			messageID = RemoveSpecialSymbols(messageID);
			messageID = DeleteHTML(messageID);
			messageID = SymbolReplace(messageID, "\r\n", "<br>");
			messageID = SymbolReplace(messageID, "\r", "");
			messageID = SymbolReplace(messageID, "\n", "<br>");
			trim(messageID);

			{
				CLog	log;
				log.Write(DEBUG, "int main(void):action == AJAX_chatMarkMessageReadByMessageID: message [", messageID, "]");
			}

			if(messageID.length())
			{
				ost.str("");
				ost << "UPDATE `chat_messages` SET `messageStatus`=\"read\" \
						WHERE `toID`='" << user.GetID() << "' and `id`='" << messageID << "';";

				db.Query(ost.str());

				{
					ostFinal.str("");
					ostFinal << "\"result\": \"success\"" << std::endl;
				}
			}
			else
			{
				{
					CLog	log;
					log.Write(ERROR, "int main(void):action == AJAX_chatMarkMessageReadByMessageID: ERROR: messageID is empty");
				}
				ostFinal.str("");
				ostFinal << "\"result\": \"error\"," << std::endl;
				ostFinal << "\"description\": \"messageID is empty\"" << std::endl;
			}

		}

		indexPage.RegisterVariableForce("result", "{" + ostFinal.str() + "}");

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_chatMarkMessageReadByMessageID: end");
		}
	}

	if(action == "AJAX_notificationMarkMessageReadByMessageID")
	{
		ostringstream	ost, ostFinal;
		string			notificationID = "";

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_notificationMarkMessageReadByMessageID: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(ERROR, "int main()::AJAX_notificationMarkMessageReadByMessageID: re-login required");
			}

			ostFinal << "\"result\":\"fail\"";
		}
		else
		{
			char			*convertBuffer = new char[ 1024 * 1024];

			notificationID = indexPage.GetVarsHandler()->Get("notificationID");

			// --- clean-up message parameter
			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(notificationID.c_str(), convertBuffer, 1024*1024);
			notificationID = convertBuffer;
			delete[] convertBuffer;

			notificationID = ReplaceDoubleQuoteToQuote(notificationID);
			notificationID = RemoveSpecialSymbols(notificationID);
			notificationID = DeleteHTML(notificationID);
			notificationID = SymbolReplace(notificationID, "\r\n", "<br>");
			notificationID = SymbolReplace(notificationID, "\r", "");
			notificationID = SymbolReplace(notificationID, "\n", "<br>");
			trim(notificationID);

			{
				CLog	log;
				log.Write(DEBUG, "int main(void):action == AJAX_notificationMarkMessageReadByMessageID: message [", notificationID, "]");
			}

			if(notificationID.length())
			{
				ost.str("");
				ost << "UPDATE `users_notification` SET `notificationStatus`=\"read\" \
						WHERE `userId`='" << user.GetID() << "' and `id`='" << notificationID << "';";

				db.Query(ost.str());

				if(!db.isError())
				{
					ostFinal.str("");
					ostFinal << "\"result\": \"success\"" << std::endl;
				}
				else
				{
					{
						CLog	log;
						log.Write(ERROR, "int main()::AJAX_notificationMarkMessageReadByMessageID: ERROR updating users_notification table");
					}

					ostFinal.str("");
					ostFinal << "\"result\": \"fail\"" << std::endl;
				}
			}
			else
			{
				{
					CLog	log;
					log.Write(ERROR, "int main(void):action == AJAX_notificationMarkMessageReadByMessageID: ERROR: notificationID is empty");
				}
				ostFinal.str("");
				ostFinal << "\"result\": \"error\"," << std::endl;
				ostFinal << "\"description\": \"notificationID is empty\"" << std::endl;
			}

		}

		indexPage.RegisterVariableForce("result", "{" + ostFinal.str() + "}");

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_notificationMarkMessageReadByMessageID: end");
		}
	}

	if(action == "AJAX_chatMarkMessageReadByUserID")
	{
		ostringstream	ost, ostFinal;
		string			userID = "";

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_chatMarkMessageReadByUserID: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_chatMarkMessageReadByUserID: re-login required");
			}

			ostFinal << "result: fail";
		}
		else
		{
			char			*convertBuffer = new char[ 1024 * 1024];

			userID = indexPage.GetVarsHandler()->Get("userID");

			// --- clean-up message parameter
			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(userID.c_str(), convertBuffer, 1024*1024);
			userID = convertBuffer;
			delete[] convertBuffer;

			userID = ReplaceDoubleQuoteToQuote(userID);
			userID = RemoveSpecialSymbols(userID);
			userID = DeleteHTML(userID);
			userID = SymbolReplace(userID, "\r\n", "<br>");
			userID = SymbolReplace(userID, "\r", "");
			userID = SymbolReplace(userID, "\n", "<br>");
			trim(userID);

			{
				CLog	log;
				log.Write(DEBUG, "int main(void):action == AJAX_chatMarkMessageReadByUserID: message [", userID, "]");
			}

			if(userID.length())
			{
				ost.str("");
				ost << "UPDATE `chat_messages` SET `messageStatus`=\"read\" \
						WHERE `fromID`='" << userID << "' AND `toID`='" << user.GetID() << "' AND `messageStatus`='unread';";

				db.Query(ost.str());

				{
					ostFinal.str("");
					ostFinal << "\"result\": \"success\"," << std::endl;
				}
			}
			else
			{
				{
					CLog	log;
					log.Write(ERROR, "int main(void):action == AJAX_chatMarkMessageReadByUserID: ERROR: userID is empty");
				}
				ostFinal.str("");
				ostFinal << "\"result\": \"error\"," << std::endl;
				ostFinal << "\"description\": \"userID is empty\"" << std::endl;
			}

		}

		indexPage.RegisterVariableForce("result", "{" + ostFinal.str() + "}");

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_chatMarkMessageReadByUserID: end");
		}
	}


	// --- cleanup news feed image after closing the modal window 
	if((action == "AJAX_cleanupFeedImages") || (action == "AJAX_editCleanupFeedImages"))
	{
		string		imageTempSet;

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(): action == AJAX_cleanupFeedImages: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		imageTempSet = indexPage.GetVarsHandler()->Get("imageTempSet");
		if(imageTempSet.length() > 0)
		{
			ostringstream	ost;

			ost.str("");
			ost << "`tempSet`='" << imageTempSet << "' and `userID`='" << user.GetID() << "' and `setID`='0'";
			RemoveMessageImages(ost.str(), &db);

			// --- cleanup DB tempSet for edit images 
			ost.str("");
			ost << "update `feed_images` set `tempSet`='0', `removeFlag`=\"keep\" where `tempSet`='" << imageTempSet << "' and `userID`='" << user.GetID() << "';";
			db.Query(ost.str());
		}


		indexPage.RegisterVariableForce("result", "{\"result\" : \"success\"}");

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}


	// --- prepare new feed image to new image set loading
	if(action == "AJAX_prepareFeedImages")
	{
		string		currentEmployment, companyId;
		ostringstream	ost;

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_prepareFeedImages: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		ost.str("");
		ost << "update `feed_images` set `tempSet`='0' where `userID`='" << user.GetID() << "';";
		db.Query(ost.str());

		indexPage.RegisterVariableForce("result", "{\"result\" : \"success\"}");

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	// --- prepare edit feed image to new image set loading
	if(action == "AJAX_prepareEditFeedImages")
	{
		string			imageTempSet, messageID;
		ostringstream	ost;

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_prepareFeedImages: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		messageID = indexPage.GetVarsHandler()->Get("messageID");
		imageTempSet = indexPage.GetVarsHandler()->Get("imageTempSet");

		if((messageID.length() > 0) and (messageID != "0") and (imageTempSet.length() > 0) and (imageTempSet != "0"))
		{
			ost.str("");
			ost << "update `feed_images` set `tempSet`='0' where `userID`='" << user.GetID() << "';";
			db.Query(ost.str());

			ost.str("");
			ost << "SELECT  `feed_images`.`setID` as 'setID' FROM  `feed_images` \
					RIGHT JOIN  `feed_message` ON  `feed_message`.`imageSetID` =  `feed_images`.`setID` \
					WHERE  `feed_message`.`id` =  '" << messageID << "'";
			if(db.Query(ost.str()))
			{
				string	imageSetID = db.Get(0, "setID");

				ost.str("");
				ost << "update `feed_images` set `tempSet`='" << imageTempSet << "', `removeFlag`='keep' where `userID`='" << user.GetID() << "' and `setID`='" << imageSetID << "';";
				db.Query(ost.str());
			}

			indexPage.RegisterVariableForce("result", "{\"result\" : \"success\"}");
		}
		else
		{
			indexPage.RegisterVariableForce("result", "{\"result\" : \"error\", \"description\" : \"issue with messageID or imageTempSet parameters (empty or '0')\"}");
		}

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	// --- JSON get current employment
	if(action == "JSON_getCurrentEmployment")
	{
		string		currentEmployment, companyId;
		ostringstream	ost;
		int			affected;

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::JSON_getCurrentEmployment: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		ost.str("");
		ost << "select `company`.`name` as `company_name` from `company`, `users_company` where `users_company`.`occupation_finish`=\"0000-00-00\" and `users_company`.`company_id`=`company`.`id`;";
		if((affected = db.Query(ost.str())) > 0) 
		{
			ost.str("");
			for(int i = 0; i < affected; i++)
			{
				ost << "{ \"folder\": \"" << db.Get(i, "folder") << "\", \"filename\": \"" << db.Get(i, "filename") << "\", \"isActive\": \"" << db.Get(i, "isActive") << "\" }";
				if(i < (affected-1)) ost << ", ";
			}
		}
		else
		{
			{
				CLog	log;
				log.Write(DEBUG, "int main()::JSON_getCurrentEmployment: there are no company of current employmant of user ", user.GetID());
			}
		}
		indexPage.RegisterVariableForce("result", ost.str());

		if(!indexPage.SetTemplate("ajax_getJobTitles.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file ajax_response.htmlt was missing");
			throw CException("Template file was missing");
		}

	}
	// --- JSON part has eneded


	// --- AJAX part has started

	// --- AJAX post message to news feed
	if(action == "AJAX_postNewsFeedMessage")
	{
		ostringstream	ost;
		string			strPageToGet, strNewsOnSinglePage;
		string			newsFeedMessageTitle;
		string			newsFeedMessageLink;
		string			newsFeedMessageText;
		string			newsFeedMessageRights;
		string			newsFeedMessageImageTempSet;
		string			newsFeedMessageImageSet;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_postNewsFeedMessage: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == AJAX_postNewsFeedMessage: re-login required");
			}

			ost.str("");
			ost << "[{\"result\": \"error\"}, {\"desription\": \"session lost. Need to relogin\"}]";

			indexPage.RegisterVariableForce("result", ost.str());

			if(!indexPage.SetTemplate("json_response.htmlt"))
			{
				CLog	log;
				log.Write(ERROR, "int main(void): action == AJAX_postNewsFeedMessage: ERROR can't find template json_response.htmlt");
				throw CExceptionHTML("user not activated");
			} // if(!indexPage.SetTemplate("json_response.htmlt"))
		}
		else
		{
			// --- "new" used due to possibility of drop "standard exception"
			char			*convertBuffer = new char[ 1024 * 1024];
			int				affected;

			// --- This line will not be reached in case of error in memory allocation 
			// --- To avoid throw std exception use char *a = new(std::nothrow) char[ 0x7FFFFFFF ];
			if(!convertBuffer)
			{
				CLog	log;
				log.Write(ERROR, "int main(void): action == AJAX_postNewsFeedMessage: ERROR can't allocate memory");			
			}

			// --- Authorized user
			newsFeedMessageTitle = indexPage.GetVarsHandler()->Get("newsFeedMessageTitle");
			newsFeedMessageLink = indexPage.GetVarsHandler()->Get("newsFeedMessageLink");
			newsFeedMessageText = indexPage.GetVarsHandler()->Get("newsFeedMessageText");
			newsFeedMessageRights = indexPage.GetVarsHandler()->Get("newsFeedMessageRights");
			newsFeedMessageImageTempSet = indexPage.GetVarsHandler()->Get("newsFeedMessageImageTempSet");
			newsFeedMessageImageSet = "";

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(newsFeedMessageTitle.c_str(), convertBuffer, 1024*1024);
			newsFeedMessageTitle = convertBuffer;
			trim(newsFeedMessageTitle);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(newsFeedMessageText.c_str(), convertBuffer, 1024*1024);
			newsFeedMessageText = convertBuffer;
			trim(newsFeedMessageText);

			delete[] convertBuffer;

			// --- Clean-up the text
			newsFeedMessageTitle = ReplaceDoubleQuoteToQuote(newsFeedMessageTitle);
			newsFeedMessageTitle = DeleteHTML(newsFeedMessageTitle);
			newsFeedMessageTitle = SymbolReplace(newsFeedMessageTitle, "\r\n", "<br>");
			newsFeedMessageTitle = SymbolReplace(newsFeedMessageTitle, "\r", "<br>");
			newsFeedMessageTitle = SymbolReplace(newsFeedMessageTitle, "\n", "<br>");
			newsFeedMessageLink  = ReplaceDoubleQuoteToQuote(newsFeedMessageLink);
			newsFeedMessageLink  = DeleteHTML(newsFeedMessageLink);
			newsFeedMessageLink  = SymbolReplace(newsFeedMessageLink, "\r\n", "<br>");
			newsFeedMessageLink  = SymbolReplace(newsFeedMessageLink, "\r", "<br>");
			newsFeedMessageLink  = SymbolReplace(newsFeedMessageLink, "\n", "<br>");
			newsFeedMessageText  = ReplaceDoubleQuoteToQuote(newsFeedMessageText);
			newsFeedMessageText  = DeleteHTML(newsFeedMessageText);
			newsFeedMessageText  = SymbolReplace(newsFeedMessageText, "\r\n", "<br>");
			newsFeedMessageText  = SymbolReplace(newsFeedMessageText, "\r", "<br>");
			newsFeedMessageText  = SymbolReplace(newsFeedMessageText, "\n", "<br>");

			// ost.str("");
			// ost << "UPDATE  `feed_images` SET  `setID` = ( SELECT maxID FROM ( SELECT MAX(`setID`)+1 maxID FROM  `feed_images` ) AS t ) WHERE  `tempSet`='" << newsFeedMessageImageTempSet << "' and `userID`='" << user.GetID() << "';";
			// db.Query(ost.str());

			ost.str("");
			ost << "SELECT `id` FROM  `feed_images` WHERE  `tempSet`='" << newsFeedMessageImageTempSet << "' and `userID`='" << user.GetID() << "';";
			if(db.Query(ost.str()))
			{
				string		uniqSetID = db.Get(0, "id");

				ost.str("");
				ost << "UPDATE  `feed_images` SET  `setID` = " << uniqSetID << " WHERE  `tempSet`='" << newsFeedMessageImageTempSet << "' and `userID`='" << user.GetID() << "';";
				db.Query(ost.str());
			}
			else
			{
				CLog			log;
				ostringstream	ost;

				ost.str("");
				ost << "int main(void): action == AJAX_postNewsFeedMessage: ERROR: can't find entries in `feed_images` where `tempSet`='" << newsFeedMessageImageTempSet << "' and `userID`='" << user.GetID() << "'. Lost images will appear in `feed_images`.;";
				log.Write(ERROR, ost.str());
			}

			ost.str("");
			ost << "select * from `feed_images` where `tempSet`='" << newsFeedMessageImageTempSet << "' and `userID`='" << user.GetID() << "';";
			affected = db.Query(ost.str());
			if(affected)
			{
				newsFeedMessageImageSet = db.Get(0, "setID");

				{
					CLog			log;
					ostringstream	ost;

					ost << "int main(void): action == AJAX_postNewsFeedMessage: `setID` from db = [" << newsFeedMessageImageSet << "] has been posted";
					log.Write(DEBUG, ost.str());
				}

				if(newsFeedMessageImageSet == "0")
				{
					CLog			log;
					ostringstream	ost;

					ost << "int main(void): action == AJAX_postNewsFeedMessage: ERROR `setID` from db = [" << newsFeedMessageImageSet << "] has been posted";
					log.Write(ERROR, ost.str());
				}


				ost.str("");
				ost << "update `feed_images` set `tempSet`='0' where `tempSet`='" << newsFeedMessageImageTempSet << "' and `userID`='" << user.GetID() << "';";
				db.Query(ost.str());
			}



			if(!((newsFeedMessageTitle == "") && (newsFeedMessageText == "") && (newsFeedMessageImageSet == "")))
			{
				unsigned long	feed_messageID = 0;
				ost.str("");
				ost << "insert into `feed_message` (`title`, `link`, `message`, `imageSetID`, `access`) \
						VALUES (\
						\"" << newsFeedMessageTitle << "\", \
						\"" << newsFeedMessageLink << "\", \
						\"" << newsFeedMessageText << "\", \
						\"" << (newsFeedMessageImageSet.length() ? newsFeedMessageImageSet : "0") << "\", \
						\"" << newsFeedMessageRights << "\" \
						);";
				feed_messageID = db.InsertQuery(ost.str());
				if(feed_messageID)
				{
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"11\", " << feed_messageID << ", NOW());";

					if(db.InsertQuery(ost.str()))
					{
						ost.str("");
						ost << "[";
						ost << "{";
						ost << "\"result\": \"success\",";
						ost << "\"description\": \"message has been posted\"";
						ost << "}";
						ost << "]";
					}
					else
					{
						{
							CLog			log;
							ostringstream	ost;

							ost << "int main(void): action == AJAX_postNewsFeedMessage: ERROR inserting into feed";
							log.Write(ERROR, ost.str());
						}
						ost.str("");
						ost << "[";
						ost << "{";
						ost << "\"result\": \"error\",";
						ost << "\"description\": \"error inserting into feed\"";
						ost << "}";
						ost << "]";
					}


				}
				else
				{
					{
						CLog			log;
						ostringstream	ost;

						ost << "int main(void): action == AJAX_postNewsFeedMessage: ERROR inserting into feed_message";
						log.Write(ERROR, ost.str());
					}
					ost.str("");
					ost << "[";
					ost << "{";
					ost << "\"result\": \"error\",";
					ost << "\"description\": \"error inserting creating message\"";
					ost << "}";
					ost << "]";
				}

				indexPage.RegisterVariableForce("result", ost.str());

				if(!indexPage.SetTemplate("json_response.htmlt"))
				{
					CLog	log;
					log.Write(ERROR, "int main(void): action == AJAX_postNewsFeedMessage: ERROR can't find template json_response.htmlt");
					throw CExceptionHTML("user not activated");
				} // if(!indexPage.SetTemplate("json_response.htmlt"))

				{
					CLog			log;
					ostringstream	ost;

					ost << "int main(void): action == AJAX_postNewsFeedMessage: end (message from [" << user.GetName() << "] has been posted)";
					log.Write(DEBUG, ost.str());
				}
			} // if(!((newsFeedMessageTitle == "") && (newsFeedMessageText == "") && (newsFeedMessageImage == "")))
			else
			{
				// --- Empty title, message and image
				ost.str("");
				ost << "[";
				ost << "{";
				ost << "\"result\": \"error\",";
				ost << "\"description\": \"can't post message due to title, text and image is empty \"";
				ost << "}";
				ost << "]";

				CLog	log;
				log.Write(ERROR, "int main(void): action == AJAX_postNewsFeedMessage: ERROR can't post message due to title, text and image is empty ");
			} // if(!((newsFeedMessageTitle == "") && (newsFeedMessageText == "") && (newsFeedMessageImage == "")))
		} // if(user.GetLogin() == "Guest")

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_postNewsFeedMessage: end");
		}
	}

	if(action == "AJAX_updateUserCV")
	{
		ostringstream	ostResult;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_updateUserCV: start");
		}


		ostResult.str("");
		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == AJAX_updateUserCV: re-login required");
			}

			ostResult << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";
		}
		else
		{
			string			cvText = indexPage.GetVarsHandler()->Get("cv");
			ostringstream	ost;
			char			*convertBuffer = new char[ 1024 * 1024];

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(cvText.c_str(), convertBuffer, 1024*1024);
			cvText = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "update `users` set `cv`=\"" << cvText << "\" where `id`=\"" << user.GetID() << "\";";
			db.Query(ost.str());
			if(!db.isError())
			{

				// --- Update live feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"21\", \"0\", NOW())";
				if(db.InsertQuery(ost.str()))
				{

					ostResult << "{\"result\": \"success\"}";
				}
				else
				{
					ostResult << "{\"result\": \"error\", \"description\": \"error updating live feed\"}";

					{
						CLog	log;
						log.Write(ERROR, "int main(void): action == AJAX_updateUserCV: ERROR can't update feed table");
					}
				}
			}
			else
			{
				ostResult << "{\"result\": \"error\", \"description\": \"error updating table users\"}";

				{
					CLog	log;
					log.Write(ERROR, "int main(void): action == AJAX_updateUserCV: ERROR can't update users table");
				}
			}


		}

		indexPage.RegisterVariableForce("result", ostResult.str());
		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == AJAX_updateUserCV: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_updateUserCV: end");
		}

	}

	if(action == "AJAX_changeEditProfileCompanyEmployemtEndDateStatus")
	{
		ostringstream	ostResult;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_changeEditProfileCompanyEmployemtEndDateStatus: start");
		}


		ostResult.str("");
		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == AJAX_changeEditProfileCompanyEmployemtEndDateStatus: re-login required");
			}

			ostResult << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";
		}
		else
		{
			long int		companyID = atol(indexPage.GetVarsHandler()->Get("companyID").c_str());
			ostringstream	ost;

			ost.str("");
			ost << "select * from `users_company` where `id`=\"" << companyID << "\";";
			if(db.Query(ost.str()))
			{
				if(db.Get(0, "user_id") == user.GetID())
				{
					string	currentStatus = db.Get(0, "current_company");

					ost.str("");
					ost << "update `users_company` set `current_company`='" << (currentStatus == "0" ? "1" : "0") << "' where `id`=\"" << companyID << "\";";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"2\", \"0\", NOW())";
					if(db.InsertQuery(ost.str()))
					{
						ostResult << "{\"result\": \"success\"}";
					}
					else
					{
						ostResult << "{\"result\": \"error\", \"description\": \"error updating live feed\"}";

						{
							CLog	log;
							log.Write(ERROR, "int main(void): action == AJAX_changeEditProfileCompanyEmployemtEndDateStatus: ERROR can't update feed table");
						}
					}
				}
				else
				{
					ostringstream	ost;

					{
						CLog	log;
						log.Write(DEBUG, "int main(void): action == AJAX_changeEditProfileCompanyEmployemtEndDateStatus: user trying update company not belonging to him");
					}

					ostResult << "{\"result\": \"error\", \"desription\": \"you didn't works in that company\"}";
				}
			}
			

		}

		indexPage.RegisterVariableForce("result", ostResult.str());
		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == AJAX_changeEditProfileCompanyEmployemtEndDateStatus: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_changeEditProfileCompanyEmployemtEndDateStatus: end");
		}

	}

	if(action == "AJAX_updateRecommendationTitle")
	{

		ostringstream	ostResult;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_updateRecommendationTitle: start");
		}


		ostResult.str("");
		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == AJAX_updateRecommendationTitle: re-login required");
			}

			ostResult << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";
		}
		else
		{
			string			recommendationID = indexPage.GetVarsHandler()->Get("id");
			string			recommendationTitle = indexPage.GetVarsHandler()->Get("content");
			ostringstream	ost;
			char			*convertBuffer = new char[ 1024 * 1024];

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(recommendationTitle.c_str(), convertBuffer, 1024*1024);
			recommendationTitle = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `users_recommendation` where `id`=\"" << recommendationID << "\" and `recommending_userID`='" << user.GetID() << "';";
			if(db.Query(ost.str()))
			{
				string 		recommended_userID = db.Get(0, "recommended_userID");

				ost.str("");
				ost << "update `users_recommendation` set `title`=\"" << recommendationTitle << "\" where `id`=\"" << recommendationID << "\" and `recommending_userID`='" << user.GetID() << "';";
				db.Query(ost.str());
				if(!db.isError())
				{

					ostringstream	dictionaryStatement;
					int				affected;

					// --- check on adverse dictionnary words
				    dictionaryStatement.str("");
				    ost.str("");
				    ost << "SELECT * FROM `dictionary_adverse`;";
				    affected = db.Query(ost.str());
				    if(affected)
				    {
				        for(int i = 0; i < affected; i++)
				        {
				            if(i) dictionaryStatement << " or ";
				            dictionaryStatement << "(`title` like \"%" << db.Get(i, "word") << "%\")";
				        }
				    }

				    ost.str("");
				    ost << "select * from `users_recommendation` where `id`=\"" << recommendationID << "\" and (" << dictionaryStatement.str() << ");";
				    if(db.Query(ost.str()))
				    {
				    	ost.str("");
				    	ost << "update `users_recommendation` set `state`='potentially adverse' where `id`=\"" << recommendationID << "\";";
				    	db.Query(ost.str());
				    	if(db.isError())
				    	{
				    		CLog			log;
				    		ostringstream	ost;

				    		ost.str("");
				    		ost << "int main(void): action == AJAX_addViewProfileAddRecommendation: ERROR: can't update recommendation status to 'potentially adverse' (" << db.GetErrorMessage() << ")";
				    		log.Write(ERROR, ost.str());
				    	}
					}
					else
					{
			    		CLog	log;
			    		ostringstream	ost;

			    		ost.str("");
			    		ost << "int main(void): action == AJAX_addViewProfileAddRecommendation: adverse words not found in new `users_recommendation` (" << recommendationID << ")";

			    		log.Write(DEBUG, ost.str());
					}


					// --- Update live feed
					ost.str("");
					ost << "insert into `users_notification` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << recommended_userID << "\", \"48\", \"" << recommendationID << "\", TIMESTAMPDIFF(second, \"1970-01-01\", NOW()))";
					if(db.InsertQuery(ost.str()))
					{

						ostResult << "{\"result\": \"success\"}";
					}
					else
					{
						ostResult << "{\"result\": \"error\", \"description\": \"error updating live feed\"}";

						{
							CLog	log;
							log.Write(ERROR, "int main(void): action == AJAX_updateRecommendationTitle: ERROR can't update feed table");
						}
					}
				}
				else
				{
					ostResult << "{\"result\": \"error\", \"description\": \"error updating table users\"}";

					{
						CLog	log;
						log.Write(ERROR, "int main(void): action == AJAX_updateRecommendationTitle: ERROR can't update users table");
					}
				}
			}
			else
			{
				ostResult << "{\"result\": \"error\", \"description\": \"recommendation not found or not belongs to you\"}";

				{
					CLog	log;
					log.Write(ERROR, "int main(void): action == AJAX_updateRecommendationTitle: ERROR recommendation not found or not belongs to you");
				}
			}



		}

		indexPage.RegisterVariableForce("result", ostResult.str());
		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == AJAX_updateRecommendationTitle: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_updateRecommendationTitle: end");
		}

	}

	if(action == "AJAX_updateUserResponsibilities")
	{
		ostringstream	ostResult;

		ostResult.str("");
		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == AJAX_updateUserResponsibilities: re-login required");
			}

			ostResult << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";
		}
		else
		{
			long int		respID = atol(indexPage.GetVarsHandler()->Get("id").c_str());
			string			respText = indexPage.GetVarsHandler()->Get("content");
			char			*convertBuffer = new char[ 1024 * 1024];
			ostringstream	ost;

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(respText.c_str(), convertBuffer, 1024*1024);
			respText = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `users_company` where `id`='" << respID << "' and `user_id`='" << user.GetID() << "';";
			if(db.Query(ost.str()))
			{
				if(db.Get(0, "responsibilities") != respText)
				{
					ost.str("");
					ost << "update `users_company` set `responsibilities`=\"" << respText << "\" where `id`='" << respID << "';";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"20\", \"" << respID << "\", NOW())";
					if(db.InsertQuery(ost.str()))
					{

						ostResult << "{\"result\": \"success\"}";
					}
					else
					{
						ostResult << "{\"result\": \"error\", \"description\": \"error updating live feed\"}";

						{
							CLog	log;
							log.Write(ERROR, "int main(void): action == AJAX_updateUserResponsibilities: ERROR can't update feed table");
						}
					}
				}
				else
				{
					// --- no changes in responsibilities
					ostResult << "{\"result\": \"success\"}";
				}
			}
			else
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(ERROR, "int main(void): action == AJAX_updateUserResponsibilities: ERROR changing alien profile");
				}

				ostResult << "{\"result\": \"error\", \"desription\": \"changing alien profile\"}";
			}
		}

		indexPage.RegisterVariableForce("result", ostResult.str());
		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == AJAX_updateUserResponsibilities: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))
	}

	if(action == "AJAX_addEditProfileAddCarrierCompany")
	{
		ostringstream	ostResult;

		ostResult.str("");
		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(): action == AJAX_addEditProfileAddCarrierCompany: re-login required");
			}

			ostResult << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";
		}
		else
		{
			string			title = indexPage.GetVarsHandler()->Get("title");
			string			companyName = indexPage.GetVarsHandler()->Get("companyName");
			string			occupationStart = indexPage.GetVarsHandler()->Get("occupationStart");
			string			occupationFinish = indexPage.GetVarsHandler()->Get("occupationFinish");
			long int		currentCompany = atol(indexPage.GetVarsHandler()->Get("currentCompany").c_str());
			string			responsibilities = indexPage.GetVarsHandler()->Get("responsibilities");
			char			*convertBuffer = new char[ 1024 * 1024];
			ostringstream	ost;

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(title.c_str(), convertBuffer, 1024*1024);
			title = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(companyName.c_str(), convertBuffer, 1024*1024);
			companyName = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(occupationStart.c_str(), convertBuffer, 1024*1024);
			occupationStart = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(occupationFinish.c_str(), convertBuffer, 1024*1024);
			occupationFinish = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(responsibilities.c_str(), convertBuffer, 1024*1024);
			responsibilities = ConvertTextToHTML(convertBuffer);

			if(title.length() && companyName.length() && occupationStart.length() && (currentCompany || occupationFinish.length()))
			{
				unsigned long		titleID = 0, companyID = 0, newCarrierID = 0;
				int					affected;

				ost.str("");
				ost << "select * from users_company_position where `title`=\"" << title << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					titleID = atol(db.Get(0, "id"));

					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCarrierCompany: job title [" << title << "] already exists, no need to update DB.";
						log.Write(DEBUG, ost.str());
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCarrierCompany: new job title [" << title << "] needed to be added to DB";
						log.Write(DEBUG, ost.str());
					}
					ost.str("");
					ost << "insert into `users_company_position` (`title`) VALUES (\"" << title << "\");";
					titleID = db.InsertQuery(ost.str());
				}

				ost.str("");
				ost << "select * from `company` where `name`=\"" << companyName << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					companyID = atol(db.Get(0, "id"));

					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCarrierCompany: company [" << companyName << "] already exists, no need to update DB.";
						log.Write(DEBUG, ost.str());
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCarrierCompany: new company [" << companyName << "] needed to be added to DB";
						log.Write(DEBUG, ost.str());
					}
					ost.str("");
					ost << "insert into `company` (`name`) VALUES (\"" << companyName << "\");";
					companyID = db.InsertQuery(ost.str());
				}

				if(titleID && companyID)
				{

					ost.str("");
					ost << "insert into `users_company` (`user_id`, `company_id`, `position_title_id`, `occupation_start`, `occupation_finish`, `current_company`, `responsibilities`) "
						<< "values(\"" << user.GetID() << "\",\"" << companyID << "\", \"" << titleID << "\", \"" << occupationStart << "\", " << (currentCompany ? "NOW()" : "\"" + occupationFinish + "\"") << ", \"" << currentCompany << "\",  \"" << responsibilities << "\")";
					
					newCarrierID = db.InsertQuery(ost.str());					
					if(newCarrierID)
					{
						if(currentCompany)
						{
							// --- Update live feed
							ost.str("");
							ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"1\", \"" << companyID << "\", NOW())";
							db.Query(ost.str());
						}

						ostResult << "{\"result\": \"success\", \"carrierID\": \"" << newCarrierID << "\"}";
					}
					else
					{
						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::AJAX_addEditProfileAddCarrierCompany: ERROR: issue with inserting int DB table [users_company]";
							log.Write(ERROR, ost.str());
						}

						ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on adding carrier\"}";
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCarrierCompany: ERROR: issue with DB operations on companyName or jobTitle";
						log.Write(ERROR, ost.str());
					}

					ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on companyName or jobTitle\"}";
				}
			}
			else
			{
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::AJAX_addEditProfileAddCarrierCompany: ERROR: mandatory parameter missed or empty in HTML request [title, company, dateStart, dateFinish]";
					log.Write(ERROR, ost.str());
				}

				ostResult << "{\"result\": \"error\", \"description\": \"any mandatory parameter missed\"}";
			}

/*			ost.str("");
			ost << "select * from `users_company` where `id`='" << respID << "' and `user_id`='" << user.GetID() << "';";
			if(db.Query(ost.str()))
			{
				if(db.Get(0, "responsibilities") != respText)
				{
					ost.str("");
					ost << "update `users_company` set `responsibilities`=\"" << respText << "\" where `id`='" << respID << "';";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"20\", \"" << respID << "\", NOW())";
					if(db.InsertQuery(ost.str()))
					{

						ostResult << "{\"result\": \"success\"}";
					}
					else
					{
						ostResult << "{\"result\": \"error\", \"description\": \"error updating live feed\"}";

						{
							CLog	log;
							log.Write(ERROR, "int main(): action == AJAX_addEditProfileAddCarrierCompany: ERROR can't update feed table");
						}
					}
				}
				else
				{
					// --- no changes in responsibilities
					ostResult << "{\"result\": \"success\"}";
				}
			}
			else
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(ERROR, "int main(): action == AJAX_addEditProfileAddCarrierCompany: ERROR changing alien profile");
				}

				ostResult << "{\"result\": \"error\", \"desription\": \"changing alien profile\"}";
			}
*/
		}

		indexPage.RegisterVariableForce("result", ostResult.str());
		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(): action == AJAX_addEditProfileAddCarrierCompany: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))
	}

	if(action == "AJAX_addEditProfileAddCertificate")
	{
		ostringstream	ostResult;

		ostResult.str("");
		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(): action == AJAX_addEditProfileAddCertificate: re-login required");
			}

			ostResult << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";
		}
		else
		{
			string			vendor = indexPage.GetVarsHandler()->Get("vendor");
			string			track = indexPage.GetVarsHandler()->Get("track");
			string			number = indexPage.GetVarsHandler()->Get("number");
			char			*convertBuffer = new char[ 1024 * 1024];
			ostringstream	ost;

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(vendor.c_str(), convertBuffer, 1024*1024);
			vendor = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(track.c_str(), convertBuffer, 1024*1024);
			track = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(number.c_str(), convertBuffer, 1024*1024);
			number = ConvertTextToHTML(convertBuffer);

			if(vendor.length() && track.length() && number.length())
			{
				unsigned long		vendorID = 0, trackID = 0;
				int					affected;

				ost.str("");
				ost << "select * from `company` where `name`=\"" << vendor << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					vendorID = atol(db.Get(0, "id"));

					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCertificate: vendor [" << vendor << "] already exists, no need to update DB.";
						log.Write(DEBUG, ost.str());
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCertificate: vendor [" << vendor << "] needed to be added to DB";
						log.Write(DEBUG, ost.str());
					}
					ost.str("");
					ost << "insert into `company` (`name`) VALUES (\"" << vendor << "\");";
					vendorID = db.InsertQuery(ost.str());
				}

				ost.str("");
				ost << "select * from `certification_tracks` where `title`=\"" << track << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					trackID = atol(db.Get(0, "id"));

					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCertificate: track [" << track << "] already exists, no need to update DB.";
						log.Write(DEBUG, ost.str());
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCertificate: track [" << track << "] needed to be added to DB";
						log.Write(DEBUG, ost.str());
					}
					ost.str("");
					ost << "insert into `certification_tracks` (`title`) VALUES (\"" << track << "\");";
					trackID = db.InsertQuery(ost.str());
				}

				if(vendorID && trackID)
				{
					int		newCertificationID = 0;

					ost.str("");
					ost << "insert into `users_certifications` (`user_id`, `vendor_id`, `track_id`, `certification_number`) "
						<< "values(\"" << user.GetID() << "\",\"" << vendorID << "\", \"" << trackID << "\", \"" << number << "\");";
					
					newCertificationID = db.InsertQuery(ost.str());					
					if(newCertificationID)
					{
						// --- Update live feed
						ost.str("");
						ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"22\", \"" << newCertificationID << "\", NOW())";
						db.Query(ost.str());

						ostResult << "{\"result\": \"success\", \"certificationID\": \"" << newCertificationID << "\"}";
					}
					else
					{
						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::AJAX_addEditProfileAddCertificate: ERROR: issue with inserting into DB table [users_certification]";
							log.Write(ERROR, ost.str());
						}

						ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on adding certification\"}";
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCertificate: ERROR: issue with DB operations on vendor or track";
						log.Write(ERROR, ost.str());
					}

					ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on vendor or track\"}";
				}
			}
			else
			{
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::AJAX_addEditProfileAddCertificate: ERROR: mandatory parameter missed or empty in HTML request [vendor, track, number]";
					log.Write(ERROR, ost.str());
				}

				ostResult << "{\"result\": \"error\", \"description\": \"any mandatory parameter missed\"}";
			}

/*			ost.str("");
			ost << "select * from `users_company` where `id`='" << respID << "' and `user_id`='" << user.GetID() << "';";
			if(db.Query(ost.str()))
			{
				if(db.Get(0, "responsibilities") != respText)
				{
					ost.str("");
					ost << "update `users_company` set `responsibilities`=\"" << respText << "\" where `id`='" << respID << "';";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"20\", \"" << respID << "\", NOW())";
					if(db.InsertQuery(ost.str()))
					{

						ostResult << "{\"result\": \"success\"}";
					}
					else
					{
						ostResult << "{\"result\": \"error\", \"description\": \"error updating live feed\"}";

						{
							CLog	log;
							log.Write(ERROR, "int main(): action == AJAX_addEditProfileAddCertificate: ERROR can't update feed table");
						}
					}
				}
				else
				{
					// --- no changes in responsibilities
					ostResult << "{\"result\": \"success\"}";
				}
			}
			else
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(ERROR, "int main(): action == AJAX_addEditProfileAddCertificate: ERROR changing alien profile");
				}

				ostResult << "{\"result\": \"error\", \"desription\": \"changing alien profile\"}";
			}
*/
		}

		indexPage.RegisterVariableForce("result", ostResult.str());
		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(): action == AJAX_addEditProfileAddCertificate: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))
	}


	if(action == "AJAX_addEditProfileAddCourse")
	{
		ostringstream	ostResult;

		ostResult.str("");
		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(): action == AJAX_addEditProfileAddCourse: re-login required");
			}

			ostResult << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";
		}
		else
		{
			string			vendor = indexPage.GetVarsHandler()->Get("vendor");
			string			track = indexPage.GetVarsHandler()->Get("track");
			char			*convertBuffer = new char[ 1024 * 1024];
			ostringstream	ost;

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(vendor.c_str(), convertBuffer, 1024*1024);
			vendor = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(track.c_str(), convertBuffer, 1024*1024);
			track = ConvertTextToHTML(convertBuffer);

			if(vendor.length() && track.length())
			{
				unsigned long		vendorID = 0, trackID = 0;
				int					affected;

				ost.str("");
				ost << "select * from `company` where `name`=\"" << vendor << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					vendorID = atol(db.Get(0, "id"));

					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCourse: vendor [" << vendor << "] already exists, no need to update DB.";
						log.Write(DEBUG, ost.str());
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCourse: vendor [" << vendor << "] needed to be added to DB";
						log.Write(DEBUG, ost.str());
					}
					ost.str("");
					ost << "insert into `company` (`name`) VALUES (\"" << vendor << "\");";
					vendorID = db.InsertQuery(ost.str());
				}

				ost.str("");
				ost << "select * from `certification_tracks` where `title`=\"" << track << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					trackID = atol(db.Get(0, "id"));

					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCourse: track [" << track << "] already exists, no need to update DB.";
						log.Write(DEBUG, ost.str());
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCourse: track [" << track << "] needed to be added to DB";
						log.Write(DEBUG, ost.str());
					}
					ost.str("");
					ost << "insert into `certification_tracks` (`title`) VALUES (\"" << track << "\");";
					trackID = db.InsertQuery(ost.str());
				}

				if(vendorID && trackID)
				{
					int		newCertificationID = 0;

					ost.str("");
					ost << "insert into `users_courses` (`user_id`, `vendor_id`, `track_id`) "
						<< "values(\"" << user.GetID() << "\",\"" << vendorID << "\", \"" << trackID << "\");";
					
					newCertificationID = db.InsertQuery(ost.str());					
					if(newCertificationID)
					{
						// --- Update live feed
						ost.str("");
						ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"23\", \"" << newCertificationID << "\", NOW())";
						db.Query(ost.str());

						ostResult << "{\"result\": \"success\", \"certificationID\": \"" << newCertificationID << "\"}";
					}
					else
					{
						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::AJAX_addEditProfileAddCourse: ERROR: issue with inserting into DB table [users_courses]";
							log.Write(ERROR, ost.str());
						}

						ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on adding course\"}";
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddCourse: ERROR: issue with DB operations on vendor or track";
						log.Write(ERROR, ost.str());
					}

					ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on vendor or track\"}";
				}
			}
			else
			{
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::AJAX_addEditProfileAddCourse: ERROR: mandatory parameter missed or empty in HTML request [vendor, track]";
					log.Write(ERROR, ost.str());
				}

				ostResult << "{\"result\": \"error\", \"description\": \"any mandatory parameter missed\"}";
			}

/*			ost.str("");
			ost << "select * from `users_company` where `id`='" << respID << "' and `user_id`='" << user.GetID() << "';";
			if(db.Query(ost.str()))
			{
				if(db.Get(0, "responsibilities") != respText)
				{
					ost.str("");
					ost << "update `users_company` set `responsibilities`=\"" << respText << "\" where `id`='" << respID << "';";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"20\", \"" << respID << "\", NOW())";
					if(db.InsertQuery(ost.str()))
					{

						ostResult << "{\"result\": \"success\"}";
					}
					else
					{
						ostResult << "{\"result\": \"error\", \"description\": \"error updating live feed\"}";

						{
							CLog	log;
							log.Write(ERROR, "int main(): action == AJAX_addEditProfileAddCourse: ERROR can't update feed table");
						}
					}
				}
				else
				{
					// --- no changes in responsibilities
					ostResult << "{\"result\": \"success\"}";
				}
			}
			else
			{
				ostringstream	ost;

				{
					CLog	log;
					log.Write(ERROR, "int main(): action == AJAX_addEditProfileAddCourse: ERROR changing alien profile");
				}

				ostResult << "{\"result\": \"error\", \"desription\": \"changing alien profile\"}";
			}
*/
		}

		indexPage.RegisterVariableForce("result", ostResult.str());
		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(): action == AJAX_addEditProfileAddCourse: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))
	}

	if(action == "AJAX_addEditProfileAddLanguage")
	{
		ostringstream	ostResult;

		{
			CLog	log;
			log.Write(DEBUG, "int main(): action == AJAX_addEditProfileAddLanguage: start");
		}

		ostResult.str("");
		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(): action == AJAX_addEditProfileAddLanguage: re-login required");
			}

			ostResult << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";
		}
		else
		{
			string			title = indexPage.GetVarsHandler()->Get("title");
			string			level = indexPage.GetVarsHandler()->Get("level");
			char			*convertBuffer = new char[ 1024 * 1024];
			ostringstream	ost;

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(title.c_str(), convertBuffer, 1024*1024);
			title = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(level.c_str(), convertBuffer, 1024*1024);
			level = ConvertTextToHTML(convertBuffer);

			if(title.length() && level.length())
			{
				unsigned long		languageID = 0;
				int					affected;

				ost.str("");
				ost << "select * from `language` where `title`=\"" << title << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					languageID = atol(db.Get(0, "id"));

					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddLanguage: title [" << title << "] already exists, no need to update DB.";
						log.Write(DEBUG, ost.str());
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddLanguage: title [" << title << "] needed to be added to DB";
						log.Write(DEBUG, ost.str());
					}
					ost.str("");
					ost << "insert into `language` (`title`) VALUES (\"" << title << "\");";
					languageID = db.InsertQuery(ost.str());
				}

				if(languageID)
				{
					int		newLanguageID = 0;

					ost.str("");
					ost << "insert into `users_language` (`user_id`, `language_id`, `level`) "
						<< "values(\"" << user.GetID() << "\",\"" << languageID << "\", \"" << level << "\");";
					
					newLanguageID = db.InsertQuery(ost.str());					
					if(newLanguageID)
					{
						// --- Update live feed
						ost.str("");
						ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"29\", \"" << newLanguageID << "\", NOW())";
						db.Query(ost.str());

						ostResult << "{\"result\": \"success\", \"languageID\": \"" << newLanguageID << "\"}";
					}
					else
					{
						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::AJAX_addEditProfileAddLanguage: ERROR: issue with inserting into DB table [users_Languages]";
							log.Write(ERROR, ost.str());
						}

						ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on adding Language\"}";
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddLanguage: ERROR: issue with DB operations on title or level";
						log.Write(ERROR, ost.str());
					}

					ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on title or level\"}";
				}
			}
			else
			{
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::AJAX_addEditProfileAddLanguage: ERROR: mandatory parameter missed or empty in HTML request [title, level]";
					log.Write(ERROR, ost.str());
				}

				ostResult << "{\"result\": \"error\", \"description\": \"any mandatory parameter missed\"}";
			}
		}

		indexPage.RegisterVariableForce("result", ostResult.str());
		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(): action == AJAX_addEditProfileAddLanguage: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))

		{
			CLog	log;
			log.Write(DEBUG, "int main(): action == AJAX_addEditProfileAddLanguage: finish");
		}
	}

	if(action == "AJAX_addEditProfileAddSkill")
	{
		ostringstream	ostResult;

		ostResult.str("");
		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(): action == AJAX_addEditProfileAddSkill: re-login required");
			}

			ostResult << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";
		}
		else
		{
			string			title = indexPage.GetVarsHandler()->Get("title");
			char			*convertBuffer = new char[ 1024 * 1024];
			ostringstream	ost;

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(title.c_str(), convertBuffer, 1024*1024);
			title = ConvertTextToHTML(convertBuffer);

			if(title.length())
			{
				unsigned long		skillID = 0;
				int					affected;

				ost.str("");
				ost << "select * from `skill` where `title`=\"" << title << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					skillID = atol(db.Get(0, "id"));

					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddSkill: title [" << title << "] already exists, no need to update DB.";
						log.Write(DEBUG, ost.str());
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddSkill: title [" << title << "] needed to be added to DB";
						log.Write(DEBUG, ost.str());
					}
					ost.str("");
					ost << "insert into `skill` (`title`) VALUES (\"" << title << "\");";
					skillID = db.InsertQuery(ost.str());
				}

				if(skillID)
				{
					int		newSkillID = 0;

					ost.str("");
					ost << "insert into `users_skill` (`user_id`, `skill_id`) "
						<< "values(\"" << user.GetID() << "\",\"" << skillID << "\");";
					
					newSkillID = db.InsertQuery(ost.str());					
					if(newSkillID)
					{
						// --- Update live feed
						ost.str("");
						ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"41\", \"" << newSkillID << "\", NOW())";
						db.Query(ost.str());

						ostResult << "{\"result\": \"success\", \"skillID\": \"" << newSkillID << "\"}";
					}
					else
					{
						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::AJAX_addEditProfileAddSkill: ERROR: issue with inserting into DB table [users_skill]";
							log.Write(ERROR, ost.str());
						}

						ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on adding skill\"}";
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddSkill: ERROR: issue with DB operations on title or level";
						log.Write(ERROR, ost.str());
					}

					ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on title or level\"}";
				}
			}
			else
			{
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::AJAX_addEditProfileAddSkill: ERROR: mandatory parameter missed or empty in HTML request [title]";
					log.Write(ERROR, ost.str());
				}

				ostResult << "{\"result\": \"error\", \"description\": \"any mandatory parameter missed\"}";
			}
		}

		indexPage.RegisterVariableForce("result", ostResult.str());
		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(): action == AJAX_addEditProfileAddSkill: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))
	}


	if(action == "AJAX_addEditProfileAddSchool")
	{
		ostringstream	ostResult;

		ostResult.str("");
		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(): action == AJAX_addEditProfileAddSchool: re-login required");
			}

			ostResult << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";
		}
		else
		{
			string			locality = indexPage.GetVarsHandler()->Get("locality");
			string			title = indexPage.GetVarsHandler()->Get("title");
			string			periodStart = indexPage.GetVarsHandler()->Get("periodStart");
			string			periodFinish = indexPage.GetVarsHandler()->Get("periodFinish");
			char			*convertBuffer = new char[ 1024 * 1024];
			ostringstream	ost;

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(locality.c_str(), convertBuffer, 1024*1024);
			locality = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(title.c_str(), convertBuffer, 1024*1024);
			title = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(periodStart.c_str(), convertBuffer, 1024*1024);
			periodStart = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(periodFinish.c_str(), convertBuffer, 1024*1024);
			periodFinish = ConvertTextToHTML(convertBuffer);

			if(locality.length() && title.length() && periodStart.length() && periodFinish.length() && (atoi(periodStart.c_str()) <= atoi(periodFinish.c_str())))
			{
				unsigned long		localityID = 0, titleID = 0;
				int					affected;

				ost.str("");
				ost << "select * from `geo_locality` where `title`=\"" << locality << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					localityID = atol(db.Get(0, "id"));

					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddSchool: locality [" << locality << "] already exists, no need to update DB.";
						log.Write(DEBUG, ost.str());
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddSchool: locality [" << locality << "] needed to be added to DB";
						log.Write(DEBUG, ost.str());
					}
					ost.str("");
					ost << "insert into `geo_locality` (`title`) VALUES (\"" << locality << "\");";
					localityID = db.InsertQuery(ost.str());
				}

				ost.str("");
				ost << "select * from `school` where `title`=\"" << title << "\" and `geo_locality_id`=\"" << localityID << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					titleID = atol(db.Get(0, "id"));

					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddSchool: school [" << title << "] already exists, no need to update DB.";
						log.Write(DEBUG, ost.str());
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddSchool: school [" << title << "] needed to be added to DB";
						log.Write(DEBUG, ost.str());
					}
					ost.str("");
					ost << "insert into `school` (`geo_locality_id`, `title`) VALUES (\"" << localityID << "\",\"" << title << "\");";
					titleID = db.InsertQuery(ost.str());
				}

				if(localityID && titleID)
				{
					int		newSchoolID = 0;

					ost.str("");
					ost << "insert into `users_school` (`user_id`, `school_id`, `occupation_start`, `occupation_finish`) "
						<< "values(\"" << user.GetID() << "\", \"" << titleID << "\", \"" << periodStart << "\", \"" << periodFinish << "\");";
					
					newSchoolID = db.InsertQuery(ost.str());					
					if(newSchoolID)
					{
						// --- Update live feed
						ost.str("");
						ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"27\", \"" << newSchoolID << "\", NOW())";
						db.Query(ost.str());

						ostResult << "{\"result\": \"success\", \"schoolID\": \"" << newSchoolID << "\"}";
					}
					else
					{
						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::AJAX_addEditProfileAddSchool: ERROR: issue with inserting into DB table [users_school]";
							log.Write(ERROR, ost.str());
						}

						ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on adding school\"}";
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddSchool: ERROR: issue with DB operations on locality or school";
						log.Write(ERROR, ost.str());
					}

					ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on locality or school\"}";
				}
			}
			else
			{
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::AJAX_addEditProfileAddSchool: ERROR: mandatory parameter missed or empty in HTML request [locality, school, periodStart, periodFinish] or (periodStart > periodFinish)";
					log.Write(ERROR, ost.str());
				}

				ostResult << "{\"result\": \"error\", \"description\": \"any mandatory parameter missed\"}";
			}
		}

		indexPage.RegisterVariableForce("result", ostResult.str());
		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(): action == AJAX_addEditProfileAddSchool: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))
	}

	if(action == "AJAX_addEditProfileAddUniversity")
	{
		ostringstream	ostResult;

		ostResult.str("");
		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(): action == AJAX_addEditProfileAddUniversity: re-login required");
			}

			ostResult << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";
		}
		else
		{
			string			region = indexPage.GetVarsHandler()->Get("region");
			string			title = indexPage.GetVarsHandler()->Get("title");
			string			degree = indexPage.GetVarsHandler()->Get("degree");
			string			periodStart = indexPage.GetVarsHandler()->Get("periodStart");
			string			periodFinish = indexPage.GetVarsHandler()->Get("periodFinish");
			char			*convertBuffer = new char[ 1024 * 1024];
			ostringstream	ost;

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(region.c_str(), convertBuffer, 1024*1024);
			region = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(title.c_str(), convertBuffer, 1024*1024);
			title = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(degree.c_str(), convertBuffer, 1024*1024);
			degree = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(periodStart.c_str(), convertBuffer, 1024*1024);
			periodStart = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(periodFinish.c_str(), convertBuffer, 1024*1024);
			periodFinish = ConvertTextToHTML(convertBuffer);

			if(region.length() && title.length() && degree.length() && periodStart.length() && periodFinish.length() && (atoi(periodStart.c_str()) <= atoi(periodFinish.c_str())))
			{
				unsigned long		regionID = 0, titleID = 0;
				int					affected;

				ost.str("");
				ost << "select * from `geo_region` where `title`=\"" << region << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					regionID = atol(db.Get(0, "id"));

					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddUniversity: region [" << region << "] already exists, no need to update DB.";
						log.Write(DEBUG, ost.str());
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddUniversity: region [" << region << "] needed to be added to DB";
						log.Write(DEBUG, ost.str());
					}
					ost.str("");
					ost << "insert into `geo_region` (`title`) VALUES (\"" << region << "\");";
					regionID = db.InsertQuery(ost.str());
				}

				ost.str("");
				ost << "select * from `university` where `title`=\"" << title << "\" and  `geo_region_id`=\"" << regionID << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					titleID = atol(db.Get(0, "id"));

					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddUniversity: university [" << title << "] already exists, no need to update DB.";
						log.Write(DEBUG, ost.str());
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddUniversity: university [" << title << "] needed to be added to DB";
						log.Write(DEBUG, ost.str());
					}
					ost.str("");
					ost << "insert into `university` (`title`, `geo_region_id`) VALUES (\"" << title << "\", \"" << regionID << "\");";
					titleID = db.InsertQuery(ost.str());
				}

				if(regionID && titleID)
				{
					int		newUniversityID = 0;

					ost.str("");
					ost << "insert into `users_university` (`user_id`, `university_id`, `degree`, `occupation_start`, `occupation_finish`) "
						<< "values(\"" << user.GetID() << "\", \"" << titleID << "\", \"" << degree << "\", \"" << periodStart << "\", \"" << periodFinish << "\");";
					
					newUniversityID = db.InsertQuery(ost.str());					
					if(newUniversityID)
					{
						// --- Update live feed
						ost.str("");
						ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"28\", \"" << newUniversityID << "\", NOW())";
						db.Query(ost.str());

						ostResult << "{\"result\": \"success\", \"universityID\": \"" << newUniversityID << "\"}";
					}
					else
					{
						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::AJAX_addEditProfileAddUniversity: ERROR: issue with inserting into DB table [users_university]";
							log.Write(ERROR, ost.str());
						}

						ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on adding university\"}";
					}
				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addEditProfileAddUniversity: ERROR: issue with DB operations on region or university";
						log.Write(ERROR, ost.str());
					}

					ostResult << "{\"result\": \"error\", \"description\": \"issue with DB operations on region or university\"}";
				}
			}
			else
			{
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::AJAX_addEditProfileAddUniversity: ERROR: mandatory parameter missed or empty in HTML request [region, university, degree, period] or (periodStart > periodFinish)";
					log.Write(ERROR, ost.str());
				}

				ostResult << "{\"result\": \"error\", \"description\": \"any mandatory parameter missed\"}";
			}
		}

		indexPage.RegisterVariableForce("result", ostResult.str());
		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(): action == AJAX_addEditProfileAddUniversity: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))
	}

	if(action == "AJAX_addViewProfileAddRecommendation")
	{
		ostringstream	ostResult;

		ostResult.str("");
		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(): action == AJAX_addViewProfileAddRecommendation: re-login required");
			}

			ostResult << "{\"result\": \"error\", \"description\": \"session lost. Need to relogin\"}";
		}
		else
		{
			string			title = indexPage.GetVarsHandler()->Get("title");
			string			recommendedUserID = indexPage.GetVarsHandler()->Get("recommendedUserID");
			string			eventTimestamp = indexPage.GetVarsHandler()->Get("eventTimestamp");
			char			*convertBuffer = new char[ 1024 * 1024];
			ostringstream	ost;

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(title.c_str(), convertBuffer, 1024*1024);
			title = ConvertTextToHTML(convertBuffer);
			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(recommendedUserID.c_str(), convertBuffer, 1024*1024);
			recommendedUserID = ConvertTextToHTML(convertBuffer);
			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(eventTimestamp.c_str(), convertBuffer, 1024*1024);
			eventTimestamp = ConvertTextToHTML(convertBuffer);

			if(title.length() && eventTimestamp.length() && recommendedUserID.length())
			{
				unsigned long		newRecommendationID = 0;

				ost.str("");
				ost << "insert into `users_recommendation` (`recommended_userID`, `recommending_userID`, `title`, `eventTimestamp`) VALUES \
				(\"" << recommendedUserID << "\",\"" << user.GetID() << "\",\"" << title << "\",\"" << eventTimestamp << "\");";
				newRecommendationID = db.InsertQuery(ost.str());

				if(newRecommendationID)
				{
					ostringstream	dictionaryStatement;
					int				affected;

					// --- check on adverse dictionnary words
				    dictionaryStatement.str("");
				    ost.str("");
				    ost << "SELECT * FROM `dictionary_adverse`;";
				    affected = db.Query(ost.str());
				    if(affected)
				    {
				        for(int i = 0; i < affected; i++)
				        {
				            if(i) dictionaryStatement << " or ";
				            dictionaryStatement << "(`title` like \"%" << db.Get(i, "word") << "%\")";
				        }
				    }

				    ost.str("");
				    ost << "select * from `users_recommendation` where `id`=\"" << newRecommendationID << "\" and (" << dictionaryStatement.str() << ");";
				    if(db.Query(ost.str()))
				    {
				    	ost.str("");
				    	ost << "update `users_recommendation` set `state`='potentially adverse' where `id`=\"" << newRecommendationID << "\";";
				    	db.Query(ost.str());
				    	if(db.isError())
				    	{
				    		CLog			log;
				    		ostringstream	ost;

				    		ost.str("");
				    		ost << "int main(void): action == AJAX_addViewProfileAddRecommendation: ERROR: can't update recommendation status to 'potentially adverse' (" << db.GetErrorMessage() << ")";
				    		log.Write(ERROR, ost.str());
				    	}
					}
					else
					{
			    		CLog	log;
			    		ostringstream	ost;

			    		ost.str("");
			    		ost << "int main(void): action == AJAX_addViewProfileAddRecommendation: adverse words not found in new `users_recommendation` (" << newRecommendationID << ")";

			    		log.Write(DEBUG, ost.str());
					}

					// --- Update live feed
					ost.str("");
					ost << "insert into `users_notification` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << recommendedUserID << "\", \"45\", \"" << newRecommendationID << "\", TIMESTAMPDIFF(second, \"1970-01-01\", NOW()) );";
					if(db.InsertQuery(ost.str()))
					{
						ostResult << "{\"result\": \"success\", \"recommendationID\": \"" << newRecommendationID << "\"}";
					}
					else
					{
						ostResult << "{\"result\": \"error\", \"description\": \"error updating live feed\"}";

						{
							CLog	log;
							log.Write(ERROR, "int main(void): action == AJAX_addViewProfileAddRecommendation: ERROR can't update feed table");
						}
					}

				}
				else
				{
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::AJAX_addViewProfileAddRecommendation: ERROR: inserting into users_recommendation table";
						log.Write(ERROR, ost.str());
					}

					ostResult << "{\"result\": \"error\", \"description\": \"inserting into users_recommendation table\"}";
				}
			}
			else
			{
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::AJAX_addViewProfileAddRecommendation: ERROR: mandatory parameter missed or empty in HTML request [title or eventTimestamp or recommendedUserID]";
					log.Write(ERROR, ost.str());
				}

				ostResult << "{\"result\": \"error\", \"description\": \"any mandatory parameter missed\"}";
			}
		}

		indexPage.RegisterVariableForce("result", ostResult.str());
		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(): action == AJAX_addViewProfileAddRecommendation: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))
	}



	// --- AJAX update message to news feed
	if(action == "AJAX_updateNewsFeedMessage")
	{
		ostringstream	ost;
		string			strPageToGet, strNewsOnSinglePage;
		string			newsFeedMessageID;
		string			newsFeedMessageTitle;
		string			newsFeedMessageLink;
		string			newsFeedMessageText;
		string			newsFeedMessageRights;
		string			newsFeedMessageImageTempSet;
		string			newsFeedMessageImageSet;

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == AJAX_updateNewsFeedMessage: re-login required");
			}

			ost.str("");
			ost << "[{\"result\": \"error\"}, {\"desription\": \"session lost. Need to relogin\"}]";

			indexPage.RegisterVariableForce("result", ost.str());

			if(!indexPage.SetTemplate("json_response.htmlt"))
			{
				CLog	log;
				log.Write(ERROR, "int main(void): action == AJAX_updateNewsFeedMessage: ERROR can't find template json_response.htmlt");
				throw CExceptionHTML("user not activated");
			} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))
		}
		else
		{
			// --- "new" used due to possibility of drop "standard exception"
			char			*convertBuffer = new char[ 1024 * 1024];

			// --- This line will not be reached in case of error in memory allocation 
			// --- To avoid throw std exception use char *a = new(std::nothrow) char[ 0x7FFFFFFF ];
			if(!convertBuffer)
			{
				CLog	log;
				log.Write(ERROR, "int main(void): action == AJAX_updateNewsFeedMessage: ERROR can't allocate memory");			
			}

			// --- Authorized user
			newsFeedMessageID = indexPage.GetVarsHandler()->Get("newsFeedMessageID");
			newsFeedMessageTitle = indexPage.GetVarsHandler()->Get("newsFeedMessageTitle");
			newsFeedMessageLink = indexPage.GetVarsHandler()->Get("newsFeedMessageLink");
			newsFeedMessageText = indexPage.GetVarsHandler()->Get("newsFeedMessageText");
			newsFeedMessageRights = indexPage.GetVarsHandler()->Get("newsFeedMessageRights");
			newsFeedMessageImageTempSet = indexPage.GetVarsHandler()->Get("newsFeedMessageImageTempSet");
			newsFeedMessageImageSet = "";

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(newsFeedMessageTitle.c_str(), convertBuffer, 1024*1024);
			newsFeedMessageTitle = convertBuffer;
			trim(newsFeedMessageTitle);

			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(newsFeedMessageText.c_str(), convertBuffer, 1024*1024);
			newsFeedMessageText = convertBuffer;
			trim(newsFeedMessageText);

			delete[] convertBuffer;

			// --- Clean-up the text
			newsFeedMessageID = ReplaceDoubleQuoteToQuote(newsFeedMessageID);
			newsFeedMessageID = DeleteHTML(newsFeedMessageID);
			newsFeedMessageID = SymbolReplace(newsFeedMessageID, "\r\n", "<br>");
			newsFeedMessageID = SymbolReplace(newsFeedMessageID, "\r", "<br>");
			newsFeedMessageID = SymbolReplace(newsFeedMessageID, "\n", "<br>");
			newsFeedMessageTitle = ReplaceDoubleQuoteToQuote(newsFeedMessageTitle);
			newsFeedMessageTitle = DeleteHTML(newsFeedMessageTitle);
			newsFeedMessageTitle = SymbolReplace(newsFeedMessageTitle, "\r\n", "<br>");
			newsFeedMessageTitle = SymbolReplace(newsFeedMessageTitle, "\r", "<br>");
			newsFeedMessageTitle = SymbolReplace(newsFeedMessageTitle, "\n", "<br>");
			newsFeedMessageLink  = ReplaceDoubleQuoteToQuote(newsFeedMessageLink);
			newsFeedMessageLink  = DeleteHTML(newsFeedMessageLink);
			newsFeedMessageLink  = SymbolReplace(newsFeedMessageLink, "\r\n", "<br>");
			newsFeedMessageLink  = SymbolReplace(newsFeedMessageLink, "\r", "<br>");
			newsFeedMessageLink  = SymbolReplace(newsFeedMessageLink, "\n", "<br>");
			newsFeedMessageText  = ReplaceDoubleQuoteToQuote(newsFeedMessageText);
			newsFeedMessageText  = DeleteHTML(newsFeedMessageText);
			newsFeedMessageText  = SymbolReplace(newsFeedMessageText, "\r\n", "<br>");
			newsFeedMessageText  = SymbolReplace(newsFeedMessageText, "\r", "<br>");
			newsFeedMessageText  = SymbolReplace(newsFeedMessageText, "\n", "<br>");

			// --- messageID defined
			if(newsFeedMessageID.length() > 0)
			{
				// --- handle images assigned to message
				// --- remove all images subjected to removal
				ost.str("");
				ost << "`tempSet`='" << newsFeedMessageImageTempSet << "' and `userID`='" << user.GetID() << "' and `removeFlag`='remove';";
				RemoveMessageImages(ost.str(), &db);

				// --- define new SetID from existing images or newly uploaded
				ost.str("");
				ost << "select `setID` from `feed_images` where `setID`<>'0' and `tempSet`='" << newsFeedMessageImageTempSet << "' and `userID`='" << user.GetID() << "';";
				if(db.Query(ost.str()))
				{
					newsFeedMessageImageSet = db.Get(0, "setID");
				}
				else
				{
					ost.str("");
					ost << "select `id` from `feed_images` where `tempSet`='" << newsFeedMessageImageTempSet << "' and `userID`='" << user.GetID() << "' LIMIT 0,1;";
					if(db.Query(ost.str()))
					{
						newsFeedMessageImageSet = db.Get(0, "id");
					}
					else
					{
						CLog			log;
						ostringstream	ost;

						ost.str("");
						ost << "int main(void): action == AJAX_updateNewsFeedMessage: ERROR: can't find entries in `feed_images` where `tempSet`='" << newsFeedMessageImageTempSet << "' and `userID`='" << user.GetID() << "'. Lost images will appear in `feed_images`.;";
						log.Write(ERROR, ost.str());
					}
				}
				if(newsFeedMessageImageSet.length() > 0)
				{
					ost.str("");
					ost << "update `feed_images` set `setID`='" << newsFeedMessageImageSet << "', `tempSet`='0' where `tempSet`='" << newsFeedMessageImageTempSet << "' and `userID`='" << user.GetID() << "';";
					db.Query(ost.str());
				}


				if(!((newsFeedMessageTitle == "") && (newsFeedMessageText == "") && (newsFeedMessageImageTempSet == "") && (newsFeedMessageImageSet == "")))
				{
					string		messageId;
		
					ost.str("");
					ost << "update `feed_message` set  \
							`title`=\"" << newsFeedMessageTitle << "\", \
							`link`=\"" << newsFeedMessageLink << "\", \
							`message`=\"" << newsFeedMessageText << "\", \
							`imageSetID`=\"" << (newsFeedMessageImageSet == "" ? "0" : newsFeedMessageImageSet) << "\", \
							`access`=\"" << newsFeedMessageRights << "\" \
							where `id`='" << newsFeedMessageID << "';";
					db.Query(ost.str());

					ost.str("");
					ost << "[";
					ost << "{";
					ost << "\"result\": \"success\",";
					ost << "\"description\": \"message has been updated\"";
					ost << "}";
					ost << "]";

					{
						CLog			log;
						ostringstream	ost;

						ost << "int main(void): action == AJAX_updateNewsFeedMessage: message [id = " << messageId << "] from [" << user.GetName() << "] has been posted";
						log.Write(DEBUG, ost.str());
					}


					indexPage.RegisterVariableForce("result", ost.str());

					if(!indexPage.SetTemplate("json_response.htmlt"))
					{
						CLog	log;
						log.Write(ERROR, "int main(void): action == AJAX_updateNewsFeedMessage: ERROR can't find template json_response.htmlt");
						throw CExceptionHTML("user not activated");
					} // if(!indexPage.SetTemplate("json_response.htmlt"))
				} // if(!((newsFeedMessageTitle == "") && (newsFeedMessageText == "") && (newsFeedMessageImage == "")))
				else
				{
					// --- Empty title, message and image
					ost.str("");
					ost << "[";
					ost << "{";
					ost << "\"result\": \"error\",";
					ost << "\"description\": \"can't post message due to title, text and image is empty \"";
					ost << "}";
					ost << "]";

					CLog	log;

					log.Write(ERROR, "int main(void): action == AJAX_updateNewsFeedMessage: ERROR can't post message due to title, text and image is empty ");
				} // if(!((newsFeedMessageTitle == "") && (newsFeedMessageText == "") && (newsFeedMessageImage == "")))
	
			} // if(newsFeedMessageID.length() > 0)
			else
			{
				// --- Empty title, message and image
				ost.str("");
				ost << "[";
				ost << "{";
				ost << "\"result\": \"error\",";
				ost << "\"description\": \"can't update message due to messageID is not defined\"";
				ost << "}";
				ost << "]";

				CLog	log;
				log.Write(ERROR, "int main(void): action == AJAX_updateNewsFeedMessage: ERROR can't update message due to messageID is not defined");
			}
		} // if(user.GetLogin() == "Guest")

		indexPage.RegisterVariableForce("result", ost.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == AJAX_updateNewsFeedMessage: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("Template file was missing");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))

	}

	// --- AJAX comment on message in news feed
	if(action == "AJAX_commentOnMessageInNewsFeed")
	{
		ostringstream	ost;
		string			strPageToGet, strNewsOnSinglePage;
		string			newsFeedMessageID;
		string			newsFeedMessageComment;

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == AJAX_commentOnMessageInNewsFeed: re-login required");
			}

			ost.str("");
			ost << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";

			indexPage.RegisterVariableForce("result", ost.str());

			if(!indexPage.SetTemplate("json_response.htmlt"))
			{
				CLog	log;
				log.Write(ERROR, "int main(void): action == AJAX_commentOnMessageInNewsFeed: ERROR can't find template json_response.htmlt");
				throw CExceptionHTML("user not activated");
			} // if(!indexPage.SetTemplate("json_response.htmlt"))
		}
		else
		{
			// --- "new" used due to possibility of drop "standard exception"
			char			*convertBuffer = new char[ 1024 * 1024];

			// --- This line will not be reached in case of error in memory allocation 
			// --- To avoid throw std exception use char *a = new(std::nothrow) char[ 0x7FFFFFFF ];
			if(!convertBuffer)
			{
				CLog	log;
				log.Write(ERROR, "int main(void): action == AJAX_commentOnMessageInNewsFeed: ERROR can't allocate memory");
			}

			// --- Authorized user
			newsFeedMessageID = indexPage.GetVarsHandler()->Get("messageID");
			newsFeedMessageComment = indexPage.GetVarsHandler()->Get("comment");

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(newsFeedMessageComment.c_str(), convertBuffer, 1024*1024);
			newsFeedMessageComment = convertBuffer;
			trim(newsFeedMessageComment);

			delete[] convertBuffer;

			// --- Clean-up the text
			newsFeedMessageID = ReplaceDoubleQuoteToQuote(newsFeedMessageID);
			newsFeedMessageID = DeleteHTML(newsFeedMessageID);
			newsFeedMessageID = SymbolReplace(newsFeedMessageID, "\r\n", "<br>");
			newsFeedMessageID = SymbolReplace(newsFeedMessageID, "\r", "<br>");
			newsFeedMessageID = SymbolReplace(newsFeedMessageID, "\n", "<br>");
			newsFeedMessageComment = ReplaceDoubleQuoteToQuote(newsFeedMessageComment);
			newsFeedMessageComment = DeleteHTML(newsFeedMessageComment);
			newsFeedMessageComment = SymbolReplace(newsFeedMessageComment, "\r\n", "<br>");
			newsFeedMessageComment = SymbolReplace(newsFeedMessageComment, "\r", "<br>");
			newsFeedMessageComment = SymbolReplace(newsFeedMessageComment, "\n", "<br>");

			// --- messageID defined
			if(newsFeedMessageID.length() > 0)
			{
				// --- check that message exists
				ost.str("");
				ost << "select `id` from `feed_message` where `id`='" << newsFeedMessageID << "';";
				if(db.Query(ost.str()))
				{
					// --- looking for message owner
					ost.str("");
					ost << "select * from `feed` where `actionTypeId`=\"11\" and `actionId`=\"" << newsFeedMessageID << "\";";
					if(db.Query(ost.str()))
					{
						unsigned long 		feed_message_comment_id;
						string				messageOwnerID = db.Get(0, "userId");

						// --- insert comment
						ost.str("");
						ost << "insert into `feed_message_comment` (`messageID`, `userID`, `comment`, `eventTimestamp`) VALUES (\"" << newsFeedMessageID << "\", \"" << user.GetID() << "\", \"" << newsFeedMessageComment << "\", NOW());";
						feed_message_comment_id = db.InsertQuery(ost.str());
						if(feed_message_comment_id)
						{

							if(messageOwnerID != user.GetID()) // --- if comment written by other users to your message
							{
								// --- insert user notification
								ost.str("");
								ost << "insert into `users_notification` (`userID`, `actionTypeId`, `actionId`, `eventTimestamp`) VALUES ('" << messageOwnerID << "', '19', '" << feed_message_comment_id << "', TIMESTAMPDIFF(second, \"1970-01-01\", NOW()) );";
								if(db.InsertQuery(ost.str()))
								{
									{
										CLog	log;
										log.Write(DEBUG, "int main(void): action == AJAX_commentOnMessageInNewsFeed: success comment submission");
									}

								}
								else
								{
									{
										CLog	log;
										log.Write(ERROR, "int main(void): action == AJAX_commentOnMessageInNewsFeed: ERROR inserting into users_notification table");
									}
								}
							}

							ost.str("");
							ost << "{";
							ost << "\"result\": \"success\",";
							ost << "\"description\": \"\"";
							ost << "}";
						}
						else
						{
							{
								CLog	log;
								log.Write(ERROR, "int main(void): action == AJAX_commentOnMessageInNewsFeed: ERROR inserting into feed_message_comment");
							}

							ost.str("");
							ost << "{";
							ost << "\"result\": \"error\",";
							ost << "\"description\": \"ERROR inserting into feed_message_comment\"";
							ost << "}";
						}
					}
					else
					{
						{
							CLog	log;
							log.Write(ERROR, "int main(void): action == AJAX_commentOnMessageInNewsFeed: ERROR finding messageID in feed");
						}

						ost.str("");
						ost << "{";
						ost << "\"result\": \"error\",";
						ost << "\"description\": \"ERROR finding messageID in feed\"";
						ost << "}";
					}



				}
				else
				{
					// --- Empty title, message and image
					ost.str("");
					ost << "{";
					ost << "\"result\": \"error\",";
					ost << "\"description\": \"messageID[" << newsFeedMessageID << "] is not exists\"";
					ost << "}";

					CLog	log;

					log.Write(ERROR, "int main(void): action == AJAX_commentOnMessageInNewsFeed: ERROR can't post message due to missing messageID");
				}
			} // if(newsFeedMessageID.length() > 0)
			else
			{
				// --- Empty title, message and image
				ost.str("");
				ost << "{";
				ost << "\"result\": \"error\",";
				ost << "\"description\": \"can't update message due to messageID is not defined\"";
				ost << "}";

				CLog	log;
				log.Write(ERROR, "int main(void): action == AJAX_commentOnMessageInNewsFeed: ERROR can't update message due to messageID is not defined");
			}
		} // if(user.GetLogin() == "Guest")

		indexPage.RegisterVariableForce("result", ost.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == AJAX_commentOnMessageInNewsFeed: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("Template file was missing");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))

	}


	// --- AJAX change user password
	if(action == "AJAX_changeUserPassword")
	{
		ostringstream	ostResult;
		string			newPassword, cleanedPassword;

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == AJAX_changeUserPassword: re-login required");
			}

			ost.str("");
			ost << "{\"result\": \"error\", \"desription\": \"session lost. Need to relogin\"}";

			indexPage.RegisterVariableForce("result", ost.str());

			if(!indexPage.SetTemplate("json_response.htmlt"))
			{
				CLog	log;
				log.Write(ERROR, "int main(void): action == AJAX_changeUserPassword: ERROR can't find template json_response.htmlt");
				throw CExceptionHTML("user not activated");
			} // if(!indexPage.SetTemplate("json_response.htmlt"))
		}
		else
		{
			// --- "new" used due to possibility of drop "standard exception"
			char			*convertBuffer = new char[ 1024 * 1024];

			// --- This line will not be reached in case of error in memory allocation 
			// --- To avoid throw std exception use char *a = new(std::nothrow) char[ 0x7FFFFFFF ];
			if(!convertBuffer)
			{
				CLog	log;
				log.Write(ERROR, "int main(void): action == AJAX_changeUserPassword: ERROR can't allocate memory");
			}

			// --- Authorized user
			newPassword = indexPage.GetVarsHandler()->Get("password");

			// --- Convert from UTF-8 to cp1251
			memset(convertBuffer, 0, 1024*1024);
			convert_utf8_to_windows1251(newPassword.c_str(), convertBuffer, 1024*1024);
			newPassword = convertBuffer;
			cleanedPassword = newPassword;

			trim(cleanedPassword);

			delete[] convertBuffer;

			// --- Clean-up the text
			cleanedPassword = ReplaceDoubleQuoteToQuote(cleanedPassword);
			cleanedPassword = DeleteHTML(cleanedPassword);
			cleanedPassword = SymbolReplace(cleanedPassword, "\r\n", "<br>");
			cleanedPassword = SymbolReplace(cleanedPassword, "\r", "<br>");
			cleanedPassword = SymbolReplace(cleanedPassword, "\n", "<br>");

			if(cleanedPassword != newPassword)
			{
				CLog			log;
				ostringstream	ost;

				ost.str("");
				ost << "int main(void): action == AJAX_changeUserPassword: password having wrong symbols change the password to a new one [" << newPassword << "] <> [" << cleanedPassword << "]";
				log.Write(DEBUG, ost.str());

				ostResult.str("");
				ostResult << "{";
				ostResult << "\"result\": \"error\",";
				ostResult << "\"description\": \"ѕароль не должен содержать символов [(кавычки), (перевод строки), '<>] \"";
				ostResult << "}";
			}
			else
			{
				// --- newPassword not empty
				if(newPassword.length() > 0)
				{
					ostringstream	ost;

					ost.str("");
					ost << "select * from `users_passwd` where `userID`='" << user.GetID() << "' and `passwd`='" << newPassword << "';";
					if(db.Query(ost.str()))
					{

						CLog			log;
						ostringstream	ost;

						ost.str("");
						ost << "int main(void): action == AJAX_changeUserPassword: new password is the same as earlier";
						log.Write(DEBUG, ost.str());

						ostResult.str("");
						ostResult << "{";
						ostResult << "\"result\": \"error\",";
						ostResult << "\"description\": \"ѕароль не должен совпадать с одним из прошлых паролей\"";
						ostResult << "}";
					}
					else
					{
						// --- Change password
						ostringstream	ost;

						ost.str("");
						ost << "update `users_passwd` set `isActive`='false' where `userID`='" << user.GetID() << "';";
						db.Query(ost.str());

						ost.str("");
						ost << "insert into `users_passwd` (`userID`, `passwd`, `isActive`, `eventTimestamp`) VALUES \
								('" << user.GetID() << "', '" << newPassword << "', 'true', NOW());";
						db.Query(ost.str());

						ostResult.str("");
						ostResult << "{";
						ostResult << "\"result\": \"success\",";
						ostResult << "\"description\": \"\"";
						ostResult << "}";

						{
							CLog	log;
							log.Write(ERROR, "int main(void): action == AJAX_changeUserPassword: password has been changed successfully");
						}
					}
				} // if(newPassword.length() > 0)
				else
				{
					// --- Empty title, message and image
					ostResult.str("");
					ostResult << "{";
					ostResult << "\"result\": \"error\",";
					ostResult << "\"description\": \"can't change to empty password\"";
					ostResult << "}";

					CLog	log;
					log.Write(DEBUG, "int main(void): action == AJAX_changeUserPassword: can't change to empty password");
				}

			}


		} // if(user.GetLogin() == "Guest")

		indexPage.RegisterVariableForce("result", ostResult.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == AJAX_changeUserPassword: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("Template file was missing");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))

	}


	if(action == "ajax_regNewUser_checkUser") {
		ostringstream	ost;
		string		sessid;
		string		randomValue = GetRandom(4);
		string 		userToCheck;

		{
			CLog	log;
			log.Write(DEBUG, "action == regNewUser_checkUser: start");
		}

		userToCheck = indexPage.GetVarsHandler()->Get("regEmail"); 

		if(CheckUserEmailExisting(userToCheck, &db)) {
			{
				CLog	log;
				log.Write(DEBUG, "action == regNewUser_checkUser: login or email already registered");
			}
			indexPage.RegisterVariableForce("result", "already used");
		}
		else {
			{
				CLog	log;
				log.Write(DEBUG, "action == regNewUser_checkUser: login or email not yet exists");
			}
			indexPage.RegisterVariableForce("result", "free");
		}

		if(!indexPage.SetTemplate("ajax_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file ajax_response.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	if(action == "view_profile")
	{
		ostringstream	ost;
		string			sessid, friendID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == view_profile: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::action == view_profile re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		friendID = indexPage.GetVarsHandler()->Get("userid");
		if(friendID.length() && atol(friendID.c_str()))
		{
			if(friendID != user.GetID())
			{
				// --- update user watched page
				ost.str("");
				ost << "select `id` from `users_watched` where `watched_userID`=\"" << friendID << "\" and `watching_userID`=\"" << user.GetID() << "\";";
				if(db.Query(ost.str()))
				{
					string		profile_watched_id = db.Get(0, "id");

					ost.str("");
					ost << "update `users_watched` set `eventTimestamp`=TIMESTAMPDIFF(second, \"1970-01-01\", NOW()) where `id`='" << profile_watched_id << "';";
					db.Query(ost.str());
					if(db.isError())
					{
						CLog	log;

						log.Write(ERROR, "main: action == view_profile: ERROR updating table users_watched");
					}
				}
				else
				{
					ost.str("");
					ost << "insert `users_watched` (`watched_userID`, `watching_userID`, `eventTimestamp`) VALUE (\"" << friendID << "\", \"" << user.GetID() << "\", TIMESTAMPDIFF(second, \"1970-01-01\", NOW()));";
					if(!db.InsertQuery(ost.str()))
					{
						CLog	log;

						log.Write(ERROR, "main: action == view_profile: ERROR inserting into table users_watched");
					}
				}
			}

			// --- get user profile details
			ost.str("");
			ost << "select * from `users` where `id`='" << friendID << "' and `isactivated`='Y' and `isblocked`='N';";
			if(db.Query(ost.str()))
			{

				string		friendID = db.Get(0, "id");
				string		friendLogin = db.Get(0, "login");
				string		friendEmail = db.Get(0, "email");
				string		friendName = db.Get(0, "name");
				string		friendNameLast = db.Get(0, "nameLast");
				string		friendCV = db.Get(0, "cv");
				string		friendLastOnline = db.Get(0, "last_online");
				string		friendIP = db.Get(0, "ip");
				string		friendActivated = db.Get(0, "activated");
				string		friendFriendshipState = "empty";
				int			affected = 0;
				string		current_company = "0";

				friendCV = ReplaceCRtoHTML(friendCV); 
				indexPage.RegisterVariableForce("friendID", friendID);
				indexPage.RegisterVariableForce("friendLogin", friendLogin);
				indexPage.RegisterVariableForce("friendEmail", friendEmail);
				indexPage.RegisterVariableForce("friendName", friendName);
				indexPage.RegisterVariableForce("friendNameLast", friendNameLast);
				indexPage.RegisterVariableForce("friendCV", friendCV);
				indexPage.RegisterVariableForce("friendLastOnline", friendLastOnline);
				indexPage.RegisterVariableForce("friendIP", friendIP);
				indexPage.RegisterVariableForce("friendActivated", friendActivated);
				indexPage.RegisterVariableForce("friendActivatedDifferenceFromNow", GetHumanReadableTimeDifferenceFromNow(friendActivated));
				if(GetTimeDifferenceFromNow(friendLastOnline) < SESSION_LEN * 60)
				{
					indexPage.RegisterVariableForce("friendLastOnlineStatement", "—ейчас на сайте");
				}
				else
				{
					ostringstream		ost;

					ost.str("");
					ost << "ѕоследний раз заходил(а) на сайт " << GetHumanReadableTimeDifferenceFromNow(friendLastOnline);
					indexPage.RegisterVariableForce("friendLastOnlineStatement", ost.str());
				}



				ost.str("");
				ost << "select * from `users_avatars` where `userID`=\"" << friendID << "\" and `isActive`=\"1\";";
				if(db.Query(ost.str()))
				{
					ost.str("");
					ost << "/images/avatars/avatars" << db.Get(0, "folder") << "/" << db.Get(0, "filename");
					indexPage.RegisterVariableForce("friendAvatar", ost.str());
				}

				ost.str("");
				ost << "select * from `users_friends` where `userid`='" << user.GetID() << "' and `friendID`='" << friendID << "';";
				if(db.Query(ost.str()))
				{
					friendFriendshipState = db.Get(0, "state");
				}
				indexPage.RegisterVariableForce("friendFriendshipState", friendFriendshipState);


				ost.str("");
				ost << "SELECT `users_company`.`id` as `users_company_id`, `company`.`name` as `company_name`, `occupation_start`, `occupation_finish`, `current_company`, `responsibilities`, `users_company_position`.`title` as `title` \
						FROM  `company` ,  `users_company` ,  `users_company_position` \
						WHERE  `user_id` =  '" << friendID << "' \
						AND  `company`.`id` =  `users_company`.`company_id`  \
						AND  `users_company_position`.`id` =  `users_company`.`position_title_id`  \
						ORDER BY  `users_company`.`occupation_start` DESC ";
				affected = db.Query(ost.str());
				if(affected > 0) {
						ostringstream	ost1, ost2, ost3, ost4;
						string			occupationFinish;
						ost1.str("");
						ost2.str("");
						ost3.str("");
						ost4.str("");

						for(int i = 0; i < affected; i++, current_company = "0") {
							occupationFinish = db.Get(i, "occupation_finish");
							if(occupationFinish == "0000-00-00") {
								current_company = "1";
								ost2.str("");
								ost2 << indexPage.GetVarsHandler()->Get("currentCompany");
								if(ost2.str().length() > 1) ost2 << ", ";
								ost2 << db.Get(i, "company_name");
								indexPage.RegisterVariableForce("currentCompany", ost2.str());
							}

							ost1 << "<div class='row'>\n";
							ost1 << "<div class='col-md-4'>";
							ost1 << "<p" << (current_company == "1" ? " class=\"current_company\"" : "") << ">с ";
							ost1 << "<span data-id='" << db.Get(i, "users_company_id") << "' data-action='update_occupation_start' class='occupation_start datePick'>" << db.Get(i, "occupation_start") << "</span> по ";
							ost1 << "<span data-id='" << db.Get(i, "users_company_id") << "' data-action='update_occupation_finish' class='occupation_finish editableSpan'>" << (occupationFinish == "0000-00-00" ? "насто€щее врем€" : occupationFinish)  << "</span></p>";
							ost1 << "</div>\n";
							ost1 << "<div class='col-md-8'>";
							ost1 << "<p" << (current_company == "1" ? " class=\"current_company\" " : "") << "> \
							<span data-id='" << db.Get(i, "users_company_id") << "' data-action='updateJobTitle' class='jobTitle editableSpan'>"  << db.Get(i, "title") << "</span> в \
							<span data-id='" << db.Get(i, "users_company_id") << "' data-action='updateCompanyName' class='companyName editableSpan'>" << db.Get(i, "company_name") << "</span>";
							// ost1 << (current_company == "1" ? " (текущее место работы)" : "") << "</p>";
							ost1 << "</div>\n";
							ost1 << "</div> <!-- row -->\n\n";
							ost1 << "<div class='row'>\n";
							ost1 << "<div class='col-md-1'>";
							ost1 << "</div>\n";
							ost1 << "<div class='col-md-9'>";
							ost1 << "<p>"  << db.Get(i,"responsibilities") << "</p>";
							ost1 << "</div>\n";
							ost1 << "<div class='col-md-1'>";
							ost1 << "</div>\n";
							ost1 << "</div>\n\n";
						}
						indexPage.RegisterVariableForce("carrierPath", ost1.str());
				}
				else 
				{
					indexPage.RegisterVariableForce("carrierPath", "Ќет данных");
				}
			}
			else
			{
				CLog	log;

				log.Write(ERROR, "main: action == view_profile: ERROR user not found , not activated or blocked");
			}
		}
		else
		{
			CLog	log;
			log.Write(ERROR, "main: action == view_profile: ERROR userID is missing or equal zero");
		}


		ost.str("");
		ost << "select * from `users_avatars` where `userID`=\"" << user.GetID() << "\" and `isActive`=\"1\";";
		if(db.Query(ost.str()))
		{
			ost.str("");
			ost << "/images/avatars/avatars" << db.Get(0, "folder") << "/" << db.Get(0, "filename");
			indexPage.RegisterVariableForce("myUserAvatar", ost.str());
		}

		if(!indexPage.SetTemplate("view_profile.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file view_profile.htmlt was missing");
			throw CException("Template file was missing");
		}
	}

	// --- JSON get user notifications
	if(action == "AJAX_getUserNotification")
	{
		ostringstream	ost;
		string			strPageToGet, strNewsOnSinglePage, strFriendList;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_getUserNotification: start");
		}

		strNewsOnSinglePage	= indexPage.GetVarsHandler()->Get("NewsOnSinglePage");
		strPageToGet 		= indexPage.GetVarsHandler()->Get("page");
		if(strPageToGet.empty()) strPageToGet = "0";
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_getUserNotification: page ", strPageToGet, " requested");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == AJAX_getUserNotification: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}


		ost.str("");
		ost << "SELECT `users_notification`.`eventTimestamp` as `feed_eventTimestamp`, `users_notification`.`actionId` as `feed_actionId` , `users_notification`.`actionTypeId` as `feed_actionTypeId`, \
			`users_notification`.`id` as `users_notification_id`, `action_types`.`title` as `action_types_title`, \
			`users`.`id` as `user_id`, `users`.`name` as `user_name`, `users`.`nameLast` as `user_nameLast`, `users`.`email` as `user_email`, \
			`action_category`.`title` as `action_category_title`, `action_category`.`id` as `action_category_id` \
			FROM `users_notification` \
			INNER JOIN  `action_types` 		ON `users_notification`.`actionTypeId`=`action_types`.`id` \
			INNER JOIN  `action_category` 	ON `action_types`.`categoryID`=`action_category`.`id` \
			INNER JOIN  `users` 			ON `users_notification`.`userId`=`users`.`id` \
			WHERE `users_notification`.`userId`=\"" << user.GetID() << "\" AND `action_types`.`isShowNotification`='1' \
			ORDER BY  `users_notification`.`eventTimestamp` DESC LIMIT " << stoi(strPageToGet, nullptr, 10) * stoi(strNewsOnSinglePage, nullptr, 10) << " , " << stoi(strNewsOnSinglePage, nullptr, 10);

		indexPage.RegisterVariableForce("result", GetUserNotificationInJSONFormat(ost.str(), &db));

		ost.str("");
		ost << "update `users_notification` set `notificationStatus`=\"read\" where `userId`=\"" << user.GetID() << "\";";
		db.Query(ost.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == AJAX_getUserNotification: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("json_response.htmlt"))

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_getUserNotification: end");
		}
	}


	if(action == "user_notifications")
	{
		ostringstream	ost;
		string			sessid, activeUserID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == user_notifications: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(): action == user_notifications re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		if(!indexPage.SetTemplate("user_notifications.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file user_notifications.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == user_notifications: end");
		}
	}

	if(action == "chat")
	{
		ostringstream	ost;
		string			sessid, activeUserID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == chat: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main(): action == chat re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		if(!indexPage.SetTemplate("chat.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file chat.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == chat: end");
		}
	}

	if(action == "login")
	{
		ostringstream	ost;
		string		sessid;
		string		randomValue = GetRandom(4);
		string 		captchaFile = GenerateImage(randomValue);
		int 		affected;

		{
			CLog	log;
			log.Write(DEBUG, "main: action == login: start");
		}

		sessid = indexPage.GetCookie("sessid");

		if(sessid.length() < 5) {
			CLog	log;
			log.Write(ERROR, "main: action == login: error in session id [", sessid, "]");
			throw CException("Please enable cookie in browser.");
		}

		{
			CLog	log;
			log.Write(DEBUG, "main: action == login: get login captcha for session ", sessid);
		}


		ost.str("");
		ost << "select id from captcha where `session`=\"" << sessid << "\" and `purpose`='regNewUser'";

		if((affected = db.Query(ost.str())) > 0) {
			// ------ Update session
			{
				CLog	log;
				log.Write(DEBUG, "action == login: update captcha for session ", sessid);
			}

			ost.str("");
			ost << "update `captcha` set `code`='" << randomValue << "', `filename`='" << captchaFile << "', `timestamp`=NOW() where `session`=\"" << sessid << "\" and `purpose`='regNewUser'";
		}
		else {
			// ------ Create new session
			{
				CLog	log;
				log.Write(DEBUG, "action == login: create new session in captcha table", sessid);
			}


			ost.str("");
			ost << "INSERT INTO  `connme`.`captcha` (`session` ,`code` ,`filename` ,`purpose`, `timestamp`) VALUES ('" << sessid << "',  '" << randomValue << "',  '" << captchaFile << "',  'regNewUser', NOW());";
		}
		db.Query(ost.str());

		{
			CLog	log;
			log.Write(DEBUG, "action == login: register variables");
		}


		indexPage.RegisterVariableForce("title", "ƒобро пожаловать");
		indexPage.RegisterVariable("regEmail_checked", "0");


		indexPage.RegisterVariableForce("securityFile", captchaFile);


		if(!indexPage.SetTemplate("login.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file login.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "action == login: end");
		}
	}
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

		if(!indexPage.SetTemplate("logout.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "main: action == logout: ERROR: template file logoug.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "main: action == logout: end");
		}
	}

	if(action == "forget_password_page")
	{
		{
			CLog	log;
			log.Write(DEBUG, "action == forgot_password_page: start");
		}

		if(!indexPage.SetTemplate("forget_password.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "action == forgot_password_page: ERROR template file forgot_password_page.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "action == forgot_password_page: end");
		}

	}

		// --- AJAX_forgetpassword
	if(action == "AJAX_forgetPassword")
	{
		string		login, lng, sessid;
		CUser		user;
		ostringstream	ost1, ostResult;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_forgetpassword: start");
		}

		sessid = indexPage.GetCookie("sessid");
		if(sessid.length() < 5)
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_forgetpassword: with session id derived from cookies");

			ostResult.str("");
			ostResult << "{";
			ostResult << "\"result\": \"error\",";
			ostResult << "\"description\": \"session ID derived from cookie is wrong\",";
			ostResult << "\"type\": \"redirect\",";
			ostResult << "\"url\": \"/\"";
			ostResult << "}";
		}
		else 
		{
			login = indexPage.GetVarsHandler()->Get("email");
			lng = indexPage.GetLanguage();

			login = ReplaceDoubleQuoteToQuote(login);
			login = DeleteHTML(login);
			login = SymbolReplace(login, "\r\n", "<br>");
			login = SymbolReplace(login, "\r", "<br>");
			login = SymbolReplace(login, "\n", "<br>");

			user.SetDB(&db);
			if(!user.GetFromDBbyEmail(login)) 
			{
				CLog	log;
				log.Write(DEBUG, "main.action == AJAX_forgetpassword: user [", user.GetLogin(), "] not found");

				ostResult.str("");
				ostResult << "{";
				ostResult << "\"result\": \"error\",";
				ostResult << "\"description\": \"ѕользовател€ с таким e-mail не существует\"";
				ostResult << "}";
			}
			else 
			{

				if(!user.isActive()) 
				{
					CLog	log;
					log.Write(ERROR, "main.action == AJAX_forgetpassword: ERROR user [", user.GetLogin(), "] not activated");

					ostResult.str("");
					ostResult << "{";
					ostResult << "\"result\": \"error\",";
					ostResult << "\"description\": \"пользователь неактивирован, необходима активаци€\"";
					ostResult << "}";
				}
				else 
				{
					CMailLocal	mail;

					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main(void): action == AJAX_forgetpassword: sending mail message with password to user (" << user.GetLogin() << ")";
						log.Write(DEBUG, ost.str());
					}

					indexPage.RegisterVariableForce("login", user.GetLogin());
					indexPage.RegisterVariableForce("name", user.GetName());
					indexPage.RegisterVariableForce("nameLast", user.GetNameLast());
					indexPage.RegisterVariableForce("passwd", user.GetPasswd());
					indexPage.RegisterVariableForce("MAIL_FROM", getenv("SERVER_NAME"));
					mail.Send(user.GetEmail(), "forget", indexPage.GetVarsHandler(), &db);

					ost1.str("");
					ost1 << "/login?rand=" << GetRandom(10) << "&signinInputEmail=" << user.GetEmail();

					ostResult.str("");
					ostResult << "{";
					ostResult << "\"result\": \"success\",";
					ostResult << "\"description\": \"\",";
					ostResult << "\"url\": \"" << ost1.str() << "\",";
					ostResult << "\"email\": \"" << user.GetEmail() << "\"";
					ostResult << "}";

				}  // if(!user.isActive()) 
			}  // if(!user.isFound()) 
		} // if(sessid.length() < 5)

		indexPage.RegisterVariableForce("result", ostResult.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == AJAX_changeUserPassword: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("Template file was missing");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_forgetpassword: finish");
		}

	} // --- if(action == AJAX_forgetpassword)



	if(action == "login_user")
	{
		string		login, password, lng, sessid, rememberMe;
		CUser		user;
		ostringstream	ost1;

		sessid = indexPage.GetCookie("sessid");
		if(sessid.length() < 5)
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == login_user: with session id derived from cookies");

			if(!indexPage.SetTemplate("weberror_cookie_disabled.htmlt.htmlt"))
			{
				CLog	log;
				log.Write(ERROR, "int main(void): action == login_user: ERROR template weberror_cookie_disabled.htmlt can't be found");
				throw CExceptionHTML("cookies");
			}

		}
		else 
		{
			login = indexPage.GetVarsHandler()->Get("signinInputEmail");
			password = indexPage.GetVarsHandler()->Get("signinInputPassword");
			rememberMe = indexPage.GetVarsHandler()->Get("signinRemember");
			lng = indexPage.GetLanguage();

			user.SetDB(&db);
			if(!user.GetFromDBbyEmail(login)) 
			{
				CLog	log;
				log.Write(DEBUG, "main.action == login_user: user [", user.GetLogin(), "] not found");

				if(!indexPage.SetTemplate("weberror_user_not_found.htmlt"))
				{
					throw CExceptionHTML("template page missing");
				}

			}
			else 
			{

				if(!user.isActive()) 
				{
					CLog	log;
					log.Write(ERROR, "main.action == login_user: ERROR user [", user.GetLogin(), "] not activated");

					if(!indexPage.SetTemplate("weberror_user_not_activared.htmlt"))
					{
						throw CExceptionHTML("template page missing");
					}
				}
				else 
				{
					if((password != user.GetPasswd()) || (user.GetPasswd() == "")) 
					{
						CLog	log;
						log.Write(DEBUG, "int main(void): action == login_user: user [", user.GetLogin(), "] failed to login due to passwd error");

						if(!indexPage.SetTemplate("weberror_user_passwd_not_correct.htmlt"))
						{
							throw CExceptionHTML("template page missing");
						}
					}
					else 
					{

						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main(void): action == login_user: switching session (" << sessid << ") from Guest to user (" << user.GetLogin() << ")";
							log.Write(DEBUG, ost.str());
						}

// --- 2do check user login, session switching and session expiration

						// --- 2delete if login works till Nov 1
						// ost1.str("");
						// ost1 << "update `users` set `last_online`=NOW(), `ip`='" << getenv("REMOTE_ADDR") << "' where `login`='" << user.GetLogin() << "'";
						// db.Query(ost1.str());

						// --- place 2change user from user.email to user.id 
						ost1.str("");
						ost1 << "update `sessions` set `user`='" << user.GetEmail() << "', `ip`='" << getenv("REMOTE_ADDR") << "', `expire`=" << (rememberMe == "remember-me" ? 0 : SESSION_LEN * 60) << " where `id`='" << sessid << "'";
						db.Query(ost1.str());

						if(rememberMe == "remember-me") 
						{
							if(!indexPage.CookieUpdateTS("sessid", 0))
							{
								CLog	log;
								ostringstream	ost;

								ost.str("");
								ost << "int main(void): login_user: ERROR in setting up expiration sessid cookie to infinite";
								log.Write(ERROR, ost.str());
							}
						}

						indexPage.RegisterVariableForce("loginUser", user.GetLogin());
						indexPage.RegisterVariableForce("menu_main_active", "active");


						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main(void): login_user: redirection to \"news_feed?rand=xxxxxx\"";
							log.Write(DEBUG, ost.str());
						}
						ost1.str("");
						ost1 << "/news_feed?rand=" << GetRandom(10);
						indexPage.Redirect(ost1.str().c_str());

					} // if(password != user.GetPasswd())
				}  // if(!user.isActive()) 
			}  // if(!user.isFound()) 
		} // if(sessid.length() < 5)
	} // --- if(action == login_user)

	// --- AJAX_loginUser
	if(action == "AJAX_loginUser")
	{
		string		login, password, lng, sessid, rememberMe;
		CUser		user;
		ostringstream	ost1, ostResult;

		sessid = indexPage.GetCookie("sessid");
		if(sessid.length() < 5)
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == AJAX_loginUser: with session id derived from cookies");

			ostResult.str("");
			ostResult << "{";
			ostResult << "\"result\": \"error\",";
			ostResult << "\"description\": \"session ID derived from cookie is wrong\",";
			ostResult << "\"type\": \"redirect\",";
			ostResult << "\"url\": \"/\"";
			ostResult << "}";
		}
		else 
		{
			login = indexPage.GetVarsHandler()->Get("login");
			password = indexPage.GetVarsHandler()->Get("password");
			rememberMe = indexPage.GetVarsHandler()->Get("remember");
			lng = indexPage.GetLanguage();

			user.SetDB(&db);
			if(!user.GetFromDBbyEmail(login)) 
			{
				CLog	log;
				log.Write(DEBUG, "main.action == AJAX_loginUser: user [", user.GetLogin(), "] not found");

				ostResult.str("");
				ostResult << "{";
				ostResult << "\"result\": \"error\",";
				ostResult << "\"description\": \"ѕочта или ѕароль указаны не верно.\"";
				ostResult << "}";
			}
			else 
			{

				if(!user.isActive()) 
				{
					CLog	log;
					log.Write(ERROR, "main.action == AJAX_loginUser: ERROR user [", user.GetLogin(), "] not activated");

					ostResult.str("");
					ostResult << "{";
					ostResult << "\"result\": \"error\",";
					ostResult << "\"description\": \"пользователь неактивирован, необходима активаци€\"";
					ostResult << "}";
				}
				else 
				{
					if((password != user.GetPasswd()) || (user.GetPasswd() == "")) 
					{
						ostringstream		ost;

						ost.str("");
						ost << "select * from `users_passwd` where `userID`='" << user.GetID() << "' and `passwd`='" << password << "';";
						if(db.Query(ost.str()))
						{
							// --- earlier password is user for user login

							{
								CLog	log;
								log.Write(DEBUG, "int main(void): action == AJAX_loginUser: old password has been used for user [", user.GetLogin(), "] login");
							}

							ostResult.str("");
							ostResult << "{";
							ostResult << "\"result\": \"error\",";
							ostResult << "\"description\": \"этот пароль был изменен " << GetHumanReadableTimeDifferenceFromNow(db.Get(0, "eventTimestamp")) << "\"";
							ostResult << "}";
						}						
						else
						{
							// --- password is wrong for user

							{
								CLog	log;
								log.Write(DEBUG, "int main(void): action == AJAX_loginUser: user [", user.GetLogin(), "] failed to login due to passwd error");
							}

							ostResult.str("");
							ostResult << "{";
							ostResult << "\"result\": \"error\",";
							ostResult << "\"description\": \"логин или пароль указаны не верно\"";
							ostResult << "}";
						}

					}
					else 
					{

						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main(void): action == AJAX_loginUser: switching session (" << sessid << ") from Guest to user (" << user.GetLogin() << ")";
							log.Write(DEBUG, ost.str());
						}

						// --- 2delete if login works till Nov 1
						// ost1.str("");
						// ost1 << "update `users` set `last_online`=NOW(), `ip`='" << getenv("REMOTE_ADDR") << "' where `login`='" << user.GetLogin() << "'";
						// db.Query(ost1.str());

						ost1.str("");
						ost1 << "update `sessions` set `user`='" << user.GetEmail() << "', `ip`='" << getenv("REMOTE_ADDR") << "', `expire`=" << (rememberMe == "remember-me" ? 0 : SESSION_LEN * 60) << " where `id`='" << sessid << "'";
						db.Query(ost1.str());

						if(rememberMe == "remember-me") 
						{
							if(!indexPage.CookieUpdateTS("sessid", 0))
							{
								CLog	log;
								ostringstream	ost;

								ost.str("");
								ost << "int main(void): AJAX_loginUser: ERROR in setting up expiration sessid cookie to infinite";
								log.Write(ERROR, ost.str());
							}
						}

						indexPage.RegisterVariableForce("loginUser", user.GetLogin());
						indexPage.RegisterVariableForce("menu_main_active", "active");

						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main(void): AJAX_loginUser: redirection to \"news_feed?rand=xxxxxx\"";
							log.Write(DEBUG, ost.str());
						}
						ost1.str("");
						ost1 << "/news_feed?rand=" << GetRandom(10);

						ostResult.str("");
						ostResult << "{";
						ostResult << "\"result\": \"success\",";
						ostResult << "\"description\": \"\",";
						ostResult << "\"url\": \"" << ost1.str() << "\"";
						ostResult << "}";

					} // if(password != user.GetPasswd())
				}  // if(!user.isActive()) 
			}  // if(!user.isFound()) 
		} // if(sessid.length() < 5)

		indexPage.RegisterVariableForce("result", ostResult.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == AJAX_changeUserPassword: ERROR can't find template json_response.htmlt");
			throw CExceptionHTML("Template file was missing");
		} // if(!indexPage.SetTemplate("AJAX_getNewsFeed.htmlt"))

	} // --- if(action == AJAX_loginUser)


	if(action == "regNewUser")
	{
		string			actID, sessid;
		CActivator		act;
		string 			regSecurityCode, regPassword, regEmail;
		ostringstream	ost;
		int 			affected;

		{
			CLog	log;
			log.Write(DEBUG, "main: action == regNewUser: start");
		}

		sessid = indexPage.GetCookie("sessid");
		if(sessid.length() < 5) {
			CLog	log;
			log.Write(ERROR, "main: action == regNewUser: error in session id [", sessid, "]");
			throw CException("Please enable cookie in browser.");
		}

		regEmail = indexPage.GetVarsHandler()->Get("regEmail");
		regPassword = indexPage.GetVarsHandler()->Get("regPassword");
		regSecurityCode = indexPage.GetVarsHandler()->Get("regSecurityCode");

		if(regEmail.length() <= 3) {
			{
				CLog	log;
				log.Write(DEBUG, "main: action == regNewUser: email is incorrect [", regEmail, "]");
			}

			if(!indexPage.SetTemplate("weberror_email_incorrect.htmlt"))
			{
				CLog	log;

				log.Write(ERROR, "template file index.htmlt was missing");
				throw CException("Template file was missing");
			}
		}

		if(CheckUserEmailExisting(regEmail, &db)) {
			{
				CLog	log;
				log.Write(DEBUG, "main: action == regNewUser: login or email already registered");
			}

			if(!indexPage.SetTemplate("weberror_duplicate_user.htmlt"))
			{
				CLog	log;

				log.Write(ERROR, "template file index.htmlt was missing");
				throw CException("Template file was missing");
			}

		}
		else {
			{
				CLog	log;
				log.Write(DEBUG, "main: action == regNewUser: login or email not yet exists");
			}


			// ----- Check captcha and session coincidence
			ost.str("");
			ost << "select id from `captcha` where `purpose`='regNewUser' and `code`='" << regSecurityCode << "' and `session`='" << sessid << "' and `timestamp` > NOW() - INTERVAL " << SESSION_LEN << " MINUTE";
			if((affected = db.Query(ost.str())) == 0) {
				{
					CLog	log;
					log.Write(DEBUG, "main: action = regNewUser: check captcha fail");
				}

				indexPage.RegisterVariableForce("regInputEmail", indexPage.GetVarsHandler()->Get("regInputEmail"));


				if(!indexPage.SetTemplate("weberror_captcha.htmlt"))
				{
					CLog	log;

					log.Write(ERROR, "template file register_user_complete.htmlt was missing");
					throw CExceptionHTML("Template file was missing");
				} // if SetTemplate ("weberror_captcha.htmlt")
			}
			else {
				CActivator 	act;
				CMailLocal	mail;
				CUser		userTemporary;
				string		remoteIP;

				{
					CLog	log;
					log.Write(DEBUG, "main: action = regNewUser: check captcha success");
				}

				remoteIP = getenv("REMOTE_ADDR");

				ost.str("");
				ost << "delete from `captcha` where `purpose`='regNewUser' and `code`='" << regSecurityCode << "' and `session`='" << sessid << "'";
				affected = db.Query(ost.str());
				if(affected != 0) {
					CLog 			log;
					ostringstream 	ost;


					ost << ", [affected rows = " << affected << "]";
					log.Write(ERROR, "main: action == regNewUser: error in cleanup captcha table for type=regNewUser and captcha=", regSecurityCode , ost.str());
				}

				// --- Create temporarily user
				userTemporary.SetLogin(regEmail);
				userTemporary.SetEmail(regEmail);
				userTemporary.SetPasswd(regPassword);
				userTemporary.SetType("user");
				userTemporary.SetIP(getenv("REMOTE_ADDR"));
				userTemporary.SetLng(indexPage.GetLanguage());
				userTemporary.SetDB(&db);
				userTemporary.Create();


				// -----  Create activator for new user
				act.SetCgi(&indexPage);
				act.SetDB(&db);
				act.SetUser(indexPage.GetVarsHandler()->Get("regEmail"));
				act.SetType("regNewUser");
				act.Save();
				// act.Activate();
				
				indexPage.RegisterVariableForce("activator_regNewUser", act.GetID());
				mail.Send(regEmail, "activator_regNewUser", indexPage.GetVarsHandler(), &db);
				
		
				if(!indexPage.SetTemplate("activator_regNewUser.htmlt"))
				{
					CLog	log;

					log.Write(ERROR, "int main(void): action == regNewUser: template file index.htmlt was missing");
					throw CException("Template file was missing");
				} // if(!indexPage.SetTemplate("activator_regNewUser.htmlt"))

			} // if captcha correct

		} // if(CheckUserEmailExisting(regEmail))

		{
			CLog	log;
			log.Write(DEBUG, "main: action == regNewUser: end");
		}
	}

	if(action == "activateNewUser") 
	{
		ostringstream 	ost;
		CActivator 		act;
		string			activatorID;

		{
			CLog	log;
			log.Write(DEBUG, "main: action = activateNewUser: start");
		}

		activatorID = "";
		activatorID = indexPage.GetVarsHandler()->Get("activator");

		act.SetCgi(&indexPage);
		act.SetDB(&db);
		if(!act.Load(activatorID)) 
		{
			{
				CLog	log;
				log.Write(DEBUG, "main: action = activateNewUser: failed to Load activator [", activatorID, "]");
			}

			if(!indexPage.SetTemplate("weberror_activator_notfound.htmlt"))
			{
				CLog	log;

				log.Write(ERROR, "template file weberror_activator_notfound.htmlt was missing");
				throw CExceptionHTML("Template file was missing");
			}

		}
		else 
		{
			// --- account activated
			act.Activate();

			// --- improve the user expirience by automatically sign-in user
			// --- automatic sing-in
			string		sessid, login, rememberMe, lng;
			CUser		user;

			sessid = indexPage.GetCookie("sessid");
			if(sessid.length() < 5)
			{
				CLog	log;
				log.Write(DEBUG, "int main(void): action == activateNewUser: with session id derived from cookies");

				if(!indexPage.SetTemplate("weberror_cookie_disabled.htmlt.htmlt"))
				{
					CLog	log;
					log.Write(ERROR, "int main(void): action == activateNewUser: ERROR template weberror_cookie_disabled.htmlt can't be found");
					throw CExceptionHTML("cookies");
				}

			} // --- if(sessid.length() < 5)
			else 
			{
				login = act.GetUser();
				rememberMe = "remember-me";
				lng = indexPage.GetLanguage();

				user.SetDB(&db);
				if(!user.GetFromDBbyEmail(login)) 
				{
					CLog	log;
					log.Write(DEBUG, "main.action == activateNewUser: user [", user.GetLogin(), "] not found");

					if(!indexPage.SetTemplate("weberror_user_not_found.htmlt"))
					{
						throw CExceptionHTML("template page missing");
					}

				}
				else 
				{

					if(!user.isActive()) 
					{
						CLog	log;
						log.Write(ERROR, "main.action == activateNewUser: ERROR user [", user.GetLogin(), "] not activated");

						if(!indexPage.SetTemplate("weberror_user_not_activared.htmlt"))
						{
							throw CExceptionHTML("template page missing");
						}
					}
					else 
					{
						ostringstream	ost1;

						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main(void): action == activateNewUser: switching session (" << sessid << ") from Guest to user (" << user.GetLogin() << ")";
							log.Write(DEBUG, ost.str());
						}

						// --- 2delete if login works till Nov 1
						// ost1.str("");
						// ost1 << "update `users` set `last_online`=NOW(), `ip`='" << getenv("REMOTE_ADDR") << "' where `login`='" << user.GetLogin() << "';";
						// db.Query(ost1.str());

						ost1.str("");
						ost1 << "update `sessions` set `user`='" << login << "', `ip`='" << getenv("REMOTE_ADDR") << "', `expire`=" << (rememberMe == "remember-me" ? 0 : SESSION_LEN * 60) << " where `id`='" << sessid << "';";
						db.Query(ost1.str());

						if(rememberMe == "remember-me") 
						{
							if(!indexPage.CookieUpdateTS("sessid", 0))
							{
								CLog	log;
								ostringstream	ost;

								ost.str("");
								ost << "int main(void): activateNewUser: ERROR in setting up expiration sessid cookie to infinite";
								log.Write(ERROR, ost.str());
							}
						}

						indexPage.RegisterVariableForce("loginUser", user.GetLogin());
						indexPage.RegisterVariableForce("menu_main_active", "active");


						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main(void): activateNewUser: redirection to \"news_feed?rand=xxxxxx\"";
							log.Write(DEBUG, ost.str());
						}
						ost1.str("");
						ost1 << "/news_feed?rand=" << GetRandom(10);
						indexPage.Redirect(ost1.str().c_str());

					}  // if(!user.isActive()) 
				}  // if(!user.isFound()) 
			} // if(sessid.length() < 5)
		} // if(!act.Load(activatorID))


		{
			CLog	log;
			log.Write(DEBUG, "main: action = activateNewUser: end");
		}
	}

	// --- Account properties
	if(action == "user_account_properties")
	{
		ostringstream	ost;
		int		affected;
		string		userID, name, nameLast, age, cv, pass, address, phone, email, isBlocked, avatarFileName, avatarFolderName, current_company;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == user_account_properties: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::user_account_properties: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		ost.str("");
		ost << "SELECT \
					`users`.`id` 				as `users_id`, \
					`users`.`name` 				as `users_name`, \
					`users`.`nameLast`			as `users_nameLast`, \
					`users`.`cv` 				as `users_cv`, \
					`users_passwd`.`passwd` 	as `users_passwd_passwd`, \
					`users`.`address`			as `users_address`, \
					`users`.`phone`				as `users_phone`, \
					`users`.`email`				as `users_email`, \
					`users`.`isblocked`			as `users_isblocked` \
				FROM `users` \
				INNER JOIN `users_passwd` ON `users_passwd`.`userID`=`users`.`id` \
				WHERE `users`.`id`='" << user.GetID() << "' AND `users_passwd`.`isActive`='true';";
		affected = db.Query(ost.str());
		if(affected)
		{
			indexPage.RegisterVariableForce("menu_profile_active", "active");


			userID = db.Get(0, "users_id");
			name = db.Get(0, "users_name"); 		indexPage.RegisterVariableForce("name", name);
									  				indexPage.RegisterVariableForce("myFirstName", name);
			nameLast = db.Get(0, "users_nameLast"); indexPage.RegisterVariableForce("nameLast", nameLast);
													indexPage.RegisterVariableForce("myLastName", nameLast);
			cv = db.Get(0, "users_cv");				indexPage.RegisterVariableForce("cv", cv);
			pass = db.Get(0, "users_passwd_passwd");indexPage.RegisterVariableForce("pass", pass);
			address = db.Get(0, "users_address");	indexPage.RegisterVariableForce("address", address);
			phone = db.Get(0, "users_phone");		indexPage.RegisterVariableForce("phone", phone);
			email = db.Get(0, "users_email"); 		indexPage.RegisterVariableForce("email", email);
			isBlocked = db.Get(0, "users_isblocked");  

			if(isBlocked == "Y")
			{
				indexPage.RegisterVariableForce("isblocked", "<button type=\"button\" class=\"btn btn-danger user-account-properties-visible\" id=\"ButtonAccountEnable1\">јккаунт заблокирован</button> <button type=\"button\" class=\"btn btn-success user-account-properties-hidden\" id=\"ButtonAccountBlock1\">јккаунт активен</button>");
			}
			else
			{
				indexPage.RegisterVariableForce("isblocked", "<button type=\"button\" class=\"btn btn-danger user-account-properties-hidden\" id=\"ButtonAccountEnable1\">јккаунт заблокирован</button> <button type=\"button\" class=\"btn btn-success user-account-properties-visible\" id=\"ButtonAccountBlock1\">јккаунт активен</button>");
			}

			{ 
				CLog	log; 
				log.Write(DEBUG, "int main(void): user_account_properties: user details isBlocked:[", isBlocked, "]"); 
			}
		}

		if(!indexPage.SetTemplate("user_account_properties.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file user_account_properties.htmlt was missing");
			throw CExceptionHTML("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == user_account_properties: end");
		}
	}

/*
	if(action == "registration_user")
	{
		string		login, passwd, passwdConfirm, email, name, nameLast, age, ismale, club, description, lng, ip, agreement, address, type, partnerID, phone1, phone2, phone;
		ostringstream	ost;

		login = indexPage.GetVarsHandler()->Get("login");
		passwd = indexPage.GetVarsHandler()->Get("passwd");
		passwdConfirm = indexPage.GetVarsHandler()->Get("passwdconfirm");
		email = indexPage.GetVarsHandler()->Get("email");
		name = indexPage.GetVarsHandler()->Get("name");
		nameLast = indexPage.GetVarsHandler()->Get("nameLast");
		address = indexPage.GetVarsHandler()->Get("address");
		description = indexPage.GetVarsHandler()->Get("description");
		lng = indexPage.GetVarsHandler()->Get("lng");
		ip = getenv("REMOTE_ADDR");
		agreement = indexPage.GetVarsHandler()->Get("agreement");
		phone1 = indexPage.GetVarsHandler()->Get("phone1");
		phone2 = indexPage.GetVarsHandler()->Get("phone2");
		phone = phone1 + phone2;

		CUser	user(RemoveQuotas(login), RemoveQuotas(passwd), RemoveQuotas(passwdConfirm), RemoveQuotas(email), RemoveQuotas(lng), RemoveQuotas(ip), RemoveQuotas(agreement), RemoveQuotas(type), RemoveQuotas(partnerID), RemoveQuotas(phone));

		user.SetDB(&db);
		user.SetVars(indexPage.GetVarsHandler());

		if(user.isLoginExist()) throw CExceptionHTML("login exist");
    		if(login.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") != string::npos) throw CExceptionHTML("login error");
		if(!user.isEmailCorrect()) throw CExceptionHTML("email error");
		if(user.isEmailDuplicate()) throw CExceptionHTML("email duplicate");
		//if(!user.isPhoneCorrect()) throw CExceptionHTML("phone error");
		//if(user.isPhoneDuplicate()) throw CExceptionHTML("phone duplicate");
		if(!user.isAgree()) throw CExceptionHTML("agreement error");
		if(agreement.length() == 0) throw CExceptionHTML("agreement error");
		//if(!user.isTypeCorrect()) user.SetType("user");
		//if(name.length() == 0) throw CExceptionHTML("invalid user name");
		//if(nameLast.length() == 0) throw CExceptionHTML("invalid user nameLast");
		if(user.isPasswdError()) throw CExceptionHTML("passwd error");

		user.Create();

		ost.str("");
		ost << "update `users` set `name`=\"" << name << "\", `nameLast`=\"" << nameLast << "\", `address`=\"" << address << "\", `description`=\"" << description << "\" where `login`=\"" << user.GetLogin() << "\"";
		db.Query(ost.str());

		if(!indexPage.SetTemplate("register_user_complete.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file register_user_complete.htmlt was missing");
			throw CException("Template file was missing");
		}
		indexPage.RegisterVariableForce("login", user.GetLogin());
		indexPage.RegisterVariableForce("email", user.GetEmail());

		{
		    CMailLocal	mail;
		    mail.Send("admin", "new_user", indexPage.GetVarsHandler(), &db);
		}
	}
*/



	if(action == "edit_profile")
	{
		ostringstream	ost;
		int		affected;
		string		userID, name, nameLast, age, cv, pass, address, phone, email, isBlocked, avatarFileName, avatarFolderName, current_company;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == edit_profile: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::edit_profile: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}


		ost.str("");
		ost << "SELECT \
					`users`.`id` 				as `users_id`, \
					`users`.`name` 				as `users_name`, \
					`users`.`nameLast`			as `users_nameLast`, \
					`users`.`cv` 				as `users_cv`, \
					`users_passwd`.`passwd` 	as `users_passwd_passwd`, \
					`users`.`address`			as `users_address`, \
					`users`.`phone`				as `users_phone`, \
					`users`.`email`				as `users_email`, \
					`users`.`isblocked`			as `users_isblocked` \
				FROM `users` \
				INNER JOIN `users_passwd` ON `users_passwd`.`userID`=`users`.`id` \
				WHERE `users`.`id`='" << user.GetID() << "' AND `users_passwd`.`isActive`='true';";
		affected = db.Query(ost.str());
		if(affected)
		{
			indexPage.RegisterVariableForce("menu_profile_active", "active");


			userID = db.Get(0, "users_id");
			name = db.Get(0, "users_name"); 		indexPage.RegisterVariableForce("name", name);
									  				indexPage.RegisterVariableForce("myFirstName", name);
			nameLast = db.Get(0, "users_nameLast"); indexPage.RegisterVariableForce("nameLast", nameLast);
													indexPage.RegisterVariableForce("myLastName", nameLast);
			cv = db.Get(0, "users_cv");				
			if(cv == "") cv = "Ќапишите несколько слов о себе";
													indexPage.RegisterVariableForce("cv", cv);
			pass = db.Get(0, "users_passwd_passwd");indexPage.RegisterVariableForce("pass", pass);
			address = db.Get(0, "users_address");	indexPage.RegisterVariableForce("address", address);
			phone = db.Get(0, "users_phone");		indexPage.RegisterVariableForce("phone", phone);
			email = db.Get(0, "users_email"); 		indexPage.RegisterVariableForce("email", email);
			isBlocked = db.Get(0, "users_isblocked");  

			{ 
				CLog	log; 
				log.Write(DEBUG, "int main(void): edit_profile: user details isBlocked:[", isBlocked, "]"); 
			}

			ost.str("");
			ost << "SELECT `users_company`.`id` as `users_company_id`, `company`.`name` as `company_name`, `occupation_start`, `occupation_finish`, `current_company`, `responsibilities`, `users_company_position`.`title` as `title` \
	FROM  `company` ,  `users_company` ,  `users_company_position` \
	WHERE  `user_id` =  '" << userID << "' \
	AND  `company`.`id` =  `users_company`.`company_id`  \
	AND  `users_company_position`.`id` =  `users_company`.`position_title_id`  \
	ORDER BY  `users_company`.`occupation_start` DESC ";
			affected = db.Query(ost.str());
			if(affected > 0) {
					ostringstream	ost1, ost2, ost3, ost4;
					string			occupationFinish;
					ost1.str("");
					ost2.str("");
					ost3.str("");
					ost4.str("");

					for(int i = 0; i < affected; i++, current_company = "0") {
						occupationFinish = db.Get(i, "occupation_finish");
						if(occupationFinish == "0000-00-00") {
							current_company = "1";
							ost2.str("");
							ost2 << indexPage.GetVarsHandler()->Get("currentCompany");
							if(ost2.str().length() > 1) ost2 << ", ";
							ost2 << db.Get(i, "company_name");
							indexPage.RegisterVariableForce("currentCompany", ost2.str());
						}

						ost1 << "<div class='row'>\n";
						ost1 << "<div class='col-xs-4'>";
						ost1 << "<p" << (current_company == "1" ? " class=\"current_company\"" : "") << ">с ";
						ost1 << "<span data-id='" << db.Get(i, "users_company_id") << "' data-action='update_occupation_start' class='occupation_start datePick'>" << db.Get(i, "occupation_start") << "</span> по ";
						ost1 << "<span data-id='" << db.Get(i, "users_company_id") << "' data-action='update_occupation_finish' class='occupation_finish editableSpan'>" << (occupationFinish == "0000-00-00" ? "насто€щее врем€" : occupationFinish)  << "</span></p>";
						ost1 << "</div>\n";
						ost1 << "<div class='col-xs-6'>";
						ost1 << "<p" << (current_company == "1" ? " class=\"current_company\" " : "") << "> \
						<span data-id='" << db.Get(i, "users_company_id") << "' data-action='updateJobTitle' class='jobTitle editableSpan'>"  << db.Get(i, "title") << "</span> в \
						<span data-id='" << db.Get(i, "users_company_id") << "' data-action='updateCompanyName' class='companyName editableSpan'>" << db.Get(i, "company_name") << "</span>";
						// ost1 << (current_company == "1" ? " (текущее место работы)" : "") << "</p>";
						ost1 << "</div>\n";
						ost1 << "<div class='col-xs-1'>";
						ost1 << "<span class=\"glyphicon glyphicon-remove animateClass removeCompanyExperience\" aria-hidden=\"true\" data-id='" << db.Get(i, "users_company_id") << "' data-action='AJAX_removeCompanyExperience'></span>";
						ost1 << "</div>\n";
						ost1 << "</div> <!-- row -->\n\n";
						ost1 << "<div class='row'>\n";
						ost1 << "<div class='col-xs-1'>";
						ost1 << "</div>\n";
						ost1 << "<div class='col-xs-9'>";
						ost1 << "<p id=\"responsibilities" << db.Get(i, "users_company_id") << "\" class=\"editableParagraph\" data-action=\"update_responsibilities\" data-id=\"" << db.Get(i, "users_company_id") << "\">";
						if(strlen(db.Get(i,"responsibilities")))
							ost1 << db.Get(i,"responsibilities");
						else
							ost1 << "ќпишите круг своих об€занностей работы в компании";
						ost1 << "</p>";
						ost1 << "</div>\n";
						ost1 << "<div class='col-xs-1'>";
						ost1 << "</div>\n";
						ost1 << "</div>\n\n";
					}
					indexPage.RegisterVariableForce("carrierPath", ost1.str());
			}
			else 
			{
				indexPage.RegisterVariableForce("carrierPath", "¬ы не заполнили опыт работы");
			}
		}
		else
		{
			CLog	log;

			log.Write(DEBUG, "main: action == edit_profile: ERROR: there is no user in DB [", indexPage.GetVarsHandler()->Get("loginUser"), "]");
			CExceptionHTML("no user");
		}

		indexPage.RegisterVariableForce("title", "ћо€ страница");

		if(!indexPage.SetTemplate("edit_profile.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file edit_profile.htmlt was missing");
			throw CException("Template file edit_profile.htmlt was missing");
		}  // if(!indexPage.SetTemplate("edit_profile.htmlt"))
		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == edit_profile: end");
		}
	} 	// if(action == "edit_profile")

	if(action == "edit_company")
	{
		ostringstream	ost;
		string		userID, name, nameLast, age, cv, pass, address, phone, email, isBlocked, avatarFileName, avatarFolderName, current_company;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == edit_company: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::edit_company: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		indexPage.RegisterVariableForce("title", "–едактирование данных компании");

		if(!indexPage.SetTemplate("edit_company.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file edit_company.htmlt was missing");
			throw CException("Template file edit_company.htmlt was missing");
		}  // if(!indexPage.SetTemplate("edit_company.htmlt"))

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == edit_company: end");
		}
	} 	// if(action == "edit_company")

	if(action == "JSON_getUserProfile")
	{
		ostringstream	ost, ostResult;
		int				affected;
		string			userID, name, nameLast, age, sex, cv, pass, address, phone, email, isBlocked, avatarFileName, avatarFolderName, current_company;
		char			convertBuffer[1024];


		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::JSON_getUserProfile: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		userID = indexPage.GetVarsHandler()->Get("id");

		memset(convertBuffer, 0, sizeof(convertBuffer));
		convert_utf8_to_windows1251(userID.c_str(), convertBuffer, sizeof(convertBuffer));
		userID = ConvertTextToHTML(convertBuffer);

		if(!userID.length())
		{
			ostringstream		ost;

			ost.str("");
			ost << user.GetID();
			userID = ost.str();
		}

		ostResult.str("");
		ost.str("");
		ost << "SELECT \
					`users`.`id` 				as `users_id`, \
					`users`.`name` 				as `users_name`, \
					`users`.`nameLast`			as `users_nameLast`, \
					`users`.`cv` 				as `users_cv`, \
					`users_passwd`.`passwd` 	as `users_passwd_passwd`, \
					`users`.`address`			as `users_address`, \
					`users`.`phone`				as `users_phone`, \
					`users`.`email`				as `users_email`, \
					`users`.`sex`				as `users_sex`, \
					`users`.`isblocked`			as `users_isblocked` \
				FROM `users` \
				INNER JOIN `users_passwd` ON `users_passwd`.`userID`=`users`.`id` \
				WHERE `users`.`id`='" << userID << "' AND `users_passwd`.`isActive`='true';";
		affected = db.Query(ost.str());
		if(affected)
		{
			map<string, string>		skillMap;

			name = db.Get(0, "users_name");
			nameLast = db.Get(0, "users_nameLast");
			cv = db.Get(0, "users_cv");				
			if(cv == "") cv = "Ќапишите несколько слов о себе";
			pass = db.Get(0, "users_passwd_passwd");
			address = db.Get(0, "users_address");
			phone = db.Get(0, "users_phone");
			email = db.Get(0, "users_email");
			sex = db.Get(0, "users_sex");
			isBlocked = db.Get(0, "users_isblocked");  

			ostResult << "{" 
				<< "\"result\": \"success\","
				<< "\"userID\": \"" << userID << "\","
				<< "\"name\": \"" << name << "\","
				<< "\"nameLast\": \"" << nameLast << "\","
				<< "\"cv\": \"" << cv << "\","
				<< "\"address\": \"" << address << "\","
				<< "\"phone\": \"" << phone << "\","
				<< "\"isBlocked\": \"" << isBlocked << "\","
				<< "\"sex\": \"" << sex << "\","

				<< "\"school\": [";
			ost.str("");
			ost << "\
SELECT `users_school`.`id` as 'users_school_id', `school`.`title` as 'school_title', `geo_locality`.`title` as 'school_locality',  `users_school`.`occupation_start` as 'users_school_occupation_start', `users_school`.`occupation_finish` as 'users_school_occupation_finish' \
FROM `users_school` \
RIGHT JOIN `school` ON `users_school`.`school_id`=`school`.`id` \
RIGHT JOIN `geo_locality` ON `school`.`geo_locality_id`=`geo_locality`.`id` \
where `users_school`.`user_id`=\"" << userID << "\";";
			affected = db.Query(ost.str());
			if(affected)
			{
					for(int i = 0; i < affected; i++) 
					{
						ostResult << (i ? "," : "")
						<< "{"
						<< "\"schoolID\":\"" << db.Get(i, "users_school_id") << "\","
						<< "\"schoolTitle\":\"" << db.Get(i, "school_title") << "\","
						<< "\"schoolLocality\":\"" << db.Get(i, "school_locality") << "\","
						<< "\"schoolOccupationStart\":\"" << db.Get(i, "users_school_occupation_start") << "\","
						<< "\"schoolOccupationFinish\":\"" << db.Get(i, "users_school_occupation_finish") << "\""
						<< "}";
					}
			}
			else 
			{
				CLog	log;
				log.Write(DEBUG, "main: action == JSON_getUserProfile: DEBUG: school path is empty");
			}
			ostResult << "],";

			ostResult << "\"language\": [";
			ost.str("");
			ost << "\
SELECT `users_language`.`id` as 'users_language_id', `language`.`title` as 'language_title', `users_language`.`level` as 'language_level' \
FROM `users_language` \
RIGHT JOIN `language` ON `users_language`.`language_id`=`language`.`id` \
where `users_language`.`user_id`=\"" << userID << "\";";
			affected = db.Query(ost.str());
			if(affected)
			{
					for(int i = 0; i < affected; i++) 
					{
						ostResult << (i ? "," : "")
						<< "{"
						<< "\"languageID\":\"" << db.Get(i, "users_language_id") << "\","
						<< "\"languageTitle\":\"" << db.Get(i, "language_title") << "\","
						<< "\"languageLevel\":\"" << db.Get(i, "language_level") << "\""
						<< "}";
					}
			}
			else 
			{
				CLog	log;
				log.Write(DEBUG, "main: action == JSON_getUserProfile: DEBUG: language path is empty");
			}
			ostResult << "],";

			skillMap.clear();
			ostResult << "\"skill\": [";
			ost.str("");
			ost << "\
SELECT `users_skill`.`id` as 'users_skill_id', `skill`.`title` as 'skill_title' \
FROM `users_skill` \
RIGHT JOIN `skill` ON `users_skill`.`skill_id`=`skill`.`id` \
where `users_skill`.`user_id`=\"" << userID << "\";";
			affected = db.Query(ost.str());
			if(affected)
			{
					for(int i = 0; i < affected; i++) 
					{
						skillMap[db.Get(i, "users_skill_id")] = db.Get(i, "skill_title");
					}
					for(auto it = skillMap.begin(); it != skillMap.end(); ++it) 
					{
						ostResult << ((it != skillMap.begin()) ? "," : "");
						ostResult << "{";
						ostResult << "\"skillID\":\"" << it->first << "\",";
						ostResult << "\"skillTitle\":\"" << it->second << "\",";

						ostResult << "\"skillConfirmed\":[";
						ost.str("");
						ost << "select * from `skill_confirmed` where `users_skill_id`=\"" << it->first << "\";";
						if(int affected1 = db.Query(ost.str()))
						{
							for(int i = 0; i < affected1; ++i)
							{
								ostResult << (i ? "," : "");
								ostResult << db.Get(i, "approver_userID");
							}
						}
						ostResult << "]";

						ostResult << "}";
					}
			}
			else 
			{
				CLog	log;
				log.Write(DEBUG, "main: action == JSON_getUserProfile: DEBUG: language path is empty");
			}
			ostResult << "],";

			ostResult << "\"university\": [";
			ost.str("");
			ost << "\
SELECT `users_university`.`id` as 'users_university_id', `university`.`title` as 'university_title', `users_university`.`degree` as 'university_degree', `geo_region`.`title` as 'university_region', `users_university`.`occupation_start` as 'users_university_occupation_start', `users_university`.`occupation_finish` as 'users_university_occupation_finish' \
FROM `users_university` \
RIGHT JOIN `university` ON `users_university`.`university_id`=`university`.`id` \
RIGHT JOIN `geo_region` ON `university`.`geo_region_id`=`geo_region`.`id` \
where `users_university`.`user_id`=\"" << userID << "\";";
			affected = db.Query(ost.str());
			if(affected)
			{
					for(int i = 0; i < affected; i++) 
					{
						ostResult << (i ? "," : "")
						<< "{"
						<< "\"universityID\":\"" << db.Get(i, "users_university_id") << "\","
						<< "\"universityTitle\":\"" << db.Get(i, "university_title") << "\","
						<< "\"universityRegion\":\"" << db.Get(i, "university_region") << "\","
						<< "\"universityDegree\":\"" << db.Get(i, "university_degree") << "\","
						<< "\"universityOccupationStart\":\"" << db.Get(i, "users_university_occupation_start") << "\","
						<< "\"universityOccupationFinish\":\"" << db.Get(i, "users_university_occupation_finish") << "\""
						<< "}";
					}
			}
			else 
			{
				CLog	log;
				log.Write(DEBUG, "main: action == JSON_getUserProfile: DEBUG: university path is empty");
			}
			ostResult << "],";

			ostResult << "\"certifications\": [";
			ost.str("");
			ost << "\
SELECT `users_certifications`.`id` as 'users_certifications_id', `company`.`name` as 'certification_vendors_title', `certification_tracks`.`title` as 'certification_tracks_title', `users_certifications`.`certification_number` as 'users_certifications_certification_number' \
FROM `users_certifications` \
RIGHT JOIN `company` ON `users_certifications`.`vendor_id`=`company`.`id` \
RIGHT JOIN `certification_tracks` on `users_certifications`.`track_id`=`certification_tracks`.`id` \
where `users_certifications`.`user_id`=\"" << userID << "\";";
			affected = db.Query(ost.str());
			if(affected)
			{
					for(int i = 0; i < affected; i++, current_company = "0") 
					{
						ostResult << (i ? "," : "")
						<< "{"
						<< "\"certificationID\":\"" << db.Get(i, "users_certifications_id") << "\","
						<< "\"certificationVendor\":\"" << db.Get(i, "certification_vendors_title") << "\","
						<< "\"certificationTrack\":\"" << db.Get(i, "certification_tracks_title") << "\","
						<< "\"certificationNumber\":\"" << db.Get(i, "users_certifications_certification_number") << "\""
						<< "}";
					}
			}
			else 
			{
				CLog	log;
				log.Write(DEBUG, "main: action == JSON_getUserProfile: DEBUG: certification path is empty");
			}
			ostResult << "],";

			ostResult << "\"courses\": [";
			ost.str("");
			ost << "\
SELECT `users_courses`.`id` as 'users_courses_id', `company`.`name` as 'course_vendors_title', `certification_tracks`.`title` as 'course_tracks_title' \
FROM `users_courses` \
RIGHT JOIN `company` ON `users_courses`.`vendor_id`=`company`.`id` \
RIGHT JOIN `certification_tracks` on `users_courses`.`track_id`=`certification_tracks`.`id` \
where `users_courses`.`user_id`=\"" << userID << "\";";
			affected = db.Query(ost.str());
			if(affected)
			{
					for(int i = 0; i < affected; i++, current_company = "0") 
					{
						ostResult << (i ? "," : "")
						<< "{"
						<< "\"courseID\":\"" << db.Get(i, "users_courses_id") << "\","
						<< "\"courseVendor\":\"" << db.Get(i, "course_vendors_title") << "\","
						<< "\"courseTrack\":\"" << db.Get(i, "course_tracks_title") << "\""
						<< "}";
					}
			}
			else 
			{
				CLog	log;
				log.Write(DEBUG, "main: action == JSON_getUserProfile: DEBUG: course path is empty");
			}
			ostResult << "],";

			ostResult << "\"recommendation\": [";
			ost.str("");
			ost << "SELECT * FROM `users_recommendation` where `recommended_userID`=\"" << userID << "\";";
			affected = db.Query(ost.str());
			if(affected)
			{
					for(int i = 0; i < affected; i++) 
					{
						ostResult << (i ? "," : "")
						<< "{"
						<< "\"recommendationID\":\"" << db.Get(i, "id") << "\","
						<< "\"recommendationTitle\":\"" << db.Get(i, "title") << "\","
						<< "\"recommendationRecommendedUserID\":\"" << db.Get(i, "recommended_userID") << "\","
						<< "\"recommendationRecommendingUserID\":\"" << db.Get(i, "recommending_userID") << "\","
						<< "\"recommendationTimestamp\":\"" << db.Get(i, "eventTimestamp") << "\","
						<< "\"recommendationState\":\"" << db.Get(i, "state") << "\""
						<< "}";
					}
			}
			else 
			{
				CLog	log;
				log.Write(DEBUG, "main: action == JSON_getUserProfile: DEBUG: recommendation path is empty");
			}
			ostResult << "],";

			ostResult << "\"companies\": [";
			ost.str("");
			ost << "SELECT `users_company`.`id` as `users_company_id`, `company`.`name` as `company_name`, `occupation_start`, `occupation_finish`, `current_company`, `responsibilities`, `users_company_position`.`title` as `title` \
	FROM  `company` ,  `users_company` ,  `users_company_position` \
	WHERE  `user_id` =  '" << userID << "' \
	AND  `company`.`id` =  `users_company`.`company_id`  \
	AND  `users_company_position`.`id` =  `users_company`.`position_title_id`  \
	ORDER BY  `users_company`.`occupation_start` DESC ";
			affected = db.Query(ost.str());
			if(affected > 0) 
			{
					ostringstream	ost1, ost2, ost3, ost4;
					string			occupationFinish;
					ost1.str("");
					ost2.str("");
					ost3.str("");
					ost4.str("");

					for(int i = 0; i < affected; i++, current_company = "0") 
					{
						ostResult << (i ? "," : "")
						<< "{"
						<< "\"companyID\":\"" << db.Get(i, "users_company_id") << "\","
						<< "\"companyName\":\"" << db.Get(i, "company_name") << "\","
						<< "\"occupationStart\":\"" << db.Get(i, "occupation_start") << "\","
						<< "\"occupationFinish\":\"" << db.Get(i, "occupation_finish") << "\","
						<< "\"currentCompany\":\"" << db.Get(i, "current_company") << "\","
						<< "\"title\":\"" << db.Get(i, "title") << "\","
						<< "\"responsibilities\":\"" << db.Get(i, "responsibilities") << "\""
						<< "}";
					}
			}
			else 
			{
				CLog	log;
				log.Write(DEBUG, "main: action == JSON_getUserProfile: DEBUG: carrier path is empty");
			}
			ostResult << "]}";

		}
		else
		{
			CLog	log;

			log.Write(DEBUG, "main: action == JSON_getUserProfile: ERROR: there is no user in DB [", indexPage.GetVarsHandler()->Get("loginUser"), "]");

			ostResult.str("");
			ostResult << "{result:\"error\", description:\"user not found\"}";
		}

		indexPage.RegisterVariableForce("result", ostResult.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file json_response.htmlt was missing");
		}  // if(!indexPage.SetTemplate("JSON_getUserProfile.htmlt"))
	} 	// if(action == "JSON_getUserProfile")


	if(action == "JSON_getCompanyProfile")
	{
		ostringstream	ost, ostResult;
		string			companyID, name, nameLast, age, sex, cv, pass, address, phone, email, isBlocked, avatarFileName, avatarFolderName, current_company;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_getCompanyProfile: start");
		}


		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::JSON_getCompanyProfile: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		companyID = indexPage.GetVarsHandler()->Get("id");

		if(stol(companyID))
		{
			ostringstream		ost;

			companyID = to_string(stol(companyID));
			ost.str("");
			ost << "select * from `company` where `id`=\"" << companyID << "\" and `admin_userID`=\"" << user.GetID() << "\" and `isBlocked`='N';";

			ostResult << "{\"result\":\"success\",\"companies\":[" << GetCompanyListInJSONFormat(ost.str(), &db, &user, false) << "]}";
		}
		else
		{
			CLog	log;
			log.Write(DEBUG, "main: action == JSON_getCompanyProfile: ERROR: error in companyID [", companyID, "]");

			ostResult << "{\"result\":\"error\",\"description\":\"ERROR in companyID\",\"companies\":[]}";
		}

		indexPage.RegisterVariableForce("result", ostResult.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file json_response.htmlt was missing");
		}  // if(!indexPage.SetTemplate("JSON_getCompanyProfile.htmlt"))

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == JSON_getCompanyProfile: end");
		}

	} 	// if(action == "JSON_getCompanyProfile")


	if(action == "updateJobTitle")
	{
		string		newJobTitle, companyId;

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateJobTitle: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		newJobTitle = indexPage.GetVarsHandler()->Get("value");
		companyId = indexPage.GetVarsHandler()->Get("id");

		if((newJobTitle.length() > 0) && (companyId.length() > 0)) 
		{
			ostringstream	ost;
			int				affected;
			char			newJobTitle_cp1251_char[1024];
			string			newJobTitle_cp1251;

			memset(newJobTitle_cp1251_char, 0, sizeof(newJobTitle_cp1251_char));
			convert_utf8_to_windows1251(newJobTitle.c_str(), newJobTitle_cp1251_char, sizeof(newJobTitle_cp1251_char));
			newJobTitle_cp1251 = newJobTitle_cp1251_char;
			trim(newJobTitle_cp1251);

			ost.str("");
			ost << "select * from users_company_position where `title`='" << newJobTitle_cp1251 << "';";
			if((affected = db.Query(ost.str())) > 0)
			{
				string	titleId = db.Get(0, "id");

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateJobTitle: job title [" << newJobTitle_cp1251 << "] already exists, no need to update DB.";
					log.Write(DEBUG, ost.str());
				}

				ost.str("");
				ost << "update users_company set `position_title_id`='" << titleId << "' where `user_id`='" << user.GetID() << "' and `id`='" << companyId << "'";
				db.Query(ost.str());

				// --- Update live feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"3\", \"" << companyId << "\", NOW())";
				db.Query(ost.str());

			}
			else
			{
				string	titleId;

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateJobTitle: new job title [" << newJobTitle_cp1251 << "] needed to be added to DB";
					log.Write(DEBUG, ost.str());
				}
				ost.str("");
				ost << "insert into `users_company_position` (`title`) VALUES (\"" << newJobTitle_cp1251 << "\");";
				db.Query(ost.str());

				ost.str("");
				ost << "select `id` from `users_company_position` where `title`=\"" << newJobTitle_cp1251 << "\";";
				if(db.Query(ost.str()) == 0)
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateJobTitle: ERROR id of newly inserted job [" << newJobTitle_cp1251 << "] is not defined";
					log.Write(ERROR, ost.str());
				}
				else
				{
					titleId = db.Get(0, "id");

					ost.str("");
					ost << "update users_company set `position_title_id`=\"" << titleId << "\" where `user_id`='" << user.GetID() << "' and `id`='" << companyId << "'";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"3\", \"" << companyId << "\", NOW())";
					db.Query(ost.str());

				}
			}

		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateJobTitle: newJobTitle [" << newJobTitle << "] or companyId [" << companyId << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

	}

	// --- updateCompanyName
	if(action == "updateCompanyName")
	{
		ostringstream	ostFinal;
		string			newCompanyName, companyId;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateCompanyName: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateCompanyName: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		newCompanyName = indexPage.GetVarsHandler()->Get("value");
		companyId = indexPage.GetVarsHandler()->Get("id");

		if((newCompanyName.length() > 0) && (companyId.length() > 0)) 
		{
			ostringstream	ost;
			int				affected;
			char			newCompanyName_cp1251_char[1024];
			string			newCompanyName_cp1251;

			memset(newCompanyName_cp1251_char, 0, sizeof(newCompanyName_cp1251_char));
			convert_utf8_to_windows1251(newCompanyName.c_str(), newCompanyName_cp1251_char, sizeof(newCompanyName_cp1251_char));
			newCompanyName_cp1251 = newCompanyName_cp1251_char;
			trim(newCompanyName_cp1251);
			newCompanyName_cp1251 = ReplaceDoubleQuoteToQuote(newCompanyName_cp1251);

			ost.str("");
			ost << "select * from `company` where `name`=\"" << newCompanyName_cp1251 << "\";";
			if((affected = db.Query(ost.str())) > 0)
			{
				string	companyName = db.Get(0, "id");

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCompanyName: company [" << newCompanyName_cp1251 << "] already exists, no need to update DB.";
					log.Write(DEBUG, ost.str());
				}

				ost.str("");
				ost << "update `users_company` set `company_id`=\"" << companyName << "\" where `user_id`='" << user.GetID() << "' and `id`='" << companyId << "'";
				db.Query(ost.str());

				// --- Update live feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"1\", \"" << companyId << "\", NOW())";
				if(db.InsertQuery(ost.str()))
				{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"," << std::endl;
					ostFinal << "\"description\" : \"\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{

					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateCompanyName: error inserting into DB (" << ost.str() << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
					ostFinal << "}" << std::endl;
				}

			}
			else
			{
				string	companyName;

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCompanyName: new job title [" << newCompanyName_cp1251 << "] needed to be added to DB";
					log.Write(DEBUG, ost.str());
				}
				ost.str("");
				ost << "insert into `company` (`name`) VALUES (\"" << newCompanyName_cp1251 << "\");";
				db.Query(ost.str());

				ost.str("");
				ost << "select `id` from `company` where `name`=\"" << newCompanyName_cp1251 << "\";";
				if(db.Query(ost.str()) == 0)
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCompanyName: ERROR id of newly inserted job [" << newCompanyName_cp1251 << "] is not defined";
					log.Write(ERROR, ost.str());
				}
				else
				{
					companyName = db.Get(0, "id");

					ost.str("");
					ost << "update users_company set `company_id`=\"" << companyName << "\" where `user_id`='" << user.GetID() << "' and `id`='" << companyId << "'";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"1\", \"" << companyId << "\", NOW())";
					db.Query(ost.str());
				}
			}

		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateCompanyName: newCompanyName [" << newCompanyName << "] or companyId [" << companyId << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateCompanyName: end");
		}
	}

	// --- updateCertificationTrack
	if(action == "updateCertificationTrack")
	{
		ostringstream	ostFinal;
		string			newCertificationTrack, userCertificationID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateCertificationTrack: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateCertificationTrack: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		newCertificationTrack = indexPage.GetVarsHandler()->Get("value");
		userCertificationID = indexPage.GetVarsHandler()->Get("id");

		if((newCertificationTrack.length() > 0) && (userCertificationID.length() > 0)) 
		{
			ostringstream	ost;
			int				affected;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(newCertificationTrack.c_str(), convertBuffer, sizeof(convertBuffer));
			newCertificationTrack = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userCertificationID.c_str(), convertBuffer, sizeof(convertBuffer));
			userCertificationID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `certification_tracks` where `title`=\"" << newCertificationTrack << "\";";
			if((affected = db.Query(ost.str())) > 0)
			{
				string	trackID = db.Get(0, "id");

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCertificationTrack: company [" << newCertificationTrack << "] already exists, no need to update DB.";
					log.Write(DEBUG, ost.str());
				}

				ost.str("");
				ost << "update `users_certifications` set `track_id`=\"" << trackID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userCertificationID << "'";
				db.Query(ost.str());

				// --- Update live feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"24\", \"" << userCertificationID << "\", NOW())";
				if(db.InsertQuery(ost.str()))
				{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"," << std::endl;
					ostFinal << "\"description\" : \"\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{

					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateCertificationTrack: error inserting into DB (" << ost.str() << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
					ostFinal << "}" << std::endl;
				}

			}
			else
			{
				unsigned long	trackID;

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCertificationTrack: new certification title [" << newCertificationTrack << "] needed to be added to DB";
					log.Write(DEBUG, ost.str());
				}
				ost.str("");
				ost << "insert into `certification_tracks` (`title`) VALUES (\"" << newCertificationTrack << "\");";
				trackID = db.InsertQuery(ost.str());

				if(trackID)
				{
					ost.str("");
					ost << "update users_certifications set `track_id`=\"" << trackID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userCertificationID << "'";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"24\", \"" << userCertificationID << "\", NOW())";
					db.Query(ost.str());
				}
				else
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCertificationTrack: ERROR id of newly inserted job [" << newCertificationTrack << "] is not defined";
					log.Write(ERROR, ost.str());
				}
			}

		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateCertificationTrack: newCertificationTrack [" << newCertificationTrack << "] or userCertificationID [" << userCertificationID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateCertificationTrack: end");
		}
	}

	// --- updateCourseTrack
	if(action == "updateCourseTrack")
	{
		ostringstream	ostFinal;
		string			newCourseTrack, userCourseID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateCourseTrack: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateCourseTrack: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		newCourseTrack = indexPage.GetVarsHandler()->Get("value");
		userCourseID = indexPage.GetVarsHandler()->Get("id");

		if((newCourseTrack.length() > 0) && (userCourseID.length() > 0)) 
		{
			ostringstream	ost;
			int				affected;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(newCourseTrack.c_str(), convertBuffer, sizeof(convertBuffer));
			newCourseTrack = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userCourseID.c_str(), convertBuffer, sizeof(convertBuffer));
			userCourseID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `certification_tracks` where `title`=\"" << newCourseTrack << "\";";
			if((affected = db.Query(ost.str())) > 0)
			{
				string	trackID = db.Get(0, "id");

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCourseTrack: company [" << newCourseTrack << "] already exists, no need to update DB.";
					log.Write(DEBUG, ost.str());
				}

				ost.str("");
				ost << "update `users_courses` set `track_id`=\"" << trackID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userCourseID << "'";
				db.Query(ost.str());

				// --- Update live feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"25\", \"" << userCourseID << "\", NOW())";
				if(db.InsertQuery(ost.str()))
				{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"," << std::endl;
					ostFinal << "\"description\" : \"\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{

					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateCourseTrack: error inserting into DB (" << ost.str() << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
					ostFinal << "}" << std::endl;
				}

			}
			else
			{
				unsigned long	trackID;

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCourseTrack: new certification title [" << newCourseTrack << "] needed to be added to DB";
					log.Write(DEBUG, ost.str());
				}
				ost.str("");
				ost << "insert into `certification_tracks` (`title`) VALUES (\"" << newCourseTrack << "\");";
				trackID = db.InsertQuery(ost.str());

				if(trackID)
				{
					ost.str("");
					ost << "update `users_courses` set `track_id`=\"" << trackID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userCourseID << "'";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"25\", \"" << userCourseID << "\", NOW())";
					db.Query(ost.str());
				}
				else
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCourseTrack: ERROR id of newly inserted job [" << newCourseTrack << "] is not defined";
					log.Write(ERROR, ost.str());
				}
			}

		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateCourseTrack: newCourseTrack [" << newCourseTrack << "] or userCourseID [" << userCourseID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateCourseTrack: end");
		}
	}

	// --- updateCertificationVendor
	if(action == "updateCertificationVendor")
	{
		ostringstream	ostFinal;
		string			newCertificationVendor, userCertificationID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateCertificationVendor: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateCertificationVendor: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		newCertificationVendor = indexPage.GetVarsHandler()->Get("value");
		userCertificationID = indexPage.GetVarsHandler()->Get("id");

		if((newCertificationVendor.length() > 0) && (userCertificationID.length() > 0)) 
		{
			ostringstream	ost;
			int				affected;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(newCertificationVendor.c_str(), convertBuffer, sizeof(convertBuffer));
			newCertificationVendor = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userCertificationID.c_str(), convertBuffer, sizeof(convertBuffer));
			userCertificationID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `company` where `name`=\"" << newCertificationVendor << "\";";
			if((affected = db.Query(ost.str())) > 0)
			{
				string	vendorID = db.Get(0, "id");

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCertificationVendor: company [" << newCertificationVendor << "] already exists, no need to update DB.";
					log.Write(DEBUG, ost.str());
				}

				ost.str("");
				ost << "update `users_certifications` set `vendor_id`=\"" << vendorID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userCertificationID << "'";
				db.Query(ost.str());

				// --- Update live feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"26\", \"" << userCertificationID << "\", NOW())";
				if(db.InsertQuery(ost.str()))
				{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"," << std::endl;
					ostFinal << "\"description\" : \"\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{

					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateCertificationVendor: error inserting into DB (" << ost.str() << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
					ostFinal << "}" << std::endl;
				}

			}
			else
			{
				unsigned long	vendorID;

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCertificationVendor: new certification title [" << newCertificationVendor << "] needed to be added to DB";
					log.Write(DEBUG, ost.str());
				}
				ost.str("");
				ost << "insert into `company` (`name`) VALUES (\"" << newCertificationVendor << "\");";
				vendorID = db.InsertQuery(ost.str());

				if(vendorID)
				{
					ost.str("");
					ost << "update users_certifications set `vendor_id`=\"" << vendorID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userCertificationID << "'";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"26\", \"" << userCertificationID << "\", NOW())";
					db.Query(ost.str());
				}
				else
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCertificationVendor: ERROR id of newly inserted job [" << newCertificationVendor << "] is not defined";
					log.Write(ERROR, ost.str());
				}
			}

		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateCertificationVendor: newCertificationVendor [" << newCertificationVendor << "] or userCertificationID [" << userCertificationID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateCertificationVendor: end");
		}
	}

	// --- updateCourseVendor
	if(action == "updateCourseVendor")
	{
		ostringstream	ostFinal;
		string			newCourseVendor, userCourseID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateCourseVendor: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateCourseVendor: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		newCourseVendor = indexPage.GetVarsHandler()->Get("value");
		userCourseID = indexPage.GetVarsHandler()->Get("id");

		if((newCourseVendor.length() > 0) && (userCourseID.length() > 0)) 
		{
			ostringstream	ost;
			int				affected;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(newCourseVendor.c_str(), convertBuffer, sizeof(convertBuffer));
			newCourseVendor = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userCourseID.c_str(), convertBuffer, sizeof(convertBuffer));
			userCourseID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `company` where `name`=\"" << newCourseVendor << "\";";
			if((affected = db.Query(ost.str())) > 0)
			{
				string	vendorID = db.Get(0, "id");

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCourseVendor: company [" << newCourseVendor << "] already exists, no need to update DB.";
					log.Write(DEBUG, ost.str());
				}

				ost.str("");
				ost << "update `users_courses` set `vendor_id`=\"" << vendorID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userCourseID << "'";
				db.Query(ost.str());

				// --- Update live feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"27\", \"" << userCourseID << "\", NOW())";
				if(db.InsertQuery(ost.str()))
				{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"," << std::endl;
					ostFinal << "\"description\" : \"\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{

					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateCourseVendor: error inserting into DB (" << ost.str() << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
					ostFinal << "}" << std::endl;
				}

			}
			else
			{
				unsigned long	vendorID;

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCourseVendor: new certification title [" << newCourseVendor << "] needed to be added to DB";
					log.Write(DEBUG, ost.str());
				}
				ost.str("");
				ost << "insert into `company` (`name`) VALUES (\"" << newCourseVendor << "\");";
				vendorID = db.InsertQuery(ost.str());

				if(vendorID)
				{
					ost.str("");
					ost << "update `users_courses` set `vendor_id`=\"" << vendorID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userCourseID << "'";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"27\", \"" << userCourseID << "\", NOW())";
					db.Query(ost.str());
				}
				else
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateCourseVendor: ERROR id of newly inserted job [" << newCourseVendor << "] is not defined";
					log.Write(ERROR, ost.str());
				}
			}

		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateCourseVendor: newCourseVendor [" << newCourseVendor << "] or userCourseID [" << userCourseID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateCourseVendor: end");
		}
	}

	// --- updateSchoolTitle
	if(action == "updateSchoolTitle")
	{
		ostringstream	ostFinal;
		string			newSchoolNumber, userSchoolID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateSchoolTitle: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateSchoolTitle: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		newSchoolNumber = indexPage.GetVarsHandler()->Get("value");
		userSchoolID = indexPage.GetVarsHandler()->Get("id");

		if((newSchoolNumber.length() > 0) && (userSchoolID.length() > 0)) 
		{
			ostringstream	ost;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(newSchoolNumber.c_str(), convertBuffer, sizeof(convertBuffer));
			newSchoolNumber = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userSchoolID.c_str(), convertBuffer, sizeof(convertBuffer));
			userSchoolID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `users_school` where `user_id`=\"" << user.GetID() << "\" and `id`=\"" << userSchoolID << "\";";
			if(db.Query(ost.str()))
			{
				unsigned long 	existingSchoolID = atol(db.Get(0, "school_id"));

				ost.str("");
				ost << "select * from `school` where `id`=\"" << existingSchoolID << "\";";
				if(db.Query(ost.str()))
				{
					unsigned long	existingLocalityID = atol(db.Get(0, "geo_locality_id"));
					unsigned long	newSchoolID;

					ost.str("");
					ost << "select * from `school` where `geo_locality_id`=\"" << existingLocalityID << "\" and `title`=\"" << newSchoolNumber << "\";";
					if(db.Query(ost.str()))
					{
						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::updateSchoolTitle: school [locality " << existingLocalityID << ", newSchoolNumber " << newSchoolNumber << "] already exists, no need to update DB.";
							log.Write(DEBUG, ost.str());
						}

						newSchoolID = atol(db.Get(0, "id"));
					}
					else
					{
						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::updateSchoolTitle: require to add new school [locality " << existingLocalityID << ", newSchoolNumber " << newSchoolNumber << "]";
							log.Write(DEBUG, ost.str());
						}

						ost.str("");
						ost << "insert into `school` (`geo_locality_id`, `title`) VALUES (\"" << existingLocalityID << "\",\"" << newSchoolNumber << "\");";
						if(!(newSchoolID = db.InsertQuery(ost.str())))
						{
							{
								CLog			log;
								ostringstream	ostTmp;

								ostTmp.str("");
								ostTmp << "int main()::updateSchoolTitle: error inserting new school (" << newSchoolNumber << ")";
								log.Write(ERROR, ostTmp.str());
							}

							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"error\"," << std::endl;
							ostFinal << "\"description\" : \"error inserting new school\"" << std::endl;
							ostFinal << "}" << std::endl;
						}
					}

					if(newSchoolID)
					{
						ost.str("");
						ost << "update `users_school` set `school_id`=\"" << newSchoolID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userSchoolID << "'";
						db.Query(ost.str());

						// --- Update live feed
						ost.str("");
						ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"32\", \"" << userSchoolID << "\", NOW())";
						if(db.InsertQuery(ost.str()))
						{
							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"success\"," << std::endl;
							ostFinal << "\"description\" : \"\"" << std::endl;
							ostFinal << "}" << std::endl;
						}
						else
						{

							{
								CLog			log;
								ostringstream	ostTmp;

								ostTmp.str("");
								ostTmp << "int main()::updateSchoolTitle: error inserting into DB (" << ost.str() << ")";
								log.Write(ERROR, ostTmp.str());
							}

							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"error\"," << std::endl;
							ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
							ostFinal << "}" << std::endl;
						}

					}
					else
					{
						{
							CLog			log;
							ostringstream	ostTmp;

							ostTmp.str("");
							ostTmp << "int main()::updateSchoolTitle: school can't be found";
							log.Write(ERROR, ostTmp.str());
						}

						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"error\"," << std::endl;
						ostFinal << "\"description\" : \"school can't be found\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
				}
				else
				{
					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateSchoolTitle: error finding school (id=" << existingSchoolID << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error finding school in DB\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
			}
			else
			{

				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::updateSchoolTitle: error selecting from users_school (id=" << userSchoolID << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error finding user schools in DB\"" << std::endl;
				ostFinal << "}" << std::endl;

			}

		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateSchoolTitle: either newSchoolNumber [" << newSchoolNumber << "] or userSchoolID [" << userSchoolID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateSchoolTitle: end");
		}
	}

	// --- updateUniversityTitle
	if(action == "updateUniversityTitle")
	{
		ostringstream	ostFinal;
		string			newUniversityTitle, userUniversityID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateUniversityTitle: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateUniversityTitle: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		newUniversityTitle = indexPage.GetVarsHandler()->Get("value");
		userUniversityID = indexPage.GetVarsHandler()->Get("id");

		if((newUniversityTitle.length() > 0) && (userUniversityID.length() > 0)) 
		{
			ostringstream	ost;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(newUniversityTitle.c_str(), convertBuffer, sizeof(convertBuffer));
			newUniversityTitle = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userUniversityID.c_str(), convertBuffer, sizeof(convertBuffer));
			userUniversityID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `users_university` where `user_id`=\"" << user.GetID() << "\" and `id`=\"" << userUniversityID << "\";";
			if(db.Query(ost.str()))
			{
				unsigned long 	existingUniversityID = atol(db.Get(0, "university_id"));

				ost.str("");
				ost << "select * from `university` where `id`=\"" << existingUniversityID << "\";";
				if(db.Query(ost.str()))
				{
					unsigned long	existingRegionID = atol(db.Get(0, "geo_region_id"));
					unsigned long	newUniversityID;

					ost.str("");
					ost << "select * from `university` where `geo_region_id`=\"" << existingRegionID << "\" and `title`=\"" << newUniversityTitle << "\";";
					if(db.Query(ost.str()))
					{
						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::updateUniversityTitle: university [locality " << existingRegionID << ", newUniversityTitle " << newUniversityTitle << "] already exists, no need to update DB.";
							log.Write(DEBUG, ost.str());
						}

						newUniversityID = atol(db.Get(0, "id"));
					}
					else
					{
						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::updateUniversityTitle: require to add new university [locality " << existingRegionID << ", newUniversityTitle " << newUniversityTitle << "]";
							log.Write(DEBUG, ost.str());
						}

						ost.str("");
						ost << "insert into `university` (`geo_region_id`, `title`) VALUES (\"" << existingRegionID << "\",\"" << newUniversityTitle << "\");";
						if(!(newUniversityID = db.InsertQuery(ost.str())))
						{
							{
								CLog			log;
								ostringstream	ostTmp;

								ostTmp.str("");
								ostTmp << "int main()::updateUniversityTitle: error inserting new university (" << newUniversityTitle << ")";
								log.Write(ERROR, ostTmp.str());
							}

							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"error\"," << std::endl;
							ostFinal << "\"description\" : \"error inserting new university\"" << std::endl;
							ostFinal << "}" << std::endl;
						}
					}

					if(newUniversityID)
					{
						ost.str("");
						ost << "update `users_university` set `university_id`=\"" << newUniversityID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userUniversityID << "'";
						db.Query(ost.str());

						// --- Update live feed
						ost.str("");
						ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"33\", \"" << userUniversityID << "\", NOW())";
						if(db.InsertQuery(ost.str()))
						{
							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"success\"," << std::endl;
							ostFinal << "\"description\" : \"\"" << std::endl;
							ostFinal << "}" << std::endl;
						}
						else
						{

							{
								CLog			log;
								ostringstream	ostTmp;

								ostTmp.str("");
								ostTmp << "int main()::updateUniversityTitle: error inserting into DB (" << ost.str() << ")";
								log.Write(ERROR, ostTmp.str());
							}

							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"error\"," << std::endl;
							ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
							ostFinal << "}" << std::endl;
						}

					}
					else
					{
						{
							CLog			log;
							ostringstream	ostTmp;

							ostTmp.str("");
							ostTmp << "int main()::updateUniversityTitle: university can't be found";
							log.Write(ERROR, ostTmp.str());
						}

						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"error\"," << std::endl;
						ostFinal << "\"description\" : \"university can't be found\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
				}
				else
				{
					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateUniversityTitle: error finding university (id=" << existingUniversityID << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error finding university in DB\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
			}
			else
			{

				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::updateUniversityTitle: error selecting from users_university (id=" << userUniversityID << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error finding user university in DB\"" << std::endl;
				ostFinal << "}" << std::endl;

			}

		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateUniversityTitle: either newUniversityTitle [" << newUniversityTitle << "] or userUniversityID [" << userUniversityID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateUniversityTitle: end");
		}
	}

	// --- updateLanguageTitle
	if(action == "updateLanguageTitle")
	{
		ostringstream	ostFinal;
		string			newLanguageTitle, userLanguageID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateLanguageTitle: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateLanguageTitle: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		newLanguageTitle = indexPage.GetVarsHandler()->Get("value");
		userLanguageID = indexPage.GetVarsHandler()->Get("id");

		if((newLanguageTitle.length() > 0) && (userLanguageID.length() > 0)) 
		{
			ostringstream	ost;
			int				affected;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(newLanguageTitle.c_str(), convertBuffer, sizeof(convertBuffer));
			newLanguageTitle = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userLanguageID.c_str(), convertBuffer, sizeof(convertBuffer));
			userLanguageID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `language` where `title`=\"" << newLanguageTitle << "\";";
			if((affected = db.Query(ost.str())) > 0)
			{
				string	languageID = db.Get(0, "id");

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateLanguageTitle: company [" << newLanguageTitle << "] already exists, no need to update DB.";
					log.Write(DEBUG, ost.str());
				}

				ost.str("");
				ost << "update `users_language` set `language_id`=\"" << languageID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userLanguageID << "'";
				db.Query(ost.str());

				// --- Update live feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"34\", \"" << userLanguageID << "\", NOW())";
				if(db.InsertQuery(ost.str()))
				{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"," << std::endl;
					ostFinal << "\"description\" : \"\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{

					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateLanguageTitle: error inserting into DB (" << ost.str() << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
					ostFinal << "}" << std::endl;
				}

			}
			else
			{
				unsigned long	languageID;

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateLanguageTitle: new language title [" << newLanguageTitle << "] needed to be added to DB";
					log.Write(DEBUG, ost.str());
				}
				ost.str("");
				ost << "insert into `language` (`title`) VALUES (\"" << newLanguageTitle << "\");";
				languageID = db.InsertQuery(ost.str());

				if(languageID)
				{
					ost.str("");
					ost << "update users_language set `language_id`=\"" << languageID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userLanguageID << "'";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"34\", \"" << userLanguageID << "\", NOW())";
					if(db.InsertQuery(ost.str()))
					{
						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"success\"," << std::endl;
						ostFinal << "\"description\" : \"\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
					else
					{

						{
							CLog			log;
							ostringstream	ostTmp;

							ostTmp.str("");
							ostTmp << "int main()::updateLanguageTitle: error inserting into DB (" << ost.str() << ")";
							log.Write(ERROR, ostTmp.str());
						}

						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"error\"," << std::endl;
						ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
				}
				else
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateLanguageTitle: ERROR id of newly inserted language [" << newLanguageTitle << "] is not defined";
					log.Write(ERROR, ost.str());

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error creating new language\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
			}

		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateLanguageTitle: either newLanguageTitle [" << newLanguageTitle << "] or userLanguageID [" << userLanguageID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"error parameters sent to server\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateLanguageTitle: end");
		}
	}

	// --- updateSkillTitle
	if(action == "updateSkillTitle")
	{
		ostringstream	ostFinal;
		string			newSkillTitle, userSkillID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateSkillTitle: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateSkillTitle: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		newSkillTitle = indexPage.GetVarsHandler()->Get("value");
		userSkillID = indexPage.GetVarsHandler()->Get("id");

		if((newSkillTitle.length() > 0) && (userSkillID.length() > 0)) 
		{
			ostringstream	ost;
			int				affected;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(newSkillTitle.c_str(), convertBuffer, sizeof(convertBuffer));
			newSkillTitle = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userSkillID.c_str(), convertBuffer, sizeof(convertBuffer));
			userSkillID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `skill` where `title`=\"" << newSkillTitle << "\";";
			if((affected = db.Query(ost.str())) > 0)
			{
				string	skillID = db.Get(0, "id");

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateSkillTitle: company [" << newSkillTitle << "] already exists, no need to update DB.";
					log.Write(DEBUG, ost.str());
				}

				ost.str("");
				ost << "update `users_skill` set `skill_id`=\"" << skillID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userSkillID << "'";
				db.Query(ost.str());

				// --- Update live feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"42\", \"" << userSkillID << "\", NOW())";
				if(db.InsertQuery(ost.str()))
				{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"," << std::endl;
					ostFinal << "\"description\" : \"\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{

					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateSkillTitle: error inserting into DB (" << ost.str() << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
					ostFinal << "}" << std::endl;
				}

			}
			else
			{
				unsigned long	skillID;

				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateSkillTitle: new skill title [" << newSkillTitle << "] needed to be added to DB";
					log.Write(DEBUG, ost.str());
				}
				ost.str("");
				ost << "insert into `skill` (`title`) VALUES (\"" << newSkillTitle << "\");";
				skillID = db.InsertQuery(ost.str());

				if(skillID)
				{
					ost.str("");
					ost << "update users_skill set `skill_id`=\"" << skillID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userSkillID << "'";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"42\", \"" << userSkillID << "\", NOW())";
					db.Query(ost.str());

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"," << std::endl;
					ostFinal << "\"description\" : \"\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateSkillTitle: ERROR id of newly inserted skill [" << newSkillTitle << "] is not defined";
					log.Write(ERROR, ost.str());

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error creating new skill\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
			}

		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateSkillTitle: either newSkillTitle [" << newSkillTitle << "] or userSkillID [" << userSkillID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"error parameters sent to server\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateSkillTitle: end");
		}
	}


	// --- updateCertificationNumber
	if(action == "updateCertificationNumber")
	{
		ostringstream	ostFinal;
		string			newCertificationNumber, userCertificationID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateCertificationNumber: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateCertificationNumber: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		newCertificationNumber = indexPage.GetVarsHandler()->Get("value");
		userCertificationID = indexPage.GetVarsHandler()->Get("id");

		if((newCertificationNumber.length() > 0) && (userCertificationID.length() > 0)) 
		{
			ostringstream	ost;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(newCertificationNumber.c_str(), convertBuffer, sizeof(convertBuffer));
			newCertificationNumber = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userCertificationID.c_str(), convertBuffer, sizeof(convertBuffer));
			userCertificationID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "update `users_certifications` set `certification_number`=\"" << newCertificationNumber << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userCertificationID << "'";
			db.Query(ost.str());

			// --- Update live feed
			ost.str("");
			ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"28\", \"" << userCertificationID << "\", NOW())";
			if(db.InsertQuery(ost.str()))
			{
				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"success\"," << std::endl;
				ostFinal << "\"description\" : \"\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
			else
			{

				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::updateCertificationNumber: error inserting into DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
				ostFinal << "}" << std::endl;
			}

		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateCertificationNumber: newCertificationNumber [" << newCertificationNumber << "] or userCertificationID [" << userCertificationID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateCertificationNumber: end");
		}
	}

	// --- updateSchoolLocality
	if(action == "updateSchoolLocality")
	{
		ostringstream	ostFinal;
		string			newSchoolLocality, userSchoolID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateSchoolLocality: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateSchoolLocality: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		newSchoolLocality = indexPage.GetVarsHandler()->Get("value");
		userSchoolID = indexPage.GetVarsHandler()->Get("id");

		if((newSchoolLocality.length() > 0) && (userSchoolID.length() > 0)) 
		{
			ostringstream	ost;
			int				affected;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(newSchoolLocality.c_str(), convertBuffer, sizeof(convertBuffer));
			newSchoolLocality = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userSchoolID.c_str(), convertBuffer, sizeof(convertBuffer));
			userSchoolID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `users_school` where `id`=\"" << userSchoolID << "\" and `user_id`=\"" << user.GetID() << "\";";
			if((affected = db.Query(ost.str())) > 0)
			{
				string 	existingSchoolID = db.Get(0, "school_id");

				ost.str("");
				ost << "select * from `school` where `id`=\"" << existingSchoolID << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					long int	localityID = 0;
					string 	existingSchoolTitle = db.Get(0, "title");

					ost.str("");
					ost << "select * from `geo_locality` where `title`=\"" << newSchoolLocality << "\";";
					if((affected = db.Query(ost.str())) > 0)
					{
						localityID = atol(db.Get(0, "id"));

						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::updateSchoolLocality: locality [" << newSchoolLocality << "] already exists, no need to update DB.";
							log.Write(DEBUG, ost.str());
						}

					}
					else
					{

						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::updateSchoolLocality: new locality title [" << newSchoolLocality << "] needed to be added to DB";
							log.Write(DEBUG, ost.str());
						}
						ost.str("");
						ost << "insert into `geo_locality` (`title`) VALUES (\"" << newSchoolLocality << "\");";
						localityID = db.InsertQuery(ost.str());
					}

					if(localityID && existingSchoolTitle.length())
					{
						long int	newSchoolID = 0;

						ost.str("");
						ost << "select `id` from `school` where `geo_locality_id`=\"" << localityID << "\" and `title`=\"" << existingSchoolTitle << "\";";
						if(db.Query(ost.str()))
						{
							newSchoolID = atol(db.Get(0, "id"));
						}
						else
						{
							ost.str("");
							ost << "insert into `school` (`geo_locality_id`, `title`) VALUES (\"" << localityID << "\", \"" << existingSchoolTitle << "\");";
							newSchoolID = db.InsertQuery(ost.str());
						}

						if(newSchoolID)
						{
							ost.str("");
							ost << "update `users_school` set `school_id`=\"" << newSchoolID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userSchoolID << "';";
							db.Query(ost.str());

							// --- Update live feed
							ost.str("");
							ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"30\", \"" << userSchoolID << "\", NOW())";
							db.Query(ost.str());

							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"success\"," << std::endl;
							ostFinal << "\"description\" : \"\"" << std::endl;
							ostFinal << "}" << std::endl;
						}
						else
						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::updateSchoolLocality: ERROR id of newly inserted school [" << localityID << ", " << existingSchoolTitle << "] is not defined";
							log.Write(ERROR, ost.str());

							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"error\"," << std::endl;
							ostFinal << "\"description\" : \"ERROR id of newly inserted school [" << localityID << ", " << existingSchoolTitle << "] is not defined\"" << std::endl;
							ostFinal << "}" << std::endl;

						}

					}
					else
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::updateSchoolLocality: ERROR localityID [" << localityID << "] or existingSchoolTitle [" << existingSchoolTitle << "] is not defined";
						log.Write(ERROR, ost.str());

						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"error\"," << std::endl;
						ostFinal << "\"description\" : \" localityID [" << localityID << "] or existingSchoolTitle [" << existingSchoolTitle << "] is not defined\"" << std::endl;
						ostFinal << "}" << std::endl;

					}
				}
				else
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateSchoolLocality: ERROR id of existing schoolID [" << existingSchoolID << "] is not defined";
					log.Write(ERROR, ost.str());

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"ERROR id of existing schoolID [" << existingSchoolID << "] is not defined\"" << std::endl;
					ostFinal << "}" << std::endl;

				}

			}
			else
			{
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateSchoolLocality: ERROR: userSchoolID [" << userSchoolID << "] is unknown/empty in DB or belongs to another user";
					log.Write(ERROR, ost.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"userSchoolID unknown/empty in DB or belongs to another user\"" << std::endl;
				ostFinal << "}" << std::endl;
			}



		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateSchoolLocality: newSchoolLocality [" << newSchoolLocality << "] or userSchoolID [" << userSchoolID << "] is unknown/empty";
				log.Write(ERROR, ost.str());
			}
			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"newSchoolLocality [" << newSchoolLocality << "] or userSchoolID [" << userSchoolID << "] is unknown/empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateSchoolLocality: end");
		}
	}

	// --- updateUniversityRegion
	if(action == "updateUniversityRegion")
	{
		ostringstream	ostFinal;
		string			newUniversityRegion, userUniversityID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateUniversityRegion: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateUniversityRegion: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		newUniversityRegion = indexPage.GetVarsHandler()->Get("value");
		userUniversityID = indexPage.GetVarsHandler()->Get("id");

		if((newUniversityRegion.length() > 0) && (userUniversityID.length() > 0)) 
		{
			ostringstream	ost;
			int				affected;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(newUniversityRegion.c_str(), convertBuffer, sizeof(convertBuffer));
			newUniversityRegion = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userUniversityID.c_str(), convertBuffer, sizeof(convertBuffer));
			userUniversityID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `users_university` where `id`=\"" << userUniversityID << "\" and `user_id`=\"" << user.GetID() << "\";";
			if((affected = db.Query(ost.str())) > 0)
			{
				string 	existingUniversityID = db.Get(0, "university_id");

				ost.str("");
				ost << "select * from `university` where `id`=\"" << existingUniversityID << "\";";
				if((affected = db.Query(ost.str())) > 0)
				{
					long int	regionID = 0;
					string 		existingUniversityTitle = db.Get(0, "title");

					ost.str("");
					ost << "select * from `geo_region` where `title`=\"" << newUniversityRegion << "\";";
					if((affected = db.Query(ost.str())) > 0)
					{
						regionID = atol(db.Get(0, "id"));

						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::updateUniversityRegion: region [" << newUniversityRegion << "] already exists, no need to update DB.";
							log.Write(DEBUG, ost.str());
						}

					}
					else
					{

						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::updateUniversityRegion: new region title [" << newUniversityRegion << "] needed to be added to DB";
							log.Write(DEBUG, ost.str());
						}
						ost.str("");
						ost << "insert into `geo_region` (`title`) VALUES (\"" << newUniversityRegion << "\");";
						regionID = db.InsertQuery(ost.str());
					}

					if(regionID && existingUniversityTitle.length())
					{
						long int	newUniversityID = 0;

						ost.str("");
						ost << "select `id` from `university` where `geo_region_id`=\"" << regionID << "\" and `title`=\"" << existingUniversityTitle << "\";";
						if(db.Query(ost.str()))
						{
							newUniversityID = atol(db.Get(0, "id"));
						}
						else
						{
							ost.str("");
							ost << "insert into `university` (`geo_region_id`, `title`) VALUES (\"" << regionID << "\", \"" << existingUniversityTitle << "\");";
							newUniversityID = db.InsertQuery(ost.str());
						}

						if(newUniversityID)
						{
							ost.str("");
							ost << "update `users_university` set `university_id`=\"" << newUniversityID << "\" where `user_id`='" << user.GetID() << "' and `id`='" << userUniversityID << "';";
							db.Query(ost.str());

							// --- Update live feed
							ost.str("");
							ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"31\", \"" << userUniversityID << "\", NOW())";
							db.Query(ost.str());

							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"success\"," << std::endl;
							ostFinal << "\"description\" : \"\"" << std::endl;
							ostFinal << "}" << std::endl;
						}
						else
						{
							CLog	log;
							ostringstream	ost;

							ost.str("");
							ost << "int main()::updateUniversityRegion: ERROR id of newly inserted university [" << regionID << ", " << existingUniversityTitle << "] is not defined";
							log.Write(ERROR, ost.str());

							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"error\"," << std::endl;
							ostFinal << "\"description\" : \"ERROR id of newly inserted university [" << regionID << ", " << existingUniversityTitle << "] is not defined\"" << std::endl;
							ostFinal << "}" << std::endl;

						}

					}
					else
					{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "int main()::updateUniversityRegion: ERROR regionID [" << regionID << "] or existingUniversityTitle [" << existingUniversityTitle << "] is not defined";
						log.Write(ERROR, ost.str());

						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"error\"," << std::endl;
						ostFinal << "\"description\" : \" regionID [" << regionID << "] or existingUniversityTitle [" << existingUniversityTitle << "] is not defined\"" << std::endl;
						ostFinal << "}" << std::endl;

					}
				}
				else
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateUniversityRegion: ERROR id of existing universityID [" << existingUniversityID << "] is not defined";
					log.Write(ERROR, ost.str());

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"ERROR id of existing universityID [" << existingUniversityID << "] is not defined\"" << std::endl;
					ostFinal << "}" << std::endl;

				}

			}
			else
			{
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::updateUniversityRegion: ERROR: userUniversityID [" << userUniversityID << "] is unknown/empty in DB or belongs to another user";
					log.Write(ERROR, ost.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"userUniversityID unknown/empty in DB or belongs to another user\"" << std::endl;
				ostFinal << "}" << std::endl;
			}



		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateUniversityRegion: newUniversityRegion [" << newUniversityRegion << "] or userUniversityID [" << userUniversityID << "] is unknown/empty";
				log.Write(ERROR, ost.str());
			}
			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"newUniversityRegion [" << newUniversityRegion << "] or userUniversityID [" << userUniversityID << "] is unknown/empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateUniversityRegion: end");
		}
	}

	if((action == "AJAX_editProfile_setRecommendationAdverse") || (action == "AJAX_editProfile_setRecommendationClean"))
	{
		ostringstream	ostFinal;
		string			userRecommendationID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_editProfile_setRecommendationAdverse: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_editProfile_setRecommendationAdverse: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		userRecommendationID = indexPage.GetVarsHandler()->Get("id");

		if((userRecommendationID.length() > 0)) 
		{
			ostringstream	ost;
			int				affected;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userRecommendationID.c_str(), convertBuffer, sizeof(convertBuffer));
			userRecommendationID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `users_recommendation` where `id`=\"" << userRecommendationID << "\" and `recommended_userID`=\"" << user.GetID() << "\";";
			if((affected = db.Query(ost.str())) > 0)
			{
				ost.str("");
				ost << "update `users_recommendation` set `state`=\"" << (action == "AJAX_editProfile_setRecommendationAdverse" ? "adverse" : "clean") << "\" where `id`=\"" << userRecommendationID << "\";";
				db.Query(ost.str());
				if(!db.isError())
				{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"," << std::endl;
					ostFinal << "\"description\" : \"\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::AJAX_editProfile_setRecommendationAdverse: ERROR updating recommendation [" << userRecommendationID << "] state";
					log.Write(ERROR, ost.str());

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"ERROR udpating recommendation[" << userRecommendationID << "] state\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
			}
			else
			{
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::AJAX_editProfile_setRecommendationAdverse: ERROR: userRecommendationID [" << userRecommendationID << "] is unknown/empty in DB or belongs to another user";
					log.Write(ERROR, ost.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"userRecommendationID unknown/empty in DB or belongs to another user\"" << std::endl;
				ostFinal << "}" << std::endl;
			}



		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::AJAX_editProfile_setRecommendationAdverse: userRecommendationID [" << userRecommendationID << "] is unknown/empty";
				log.Write(ERROR, ost.str());
			}
			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"userRecommendationID [" << userRecommendationID << "] is unknown/empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_editProfile_setRecommendationAdverse: end");
		}
	}

	if(action == "AJAX_newsFeedMarkImageToRemove")
	{
		ostringstream	ostFinal;
		string			imageIDMarkToRemove;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_newsFeedMarkImageToRemove: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_newsFeedMarkImageToRemove: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		imageIDMarkToRemove = indexPage.GetVarsHandler()->Get("imageID");

		if(imageIDMarkToRemove.length() > 0) 
		{
			ostringstream	ost;
			int				affected;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(imageIDMarkToRemove.c_str(), convertBuffer, sizeof(convertBuffer));
			imageIDMarkToRemove = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `feed_images` where `id`=\"" << imageIDMarkToRemove << "\" and `userID`=\"" << user.GetID() << "\";";
			if((affected = db.Query(ost.str())) > 0)
			{
				ost.str("");
				ost << "update `feed_images` set `removeFlag`=\"remove\" where `id`=\"" << imageIDMarkToRemove << "\";";
				db.Query(ost.str());
				if(!db.isError())
				{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"," << std::endl;
					ostFinal << "\"description\" : \"\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::AJAX_newsFeedMarkImageToRemove: ERROR marking imageID to removal (" << db.GetErrorMessage() << ")";
					log.Write(ERROR, ost.str());

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"ERROR marking imageID to removal\"" << std::endl;
					ostFinal << "}" << std::endl;

				}

			}
			else
			{
				{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "int main()::AJAX_newsFeedMarkImageToRemove: imageIDMarkToRemove [" << imageIDMarkToRemove << "]  isn't belongs to you";
					log.Write(ERROR, ost.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"imageID unknown/empty in DB or belongs to another user\"" << std::endl;
				ostFinal << "}" << std::endl;
			}



		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::AJAX_newsFeedMarkImageToRemove: imageIDMarkToRemove [" << imageIDMarkToRemove << "]  is unknown/empty";
				log.Write(ERROR, ost.str());
			}
			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"imageIDMarkToRemove [" << imageIDMarkToRemove << "]  is unknown/empty\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_newsFeedMarkImageToRemove: end");
		}
	}


	// --- viewProfile_SkillApprove
	if(action == "viewProfile_SkillApprove")
	{
		ostringstream	ostFinal;
		string			userSkillID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == viewProfile_SkillApprove: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::viewProfile_SkillApprove: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		userSkillID = indexPage.GetVarsHandler()->Get("id");

		if((userSkillID.length() > 0)) 
		{
			ostringstream	ost;
			int				affected;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userSkillID.c_str(), convertBuffer, sizeof(convertBuffer));
			userSkillID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `skill_confirmed` where `users_skill_id`=\"" << userSkillID << "\" and `approver_userID`=\"" << user.GetID() << "\";";
			if( !(affected = db.Query(ost.str())) )
			{
				unsigned long 		skill_confirmed_id;

				ost.str("");
				ost << "insert into `skill_confirmed` (`users_skill_id`, `approver_userID`) VALUE (\"" << userSkillID << "\", \"" << user.GetID() << "\");";
				skill_confirmed_id = db.InsertQuery(ost.str());
				if(skill_confirmed_id)
				{
					ost.str("");
					ost << "select * from `users_skill` where `id`=\"" << userSkillID << "\";";
					if(db.Query(ost.str()))
					{
						string		skillOwnerUserID = db.Get(0, "user_id");

						// --- Update live feed
						ost.str("");
						ost << "insert into `users_notification` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << skillOwnerUserID << "\", \"43\", \"" << skill_confirmed_id << "\", TIMESTAMPDIFF(second, \"1970-01-01\", NOW()))";
						if(db.InsertQuery(ost.str()))
						{
							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"success\"," << std::endl;
							ostFinal << "\"description\" : \"\"" << std::endl;
							ostFinal << "}" << std::endl;
						}
						else
						{

							{
								CLog			log;
								ostringstream	ostTmp;

								ostTmp.str("");
								ostTmp << "int main()::viewProfile_SkillApprove: error inserting into DB (" << ost.str() << ")";
								log.Write(ERROR, ostTmp.str());
							}

							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"error\"," << std::endl;
							ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
							ostFinal << "}" << std::endl;
						}
					}
					else
					{
						{
							CLog			log;
							ostringstream	ostTmp;

							ostTmp.str("");
							ostTmp << "int main()::viewProfile_SkillApprove: error finding skill in users_skill (" << ost.str() << ")";
							log.Write(ERROR, ostTmp.str());
						}

						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"error\"," << std::endl;
						ostFinal << "\"description\" : \"error finding skill\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
				}
				else
				{
					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::viewProfile_SkillApprove: error inserting into skill_confirmed DB (" << ost.str() << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error inserting into skill DB\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
			}
			else
			{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"," << std::endl;
					ostFinal << "\"description\" : \"already approved by this user\"" << std::endl;
					ostFinal << "}" << std::endl;
			}

		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::viewProfile_SkillApprove: userSkillID [" << userSkillID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"error parameters sent to server\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == viewProfile_SkillApprove: end");
		}
	}

	// --- viewProfile_SkillReject
	if(action == "viewProfile_SkillReject")
	{
		ostringstream	ostFinal;
		string			userSkillID;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == viewProfile_SkillReject: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::viewProfile_SkillReject: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		userSkillID = indexPage.GetVarsHandler()->Get("id");

		if((userSkillID.length() > 0)) 
		{
			ostringstream	ost;
			int				affected;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userSkillID.c_str(), convertBuffer, sizeof(convertBuffer));
			userSkillID = ConvertTextToHTML(convertBuffer);

			ost.str("");
			ost << "select * from `skill_confirmed` where `users_skill_id`=\"" << userSkillID << "\" and `approver_userID`=\"" << user.GetID() << "\";";
			if( (affected = db.Query(ost.str())) ) 
			{
				ost.str("");
				ost << "delete from `users_notification` where `actionTypeId`='43' and `actionId` in (select `id` from `skill_confirmed` where `users_skill_id`=\"" << userSkillID << "\" and `approver_userID`=\"" << user.GetID() << "\");";
				db.Query(ost.str());

				ost.str("");
				ost << "delete from `skill_confirmed` where `users_skill_id`=\"" << userSkillID << "\" and `approver_userID`=\"" << user.GetID() << "\";";
				db.Query(ost.str());

				ost.str("");
				ost << "select * from `users_skill` where `id`=\"" << userSkillID << "\";";
				if(db.Query(ost.str()))
				{
					string		skillOwnerUserID = db.Get(0, "user_id");

					// --- Update live feed
					ost.str("");
					ost << "insert into `users_notification` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << skillOwnerUserID << "\", \"44\", \"" << userSkillID << "\", TIMESTAMPDIFF(second, \"1970-01-01\", NOW()) );";
					if(db.InsertQuery(ost.str()))
					{
						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"success\"," << std::endl;
						ostFinal << "\"description\" : \"\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
					else
					{

						{
							CLog			log;
							ostringstream	ostTmp;

							ostTmp.str("");
							ostTmp << "int main()::viewProfile_SkillReject: error inserting into DB (" << ost.str() << ")";
							log.Write(ERROR, ostTmp.str());
						}

						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"error\"," << std::endl;
						ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
				}
				else
				{
					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::viewProfile_SkillReject: error finding skill in users_skill (" << ost.str() << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error finding skill\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
			}
			else
			{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"," << std::endl;
					ostFinal << "\"description\" : \"already rejected by this user\"" << std::endl;
					ostFinal << "}" << std::endl;
			}

		}
		else
		{
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::viewProfile_SkillReject: userSkillID [" << userSkillID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}

			ostFinal.str("");
			ostFinal << "{" << std::endl;
			ostFinal << "\"result\" : \"error\"," << std::endl;
			ostFinal << "\"description\" : \"error parameters sent to server\"" << std::endl;
			ostFinal << "}" << std::endl;
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == viewProfile_SkillReject: end");
		}
	}





	// --- AJAX_updateFirstName
	if(action == "AJAX_updateFirstName")
	{
		string			firstName;
		ostringstream	ostFinal;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_updateFirstName: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_updateFirstName: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		firstName = indexPage.GetVarsHandler()->Get("value");
		ostFinal.str("");

		if((firstName.length() > 0)) 
		{
			ostringstream	ost;
			char			firstName_cp1251_char[1024];
			string			firstName_cp1251;

			memset(firstName_cp1251_char, 0, sizeof(firstName_cp1251_char));
			convert_utf8_to_windows1251(firstName.c_str(), firstName_cp1251_char, sizeof(firstName_cp1251_char));
			firstName_cp1251 = firstName_cp1251_char;
			trim(firstName_cp1251);
			firstName_cp1251 = ReplaceDoubleQuoteToQuote(firstName_cp1251);

			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::AJAX_updateFirstName: name [" << firstName_cp1251 << "]";
				log.Write(DEBUG, ost.str());
			}

			ost.str("");
			ost << "update users set `name`=\"" << firstName_cp1251 << "\" where `id`='" << user.GetID() << "'";
			db.Query(ost.str());

			// --- Update live feed
			ost.str("");
			ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"5\", \"0\", NOW())";
			if(db.InsertQuery(ost.str()))
			{
				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"success\"," << std::endl;
				ostFinal << "\"description\" : \"\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
			else
			{

				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::AJAX_updateFirstName: error inserting into DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
		}
		else
		{
			ostringstream	ost;
			{
				CLog	log;

				ost.str("");
				ost << "int main()::AJAX_updateFirstName: firstName [" << firstName << "] is empty";
				log.Write(DEBUG, ost.str());
			}

			ost.str("");
			ost << "update users set `name`=\"\" where `id`='" << user.GetID() << "'";
			db.Query(ost.str());

			// --- Update live feed
			ost.str("");
			ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"6\", \"0\", NOW())";
			if(db.InsertQuery(ost.str()))
			{
				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"success\"," << std::endl;
				ostFinal << "\"description\" : \"\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
			else
			{

				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::AJAX_updateFirstName: error inserting into DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_updateFirstName: end");
		}

	}

	// --- AJAX_updateLastName
	if(action == "AJAX_updateLastName")
	{
		string			lastName;
		ostringstream	ostFinal;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_updateLastName: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_updateLastName: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		lastName = indexPage.GetVarsHandler()->Get("value");

		if((lastName.length() > 0)) 
		{
			ostringstream	ost;
			char			lastName_cp1251_char[1024];
			string			lastName_cp1251;

			memset(lastName_cp1251_char, 0, sizeof(lastName_cp1251_char));
			convert_utf8_to_windows1251(lastName.c_str(), lastName_cp1251_char, sizeof(lastName_cp1251_char));
			lastName_cp1251 = lastName_cp1251_char;
			trim(lastName_cp1251);
			lastName_cp1251 = ReplaceDoubleQuoteToQuote(lastName_cp1251);

			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::AJAX_updateLastName: name [" << lastName_cp1251 << "]";
				log.Write(DEBUG, ost.str());
			}

			ost.str("");
			ost << "update users set `nameLast`=\"" << lastName_cp1251 << "\" where `id`='" << user.GetID() << "'";
			db.Query(ost.str());

			// --- Update live feed
			ost.str("");
			ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"4\", \"0\", NOW())";
			if(db.InsertQuery(ost.str()))
			{
				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"success\"," << std::endl;
				ostFinal << "\"description\" : \"\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
			else
			{

				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::AJAX_updateFirstName: error inserting into DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
		}
		else
		{
			ostringstream	ost;
			{
				CLog	log;

				ost.str("");
				ost << "int main()::AJAX_updateLastName: lastName [" << lastName << "] is empty";
				log.Write(DEBUG, ost.str());
			}

			ost.str("");
			ost << "update users set `nameLast`=\"\" where `id`='" << user.GetID() << "'";
			db.Query(ost.str());

			// --- Update live feed
			ost.str("");
			ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"7\", \"0\", NOW())";
			if(db.InsertQuery(ost.str()))
			{
				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"success\"," << std::endl;
				ostFinal << "\"description\" : \"\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
			else
			{

				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::AJAX_updateLastName: error inserting into DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
				ostFinal << "}" << std::endl;
			}

		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_updateLastName: end");
		}
	}

	// --- AJAX_updateFirstLastName
	if(action == "AJAX_updateFirstLastName")
	{
		ostringstream	ostFinal;
		string			firstName, lastName;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_updateFirstLastName: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_updateFirstLastName: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		firstName = indexPage.GetVarsHandler()->Get("firstName");
		lastName = indexPage.GetVarsHandler()->Get("lastName");

		if((lastName.length() > 0)) 
		{
			ostringstream	ost;
			char			lastName_cp1251_char[1024];
			string			lastName_cp1251;

			memset(lastName_cp1251_char, 0, sizeof(lastName_cp1251_char));
			convert_utf8_to_windows1251(lastName.c_str(), lastName_cp1251_char, sizeof(lastName_cp1251_char));
			lastName_cp1251 = lastName_cp1251_char;
			trim(lastName_cp1251);
			lastName_cp1251 = ReplaceDoubleQuoteToQuote(lastName_cp1251);

			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::AJAX_updateFirstLastName: name [" << lastName_cp1251 << "]";
				log.Write(DEBUG, ost.str());
			}

			ost.str("");
			ost << "update users set `nameLast`=\"" << lastName_cp1251 << "\" where `id`='" << user.GetID() << "'";
			db.Query(ost.str());

			// --- Update live feed
			ost.str("");
			ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"4\", \"0\", NOW())";
			if(db.InsertQuery(ost.str()))
			{
			}
			else
			{
				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::AJAX_updateFirstLastName: error inserting into DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}
			}
		}
		else
		{
			ostringstream	ost;
			{
				CLog	log;

				ost.str("");
				ost << "int main()::AJAX_updateFirstLastName: lastName [" << lastName << "] is empty";
				log.Write(DEBUG, ost.str());
			}

			ost.str("");
			ost << "update users set `nameLast`=\"\" where `id`='" << user.GetID() << "'";
			db.Query(ost.str());

			// --- Update live feed
			ost.str("");
			ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"7\", \"0\", NOW())";
			if(db.InsertQuery(ost.str()))
			{
			}
			else
			{
				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::AJAX_updateFirstLastName: error inserting into DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}
			}

		}

		if((firstName.length() > 0)) 
		{
			ostringstream	ost;
			char			firstName_cp1251_char[1024];
			string			firstName_cp1251;

			memset(firstName_cp1251_char, 0, sizeof(firstName_cp1251_char));
			convert_utf8_to_windows1251(firstName.c_str(), firstName_cp1251_char, sizeof(firstName_cp1251_char));
			firstName_cp1251 = firstName_cp1251_char;
			trim(firstName_cp1251);
			firstName_cp1251 = ReplaceDoubleQuoteToQuote(firstName_cp1251);

			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::AJAX_updateFirstLastName: name [" << firstName_cp1251 << "]";
				log.Write(DEBUG, ost.str());
			}

			ost.str("");
			ost << "update users set `name`=\"" << firstName_cp1251 << "\" where `id`='" << user.GetID() << "'";
			db.Query(ost.str());

			// --- Update live feed
			ost.str("");
			ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"5\", \"0\", NOW())";
			if(db.InsertQuery(ost.str()))
			{
				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"success\"," << std::endl;
				ostFinal << "\"description\" : \"\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
			else
			{

				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::AJAX_updateFirstLastName: error inserting into DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
		}
		else
		{
			ostringstream	ost;
			{
				CLog	log;

				ost.str("");
				ost << "int main()::AJAX_updateFirstLastName: firstName [" << firstName << "] is empty";
				log.Write(DEBUG, ost.str());
			}

			ost.str("");
			ost << "update users set `name`=\"\" where `id`='" << user.GetID() << "'";
			db.Query(ost.str());

			// --- Update live feed
			ost.str("");
			ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"6\", \"0\", NOW())";
			if(db.InsertQuery(ost.str()))
			{
				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"success\"," << std::endl;
				ostFinal << "\"description\" : \"\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
			else
			{

				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::AJAX_updateFirstLastName: error inserting into DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
				ostFinal << "}" << std::endl;
			}


		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == AJAX_updateFirstLastName: end");
		}
	}

	// --- AJAX_update_occupation_start
	if(action == "update_occupation_start")
	{
		string		occupationStart, companyId;
		ostringstream	ostFinal;

		ostFinal.str("");

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::update_occupation_start: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		occupationStart = indexPage.GetVarsHandler()->Get("value");
		companyId = indexPage.GetVarsHandler()->Get("id");

		if((occupationStart.length() > 0) && (companyId.length() > 0)) 
		{
			ostringstream	ost;
			char			occupationStart_cp1251_char[1024];
			string			occupationStart_cp1251;

			memset(occupationStart_cp1251_char, 0, sizeof(occupationStart_cp1251_char));
			convert_utf8_to_windows1251(occupationStart.c_str(), occupationStart_cp1251_char, sizeof(occupationStart_cp1251_char));
			occupationStart_cp1251 = occupationStart_cp1251_char;
			trim(occupationStart_cp1251);
			occupationStart_cp1251 = ReplaceDoubleQuoteToQuote(occupationStart_cp1251);

			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::update_occupation_start: date [" << occupationStart_cp1251 << "]";
				log.Write(DEBUG, ost.str());
			}

			// occupationStart_cp1251 = CheckIfFurtherThanNow(occupationStart_cp1251);

			ost.str("");
			ost << "update `users_company` set `occupation_start`=\"" << occupationStart_cp1251 << "\" where `id`='" << companyId << "'";
			db.Query(ost.str());

			if(!db.isError())
			{
				// --- Update live feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"2\", \"" << companyId << "\", NOW())";
				if(db.InsertQuery(ost.str()))
				{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{

					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::update_occupation_start: ERROR inserting into feed DB (" << ost.str() << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
			}
			else
			{
				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::update_occupation_start: ERROR updating DB (" << db.isError() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error updating occupation start in compay\"" << std::endl;
				ostFinal << "}" << std::endl;

			}

		}
		else
		{
			ostFinal << "{\"result\":\"error\", \"description\":\"issue with input parameters\"}";
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::update_occupation_start: occupationStart [" << occupationStart << "] or companyId [" << companyId << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == update_occupation_start: end");
		}
	}

	// --- updateSchoolOccupationStart
	if(action == "updateSchoolOccupationStart")
	{
		string		occupationStart, userSchoolID;
		ostringstream	ostFinal;

		ostFinal.str("");

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateSchoolOccupationStart: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		occupationStart = indexPage.GetVarsHandler()->Get("value");
		userSchoolID = indexPage.GetVarsHandler()->Get("id");

		if((occupationStart.length() > 0) && (userSchoolID.length() > 0)) 
		{
			ostringstream	ost;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(occupationStart.c_str(), convertBuffer, sizeof(convertBuffer));
			occupationStart = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userSchoolID.c_str(), convertBuffer, sizeof(convertBuffer));
			userSchoolID = ConvertTextToHTML(convertBuffer);


			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateSchoolOccupationStart: date [" << occupationStart << "]";
				log.Write(DEBUG, ost.str());
			}


			ost.str("");
			ost << "select * from `users_school` where `user_id`=\"" << user.GetID() << "\" and `id`=\"" << userSchoolID << "\";";
			if(db.Query(ost.str()))
			{
				if(atoi(occupationStart.c_str()) <= atoi(db.Get(0, "occupation_finish")))
				{

					ost.str("");
					ost << "update `users_school` set `occupation_start`=\"" << occupationStart << "\" where `id`='" << userSchoolID << "'";
					db.Query(ost.str());

					if(!db.isError())
					{
						// --- Update live feed
						ost.str("");
						ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"35\", \"" << userSchoolID << "\", NOW())";
						if(db.InsertQuery(ost.str()))
						{
							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"success\"" << std::endl;
							ostFinal << "}" << std::endl;
						}
						else
						{

							{
								CLog			log;
								ostringstream	ostTmp;

								ostTmp.str("");
								ostTmp << "int main()::updateSchoolOccupationStart: ERROR inserting into feed DB (" << ost.str() << ")";
								log.Write(ERROR, ostTmp.str());
							}

							ostFinal.str("");
							ostFinal << "{" << std::endl;
							ostFinal << "\"result\" : \"error\"," << std::endl;
							ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
							ostFinal << "}" << std::endl;
						}
					}
					else
					{
						{
							CLog			log;
							ostringstream	ostTmp;

							ostTmp.str("");
							ostTmp << "int main()::updateSchoolOccupationStart: ERROR updating DB (" << db.isError() << ")";
							log.Write(ERROR, ostTmp.str());
						}

						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"error\"," << std::endl;
						ostFinal << "\"description\" : \"error updating occupation start in compay\"" << std::endl;
						ostFinal << "}" << std::endl;

					}

				}
				else
				{
					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateSchoolOccupationStart: updating period because of error in start (" << occupationStart << ") and finish dates";
						log.Write(DEBUG, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error start and finish date\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				
			}
			else
			{
				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::updateSchoolOccupationStart: ERROR finding user school entry in DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error finding user school entry in DB\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
		}
		else
		{
			ostFinal << "{\"result\":\"error\", \"description\":\"issue with input parameters\"}";
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateSchoolOccupationStart: occupationStart [" << occupationStart << "] or userSchoolID [" << userSchoolID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateSchoolOccupationStart: end");
		}
	}

	// --- updateSchoolOccupationFinish
	if(action == "updateSchoolOccupationFinish")
	{
		string		occupationFinish, userSchoolID;
		ostringstream	ostFinal;

		ostFinal.str("");

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateSchoolOccupationFinish: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		occupationFinish = indexPage.GetVarsHandler()->Get("value");
		userSchoolID = indexPage.GetVarsHandler()->Get("id");

		if((occupationFinish.length() > 0) && (userSchoolID.length() > 0)) 
		{
			ostringstream	ost;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(occupationFinish.c_str(), convertBuffer, sizeof(convertBuffer));
			occupationFinish = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userSchoolID.c_str(), convertBuffer, sizeof(convertBuffer));
			userSchoolID = ConvertTextToHTML(convertBuffer);


			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateSchoolOccupationFinish: date [" << occupationFinish << "]";
				log.Write(DEBUG, ost.str());
			}


			ost.str("");
			ost << "select * from `users_school` where `user_id`=\"" << user.GetID() << "\" and `id`=\"" << userSchoolID << "\";";
			if(db.Query(ost.str()))
			{
				if(atoi(db.Get(0, "occupation_start")) <= atoi(occupationFinish.c_str()))
				{

					ost.str("");
					ost << "update `users_school` set `occupation_finish`=\"" << occupationFinish << "\" where `id`='" << userSchoolID << "'";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"36\", \"" << userSchoolID << "\", NOW())";
					if(db.InsertQuery(ost.str()))
					{
						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"success\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
					else
					{

						{
							CLog			log;
							ostringstream	ostTmp;

							ostTmp.str("");
							ostTmp << "int main()::updateSchoolOccupationFinish: ERROR inserting into feed DB (" << ost.str() << ")";
							log.Write(ERROR, ostTmp.str());
						}

						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"error\"," << std::endl;
						ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
				}
				else
				{
					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateSchoolOccupationFinish: updating period because of error in start and finish (" << occupationFinish << ") dates";
						log.Write(DEBUG, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error start and finish date\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				
			}
			else
			{
				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::updateSchoolOccupationFinish: ERROR finding user school entry in DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error finding user school entry in DB\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
		}
		else
		{
			ostFinal << "{\"result\":\"error\", \"description\":\"issue with input parameters\"}";
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateSchoolOccupationFinish: occupationFinish [" << occupationFinish << "] or userSchoolID [" << userSchoolID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateSchoolOccupationFinish: end");
		}
	}

	// --- updateUniversityOccupationStart
	if(action == "updateUniversityOccupationStart")
	{
		string		occupationStart, userUniversityID;
		ostringstream	ostFinal;

		ostFinal.str("");

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateUniversityOccupationStart: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		occupationStart = indexPage.GetVarsHandler()->Get("value");
		userUniversityID = indexPage.GetVarsHandler()->Get("id");

		if((occupationStart.length() > 0) && (userUniversityID.length() > 0)) 
		{
			ostringstream	ost;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(occupationStart.c_str(), convertBuffer, sizeof(convertBuffer));
			occupationStart = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userUniversityID.c_str(), convertBuffer, sizeof(convertBuffer));
			userUniversityID = ConvertTextToHTML(convertBuffer);


			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateUniversityOccupationStart: date [" << occupationStart << "]";
				log.Write(DEBUG, ost.str());
			}


			ost.str("");
			ost << "select * from `users_university` where `user_id`=\"" << user.GetID() << "\" and `id`=\"" << userUniversityID << "\";";
			if(db.Query(ost.str()))
			{
				if(atoi(occupationStart.c_str()) <= atoi(db.Get(0, "occupation_finish")))
				{

					ost.str("");
					ost << "update `users_university` set `occupation_start`=\"" << occupationStart << "\" where `id`='" << userUniversityID << "'";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"37\", \"" << userUniversityID << "\", NOW())";
					if(db.InsertQuery(ost.str()))
					{
						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"success\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
					else
					{

						{
							CLog			log;
							ostringstream	ostTmp;

							ostTmp.str("");
							ostTmp << "int main()::updateUniversityOccupationStart: ERROR inserting into feed DB (" << ost.str() << ")";
							log.Write(ERROR, ostTmp.str());
						}

						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"error\"," << std::endl;
						ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
				}
				else
				{
					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateUniversityOccupationStart: updating period because of error in start (" << occupationStart << ") and finish dates";
						log.Write(DEBUG, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error start and finish date\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				
			}
			else
			{
				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::updateUniversityOccupationStart: ERROR finding user university entry in DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error finding user university entry in DB\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
		}
		else
		{
			ostFinal << "{\"result\":\"error\", \"description\":\"issue with input parameters\"}";
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateUniversityOccupationStart: occupationStart [" << occupationStart << "] or userUniversityID [" << userUniversityID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateUniversityOccupationStart: end");
		}
	}

	// --- updateUniversityOccupationFinish
	if(action == "updateUniversityOccupationFinish")
	{
		string		occupationFinish, userUniversityID;
		ostringstream	ostFinal;

		ostFinal.str("");

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateUniversityOccupationFinish: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		occupationFinish = indexPage.GetVarsHandler()->Get("value");
		userUniversityID = indexPage.GetVarsHandler()->Get("id");

		if((occupationFinish.length() > 0) && (userUniversityID.length() > 0)) 
		{
			ostringstream	ost;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(occupationFinish.c_str(), convertBuffer, sizeof(convertBuffer));
			occupationFinish = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userUniversityID.c_str(), convertBuffer, sizeof(convertBuffer));
			userUniversityID = ConvertTextToHTML(convertBuffer);


			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateUniversityOccupationFinish: date [" << occupationFinish << "]";
				log.Write(DEBUG, ost.str());
			}


			ost.str("");
			ost << "select * from `users_university` where `user_id`=\"" << user.GetID() << "\" and `id`=\"" << userUniversityID << "\";";
			if(db.Query(ost.str()))
			{
				if(atoi(db.Get(0, "occupation_start")) <= atoi(occupationFinish.c_str()))
				{

					ost.str("");
					ost << "update `users_university` set `occupation_finish`=\"" << occupationFinish << "\" where `id`='" << userUniversityID << "'";
					db.Query(ost.str());

					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"38\", \"" << userUniversityID << "\", NOW())";
					if(db.InsertQuery(ost.str()))
					{
						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"success\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
					else
					{

						{
							CLog			log;
							ostringstream	ostTmp;

							ostTmp.str("");
							ostTmp << "int main()::updateUniversityOccupationFinish: ERROR inserting into feed DB (" << ost.str() << ")";
							log.Write(ERROR, ostTmp.str());
						}

						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"error\"," << std::endl;
						ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
				}
				else
				{
					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateUniversityOccupationFinish: updating period because of error in start and finish (" << occupationFinish << ") dates";
						log.Write(DEBUG, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error start and finish date\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				
			}
			else
			{
				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::updateUniversityOccupationFinish: ERROR finding user university entry in DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error finding user university entry in DB\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
		}
		else
		{
			ostFinal << "{\"result\":\"error\", \"description\":\"issue with input parameters\"}";
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateUniversityOccupationFinish: occupationFinish [" << occupationFinish << "] or userUniversityID [" << userUniversityID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateUniversityOccupationFinish: end");
		}
	}

	// --- AJAX_update_occupation_finish
	if(action == "update_occupation_finish")
	{
		string		occupationFinish, companyId;
		ostringstream	ostFinal;

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::update_occupation_start: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		occupationFinish = indexPage.GetVarsHandler()->Get("value");
		companyId = indexPage.GetVarsHandler()->Get("id");
		ostFinal.str("");

		if((occupationFinish.length() > 0) && (companyId.length() > 0)) 
		{
			ostringstream	ost;
			char			occupationFinish_cp1251_char[1024];
			string			occupationFinish_cp1251;

			memset(occupationFinish_cp1251_char, 0, sizeof(occupationFinish_cp1251_char));
			convert_utf8_to_windows1251(occupationFinish.c_str(), occupationFinish_cp1251_char, sizeof(occupationFinish_cp1251_char));
			occupationFinish_cp1251 = occupationFinish_cp1251_char;
			trim(occupationFinish_cp1251);
			occupationFinish_cp1251 = ReplaceDoubleQuoteToQuote(occupationFinish_cp1251);

			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::update_occupation_finish: date [" << occupationFinish_cp1251 << "]";
				log.Write(DEBUG, ost.str());
			}

			// occupationFinish_cp1251 = CheckIfFurtherThanNow(occupationFinish_cp1251);

			ost.str("");
			ost << "update users_company set `occupation_finish`=\"" << occupationFinish_cp1251 << "\" where `id`='" << companyId << "'";
			db.Query(ost.str());
			if(!db.isError())
			{
				// --- Update live feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"2\", \"" << companyId << "\", NOW())";
				if(db.InsertQuery(ost.str()))
				{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{

					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::update_occupation_finish: ERROR inserting into feed DB (" << ost.str() << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
			}
			else
			{
				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::update_occupation_finish: ERROR updating DB (" << db.isError() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error updating occupation finish in compay\"" << std::endl;
				ostFinal << "}" << std::endl;

			}

		}
		else
		{
			ostFinal << "{\"result\":\"error\", \"description\":\"issue with input parameters\"}";
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::update_occupation_finish: occupationFinish [" << occupationFinish << "] or companyId [" << companyId << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}

		}


		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == update_occupation_finish: end");
		}
	}

	// --- updateUniversityDegree
	if(action == "updateUniversityDegree")
	{
		string		universityDegree, userUniversityID;
		ostringstream	ostFinal;

		ostFinal.str("");

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateUniversityDegree: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		universityDegree = indexPage.GetVarsHandler()->Get("value");
		userUniversityID = indexPage.GetVarsHandler()->Get("id");

		if((universityDegree.length() > 0) && (userUniversityID.length() > 0)) 
		{
			ostringstream	ost;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(universityDegree.c_str(), convertBuffer, sizeof(convertBuffer));
			universityDegree = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userUniversityID.c_str(), convertBuffer, sizeof(convertBuffer));
			userUniversityID = ConvertTextToHTML(convertBuffer);


			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateUniversityDegree: degree [" << universityDegree << "]";
				log.Write(DEBUG, ost.str());
			}


			ost.str("");
			ost << "select * from `users_university` where `user_id`=\"" << user.GetID() << "\" and `id`=\"" << userUniversityID << "\";";
			if(db.Query(ost.str()))
			{
				ost.str("");
				ost << "update `users_university` set `degree`=\"" << universityDegree << "\" where `id`='" << userUniversityID << "'";
				db.Query(ost.str());

				// --- Update live feed
				ost.str("");
				ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"39\", \"" << userUniversityID << "\", NOW())";
				if(db.InsertQuery(ost.str()))
				{
					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"success\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
				else
				{

					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateUniversityDegree: ERROR inserting into feed DB (" << ost.str() << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
					ostFinal << "}" << std::endl;
				}
			}
			else
			{
				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::updateUniversityDegree: ERROR finding user university entry in DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error finding user university entry in DB\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
		}
		else
		{
			ostFinal << "{\"result\":\"error\", \"description\":\"issue with input parameters\"}";
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateUniversityDegree: universityDegree [" << universityDegree << "] or userUniversityID [" << userUniversityID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateUniversityDegree: end");
		}
	}

	// --- updateLanguageLevel
	if(action == "updateLanguageLevel")
	{
		string		languageLevel, userLanguageID;
		ostringstream	ostFinal;

		ostFinal.str("");

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::updateLanguageLevel: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		languageLevel = indexPage.GetVarsHandler()->Get("value");
		userLanguageID = indexPage.GetVarsHandler()->Get("id");

		if((languageLevel.length() > 0) && (userLanguageID.length() > 0)) 
		{
			ostringstream	ost;
			char			convertBuffer[1024];

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(languageLevel.c_str(), convertBuffer, sizeof(convertBuffer));
			languageLevel = ConvertTextToHTML(convertBuffer);

			memset(convertBuffer, 0, sizeof(convertBuffer));
			convert_utf8_to_windows1251(userLanguageID.c_str(), convertBuffer, sizeof(convertBuffer));
			userLanguageID = ConvertTextToHTML(convertBuffer);


			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateLanguageLevel: level [" << languageLevel << "]";
				log.Write(DEBUG, ost.str());
			}


			ost.str("");
			ost << "select * from `users_language` where `user_id`=\"" << user.GetID() << "\" and `id`=\"" << userLanguageID << "\";";
			if(db.Query(ost.str()))
			{
				ost.str("");
				ost << "update `users_language` set `level`=\"" << languageLevel << "\" where `id`='" << userLanguageID << "'";
				db.Query(ost.str());
				if(!db.isError())
				{
					// --- Update live feed
					ost.str("");
					ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"40\", \"" << userLanguageID << "\", NOW())";
					if(db.InsertQuery(ost.str()))
					{
						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"success\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
					else
					{

						{
							CLog			log;
							ostringstream	ostTmp;

							ostTmp.str("");
							ostTmp << "int main()::updateLanguageLevel: ERROR inserting into feed DB (" << ost.str() << ")";
							log.Write(ERROR, ostTmp.str());
						}

						ostFinal.str("");
						ostFinal << "{" << std::endl;
						ostFinal << "\"result\" : \"error\"," << std::endl;
						ostFinal << "\"description\" : \"error inserting into user feed\"" << std::endl;
						ostFinal << "}" << std::endl;
					}
				}
				else
				{
					{
						CLog			log;
						ostringstream	ostTmp;

						ostTmp.str("");
						ostTmp << "int main()::updateLanguageLevel: ERROR updating DB (" << db.isError() << ")";
						log.Write(ERROR, ostTmp.str());
					}

					ostFinal.str("");
					ostFinal << "{" << std::endl;
					ostFinal << "\"result\" : \"error\"," << std::endl;
					ostFinal << "\"description\" : \"error updating language degree\"" << std::endl;
					ostFinal << "}" << std::endl;

				}

			}
			else
			{
				{
					CLog			log;
					ostringstream	ostTmp;

					ostTmp.str("");
					ostTmp << "int main()::updateLanguageLevel: ERROR finding user language entry in DB (" << ost.str() << ")";
					log.Write(ERROR, ostTmp.str());
				}

				ostFinal.str("");
				ostFinal << "{" << std::endl;
				ostFinal << "\"result\" : \"error\"," << std::endl;
				ostFinal << "\"description\" : \"error finding user language entry in DB\"" << std::endl;
				ostFinal << "}" << std::endl;
			}
		}
		else
		{
			ostFinal << "{\"result\":\"error\", \"description\":\"issue with input parameters\"}";
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::updateLanguageLevel: languageLevel [" << languageLevel << "] or userLanguageID [" << userLanguageID << "] is unknown/empty";
				log.Write(DEBUG, ost.str());
			}
		}

		indexPage.RegisterVariableForce("result", ostFinal.str());

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file json_response.htmlt was missing");
			throw CException("Template file was missing");
		}

		{
			CLog	log;
			log.Write(DEBUG, "int main(void):action == updateLanguageLevel: end");
		}
	}



	// --- AJAX_updateActiveAvatar
	if(action == "AJAX_updateActiveAvatar")
	{
		string		avatarID, companyId;

		{
			CLog	log;
			log.Write(DEBUG, "main()::AJAX_updateActiveAvatar: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::AJAX_updateActiveAvatar: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		avatarID = indexPage.GetVarsHandler()->Get("id");

		if((avatarID.length() > 0)) 
		{
			ostringstream	ost;
			char			avatarID_cp1251_char[1024];
			string			avatarID_cp1251;

			memset(avatarID_cp1251_char, 0, sizeof(avatarID_cp1251_char));
			convert_utf8_to_windows1251(avatarID.c_str(), avatarID_cp1251_char, sizeof(avatarID_cp1251_char));
			avatarID_cp1251 = avatarID_cp1251_char;
			trim(avatarID_cp1251);
			avatarID_cp1251 = ReplaceDoubleQuoteToQuote(avatarID_cp1251);

			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main()::AJAX_updateActiveAvatar: new avatar id [" << avatarID_cp1251 << "]";
				log.Write(DEBUG, ost.str());
			}

			ost.str("");
			ost << "update `users_avatars` set `isActive`=\"0\" where `userid`='" << user.GetID() << "';";
			db.Query(ost.str());

			ost.str("");
			ost << "update `users_avatars` set `isActive`=\"1\" where `id`=\"" << avatarID_cp1251 << "\" and `userid`='" << user.GetID() << "';";
			db.Query(ost.str());

			// --- Update live feed
			ost.str("");
			ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"8\", \"" << avatarID_cp1251 << "\", NOW())";
			db.Query(ost.str());
		}
		else
		{
			ostringstream	ost;
			{
				CLog	log;

				ost.str("");
				ost << "int main()::AJAX_updateActiveAvatar: avatarID [" << avatarID << "] is empty";
				log.Write(DEBUG, ost.str());
			}

			ost.str("");
			ost << "update users set `name`=\"\" where `id`='" << user.GetID() << "'";
			db.Query(ost.str());

			// --- Update live feed
			ost.str("");
			ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"6\", \"" << companyId << "\", NOW())";
			db.Query(ost.str());

		}
		{
			CLog	log;
			log.Write(DEBUG, "main()::AJAX_updateActiveAvatar: finish");
		}
	}

	// --- !!! IMPORTANT !!!
	// --- action _news_feed_ MUST BE BELOW action _login_user_
	if(action == "news_feed")
	{
		ostringstream	ost;
		string			strPageToGet, strNewsOnSinglePage;

		{
			CLog	log;
			log.Write(DEBUG, "main()::news_feed: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::news_feed: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}	

		strNewsOnSinglePage	= indexPage.GetVarsHandler()->Get("NewsOnSinglePage");
		strPageToGet 		= indexPage.GetVarsHandler()->Get("page");
		if(strPageToGet.empty()) strPageToGet = "0";
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == news_feed: page ", strPageToGet, " requested");
		}

		ost.str("");
		ost << "select * from `users_avatars` where `userID`=\"" << user.GetID() << "\" and `isActive`=\"1\";";
		if(db.Query(ost.str()))
		{
			ost.str("");
			ost << "/images/avatars/avatars" << db.Get(0, "folder") << "/" << db.Get(0, "filename");
			indexPage.RegisterVariableForce("myUserAvatar", ost.str());
		}
		indexPage.RegisterVariableForce("myFirstName", user.GetName());
		indexPage.RegisterVariableForce("myLastName", user.GetNameLast());

		if(!indexPage.SetTemplate("news_feed.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == news_feed: ERROR can't find template news_feed.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("news_feed.htmlt"))

		{
			CLog	log;
			log.Write(DEBUG, "main()::news_feed: finish");
		}

	}

	if(action == "find_friends")
	{
		ostringstream	ost;
		string			strPageToGet, strFriendsOnSinglePage, searchText;

		{
			CLog	log;
			log.Write(DEBUG, "main()::find_friends: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::find_friends: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		indexPage.RegisterVariableForce("title_head", "ѕоиск друзей");
		strFriendsOnSinglePage	= indexPage.GetVarsHandler()->Get("FriendsOnSinglePage");
		strPageToGet 			= indexPage.GetVarsHandler()->Get("page");
		if(strPageToGet.empty()) strPageToGet = "0";
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == find_friends: page ", strPageToGet, " requested");
		}

		indexPage.RegisterVariableForce("myFirstName", user.GetName());
		indexPage.RegisterVariableForce("myLastName", user.GetNameLast());


		if(!indexPage.SetTemplate("find_friends.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == find_friends: ERROR can't find template find_friends.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("find_friends.htmlt"))

		{
			CLog	log;
			log.Write(DEBUG, "main()::find_friends: finish");
		}
	}

	if((action == "my_network") || (action == "who_watched_on_me"))
	{
		ostringstream	ost;
		string			strPageToGet, strFriendsOnSinglePage;

		{
			CLog	log;
			log.Write(DEBUG, "main()::my_network: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::my_network: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		if(action == "my_network") indexPage.RegisterVariableForce("title_head", "ћои друзь€");
		if(action == "who_watched_on_me") indexPage.RegisterVariableForce("title_head", " то просматривал мой профиль");
		if(action == "companies_i_own_list") indexPage.RegisterVariableForce("title_head", "ћои компании");

		strFriendsOnSinglePage	= indexPage.GetVarsHandler()->Get("FriendsOnSinglePage");
		strPageToGet 			= indexPage.GetVarsHandler()->Get("page");
		if(strPageToGet.empty()) strPageToGet = "0";
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == my_network: page ", strPageToGet, " requested");
		}

		indexPage.RegisterVariableForce("myFirstName", user.GetName());
		indexPage.RegisterVariableForce("myLastName", user.GetNameLast());


		if(!indexPage.SetTemplate("my_network.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == my_network: ERROR can't find template my_network.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("my_network.htmlt"))

		{
			CLog	log;
			log.Write(DEBUG, "main()::my_network: finish");
		}
	}

	if(action == "companies_i_own_list")
	{
		ostringstream	ost;
		string			strPageToGet, strFriendsOnSinglePage;

		{
			CLog	log;
			log.Write(DEBUG, "main()::companies_i_own_list: start");
		}

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::my_network: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		indexPage.RegisterVariableForce("title_head", "ћои компании");

		strFriendsOnSinglePage	= indexPage.GetVarsHandler()->Get("FriendsOnSinglePage");
		strPageToGet 			= indexPage.GetVarsHandler()->Get("page");
		if(strPageToGet.empty()) strPageToGet = "0";
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == my_network: page ", strPageToGet, " requested");
		}

		indexPage.RegisterVariableForce("myFirstName", user.GetName());
		indexPage.RegisterVariableForce("myLastName", user.GetNameLast());


		if(!indexPage.SetTemplate("companies_i_own_list.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == my_network: ERROR can't find template my_network.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("my_network.htmlt"))

		{
			CLog	log;
			log.Write(DEBUG, "main()::companies_i_own_list: finish");
		}
	}

	if(action == "find_friends")
	{
		ostringstream	ost;
		string			strPageToGet, strFriendsOnSinglePage, searchText;

		if(user.GetLogin() == "Guest")
		{
			ostringstream	ost;

			{
				CLog	log;
				log.Write(DEBUG, "int main()::find_friends: re-login required");
			}

			ost.str("");
			ost << "/?rand=" << GetRandom(10);
			indexPage.Redirect(ost.str().c_str());
		}

		indexPage.RegisterVariableForce("title_head", "ѕоиск друзей");
		strFriendsOnSinglePage	= indexPage.GetVarsHandler()->Get("FriendsOnSinglePage");
		strPageToGet 			= indexPage.GetVarsHandler()->Get("page");
		if(strPageToGet.empty()) strPageToGet = "0";
		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == find_friends: page ", strPageToGet, " requested");
		}

		indexPage.RegisterVariableForce("myFirstName", user.GetName());
		indexPage.RegisterVariableForce("myLastName", user.GetNameLast());


		if(!indexPage.SetTemplate("find_friends.htmlt"))
		{
			CLog	log;
			log.Write(ERROR, "int main(void): action == find_friends: ERROR can't find template find_friends.htmlt");
			throw CExceptionHTML("user not activated");
		} // if(!indexPage.SetTemplate("find_friends.htmlt"))
	}

/*
	if(action == "submit_editinfo")
	{
		ostringstream	ost;
		int		affected;
		string		name, nameLast, age, description, pass, address, phone, email, isMale;

		if(indexPage.GetVarsHandler()->Get("loginUser").length() <= 0)
		{
			CLog	log;

			log.Write(WARNING, indexPage.GetVarsHandler()->Get("loginUser"), " try to fake system");

			indexPage.Redirect("/");
		}
		ost << "select * from   users where login='" << indexPage.GetVarsHandler()->Get("loginUser") << "'";
		affected = db.Query(ost.str());
		if(affected <= 0)
		{
			CLog	log;

			log.Write(ERROR, "there is no user ", indexPage.GetVarsHandler()->Get("loginUser"));
			throw CExceptionHTML("no user");
		}

		pass = RemoveQuotas(indexPage.GetVarsHandler()->Get("pass"));
		phone = RemoveQuotas(indexPage.GetVarsHandler()->Get("phone"));
		name = RemoveQuotas(indexPage.GetVarsHandler()->Get("name"));
		nameLast = RemoveQuotas(indexPage.GetVarsHandler()->Get("nameLast"));
		email = RemoveQuotas(indexPage.GetVarsHandler()->Get("email"));
		address = RemoveQuotas(indexPage.GetVarsHandler()->Get("address"));
		description = RemoveQuotas(indexPage.GetVarsHandler()->Get("description"));

		ost.str("");
		ost << "update `users` set `passwd`=\"" << pass << "\", `name`=\"" << name << "\", `nameLast`=\"" << nameLast << "\", `email`=\"" << email << "\", `address`=\"" << address << "\", `phone`=\"" << phone << "\", `description`=\"" << description << "\" where `login`=\"" << indexPage.GetVarsHandler()->Get("loginUser") << "\"";
		db.Query(ost.str());
		indexPage.RegisterVariableForce("content", "?зменение информации прошло успешно !");
	}
	if(action == "addpersonalphoto")
	{
		ostringstream	ost;
		int		affected;
		string		name, nameLast, age, description, pass, club, phone, email;

		if(indexPage.GetVarsHandler()->Get("loginUser").length() <= 0)
		{
			CLog	log;

			log.Write(WARNING, indexPage.GetVarsHandler()->Get("loginUser"), " try to fake system");

			indexPage.Redirect("/");
		}

		ost.str("");
		ost << "select * from `users_photo` where `user`='" << indexPage.GetVarsHandler()->Get("loginUser") << "'";
		affected = db.Query(ost.str());
{
	CLog	log;
	ostringstream	ost;
	ost << affected << " == " << MAX_PHOTO;
	log.Write(DEBUG, "- ", ost.str());
}
		if(affected < MAX_PHOTO)
		{
			ost.str("");
			ost << "select * from `users` where `login`='" << indexPage.GetVarsHandler()->Get("loginUser") << "'";
			affected = db.Query(ost.str());
			if(affected <= 0)
			{
				CLog	log;
	
				log.Write(ERROR, "there is no user ", indexPage.GetVarsHandler()->Get("loginUser"));
				throw CExceptionHTML("no user");
			}
	
			name = db.Get(0, "name"); indexPage.RegisterVariableForce("name", name);
			nameLast = db.Get(0, "nameLast"); indexPage.RegisterVariableForce("nameLast", nameLast);
			description = db.Get(0, "description"); indexPage.RegisterVariableForce("description", description);
			pass = db.Get(0, "passwd"); indexPage.RegisterVariableForce("pass", pass);
			age = db.Get(0, "age"); indexPage.RegisterVariableForce("age", age);
			club = db.Get(0, "club"); indexPage.RegisterVariableForce("club", club);
			phone = db.Get(0, "phone"); indexPage.RegisterVariableForce("phone", phone);
			email = db.Get(0, "email"); indexPage.RegisterVariableForce("email", email);
	
			if(!indexPage.SetTemplate("addphoto.htmlt"))
			{
				CLog    log;
	
				log.Write(ERROR, "template file editinfo.htmlt was missing");
				throw CException("Template file editinfo.htmlt was missing");
			}
		}
		else
		{
			ostringstream	ost;

			ost << "¬ы загрузили максимально возможное количество фотографий. ƒл€ того что-бы загрузить новую фотографию, вам необходимо удалить старую";
			ost << "¬ы можете вернутьс€ к <a href=\"/editpersonalinfo?rand=" << GetRandom(10) << "\">изменению личных параметров</a>.<br>";

			indexPage.RegisterVariableForce("content", ost.str());
		}
		indexPage.RegisterVariableForce("title", "ƒобавление фотографий");
	}
	if(action == "submitpersonalphoto")
	{
		ostringstream	ost;
		int		affected;
		string		fileName = indexPage.GetFilesHandler()->GetName(0);
		string		modifyFileName;
		FILE		*f;

		if(indexPage.GetVarsHandler()->Get("loginUser").length() <= 0)
		{
			CLog	log;

			log.Write(WARNING, indexPage.GetVarsHandler()->Get("loginUser"), " try to fake system");

			indexPage.Redirect("/");
		}
		ost << "select * from users where login='" << indexPage.GetVarsHandler()->Get("loginUser") << "'";
		affected = db.Query(ost.str());
		if(affected <= 0)
		{
			CLog	log;

			log.Write(ERROR, "there is no user ", indexPage.GetVarsHandler()->Get("loginUser"));
			throw CExceptionHTML("no user");
		}

		if(!fileName.empty())
		{
			modifyFileName = ModifyFileName(fileName, indexPage.GetVarsHandler()->Get("loginUser"));
			if(modifyFileName.length() > 1)
			{
				fileName = IMAGE_PHOTO_DIRECTORY + modifyFileName;
				f = fopen(fileName.c_str(), "w");
				if(f == NULL)
				{
					{
						CLog	log;
						log.Write(ERROR, "error writing file:", fileName.c_str());
					}
					throw CException("error writing file into server");
				}
				if((indexPage.GetFilesHandler()->Get(0) != NULL) && (indexPage.GetFilesHandler()->GetSize(0) > 0))
				{
					fwrite(indexPage.GetFilesHandler()->Get(0), indexPage.GetFilesHandler()->GetSize(0), 1, f);
				}
				fclose(f);

				ost.str("");
				ost << "insert into `users_photo` (`user`, `file`, `isActive`) VALUES ('" << indexPage.GetVarsHandler()->Get("loginUser") << "', '" << modifyFileName << "', '0')";
				db.Query(ost.str());

				ost.str("");
				ost << "¬аша фотографи€ успешно добавлена.<br><img src=\"/images/photo/" << modifyFileName << "\">";
				ost << "¬ы можете вернутьс€ к <a href=\"/editpersonalinfo?rand=" << GetRandom(10) << "\">изменению личных параметров</a>.<br>";
				indexPage.RegisterVariable("content", ost.str());
			}
			else
			{
				indexPage.RegisterVariable("content", "?м€ фотографии некорректное, им€ фаила должно содержать только английские буквы и цифры.");
			}
		}
		else
		{
			{
				CLog	log;
				log.Write(ERROR, "You forget to fill the field 'name of picture on server'");
			}
			throw CException("You forget to fill the field 'name of picture on server'");
		}
	}
	if(action == "set_active_picture")
	{
		ostringstream	ost;
		int		affected;
		string		pic;

		if(indexPage.GetVarsHandler()->Get("loginUser").length() <= 0)
		{
			CLog	log;

			log.Write(WARNING, indexPage.GetVarsHandler()->Get("loginUser"), " try to fake system");

			indexPage.Redirect("/");
		}
		ost << "select * from users where  login='" << indexPage.GetVarsHandler()->Get("loginUser") << "'";
		affected = db.Query(ost.str());
		if(affected <= 0)
		{
			CLog	log;

			log.Write(ERROR, "there is no user ", indexPage.GetVarsHandler()->Get("loginUser"));
			throw CExceptionHTML("no user");
		}

		pic = indexPage.GetVarsHandler()->Get("pic");

		if(pic.length() > 5)
		{
			ost.str("");
			ost << "select * from competitions where `isshow`='y'";
			affected = db.Query(ost.str());

			ost.str("");
			ost << "¬ыберите конкурс в котором будет участвовать эта фотографи€.<br>";
			for(int i = 0; i < affected; i++)
			{
				ost << "<a href='/choosecompetit&competit=" << db.Get(i, "id") << "&pic=" << pic << "&rand=" << GetRandom(10) <<"'>" << db.Get(i, "name") << "</a><br>";
			}

			ost << "<img width=300 src=/images/photo/" << pic << "><br>";
			ost << "¬ы можете вернутьс€ к <a href=\"/editpersonalinfo&rand=" << GetRandom(10) << "\">изменению личных параметров</a>.<br>";
		}
		else
		{
			ost.str("");
			ost << "ќшибка регистрации фотографии.<br>";
			ost << "¬ы можете вернутьс€ к <a href=\"/editpersonalinfo&rand=" << GetRandom(10) << "\">изменению личных параметров</a>.<br>";
		}
		indexPage.RegisterVariableForce("content", ost.str());
	}
	if(action == "chamge_competit_photo")
	{
		ostringstream	ost;
		int		affected;
		string		pic, competitID;

		if(indexPage.GetVarsHandler()->Get("loginUser").length() <= 0)
		{
			CLog	log;

			log.Write(WARNING, indexPage.GetVarsHandler()->Get("loginUser"), " try to fake system");

			indexPage.Redirect("/");
		}
		ost << "select * from users where  login='" << indexPage.GetVarsHandler()->Get("loginUser") << "'";
		affected = db.Query(ost.str());
		if(affected <= 0)
		{
			CLog	log;

			log.Write(ERROR, "there is no user ", indexPage.GetVarsHandler()->Get("loginUser"));
			throw CExceptionHTML("no user");
		}

		pic = indexPage.GetVarsHandler()->Get("pic");
		competitID = indexPage.GetVarsHandler()->Get("competit");

		if(pic.length() > 5)
		{
			ost.str("");
			ost << "update `users_photo` set `isActive`='0' where `user`='" << indexPage.GetVarsHandler()->Get("loginUser") << "' and `isActive`='" << competitID << "'";
			db.Query(ost.str());
			ost.str("");
			ost << "update `users_photo` set `isActive`='" << competitID << "' where `file`='" << pic << "'";
			db.Query(ost.str());

			ost.str("");
			ost << "“еперь в конкурсе будет участвовать эта фотографи€.<br>";
			ost << "<img width=300 src=/images/photo/" << pic << "><br>";
			ost << "¬ы можете вернутьс€ к <a href=\"/editpersonalinfo?rand=" << GetRandom(10) << "\">изменению личных параметров</a>.<br>";
		}
		else
		{
			ost.str("");
			ost << "ќшибка регистрации фотографии.<br>";
			ost << "¬ы можете вернутьс€ к <a href=\"/editpersonalinfo?rand=" << GetRandom(10) << "\">изменению личных параметров</a>.<br>";
		}
		indexPage.RegisterVariableForce("content", ost.str());
	}
*/
	if(action == "delete_picture")
	{
		ostringstream	ost;
		int		affected;
		string		pic;

		if(indexPage.GetVarsHandler()->Get("loginUser").length() <= 0)
		{
			CLog	log;

			log.Write(WARNING, indexPage.GetVarsHandler()->Get("loginUser"), " try to fake system");

			indexPage.Redirect("/");
		}
		ost << "select * from users where  login='" << indexPage.GetVarsHandler()->Get("loginUser") << "'";
		affected = db.Query(ost.str());
		if(affected <= 0)
		{
			CLog	log;

			log.Write(ERROR, "there is no user ", indexPage.GetVarsHandler()->Get("loginUser"));
			throw CExceptionHTML("no user");
		}

		pic = indexPage.GetVarsHandler()->Get("pic");
		if(pic.length() > 5)
		{
			ost.str("");
			ost << "delete from `users_photo` where `file`='" << pic << "'";
			db.Query(ost.str());

			ost.str("");
			ost << "‘отографи€ удалена.<br>";
			ost << "<img src=/images/photo/" << pic << "><br>";
			ost << "¬ы можете вернутьс€ к <a href=\"/editpersonalinfo?rand=" << GetRandom(10) << "\">изменению личных параметров</a>.<br>";
		}
		else
		{
			ost.str("");
			ost << "ќшибка регистрации фотографии.<br>";
			ost << "¬ы можете вернутьс€ к <a href=\"/editpersonalinfo?rand=" << GetRandom(10) << "\">изменению личных параметров</a>.<br>";
		}

		indexPage.RegisterVariableForce("content", ost.str());
	}
/*
	if(action == "listcompetit")
	{
		ostringstream	ost, result;
		int		affected;

		ost << "SELECT * from competitions where `isshow`='y'";
		affected = db.Query(ost.str());
		for(int i = 0; i < affected; i++)
		{
			result << "<a href='/showphoto&id=" << db.Get(i, "id") << "&rand=" << GetRandom(10) << "'>" << db.Get(i, "name") << "</a><br>";
		}
		indexPage.RegisterVariableForce("content", result.str());
	}
	if(action == "showphoto")
	{
		ostringstream	ost, result;
		int		affected, page;
		string		partNum, dbName, isMale, competitID;

		competitID = indexPage.GetVarsHandler()->Get("id");
		page = atoi(indexPage.GetVarsHandler()->Get("p").c_str());
		if(page < 0 || page > 10000) page = 0;

		ost << "SELECT users.id AS uid, users.name AS una, users_photo.file AS uphf\
			FROM `users` , `users_photo`\
			WHERE users.login = users_photo.user AND users_photo.isActive = '" << competitID << "' AND users.isBlocked='N' \
			LIMIT " << page * PHOTO_PER_PAGE << ", " << (page + 1) * PHOTO_PER_PAGE;
		affected = db.Query(ost.str());

		result << "<table cellspacing=\"0\" cellpadding=\"0\" width=\"100%\" border=\"0\"><tbody>";
		for(int i = 0; i < affected; i++)
		{
			if((i % 3 == 0) && (i != 0)) result << "</tr>";
			if(i % 3 == 0) result << "<tr valign=\"top\" align=\"center\">";
			result << "\
<td><br />\
<table height=\"1\" cellspacing=\"0\" cellpadding=\"0\" width=\"1\" border=\"0\"><tbody><tr><td width=\"9\" height=\"9\"><img height=\"9\" src=\"/images/k_c1.gif\" width=\"9\" /></td><td background=\"/images/k_up.gif\"><img height=\"1\" src=\"/images/1.gif\" width=\"1\" /></td><td width=\"9\"><img height=\"9\" src=\"/images/k_c2.gif\" width=\"9\" /></td></tr><tr><td background=\"/images/k_left.gif\"><img height=\"1\" src=\"/images/1.gif\" width=\"1\" /></td><td><a href=/competititem" << db.Get(i, "uid") << "&rand=" << GetRandom(10) << "><img border=0 height=\"100\" src=\"/images/photo/" << db.Get(i, "uphf") << "\" /></a></td><td background=\"/images/k_right.gif\"><img height=\"1\" src=\"/images/1.gif\" width=\"1\" /></td></tr><tr><td width=\"9\" height=\"9\"><img height=\"9\" src=\"/images/k_c4.gif\" width=\"9\" /><img height=\"1\" src=\"/images/1.gif\" width=\"1\" /></td><td background=\"/images/k_down.gif\"><img height=\"1\" src=\"/images/1.gif\" width=\"1\" /></td><td><img height=\"9\" src=\"/images/k_c3.gif\" width=\"9\" /></td></tr></tbody></table>\
<p><span class=\"blue_i\">" << db.Get(i, "una") << "</span><br></p></td>";
		}
		result << "</tbody></table>";

		ost.str("");
		ost << "SELECT count(*) FROM `users` , `users_photo` WHERE users.login = users_photo.user AND users_photo.isActive = '1' AND users.isBlocked='N'";
		db.Query(ost.str());
		if(page > 0) result << "<a href=\"/showphoto&p=" << (page - 1) << "&id=" << competitID << "&rand=" << GetRandom(10) << "\"><< предидуща€ страница</a>";
		if((page + 1) * PHOTO_PER_PAGE < atoi(db.Get(0, 0))) result << "<a href=\"/showphoto&p=" << (page + 1) << "&id=" << competitID << "&rand=" << GetRandom(10) << "\">следующа€ страница >></a>";

// ------------- Show menu at competition
		indexPage.RegisterVariableForce("breadcreams", " / <a href=/showphoto class=bread> онкурс фотографов</a>");
		indexPage.RegisterVariableForce("title", " онкурс фотографов");
		indexPage.RegisterVariableForce("content", result.str());
	}
	if(action == "showcompetititem")
	{
		ostringstream	ost;
		int		affected;

		ost << "select * from `users` where `id`='" << indexPage.GetVarsHandler()->Get("p") << "'";
		affected = db.Query(ost.str());
		if(affected > 0)
		{
			indexPage.RegisterVariableForce("name" ,db.Get(0, "name"));
			indexPage.RegisterVariableForce("nameLast", db.Get(0, "nameLast"));
			indexPage.RegisterVariableForce("description", db.Get(0, "description"));

			ost.str("");
			ost << "select * from `users_photo` where `user`='" << db.Get(0, "login") << "'";
			affected = db.Query(ost.str());
			if(affected > 0)
			{
				ostringstream	ost;
				ost.str("");
				ost << "<table  border=0 cellpadding=5 cellspacing=0 class=plain><tr>";
				for(int i = 0; i < affected; i++)
				{
					ost << "\
<td><br />\
<table height=\"1\" cellspacing=\"0\" cellpadding=\"0\" width=\"1\" border=\"0\"><tbody><tr><td width=\"9\" height=\"9\"><img height=\"9\" src=\"/images/k_c1.gif\" width=\"9\" /></td><td background=\"/images/k_up.gif\"><img height=\"1\" src=\"/images/1.gif\" width=\"1\" /></td><td width=\"9\"><img height=\"9\" src=\"/images/k_c2.gif\" width=\"9\" /></td></tr><tr><td background=\"/images/k_left.gif\"><img height=\"1\" src=\"/images/1.gif\" width=\"1\" /></td><td><a href=/images/photo/" << db.Get(i, "file") << " target=new><img border=0 height=\"100\" src=\"/images/photo/" << db.Get(i, "file") << "\" /></a></td><td background=\"/images/k_right.gif\"><img height=\"1\" src=\"/images/1.gif\" width=\"1\" /></td></tr><tr><td width=\"9\" height=\"9\"><img height=\"9\" src=\"/images/k_c4.gif\" width=\"9\" /><img height=\"1\" src=\"/images/1.gif\" width=\"1\" /></td><td background=\"/images/k_down.gif\"><img height=\"1\" src=\"/images/1.gif\" width=\"1\" /></td><td><img height=\"9\" src=\"/images/k_c3.gif\" width=\"9\" /></td></tr></tbody></table></td>";
				}
				ost << "</tr></table>";
				indexPage.RegisterVariableForce("photos", ost.str());
				indexPage.RegisterVariableForce("title", indexPage.GetVarsHandler()->Get("name"));
				indexPage.RegisterVariableForce("breadcreams", " <span class=bread>/</span> <a class=bread href=/showphoto> онкурс фотографов</a>");
			}
			if(!indexPage.SetTemplate("anketa.htmlt"))
			{
				CLog	log;

				log.Write(ERROR, "template file anketa.htmlt was missing");
				throw CException("Template file was missing");
			}
		}
		else
		{
			indexPage.RegisterVariableForce("content", "Ќет анкеты с таким номером.");
		}
	}
*/
	if(action == "showmain")
	{
			indexPage.RegisterVariableForce("title", "ƒобро пожаловать");
			if(!indexPage.SetTemplate("main.htmlt"))
			{
				CLog	log;

				log.Write(ERROR, "template file anketa.htmlt was missing");
				throw CException("Template file was missing");
			}	    
	}

/*
// ------- Catalog features
	if(action == "showcatalog")
	{

		string				breadcreams, catalogID, sortBy, sortOrder;
		ostringstream			ost, result;
		int				affected;


		catalogID = indexPage.GetVarsHandler()->Get("cat");
		ost.str("");
		ost << "select * from `manufacture` where `id`=\"" << catalogID << "\"";
		affected = db.Query(ost.str());
		if(affected == 0)
		{
			CLog	log;
			log.Write(ERROR, "can't get manufacture");
			throw CException("can't get manufacture");
		}
		if(strlen(db.Get(0, "keywords_head")) > 0) indexPage.RegisterVariableForce("keywords_head", db.Get(0, "keywords_head"));
		if(strlen(db.Get(0, "title_head")) > 0) indexPage.RegisterVariableForce("title_head", db.Get(0, "title_head"));
		if(strlen(db.Get(0, "description_head")) > 0) indexPage.RegisterVariableForce("description_head", db.Get(0, "description_head"));
		indexPage.RegisterVariableForce("content", db.Get(0, "content"));
		indexPage.RegisterVariableForce("title", db.Get(0, "name"));
		indexPage.RegisterVariableForce("file", db.Get(0, "file"));
		indexPage.RegisterVariableForce("articul", db.Get(0, "articul"));

		sortBy = indexPage.GetVarsHandler()->Get("sortby");
		sortOrder = indexPage.GetVarsHandler()->Get("sortorder");
		if(!sortBy.empty())
			indexPage.AddCookie("sortby", sortBy, "", "", "/");
		else
			sortBy = indexPage.GetCookie("sortby");
		if(!sortOrder.empty())
			indexPage.AddCookie("sortorder", sortOrder, "", "", "/");
		else
			sortOrder = indexPage.GetCookie("sortorder");

		if(sortBy.empty()) sortBy = "order";
		if(sortOrder.empty()) sortOrder = "asc";

		ost.str("");
		ost << "select * from `goods` where `parentID`=\"" << catalogID << "\" and `isShow`='Y' order by `" << sortBy << "` " << sortOrder << "";
		affected = db.Query(ost.str());
		if(affected > 0)
		{
			int	i;

			result.str("");
			for(i = 0; i < affected; i++)
			{
				string		loginName, price, isNew, isTop, imageMain, imageID, isAvailable;

				isNew = db.Get(i, "isNew");
				isTop = db.Get(i, "isTop");
				isAvailable = db.Get(i, "isAvailable");
				price = GetPriceStr(db.Get(i, "price"), &indexPage);
				imageMain = db.Get(i, "image1");
				imageID = GetRandom(10);
				if(i % 2 == 0) result << "<tr> \n\
                                      <td><img src=\"/images/1.gif\" width=\"18\" height=\"15\"></td>\n";

				result << "\n\
                                      <td width=\"30\"><a href=\"/good" << db.Get(i, "id") << "&cat=" << catalogID <<"&rand=" << GetRandom(10) << "\"><img src=\"/images/goods/" << ((imageMain == "") ? "no_foto.gif" : imageMain) << "\" width=\"29\" height=\"29\" border=\"0\" class=\"picture\">";
				if(isTop == "Y") result << "<img src=\"/images/sale.gif\" align=\"absmiddle\" border=\"0\">";
				if(isNew == "Y") result << "<img src=\"/images/new.gif\" align=\"absmiddle\" border=\"0\">";
				result <<  "</a></td>\n\
                                      <td width=\"18\"><img src=\"/images/1.gif\" width=\"18\" height=\"15\"></td>\n";
				if(i % 2 == 1) result << "\n\
                          </tr>\n\
                          <tr> \n\
                            <td><img src=\"/images/1.gif\" width=\"18\" height=\"15\"></td>\n\
                            <td><img src=\"/images/1.gif\" width=\"18\" height=\"15\"></td>\n\
                            <td><img src=\"/images/1.gif\" width=\"18\" height=\"15\"></td>\n\
                            <td><img src=\"/images/1.gif\" width=\"18\" height=\"15\"></td>\n\
                            <td><img src=\"/images/1.gif\" width=\"18\" height=\"15\"></td>\n\
                          </tr>\n";
			}
			indexPage.RegisterVariableForce("catalog", result.str());

		}
		if(!indexPage.SetTemplate("catalog.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file catalog.htmlt was missing");
			throw CException("Template file catalog.htmlt was missing");
		}
	}


	if(action == "show_good")
	{
		string			breadcreams, loginName, name, articul, content, isNew, isTop, isAvailable, price, catalogID, image1, image2, image3, image4, image5, image7, image8, catID, sessid;
		ostringstream		ost, result;
		int			affected;
		string			goodID;
		Menu			m;

		goodID = indexPage.GetVarsHandler()->Get("p");
		if(goodID.length() == 0)
		{
			CLog	log;

			log.Write(ERROR, "good identifier is missed");
			throw CExceptionHTML("no such part");
		}

		ost.str("");
		ost << "select * from goods where `id`=\"" << goodID << "\"";
		affected = db.Query(ost.str());
		if(affected <= 0)
		{
			CLog	log;
			log.Write(DEBUG, "no good with such ID");
			throw CExceptionHTML("no such part");
		}

		catID = db.Get(0, "parentID");
		name = db.Get(0, "name");
		content = db.Get(0, "content");
		articul = db.Get(0, "articul");
		isNew = db.Get(0, "isNew");
		isTop = db.Get(0, "isTop");
		isAvailable = db.Get(0, "isAvailable");
		price = GetPriceStr(db.Get(0, "price"), &indexPage);
		image1 = db.Get(0, "image1"); if(image1.empty()) image1 = "no_foto.gif";
		image2 = db.Get(0, "image2");
		image3 = db.Get(0, "image3");
		image4 = db.Get(0, "image4");
		image5 = db.Get(0, "image5");
		image7 = db.Get(0, "image7");
		image8 = db.Get(0, "image8");

		//--------- Open catalog
		{
			Catalog		m;

			if(catID.empty()) catID = "1";
			m.SetDB(&db);
			m.Load();

			GenerateAndRegisterCatalogV(catID, &m, &db, &indexPage);
		}
		//--------- End open catalog

		ost.str("");
		if(!image1.empty())
		{
			ost << "<a href=#><img src=\"/images/goods/" << image1 << "\" alt=\"”величть\" onclick=\"MM_openBrWindow('/images/goods/" << image1 << "','','menubar=yes,scrollbars=yes,resizable=yes,width=800,height=600')\" onmouseover=\"MM_swapImage('tovar','','/images/goods/" << image1 << "',1)\" border=\"0\" height=\"25\" hspace=\"5\" vspace=\"5\" width=\"30\" class=img></a>";
		}
		if(!image2.empty())
		{
			ost << "<a href=#><img src=\"/images/goods/" << image2 << "\" alt=\"”величть\" onclick=\"MM_openBrWindow('/images/goods/" << image2 << "','','menubar=yes,scrollbars=yes,resizable=yes,width=800,height=600')\" onmouseover=\"MM_swapImage('tovar','','/images/goods/" << image2 << "',1)\" border=\"0\" height=\"25\" hspace=\"5\" vspace=\"5\" width=\"30\" class=img></a>";
		}
		if(!image3.empty())
		{
			ost << "<a href=#><img src=\"/images/goods/" << image3 << "\" alt=\"”величть\" onclick=\"MM_openBrWindow('/images/goods/" << image3 << "','','menubar=yes,scrollbars=yes,resizable=yes,width=800,height=600')\" onmouseover=\"MM_swapImage('tovar','','/images/goods/" << image3 << "',1)\" border=\"0\" height=\"25\" hspace=\"5\" vspace=\"5\" width=\"30\" class=img></a>";
		}
		if(!image4.empty())
		{
			ost << "<a href=#><img src=\"/images/goods/" << image4 << "\" alt=\"”величть\" onclick=\"MM_openBrWindow('/images/goods/" << image4 << "','','menubar=yes,scrollbars=yes,resizable=yes,width=800,height=600')\" onmouseover=\"MM_swapImage('tovar','','/images/goods/" << image4 << "',1)\" border=\"0\" height=\"25\" hspace=\"5\" vspace=\"5\" width=\"30\" class=img></a>";
		}
		if(!image5.empty())
		{
			ost << "<a href=#><img src=\"/images/goods/" << image5 << "\" alt=\"”величть\" onclick=\"MM_openBrWindow('/images/goods/" << image5 << "','','menubar=yes,scrollbars=yes,resizable=yes,width=800,height=600')\" onmouseover=\"MM_swapImage('tovar','','/images/goods/" << image5 << "',1)\" border=\"0\" height=\"25\" hspace=\"5\" vspace=\"5\" width=\"30\" class=img></a>";
		}
		if(!image7.empty())
			indexPage.RegisterVariableForce("imageTop", image7);
		if(!image8.empty())
			indexPage.RegisterVariableForce("imageLeft", image8);
		indexPage.RegisterVariableForce("flash_images", ost.str());
		indexPage.RegisterVariableForce("imageMain", image1);

		indexPage.RegisterVariableForce("price", price);
		indexPage.RegisterVariableForce("content", content);
		indexPage.RegisterVariableForce("articul", articul);
		indexPage.RegisterVariableForce("name", name);
		indexPage.RegisterVariableForce("title", name);
		if(isTop == "Y") indexPage.RegisterVariableForce("isTop", "<img src='/images/sale.gif'>");
		if(isNew == "Y") indexPage.RegisterVariableForce("isNew", "<img src='/images/new.gif'>");
		if(isAvailable == "Y")
			indexPage.RegisterVariableForce("isAvailable", "в наличие");
		else
			indexPage.RegisterVariableForce("isAvailable", "под заказ");

		ost.str("");
		ost << "/buy" << goodID << "&rand=" << GetRandom(10);
		indexPage.RegisterVariableForce("buy_href", ost.str());

		ost.str("");
		ost << "select goods_recomend.id as gri, goods.id as gi, goods.name as gn, goods.image1 as gi1  from `goods_recomend`,`goods` where goods.id=goods_recomend.good_recomend and goods_recomend.good_id='" << goodID << "'";
		if((affected = db.Query(ost.str())) > 0)
		{
			ost.str("");
			ost << "                              <table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%\">\n\
                                <tbody><tr> \n\
                                  <td height=\"34\" width=\"13\"><img src=\"/images/w_c1.gif\" height=\"34\" width=\"13\"></td>\n\
                                  <td class=\"red_big\" align=\"center\" background=\"/images/w_upback.gif\"><span class=\"header_yellow\">—  этим товаром мы рекомендуем:</span></td>\n\
                                  <td width=\"13\"><img src=\"/images/w_c2.gif\" height=\"34\" width=\"13\"></td>\n\
                                </tr>\n\
                                <tr> \n\
                                  <td background=\"/images/w_left.gif\">&nbsp;</td>\n\
                                  <td bgcolor=\"#ffffff\"><table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%\">\n\
                                      <tbody><tr align=\"center\" valign=\"top\"> \n";
			for(int i = 0; i < affected; i++)
			{
				string	image;
				image = db.Get(i, "gi1");
				if(image.empty()) image = "no_foto.gif";
				ost << "<td><p><img src=\"/images/goods/" << image << "\" width=\"127\"></p>\n\
                                          <p><a href=\"/good" << db.Get(i, "gi") << "&random=" << GetRandom(10) << "\" class=\"menu\">" << db.Get(i, "gn") << "</a> <img src=\"/images/arrow_blue.gif\" align=\"absmiddle\" width=\"19\"></p></td>\n";
				if(i % 3 == 2) ost << "</tr><tr align=center valign=top>";
			}
			ost << "                                      </tr>\n\
                                    </tbody></table></td>\n\
                                  <td background=\"/images/w_right.gif\">&nbsp;</td>\n\
                                </tr>\n\
                                <tr> \n\
                                  <td><img src=\"/images/w_c4.gif\" height=\"13\" width=\"13\"></td>\n\
                                  <td background=\"/images/w_down.gif\"><img src=\"/images/1.gif\" height=\"1\" width=\"1\"></td>\n\
                                  <td height=\"13\" width=\"13\"><img src=\"/images/w_c3.gif\" height=\"13\" width=\"13\"></td>\n\
                                </tr>\n\
                              </tbody></table>\n";
			indexPage.RegisterVariableForce("goodsRecomend", ost.str());
		}

		//---------- Add good to viewed table
		sessid = indexPage.GetCookie("sessid");
		if(sessid.length() > 0)
		{
			ost.str("");
			ost << "select * from `goods_view` where `sessid`='" << sessid << "' and  `goodID`='" << goodID << "'";
			if(!db.Query(ost.str()))
			{
				ost.str("");
				ost << "insert into `goods_view` (`sessid`, `goodID`, `time`) values ('" << sessid << "', '" << goodID << "', now())";
				db.Query(ost.str());
			}
		}
		//---------- End add good to viewed table



		if(!indexPage.SetTemplate("good.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file show_good.htmlt was missing");
			throw CException("Template file show_good.htmlt was missing");
		}
	}
	if(action == "bucket_add")
	{
		Menu			m;
		string			goodID, isTop, isNew, loginName, image;
		ostringstream		ost;
		int			affected;

		goodID = indexPage.GetVarsHandler()->Get("p");
		if(goodID.length() == 0)
		{
			CLog	log;

			log.Write(ERROR, "dood identifier is missed");
			throw CExceptionHTML("no such part");
		}

		ost << "select * from goods where `id`=\"" << goodID << "\"";
		affected = db.Query(ost.str());
		if(affected <= 0)
		{
			CLog	log;
			log.Write(DEBUG, "no good with such ID");
			throw CExceptionHTML("no such part");
		}

		isNew = db.Get(0, "isNew");
		isTop = db.Get(0, "isTop");
		image = db.Get(0, "image1"); if(image.empty()) image = "no_foto.gif";
		loginName = indexPage.GetVarsHandler()->Get("loginUser");
		indexPage.RegisterVariableForce("image", image);
		indexPage.RegisterVariableForce("price", GetPriceStr(db.Get(0, "price"), &indexPage));
		indexPage.RegisterVariableForce("content", db.Get(0, "content"));
		indexPage.RegisterVariableForce("articul", db.Get(0, "articul"));
		indexPage.RegisterVariableForce("name", db.Get(0, "name"));
		indexPage.RegisterVariableForce("title", db.Get(0, "name"));

		ost.str("");
		ost << " <span class=bread> орзина <span class=bread>" << BREADCRUMBS_STR << " ƒобавление товара " << BREADCRUMBS_STR << "</span> " << db.Get(0, "name");
		indexPage.RegisterVariableForce("breadcreams", ost.str());

		if(!indexPage.SetTemplate("bucket_add.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file bucket_add.htmlt was missing");
			throw CException("Template file bucket_add.htmlt was missing");
		}
	}
	if(action == "submit_bucket_add")
	{
		CSession	sess;
		string		sessid, lng, goodID, quantity;
		ostringstream	ost;
		int		affected;
		Menu		m;

		sessid = indexPage.GetCookie("sessid");
		if(sessid.length() < 2)
		{
			ostringstream	ost1;
	
			lng = indexPage.GetLanguage();
	
			sess.SetDB(&db);
			sess.SetCgi(&indexPage);
			sess.Save("Guest", getenv("REMOTE_ADDR"), lng);
	
			indexPage.AddCookie("sessid", sess.GetID(), "", "", "/");
	
			sessid = indexPage.GetCookie("sessid");

			{
				CLog	log;
				log.Write(DEBUG, "create session for user (", sessid, ")");
			}
		}

		goodID = indexPage.GetVarsHandler()->Get("goodID");
		if(goodID.length() == 0)
		{
			CLog	log;
			log.Write(ERROR, "goodID is missing");

			throw CException("no such part");
		}
		quantity = indexPage.GetVarsHandler()->Get("quantity");
		if(quantity.length() == 0)
		{
			CLog	log;
			log.Write(ERROR, "quantity is missing");

			throw CException("no such part");
		}

		ost << "select * from goods_user where `goodID`=\"" << goodID << "\" and sessid=\"" << sessid << "\"";
		affected = db.Query(ost.str());
		ost.str("");
		if(affected > 0)
		{
			ost << "update goods_user set quantity=quantity+" << quantity << " where `id`=\"" << goodID <<"\" and sessid=\"" << sessid << "\"";
		}
		else
		{
			ost << "insert into goods_user (`sessid`, `goodID`, `quantity`) values(\"" << sessid << "\",\"" << goodID << "\",\"" << quantity << "\")";
		}
		db.Query(ost.str());
		if(!indexPage.SetTemplate("good_buyed.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file bucket_add.htmlt was missing");
			throw CException("Template file bucket_add.htmlt was missing");
		}

//------- Calculate user goods
		{
			string		loginUser;
			string		price;

			price = "price";
			ost.str("");
			ost << "select count(*) from goods_user where `sessid`=\"" << sessid << "\"";
			db.Query(ost.str());
			ost.str("");
			ost << db.Get(0, 0);
			indexPage.RegisterVariableForce("goods_count", ost.str());

			ost.str("");
			ost << "SELECT sum(goods_user.quantity*goods." << price << ") FROM `goods_user`,`goods` WHERE goods.id=goods_user.goodID and goods_user.sessid=\"" << sessid << "\"";
			db.Query(ost.str());
			ost.str("");
			ost << db.Get(0, 0);
			indexPage.RegisterVariableForce("goods_price", ost.str());
		}
//------- End calculate user goods
	}
	if(action == "show_bucket")
	{
		Menu			m;
		string			goodID, price, nameRus, loginName, sessid;
		ostringstream		ost;
		int			affected;

		loginName = indexPage.GetVarsHandler()->Get("loginUser");
		price = "price";
		sessid = indexPage.GetCookie("sessid");
		ost << "select goods.name as gn, goods." << price << " as gp, goods_user.quantity as gq, goods.id as gi, goods." << price << "*goods_user.quantity as gpgq from goods, goods_user where goods.id=goods_user.goodID and goods_user.sessid=\"" << sessid << "\"";
		affected = db.Query(ost.str());
		ost.str("");
		ost << "                        <table width=\"100%\" border=\"0\" cellspacing=\"10\" cellpadding=\"0\">\n\
                          <tr> \n\
                            <td valign=\"top\" class=\"plain\">\n\
<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n\
                                <tr>\n\
                                  <td width=\"13\" height=\"34\"><img src=\"/images/w_c1.gif\" width=\"13\" height=\"34\"></td>\n\
                                  <td background=\"/images/w_upback.gif\"><table width=\"100%\" border=\"0\" cellpadding=\"5\" cellspacing=\"0\">\n\
                                      <tr class=\"black\"> \n\
                                        <td width=\"20\" align=\"center\"><strong>є</strong></td>\n\
                                        <td><strong>ћодель</strong></td>\n\
                                        <td width=\"100\" align=\"center\"><strong><img src=\"/images/1.gif\" width=\"1\" height=\"1\" hspace=\"5\">÷ена</strong></td>\n\
                                        <td width=\"110\" align=\"center\"><strong> оличество</strong></td>\n\
                                        <td width=\"70\" align=\"center\"><strong>?того</strong></td>\n\
                                        <td width=\"60\" align=\"center\"><strong>”далить</strong></td>\n\
                                      </tr>\n\
                                    </table></td>\n\
                                  <td width=\"13\"><img src=\"/images/w_c2.gif\" width=\"13\" height=\"34\"></td>\n\
                                </tr>\n\
                                <tr>\n\
                                  <td background=\"/images/w_left.gif\">&nbsp;</td>\n\
                                  <td valign=\"top\" bgcolor=\"#FFFFFF\"><table width=\"100%\" border=\"0\" cellpadding=\"5\" cellspacing=\"0\">\n";
		for(int i = 0; i < affected; i++)
		{
			ost << "<tr valign=\"middle\"> \n<form action='/cgi-bin/index.cgi'>\n\
                                        <td width=\"20\" align=\"center\" bgcolor=\"#FFFFFF\" class=\"black\">" << (i + 1) << "</td>\n\
                                        <td bgcolor=\"#FFFFFF\"><span class=\"orange\"><a href=\"/good" << db.Get(i, "gi") << "&rand=" << GetRandom(10) << "\" class=\"red\">" << db.Get(i, "gn") << "</a></span><br> \n\
                                        </td>\n\
                                        <td width=\"100\" align=\"center\" bgcolor=\"#FFFFFF\" class=\"black\"><strong>" << GetPriceStr(db.Get(i, "gp"), &indexPage) << "</strong></td>\n\
                                        <td width=\"110\" align=\"center\" bgcolor=\"#FFFFFF\"><input class=item name=quantity style=\"width:20\" value=\"" << db.Get(i, "gq") << "\"> <input name=\"Submit\" type=\"submit\" class=\"item\" value=\"?зменить\"><input type=hidden name=goodID value=\"" << db.Get(i, "gi") << "\"><input type=hidden name=action value=\"update_good\"><input type=hidden name=rand value=\"" << GetRandom(10) << "\">\n\
 </td>\n\
                                        <td width=\"70\" align=\"center\" bgcolor=\"#FFFFFF\" class=\"black\"><strong>" << GetPriceStr(db.Get(i, "gpgq"), &indexPage) << "</strong></td>\n\
                                        <td width=\"60\" align=\"center\" bgcolor=\"#FFFFFF\"><a href=\"/delete_good" << db.Get(i, "gi") << "&rand=" << GetRandom(10) << "\" class=bread><img src=\"/images/delete.gif\" width=\"32\" height=\"32\" border=0></a></td>\n\
                                      </tr>\n\
                                      <tr valign=\"middle\"> \n\
                                        <td height=\"1\" align=\"center\" bgcolor=\"#FFFFFF\" class=\"black\"><img src=\"/images/1.gif\" width=\"1\" height=\"1\"></td>\n\
                                        <td height=\"1\" bgcolor=\"#FFFFFF\"><img src=\"/images/1.gif\" width=\"1\" height=\"1\"></td>\n\
                                        <td height=\"1\" align=\"center\" bgcolor=\"#FFFFFF\" class=\"black\"><img src=\"/images/1.gif\" width=\"1\" height=\"1\"></td>\n\
                                        <td height=\"1\" align=\"center\" bgcolor=\"#FFFFFF\" class=\"red\"><img src=\"/images/1.gif\" width=\"1\" height=\"1\"></td>\n\
                                        <td height=\"1\" align=\"center\" bgcolor=\"#FFFFFF\" class=\"black\"><img src=\"/images/1.gif\" width=\"1\" height=\"1\"></td>\n\
                                        <td height=\"0\" align=\"center\" bgcolor=\"#FFFFFF\"><img src=\"/images/1.gif\" width=\"1\" height=\"1\"></td></form>\n\
                                      </tr>\n";
		}
//		ost << "<tr><td></td><td class=header_blue><p>?“ќ√:</p></td><td></td><td></td><td class=header_blue><p>" << indexPage.GetVarsHandler()->Get("goods_price") << "</p><p><a href=/zakaz?rand=" << GetRandom(10) << "><img src=\"/images/zakaz.jpg\" border=0></a></p></td></tr>";
		ost << "                                      <tr valign=\"middle\">\n\
                                        <td align=\"center\" bgcolor=\"#FFFFFF\" class=\"black\">&nbsp;</td>\n\
                                        <td bgcolor=\"#FFFFFF\">&nbsp;</td>\n\
                                        <td align=\"center\" bgcolor=\"#FFFFFF\" class=\"black\">&nbsp;</td>\n\
                                        <td align=\"right\" bgcolor=\"#FFFFFF\" class=\"red\">¬сего:</td>\n\
                                        <td align=\"center\" class=\"red\" nowrap><strong>" << indexPage.GetVarsHandler()->Get("goods_price") << "</strong></td>\n\
                                        <td align=\"center\" bgcolor=\"#FFFFFF\">&nbsp;</td>\n\
                                      </tr>\n";
		ost << "                                    </table>\n\
                                  </td>\n\
                                  <td background=\"/images/w_right.gif\">&nbsp;</td>\n\
                                </tr>\n\
                                <tr>\n\
                                  <td><img src=\"/images/w_c4.gif\" width=\"13\" height=\"13\"></td>\n\
                                  <td background=\"/images/w_down.gif\"><img src=\"/images/1.gif\" width=\"1\" height=\"1\"></td>\n\
                                  <td width=\"13\" height=\"13\"><img src=\"/images/w_c3.gif\" width=\"13\" height=\"13\"></td>\n\
                                </tr>\n\
                              </table>\n\
                              <p align=\"center\"><br>\n\
                                <a href=\"/zakaz?rand=" << GetRandom(10) << "\"><strong>ќформить заказ</strong>\n\
                                </a><img src=\"/images/arrow_blue_black.gif\" width=\"20\" height=\"18\" align=\"absmiddle\">\n\
                                <br>\n\
                                <br>\n\
                              </p>\n\
                              </td>\n\
                          </tr>\n\
                        </table>\n";
		indexPage.RegisterVariableForce("content", ost.str());
		indexPage.RegisterVariableForce("title", "¬аша корзина");

		ost.str("");
		ost << " <span class=bread>" << BREADCRUMBS_STR << "</span> ¬аша корзина";
		indexPage.RegisterVariableForce("breadcreams", ost.str());
	}
	if(action == "update_good")
	{
		Menu			m;
		string			goodID, price, quantity, loginName, sessid;
		ostringstream		ost;
		
		m.SetDB(&db);
		m.Load();
		GenerateAndRegisterMenuV("1", &m, &db, &indexPage);

		goodID = indexPage.GetVarsHandler()->Get("goodID");
		if(goodID.length() == 0)
		{
			CLog	log;

			log.Write(ERROR, "good identifier is missed");
			throw CExceptionHTML("no such part");
		}
		quantity = indexPage.GetVarsHandler()->Get("quantity");
		if(quantity.length() == 0)
		{
			CLog	log;

			log.Write(ERROR, "good quantity is missed");
			throw CExceptionHTML("no such part");
		}

		sessid = indexPage.GetCookie("sessid");

		ost.str("");
		ost << "update `goods_user` set quantity=\"" << quantity << "\" where sessid=\"" << sessid << "\" and goodID=\"" << goodID << "\"";
		db.Query(ost.str());

		if(!indexPage.SetTemplate("good_updated.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file good_deleted.htmlt was missing");
			throw CException("Template file good_deleted.htmlt was missing");
		}

//------- Calculate user goods
		{
			string		loginUser;
			string		price;

			price = "price1";
			loginUser = indexPage.GetVarsHandler()->Get("loginUser");
			ost.str("");
			ost << "select count(*) from goods_user where `sessid`=\"" << sessid << "\"";
			db.Query(ost.str());
			ost.str("");
			ost << db.Get(0, 0);
			indexPage.RegisterVariableForce("goods_count", ost.str());

			ost.str("");
			ost << "SELECT sum(goods_user.quantity*goods." << price << ") FROM `goods_user`,`goods` WHERE goods.id=goods_user.goodID and goods_user.sessid=\"" << sessid << "\"";
			db.Query(ost.str());
			ost.str("");
			ost << db.Get(0, 0);
			indexPage.RegisterVariableForce("goods_price", ost.str());
		}
//------- End calculate user goods
	}
	if(action == "delete_good")
	{
		string			goodID, price, nameRus, loginName, sessid;
		ostringstream		ost;

		goodID = indexPage.GetVarsHandler()->Get("p");
		if(goodID.length() == 0)
		{
			CLog	log;

			log.Write(ERROR, "good identifier is missed");
			throw CExceptionHTML("no such part");
		}

		sessid = indexPage.GetCookie("sessid");

		ost.str("");
		ost << "delete from `goods_user` where sessid=\"" << sessid << "\" and goodID=\"" << goodID << "\"";
		db.Query(ost.str());

		if(!indexPage.SetTemplate("good_deleted.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file good_deleted.htmlt was missing");
			throw CException("Template file good_deleted.htmlt was missing");
		}

//------- Calculate user goods
		{
			string		loginUser;
			string		price;

			price = "price";
			ost.str("");
			ost << "select count(*) from goods_user where `sessid`=\"" << sessid << "\"";
			db.Query(ost.str());
			ost.str("");
			ost << db.Get(0, 0);
			indexPage.RegisterVariableForce("goods_count", ost.str());

			ost.str("");
			ost << "SELECT sum(goods_user.quantity*goods." << price << ") FROM `goods_user`,`goods` WHERE goods.id=goods_user.goodID and goods_user.sessid=\"" << sessid << "\"";
			db.Query(ost.str());
			ost.str("");
			ost << db.Get(0, 0);
			indexPage.RegisterVariableForce("goods_price", ost.str());
		}
//------- End calculate user goods
	}
	if(action == "make_zakaz")
	{
		ostringstream   ost;
		string          s1, s2, loginUser;
		int		affected;

		loginUser = indexPage.GetVarsHandler()->Get("loginUser");
		ost.str("");
		ost << "select * from users where `login`='" << loginUser << "'";
		affected = db.Query(ost.str());
		if(affected)
		{
			indexPage.RegisterVariableForce("email", db.Get(0, "email"));
			indexPage.RegisterVariableForce("phone", db.Get(0, "phone"));
			indexPage.RegisterVariableForce("o_comment", db.Get(0, "description"));
			indexPage.RegisterVariableForce("address", db.Get(0, "address"));
			ost.str("");
			ost << db.Get(0, "name") << " " << db.Get(0, "nameLast");
			indexPage.RegisterVariableForce("name", ost.str());
		}

		db.Query("SELECT CURDATE( ) +0, CURTIME( ) +0");
		s1 = db.Get(0, 0);
		s2 = db.Get(0, 1);
		ost.str("");
		ost << s1 << s2;
		indexPage.RegisterVariableForce("OrderID", ost.str());

		if((indexPage.GetVarsHandler()->Get("goods_price_rub")).length() == 0)
		    indexPage.RegisterVariableForce("goods_price_rub", "0");

		if(!indexPage.SetTemplate("zakaz.htmlt"))
		{
			CLog    log;
	
			log.Write(ERROR, "template file was missing");
			throw CException("Template file was missing");
		}
	}
	if(action == "mail_zakaz")
	{
		CMailLocal  mail;

		string                  goodID, price, nameRus, loginName, sessid, email, name, phone, address;
		ostringstream           ost;
		int                     affected;

		loginName = indexPage.GetVarsHandler()->Get("loginUser");
		email = indexPage.GetVarsHandler()->Get("o_email");
		name = indexPage.GetVarsHandler()->Get("o_name");
		phone = indexPage.GetVarsHandler()->Get("street1");
		address = indexPage.GetVarsHandler()->Get("address");

		if(email.empty()) throw CExceptionHTML("email error");
		if(name.empty()) throw CExceptionHTML("invalid user name");
		if(phone.empty()) throw CExceptionHTML("phone error");
		if(address.empty()) throw CExceptionHTML("address error");

		sessid = indexPage.GetCookie("sessid");
		ost << "select goods.name as gn, goods.price as gp, goods_user.quantity as gq, goods.id as gi,  goods.price*goods_user.quantity as gpgq from goods, goods_user where goods.id=goods_user.goodID and goods_user.sessid=\"" << sessid << "\"";
		affected = db.Query(ost.str());
		ost.str("");
		for(int i = 0; i < affected; i++)
		{
			ost << (i + 1) << ") " << db.Get(i, "gn") << " " << db.Get(i, "gp") << "*" << db.Get(i, "gq") << "=" << db.Get(i
, "gpgq") << "\n";
		}
		indexPage.RegisterVariableForce("basket_user", ost.str());
		indexPage.RegisterVariableForce("title", "«аказ отправлен");

		ost.str("");
		ost << " <span class=bread>" << BREADCRUMBS_STR << " «аказ отправлен</span>";
		indexPage.RegisterVariableForce("breadcreams", ost.str());

		mail.Send("admin", "new_order", indexPage.GetVarsHandler(), &db);
		indexPage.RegisterVariableForce("content", "¬аш заказ прин€т на обработку и вскоре наши менеджены св€жутс€ с вами.");
	}
	if(action == "showpartmain")
	{
		ostringstream	ost, ost2, query;
		int		affected;
		string		partNum, dbName;

		partNum = indexPage.GetVarsHandler()->Get("p");
		if(partNum.empty())
		{
			CLog	log;
			
			log.Write(WARNING, "please give me part number as 'p=XXX'");
			throw CExceptionHTML("no such part");
		}

		GenerateAndRegisterContent(partNum, &db, &indexPage);
		if(partNum == "1")
		{
//------- Generate News short-list
			query.str("");
			ost2.str("");
	
			query << "select * from `news` ORDER BY `data` DESC LIMIT 0,3";
			affected = db.Query(query.str());
			if(affected > 0)
			{
				ost2 << "<span class=\"header\">ЌјЎ? Ќќ¬ќ—“?</span></p>\n<table border=0 cellpadding=0 cellspacing=10 width=\"100%\">\n\
                          <tbody><tr>\n\
                            <td class=\"plain\">";
				for(int i = 0; i < affected; i++)
				{
					ost2 << "<p><span class=\"red\">" << db.Get(i, "data") << "</span><br>\n\
                                <span class=\"header_yellow\">" << db.Get(i, "title") << "</span><br>\n\
                                " << db.Get(i, "content_brief") << " <a href=\"/n" << db.Get(i, "id") << "&rand=" << GetRandom(10) << "\" class=\"blue\"><img border=0 src=\"/images/box_blue.gif\" height=\"9\" width=\"10\"><img src=\"/images/box_blue.gif\" border=0 height=\"9\" width=\"10\"><img src=\"/images/box_blue.gif\" height=\"9\" width=\"10\" border=0></a></p>\n";
				}
				ost2 << "                              </td>\n\
                          </tr>\n\
                        </tbody></table>\n";
			}
			indexPage.RegisterVariableForce("news", ost2.str());
//------- End generate News short-list

			indexPage.RegisterVariableForce("breadcreams", "");
			indexPage.RegisterVariableForce("infoBlock", "");
			indexPage.RegisterVariableForce("articul", "");
		}
		if(!indexPage.SetTemplate("index.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file was missing");
			throw CException("Template file was missing");
		}

	}
	if(action == "reverse_cont")
	{
		CMailLocal	mail;

		mail.Send("admin", "reverse_cont", indexPage.GetVarsHandler(), &db);

		indexPage.RegisterVariableForce("content", "¬аше сообщение отправлено!");
	}
	if(action == "registration_competit")
	{
		CMailLocal	mail;

		mail.Send("admin", "registration_competit", indexPage.GetVarsHandler(), &db);

		indexPage.RegisterVariableForce("content", "¬аше сообщение отправлено!");
	}
*/
	if(action == "forget")
	{
		ostringstream	ost;
		string		login;
		CMailLocal	mail;

		{
			CLog	log;
			log.Write(DEBUG, "int main(void): action == \"forget\": start");
		}


		login = RemoveQuotas(indexPage.GetVarsHandler()->Get("login"));
		if(login.length() > 0)
		{
			int		affected;

			ost.str("");
			ost << "SELECT \
						`users`.`login` 			as `users_login`, \
						`users`.`email` 			as `users_email`, \
						`users_passwd`.`passwd`		as `users_passwd_passwd` \
					FROM `users` \
					INNER JOIN `users_passwd` ON `users_passwd`.`userID`=`users`.`id` \
					WHERE (`users`.`login`=\"" << login << "\"  OR  `users`.`email`=\"" << login << "\")  AND (`users_passwd`.`isActive`='true');";

			affected = db.Query(ost.str());
			if(affected)
			{
				indexPage.RegisterVariableForce("login", db.Get(0, "users_login"));
				indexPage.RegisterVariableForce("passwd", db.Get(0, "users_passwd_passwd"));
				indexPage.RegisterVariableForce("ip", getenv("REMOTE_ADDR"));
				mail.Send(db.Get(0, "users_email"), "forget", indexPage.GetVarsHandler(), &db);
			}
		}
		else
		{
			{
				CLog	log;
				log.Write(ERROR, "int main(void): action == \"forget\": ERROR login is not defined");
			}
		}

		indexPage.RegisterVariableForce("content", "Ќа ваш почтовый €щик выслан пароль !");
	}

//------- Blog part
/*
	if(action == "blog_message_add")
	{
		ostringstream	ost;
		int		affected;
		string		parentID, currLogin;

		{
			Catalog				m;
			string				catID;

			catID = indexPage.GetVarsHandler()->Get("cat");
			if(catID.empty()) catID = "412";
			m.SetDB(&db);
			m.Load();

			// GenerateAndRegisterCatalogV(catID, &m, &db, &indexPage);
		}

		parentID = indexPage.GetVarsHandler()->Get("parentID");
		if(parentID.empty()) parentID = "0";

		if(indexPage.GetVarsHandler()->Get("loginUser").length() <= 0)
		{
			CLog	log;

			log.Write(WARNING, indexPage.GetVarsHandler()->Get("loginUser"), " try to fake system");

//			indexPage.Redirect("/");
		}
		
		currLogin = indexPage.GetVarsHandler()->Get("loginUser");
		if(currLogin != "")
		{
		    ost << "seLect * from users where  login='" << currLogin << "'";
		    affected = db.Query(ost.str());
		    if(affected <= 0)
		    {
			CLog	log;

			log.Write(ERROR, "there is no user ", indexPage.GetVarsHandler()->Get("loginUser"));
			throw CExceptionHTML("no user");
		    }
		}

		indexPage.RegisterVariableForce("title", "добавление");
		indexPage.RegisterVariableForce("breadcreams", " &gt; ƒобавление cообщени€");

		if(currLogin != "")
		{
		ost.str("");
		ost << "select * from blog_keywords";
		affected = db.Query(ost.str());
		ost.str("");
		ost << "<table>";
		for(int i = 0; i < affected; i++)
		{
			ost << "<tr>";
			ost << "<td>";
			ost << "<input type=checkbox name=keyw" << db.Get(i, "id") << "></td><td>" << db.Get(i, "name") << "</td>";
			ost << "</td>";
			ost << "</tr>";
		}
		ost << "</table>";
		}
		indexPage.RegisterVariableForce("keywords", ost.str());

		if(!indexPage.SetTemplate("blog_add_message.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file was missing");
			throw CException("Template file was missing");
		}
	}
	if(action == "blog_message_submit")
	{
		ostringstream	ost;
		int		affected;
		string		description, parentID, userID, blogID, currLogin;

		if(indexPage.GetVarsHandler()->Get("loginUser").length() <= 0)
		{
			CLog	log;

			log.Write(WARNING, indexPage.GetVarsHandler()->Get("loginUser"), " try to fake system");
		}

		currLogin = indexPage.GetVarsHandler()->Get("loginUser");
		if(currLogin == "")
		{
		    // Add response message from Guest

		    parentID = indexPage.GetVarsHandler()->Get("parentID");
		    if(parentID.empty()) parentID = "0";

		    description = indexPage.GetVarsHandler()->Get("description");
		    if(description.empty())
		    {
			throw CException("blog message empty");
		    }

		    ost.str("");
		    ost << "insert into `blog` (`content`, `userid`, `postDate`, `parentID`) values ('" << description << "', '" << DeleteHTML(userID) << "', NOW(), '" << DeleteHTML(parentID) << "')";
		    db.Query(ost.str());
		}
		else
		{
		    // Add messge from registered user
		    ost << "select * from users where  login='" << indexPage.GetVarsHandler()->Get("loginUser") << "'";
		    affected = db.Query(ost.str());
		    if(affected <= 0)
		    {
			CLog	log;

			log.Write(ERROR, "there is no user ", indexPage.GetVarsHandler()->Get("loginUser"));
			throw CExceptionHTML("no user");
		    }

		    userID = db.Get(0, "id");

		    parentID = indexPage.GetVarsHandler()->Get("parentID");
		    if(parentID.empty()) parentID = "0";

		    description = indexPage.GetVarsHandler()->Get("description");
		    if(description.empty())
		    {
		    	throw CException("blog message empty");
		    }

		    ost.str("");
		    ost << "insert into `blog` (`content`, `userid`, `postDate`, `parentID`) values ('" << description << "', '" << DeleteHTML(userID) << "', NOW(), '" << DeleteHTML(parentID) << "')";
		    db.Query(ost.str());

		    ost.str("");
		    ost << "select * from `blog` where `content`='" << description << "' and `userid`='" << DeleteHTML(userID) << "' and `parentID`='" << DeleteHTML(parentID) << "'";
		    db.Query(ost.str());
		    blogID = db.Get(0, "id");

		    for(int i = 0; i < 10000; i++)
		    {
			ost.str("");
			ost << "keyw" << i;
			if((indexPage.GetVarsHandler()->Get(ost.str())).length() > 0)
			{
				ost.str("");
				ost << "insert into `blog_reference` set \
					`blog_id` = '" << blogID << "', \
					`keyword_id` = '" << i << "'";
				db.Query(ost.str());
			}
		    }
		}

		indexPage.RegisterVariableForce("title", "добавлено");
		indexPage.RegisterVariableForce("breadcreams", " &gt; добавлено");
		indexPage.RegisterVariableForce("content", "¬аше сообщение добавлено");
	}
	if(action == "blog_archive_choose")
	{
		int		affected, postTotal, rootTotal, commentTotal, postLastNumber, postDeleted;
		ostringstream	ost;

		{
			Catalog				m;
			string				catID;

			catID = indexPage.GetVarsHandler()->Get("cat");
			if(catID.empty()) catID = "412";
			m.SetDB(&db);
			m.Load();

			// GenerateAndRegisterCatalogV(catID, &m, &db, &indexPage);
		}


		affected = db.Query("select COUNT(*) as dc, DATE_FORMAT( blog.postDate, '%Y-%m-%d' ) as dd from blog where parentID=0 group by DATE_FORMAT( blog.postDate, '%Y-%m-%d' ) order by 1 desc");
		if(affected)
		{
			indexPage.RegisterVariableForce("mostOfDay", db.Get(0, "dd"));
			indexPage.RegisterVariableForce("mostOfDayCount", db.Get(0, "dc"));
		}
		affected = db.Query("select COUNT(*) as dc, DATE_FORMAT( blog.postDate, '%Y-%m-%d' ) as dd from blog where parentID!=0 group by DATE_FORMAT( blog.postDate, '%Y-%m-%d' ) order by 1 desc");
		if(affected)
		{
			indexPage.RegisterVariableForce("mostOfDayComment", db.Get(0, "dd"));
			indexPage.RegisterVariableForce("mostOfDayCountComment", db.Get(0, "dc"));
		}
		affected = db.Query("select COUNT(*) as dd from blog where parentID=0");
		if(affected)
		{
			rootTotal = atoi(db.Get(0, "dd"));
			ost.str("");
			ost << rootTotal;
			indexPage.RegisterVariableForce("rootTotal", ost.str());
		}
		affected = db.Query("select COUNT(*) as dd from blog");
		if(affected)
		{
			postTotal = atoi(db.Get(0, "dd"));
			ost.str("");
			ost << postTotal;
			indexPage.RegisterVariableForce("postTotal", ost.str());
		}
		commentTotal = postTotal - rootTotal;
		ost.str("");
		ost << commentTotal;
		indexPage.RegisterVariableForce("commentTotal", ost.str());

		affected = db.Query("SELECT * FROM `blog` ORDER BY `id` DESC LIMIT 0 , 3");
		if(affected)
		{
			postLastNumber = atoi(db.Get(0, "id"));
			ost.str("");
			ost << postLastNumber;
			indexPage.RegisterVariableForce("postLastNumber", ost.str());
		}
		postDeleted = postLastNumber - postTotal;
		ost.str("");
		ost << postDeleted;
		indexPage.RegisterVariableForce("postDeleted", ost.str());


		if(!indexPage.SetTemplate("blog_archive.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file was missing");
			throw CException("Template file was missing");
		}
	}
	if(action == "blog_archive_list")
	{
		ostringstream	ost, ost1;
		int		affected;
		string		description, parentID, userID;
		CMysql		db1;

		{
			Catalog				m;
			string				catID;

			catID = indexPage.GetVarsHandler()->Get("cat");
			if(catID.empty()) catID = "412";
			m.SetDB(&db);
			m.Load();

			// GenerateAndRegisterCatalogV(catID, &m, &db, &indexPage);
		}

		if(db1.Connect(DB_NAME, DB_LOGIN, DB_PASSWORD) < 0)
		{
			CLog	log;
	
			log.Write(ERROR, "Can not connect to mysql database");
			return(1);
		}

#ifdef MYSQL_4
		db.Query("set names cp1251");
#endif

		ost << "select blog.id as bid, blog.parentID as bpid, blog.content as bc, blog.postDate as bpd, users.name as un from blog, users where users.id=blog.userID and blog.parentID=0 and DATE_FORMAT( blog.postDate, '%Y-%m-%d' )='" << indexPage.GetVarsHandler()->Get("data") << "' order by `postDate`";
		affected = db.Query(ost.str());
		if(affected <= 0)
		{
			ost.str("");
			ost << "?звините. ¬ архиве нет записей за эту дату";
		}
		else
		{
			ost.str("");
			ost << "<table>";
			for(int i = 0; i < affected; i++)
			{
				ost1.str("");
				ost1 << "select count(*) as cnt from `blog` where `parentID`='" << db.Get(i, "bid") << "'";
				db1.Query(ost1.str());
				ost << "<tr><td>" << db.Get(i, "un") << "</td><td>" << db.Get(i, "bpd") << "</td></tr>";
				ost << "<tr><td colspan=2>" << db.Get(i, "bc") << "</td></tr>";
				ost << "<tr><td></td><td>" << db1.Get(0, 0) << " <a href=/blog_comment_list" << db.Get(i, "bid") << "&rand=" << GetRandom(10) << ">комментариев</a></td></tr>";
			}
	//------- Check for answer possibility
			ost1.str("");
			ost1 << "select * from users where  login='" << indexPage.GetVarsHandler()->Get("loginUser") << "'";
			affected = db.Query(ost1.str());
			if(affected > 0)
			{
				ost << "<tr><td></td><td><a href=\"/blog_message_add&parentID=0&rand=" << GetRandom(10) << "\">ƒобавить ответ</a></td></tr>";
			}
	//------- End check for answer possibility
			ost << "</table>";
		}

		indexPage.RegisterVariableForce("content", ost.str());
		indexPage.RegisterVariableForce("title", "Ѕлог");
		indexPage.RegisterVariableForce("breadcreams", " &gt; Ѕлог");
	}
	if(action == "blog_message_list")
	{
		ostringstream	ost, ost1, userStr;
		int		affected, affected1, i, i1;
		string		description, parentID, userID, keyID, ostKey, title, user;
		CMysql		db1;

		{
			Catalog				m;
			string				catID;

			catID = indexPage.GetVarsHandler()->Get("cat");
			if(catID.empty()) catID = "412";
			m.SetDB(&db);
			m.Load();

			// GenerateAndRegisterCatalogV(catID, &m, &db, &indexPage);
		}

		if(db1.Connect(DB_NAME, DB_LOGIN, DB_PASSWORD) < 0)
		{
			CLog	log;
	
			log.Write(ERROR, "Can not connect to mysql database");
			return(1);
		}

#ifdef MYSQL_4
		db.Query("set names cp1251");
#endif


		user = indexPage.GetVarsHandler()->Get("user");
		keyID = indexPage.GetVarsHandler()->Get("key");
		if(keyID.length() > 0)
		{
			ost1.str("");
			ost1 << "select * from blog_reference where keyword_id=" << keyID;
			affected = db.Query(ost1.str());
			ostKey = " and blog.id in (";
			for(i = 0; i < affected; i++)
			{
				if(i != 0) ostKey += ", ";
				ostKey += db.Get(i, "blog_id");
			}
			ostKey += ") ";
		}

		userStr.str("");
		if(user.length() > 0)
		    userStr << " and blog.userid=" << user << " ";
		ost << "select blog.id as bid, blog.parentID as bpid, blog.content as bc, blog.postDate as bpd, users.name as un from blog, users where users.id=blog.userID and blog.parentID=0 " << ostKey << " " << userStr.str() << " order by `postDate` desc limit 0,30";
		affected = db.Query(ost.str());
		if(affected <= 0)
		{
			CLog	log;
			log.Write(ERROR, "Blog is empty. Very strange.");

			throw CException("no such part");
		}

		ost.str("");
		ost << "<table>";
		for(i = 0; i < affected; i++)
		{
			ost1.str("");
			ost1 << "select count(*) as cnt from `blog` where `parentID`='" << db.Get(i, "bid") << "'";
			db1.Query(ost1.str());
			ost << "<tr><td colspan=2><img src='/images/bullet_blog.gif'> <span class='blog_bc'>" << db.Get(i, "bc") << "</span></td></tr>";
			ost << "<tr><td><span class='blog_name'>" << db.Get(i, "un") << "</span> <span class='blog_date'>" << db.Get(i, "bpd") << "</span></td><td></td></tr>";
			ost << "<tr><td valign=top>" << db1.Get(0, 0) << " <a href=/blog_comment_list" << db.Get(i, "bid") << "&rand=" << GetRandom(10) << ">комментариев</a></td><td>";

			ost1.str("");
			ost1 << "select * from blog_reference, blog_keywords where blog_reference.keyword_id=blog_keywords.id and blog_reference.blog_id=" << db.Get(i, "bid");
			affected1 = db1.Query(ost1.str());
			for(i1 = 0; i1 < affected1; i1++)
			{
				ost << "<a href=/blog_message_list&key=" << db1.Get(i1, "keyword_id") << "&rand=" << GetRandom(10) << ">" << db1.Get(i1, "name") << "</a>, ";
			}
			ost << "<p></td></tr>";
		}

//------- Check for answer possibility
		ost1.str("");
		ost1 << "select * from users where  login='" << indexPage.GetVarsHandler()->Get("loginUser") << "'";
		affected = db.Query(ost1.str());
		if(affected > 0)
		{
			ost << "<tr><td></td><td><a href=\"/blog_message_add&parentID=0&rand=" << GetRandom(10) << "\">ƒобавить ответ</a></td></tr>";
		}
//------- End check for answer possibility
		ost << "</table>";

		title = "Ѕлог ";
		ost1.str("");
		ost1 << "select * from blog_keywords where id=" << keyID;
		affected1 = db1.Query(ost1.str());
		if(affected1 > 0)
			title += db1.Get(0, "name");
		indexPage.RegisterVariableForce("title", title);
		indexPage.RegisterVariableForce("breadcreams", " &gt; Ѕлог");
		indexPage.RegisterVariableForce("content", ost.str());
	}
	if(action == "blog_comment_list")
	{
		ostringstream	ost, ost1;
		int		affected;
		string		description, parentID, userID, blogID, blogMessageID;


		{
			Catalog				m;
			string				catID;

			catID = indexPage.GetVarsHandler()->Get("cat");
			if(catID.empty()) catID = "412";
			m.SetDB(&db);
			m.Load();

			// GenerateAndRegisterCatalogV(catID, &m, &db, &indexPage);
		}

		blogMessageID = indexPage.GetVarsHandler()->Get("p");
		if(blogMessageID.empty())
		{
			CLog	log;
			log.Write(ERROR, "There is no message with such ID.");

			throw CException("no such part");
		}

		ost << "select blog.id as bid, blog.parentID as bpid, blog.content as bc, blog.postDate as bpd, users.name as un from blog, users where users.id=blog.userID and blog.parentID=0 and blog.id='" << blogMessageID << "' order by `postDate`";
		affected = db.Query(ost.str());
		if(affected <= 0)
		{
			CLog	log;
			log.Write(ERROR, "Blog is empty. Very strange.");

			throw CException("no such part");
		}

		ost.str("");
		ost << "<table>";
		ost << "<tr><td>" << db.Get(0, "un") << "</td><td>" << db.Get(0, "bpd") << "</td></tr>";
		ost << "<tr><td colspan=2>" << db.Get(0, "bc") << "</td></tr>";
		ost << "<tr><td colspan=2 align=center><h2> омметарии</h2></td></tr>";

		ost1.str("");
		ost1 << "select blog.id as bid, blog.parentID as bpid, blog.content as bc, blog.postDate as bpd, users.name as un from blog, users where users.id=blog.userID and blog.parentID='" << db.Get(0, "bid") << "' ORDER BY `blog`.`bpd` ASC";
		affected = db.Query(ost1.str());
		for(int i = 0; i < affected; i++)
		{
			ost << "<tr><td colspan=2>" << db.Get(i, "bc") << "</td></tr>";
			ost << "<tr><td>" << db.Get(i, "un") << "</td><td>" << db.Get(i, "bpd") << "</td></tr>";
		}

		ost << "<tr><td></td><td><a href=\"/blog_message_add&parentID=" << blogMessageID << "&rand=" << GetRandom(10) << "\">ƒобавить ответ</a></td></tr>";
//------- End check for answer possibility

		ost << "</table>";

		indexPage.RegisterVariableForce("title", "Ѕлог");
		indexPage.RegisterVariableForce("breadcreams", " &gt; Ѕлог");
		indexPage.RegisterVariableForce("content", ost.str());
	}
//------- End blog part

	if(action == "get_rambler")
	{
		ostringstream	ost, ost1;
		int		affected;
		string		newthingsNum, parentCategory, price, imageURL, name, category, parentID, rambler;
		CMysql		db1;
		string::size_type	pos;
		
		if(db1.Connect(DB_NAME, DB_LOGIN, DB_PASSWORD) < 0)
		{
		    CLog	log;

	    	    log.Write(ERROR, "Can not connect to mysql database");
		    return(1);
		}
#ifdef MYSQL_4
		db1.Query("set names cp1251");
#endif

		ost << "select CURDATE(), CURTIME()";
		affected = db.Query(ost.str());
		if(affected <= 0)
		{
			CLog	log;
			log.Write(ERROR, "can't output YML");
			
			throw CExceptionHTML("no such part");
		}
		ost.str("");
		ost << db.Get(0, 0) << " " << db.Get(0, 1);
		indexPage.RegisterVariableForce("date", ost.str());

		ost.str("");
		ost << "select * from `goods` order by `id` asc";
		affected = db.Query(ost.str());
		if(affected <= 0)
		{
			CLog	log;
			log.Write(ERROR, "can't output YML");
			
			throw CExceptionHTML("no such part");
		}
		ost.str("");
		for(int i = 0; i < affected; i++)
		{
			price = db.Get(i, "price");
			pos = 0;
		        while((pos = price.find(",", pos)) != string::npos)
		        {
		            price.replace(pos, 1, ".");
		        }
			
			imageURL = db.Get(i, "image1");
			imageURL = SymbolReplace(imageURL, "[", "%5B");
			imageURL = SymbolReplace(imageURL, "]", "%5D");
			imageURL = SymbolReplace(imageURL, "&", "&amp;");
			imageURL = SymbolReplace(imageURL, "\"", "&quot;");
			imageURL = SymbolReplace(imageURL, ">", "&gt;");
			imageURL = SymbolReplace(imageURL, "<", "&lt;");
			imageURL = SymbolReplace(imageURL, "'", "&apos;");

			name = db.Get(i, "name");
			name = SymbolReplace(name, "[", "%5B");
			name = SymbolReplace(name, "]", "%5D");
			name = SymbolReplace(name, "&", "&amp;");
			name = SymbolReplace(name, "\"", "&quot;");
			name = SymbolReplace(name, ">", "&gt;");
			name = SymbolReplace(name, "<", "&lt;");
			name = SymbolReplace(name, "'", "&apos;");

			
			parentID = db.Get(i, "parentID");
			for(rambler = ""; parentID != "0" && rambler.empty();)
			{
			    ost1.str("");
			    ost1 << "select * from `manufacture` where `id`='" << parentID << "'";
			    if(db1.Query(ost1.str()) <= 0)
			    {
				CLog	log;
				log.Write(ERROR, "can't output YML (error in manufacture.id =", parentID, ")");

				throw CExceptionHTML("no such part");
			    }
			    parentID = db1.Get(0, "parentID");
			    rambler = db1.Get(0, "rambler");
			}

			ost << "\n\
    <offer id=\"" << db.Get(i, "id") << "\">\n\
       <category>" << rambler << "</category>\n\
       <title>" << name << "</title>\n\
       <price>" << GetOnlyPriceStr(price, &indexPage) << "</price>\n\
       <currencyId>RUR</currencyId>\n\
       <url>http://fotoplus.su/good" << db.Get(i, "id") << "&amp;ref=rambler</url>\n\
       <img>http://fotoplus.su/images/goods/" << imageURL << "</img>\n\
       <description></description>\n\
    </offer>\n\n";
		}
		indexPage.RegisterVariableForce("offers", ost.str());

		if(!indexPage.SetTemplate("rambler.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "template file show_newthings.htmlt was missing");
			throw CException("Template file was missing");
		}
	}
	if(action == "search")
	{
		ostringstream	ost, searchOst;
		int		affected, i;
		string		image;
	
		ost << "select * from manufacture where `content` LIKE '%" << indexPage.GetVarsHandler()->Get("text") << "%' OR `name` LIKE '%" << indexPage.GetVarsHandler()->Get("text") << "%' OR `articul` LIKE '%" << indexPage .GetVarsHandler()->Get("text") << "%'";
		if((affected = db.Query(ost.str())) > 0)
		{
			searchOst << "<ul>";
			for(i = 0; i < affected; i++)
			{
				image = db.Get(i, "image1");
				if(image.empty()) image = "no_foto.gif";
				searchOst << "<li class=plain><a href=/cat" << db.Get(i, "id") << "&rand=" << GetRandom(10) << "> " << db.Get(i,"name") << "</a> </li>";
			}
			searchOst << "</ul>";
			indexPage.RegisterVariableForce("content", searchOst.str());
		}
		else
		{
			indexPage.RegisterVariableForce("content", "Ќичего не удалось найти по заданным критери€м.");
		}
	
		indexPage.RegisterVariableForce("title", "–езультаты поиска");
		indexPage.RegisterVariableForce("breadcreams", " <span class=bread>/</span><span class=bread> ѕоиск</span>");
		if(!indexPage.SetTemplate("search.htmlt"))
		{
			CLog    log;
	
			log.Write(ERROR, "template file was missing");
			throw CException("Template file was missing");
		}
	}
	if(action == "print")
	{
		ostringstream	ost, searchOst;
		int		affected;
		
		ost << "select * from parts where `id`=" << indexPage.GetVarsHandler()->Get("p");
		if((affected = db.Query(ost.str())) > 0) indexPage.RegisterVariableForce("content", db.Get(0, "content"));
		
		if(!indexPage.SetTemplate("print.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "template file was missing");
			throw CException("Template file was missing");
		}
	}
*/

	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "int main(void): end (action's == \"" << action << "\") condition";
		log.Write(DEBUG, ost.str());
	}

	indexPage.OutTemplate();

    }
/*
    catch(CExceptionRedirect &c) {
    	CLog	log;
    	ostringstream	ost;

    	ost.str("");
    	ost << "int main(void):: catch CRedirectHTML: exception used for redirection";
    	log.Write(DEBUG, ost.str());

    	c.SetDB(&db);

    	if(!indexPage.SetTemplate(c.GetTemplate())) {

	    	ost.str("");
	    	ost << "int main(void):: catch CRedirectHTML: ERROR, template redirect.htmlt not found";
	    	log.Write(ERROR, ost.str());

    		throw CException("Template file was missing");
    	}

    	indexPage.RegisterVariableForce("content", "redirect page");
    	indexPage.OutTemplate();

    }
*/
    catch(CExceptionHTML &c)
    {
		CLog	log;

		c.SetLanguage(indexPage.GetLanguage());
		c.SetDB(&db);

		log.Write(DEBUG, "main: catch CExceptionHTML: DEBUG exception reason: [", c.GetReason(), "]");

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

		log.Write(ERROR, "main: catch CException: exception: ERROR  ", c.GetReason());

		indexPage.RegisterVariable("content", c.GetReason());
		indexPage.OutTemplate();
		return(-1);
    }
    catch(exception& e)
    {
    	CLog 	log;
		log.Write(PANIC, "main: catch(exception& e): catch standard exception: ERROR  ", e.what());

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

