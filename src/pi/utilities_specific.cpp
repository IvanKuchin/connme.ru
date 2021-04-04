#include "utilities.h"

template <class T>
static auto __GetMapValueByKey(const map<string, T> &dictionary, string key_value)
{
	MESSAGE_DEBUG("", "", "start (" + key_value + ")");

	T		result;
	auto	it		= dictionary.find(key_value);

	if(it == dictionary.end())
	{
		MESSAGE_ERROR("", "", "unknown item " + key_value);
	}
	else
	{
		result = it->second;
	}

	MESSAGE_DEBUG("", "", "finish (" + key_value + ")");

	return result;
}

/*
string	GetDefaultActionFromUserType(CUser *user, CMysql *db)
{
	MESSAGE_DEBUG("", "", "start");

	map<string, string>	dictionary = 
	{
		{ "guest"							, GUEST_USER_DEFAULT_ACTION },
		{ "user"							, LOGGEDIN_USER_DEFAULT_ACTION },
		{ "helpdesk"						, LOGGEDIN_HELPDESK_DEFAULT_ACTION },
	};

	auto	  result = __GetMapValueByKey(dictionary, user->GetType());

	MESSAGE_DEBUG("", "", "finish (" + user->GetType() + " -> " + result + ")");

	return result;
}
*/


/*
int GetSpecificData_GetNumberOfFolders(string itemType)
{
	MESSAGE_DEBUG("", "", "start");

	map<string, int>	dictionary = 
	{

		{ "certification"				, CERTIFICATIONSLOGO_NUMBER_OF_FOLDERS },
		{ "course"						, CERTIFICATIONSLOGO_NUMBER_OF_FOLDERS },
		{ "university"					, UNIVERSITYLOGO_NUMBER_OF_FOLDERS },
		{ "school"						, SCHOOLLOGO_NUMBER_OF_FOLDERS },
		{ "language"					, FLAG_NUMBER_OF_FOLDERS },
		{ "book"						, BOOKCOVER_NUMBER_OF_FOLDERS },
		{ "company"						, COMPANYLOGO_NUMBER_OF_FOLDERS },
		{ "company_profile_logo"		, COMPANYLOGO_NUMBER_OF_FOLDERS },
		{ "gift"						, GIFTIMAGE_NUMBER_OF_FOLDERS },
		{ "event"						, EVENTIMAGE_NUMBER_OF_FOLDERS },
		{ "helpdesk_ticket_attach"		, HELPDESK_TICKET_ATTACHES_NUMBER_OF_FOLDERS}
	};

	auto	  result = __GetMapValueByKey(dictionary, itemType);

	MESSAGE_DEBUG("", "", "finish (" + itemType + " -> " + to_string(result) + ")");

	return result;
}
*/


/*
int GetSpecificData_GetMaxFileSize(string itemType)
{
	MESSAGE_DEBUG("", "", "start");

	map<string, int>	dictionary = 
	{

		{ "certification"				, CERTIFICATIONSLOGO_MAX_FILE_SIZE },
		{ "course"						, CERTIFICATIONSLOGO_MAX_FILE_SIZE },
		{ "university"					, UNIVERSITYLOGO_MAX_FILE_SIZE },
		{ "school"						, SCHOOLLOGO_MAX_FILE_SIZE },
		{ "language"					, FLAG_MAX_FILE_SIZE },
		{ "book"						, BOOKCOVER_MAX_FILE_SIZE },
		{ "company"						, COMPANYLOGO_MAX_FILE_SIZE },
		{ "company_profile_logo"		, COMPANYLOGO_MAX_FILE_SIZE },
		{ "gift"						, GIFTIMAGE_MAX_FILE_SIZE },
		{ "event"						, EVENTIMAGE_MAX_FILE_SIZE },
		{ "helpdesk_ticket_attach"		, HELPDESK_TICKET_ATTACHES_MAX_FILE_SIZE}
	};

	auto	  result = __GetMapValueByKey(dictionary, itemType);

	MESSAGE_DEBUG("", "", "finish (" + itemType + " -> " + to_string(result) + ")");

	return result;
}

*/

unsigned int GetSpecificData_GetMaxWidth(string itemType)
{
	MESSAGE_DEBUG("", "", "start");

	map<string, unsigned int>	dictionary = 
	{

		{ "certification"				, CERTIFICATIONSLOGO_MAX_WIDTH },
		{ "course"						, CERTIFICATIONSLOGO_MAX_WIDTH },
		{ "university"					, UNIVERSITYLOGO_MAX_WIDTH },
		{ "school"						, SCHOOLLOGO_MAX_WIDTH },
		{ "language"					, FLAG_MAX_WIDTH },
		{ "book"						, BOOKCOVER_MAX_WIDTH },
		{ "company"						, COMPANYLOGO_MAX_WIDTH },
		{ "company_profile_logo"		, COMPANYLOGO_MAX_WIDTH },
		{ "gift"						, GIFTIMAGE_MAX_WIDTH },
		{ "event"						, EVENTIMAGE_MAX_WIDTH },
	};

	auto	  result = __GetMapValueByKey(dictionary, itemType);

	MESSAGE_DEBUG("", "", "finish (" + itemType + " -> " + to_string(result) + ")");

	return result;
}
/*
unsigned int GetSpecificData_GetMaxWidth(string itemType)
{
	int	  result = 0;

	MESSAGE_DEBUG("", "", "start");

	if(itemType == "certification")		result = CERTIFICATIONSLOGO_MAX_WIDTH;
	else if(itemType == "course")		result = CERTIFICATIONSLOGO_MAX_WIDTH;
	else if(itemType == "university")	result = UNIVERSITYLOGO_MAX_WIDTH;
	else if(itemType == "school")		result = SCHOOLLOGO_MAX_WIDTH;
	else if(itemType == "language")		result = FLAG_MAX_WIDTH;
	else if(itemType == "book")			result = BOOKCOVER_MAX_WIDTH;
	else if(itemType == "company")		result = COMPANYLOGO_MAX_WIDTH;
	else if(itemType == "gift")			result = GIFTIMAGE_MAX_WIDTH;
	else if(itemType == "event")		result = EVENTIMAGE_MAX_WIDTH;
	else
	{
		MESSAGE_ERROR("", "", "itemType [" + itemType + "] unknown");
	}

	MESSAGE_DEBUG("", "", "finish (result = " + to_string(result) + ")");
	
	return result;
}
*/


unsigned int GetSpecificData_GetMaxHeight(string itemType)
{
	MESSAGE_DEBUG("", "", "start");

	map<string, unsigned int>	dictionary = 
	{

		{ "certification"				, CERTIFICATIONSLOGO_MAX_HEIGHT },
		{ "course"						, CERTIFICATIONSLOGO_MAX_HEIGHT },
		{ "university"					, UNIVERSITYLOGO_MAX_HEIGHT },
		{ "school"						, SCHOOLLOGO_MAX_HEIGHT },
		{ "language"					, FLAG_MAX_HEIGHT },
		{ "book"						, BOOKCOVER_MAX_HEIGHT },
		{ "company"						, COMPANYLOGO_MAX_HEIGHT },
		{ "company_profile_logo"		, COMPANYLOGO_MAX_HEIGHT },
		{ "gift"						, GIFTIMAGE_MAX_HEIGHT },
		{ "event"						, EVENTIMAGE_MAX_HEIGHT },
	};

	auto	  result = __GetMapValueByKey(dictionary, itemType);

	MESSAGE_DEBUG("", "", "finish (" + itemType + " -> " + to_string(result) + ")");

	return result;
}

/*
unsigned int GetSpecificData_GetMaxHeight(string itemType)
{
	int	  result = 0;

	MESSAGE_DEBUG("", "", "start");

	if(itemType == "certification")		result = CERTIFICATIONSLOGO_MAX_HEIGHT;
	else if(itemType == "course")		result = CERTIFICATIONSLOGO_MAX_HEIGHT;
	else if(itemType == "university")	result = UNIVERSITYLOGO_MAX_HEIGHT;
	else if(itemType == "school")		result = SCHOOLLOGO_MAX_HEIGHT;
	else if(itemType == "language")		result = FLAG_MAX_HEIGHT;
	else if(itemType == "book")			result = BOOKCOVER_MAX_HEIGHT;
	else if(itemType == "company")		result = COMPANYLOGO_MAX_HEIGHT;
	else if(itemType == "gift")	  		result = GIFTIMAGE_MAX_HEIGHT;
	else if(itemType == "event")	  	result = EVENTIMAGE_MAX_HEIGHT;
	else
	{
		MESSAGE_ERROR("", "", "itemType [" + itemType + "] unknown");
	}

	MESSAGE_DEBUG("", "", "finish (result = " + to_string(result) + ")");
	
	return result;
}
*/


string GetSpecificData_GetBaseDirectory(string itemType)
{
	MESSAGE_DEBUG("", "", "start");

	map<string, string>	dictionary = 
	{

		{ "certification"				, IMAGE_CERTIFICATIONS_DIRECTORY },
		{ "course"						, IMAGE_CERTIFICATIONS_DIRECTORY },
		{ "university"					, IMAGE_UNIVERSITIES_DIRECTORY },
		{ "school"						, IMAGE_SCHOOLS_DIRECTORY },
		{ "language"					, IMAGE_FLAGS_DIRECTORY },
		{ "book"						, IMAGE_BOOKS_DIRECTORY },
		{ "company"						, IMAGE_COMPANIES_DIRECTORY },
		{ "company_profile_logo"		, IMAGE_COMPANIES_DIRECTORY },
		{ "gift"						, IMAGE_GIFTS_DIRECTORY },
		{ "event"						, IMAGE_EVENTS_DIRECTORY },
		{ "helpdesk_ticket_attach"		, HELPDESK_TICKET_ATTACHES_DIRECTORY },
	};

	auto	  result = __GetMapValueByKey(dictionary, itemType);

	MESSAGE_DEBUG("", "", "finish (" + itemType + " -> " + result + ")");

	return result;
}
/*
string GetSpecificData_GetBaseDirectory(string itemType)
{
	string	  result = "";

	MESSAGE_DEBUG("", "", "start");

	if(itemType == "certification")					result = IMAGE_CERTIFICATIONS_DIRECTORY;
	else if(itemType == "course")					result = IMAGE_CERTIFICATIONS_DIRECTORY;
	else if(itemType == "university")				result = IMAGE_UNIVERSITIES_DIRECTORY;
	else if(itemType == "school")					result = IMAGE_SCHOOLS_DIRECTORY;
	else if(itemType == "language")					result = IMAGE_FLAGS_DIRECTORY;
	else if(itemType == "book")						result = IMAGE_BOOKS_DIRECTORY;
	else if(itemType == "company")					result = IMAGE_COMPANIES_DIRECTORY;
	else if(itemType == "gift")						result = IMAGE_GIFTS_DIRECTORY;
	else if(itemType == "event")					result = IMAGE_EVENTS_DIRECTORY;
	else if(itemType == "helpdesk_ticket_attach")	result = HELPDESK_TICKET_ATTACHES_DIRECTORY;
	else
	{
		MESSAGE_ERROR("", "", "itemType [" + itemType + "] unknown");
	}

	MESSAGE_DEBUG("", "", "finish (" + result + ")");
	
	return result;
}
*/

string GetSpecificData_SelectQueryItemByID(string itemID, string itemType)
{
	string	  result = "";

	MESSAGE_DEBUG("", "", "start");

	if(itemType == "certification")		result = "select * from `certification_tracks` where `id`=\"" + itemID + "\";";
	else if(itemType == "course")		result = "select * from `certification_tracks` where `id`=\"" + itemID + "\";";
	else if(itemType == "university")	result = "select * from `university` where `id`=\"" + itemID + "\";";
	else if(itemType == "school")		result = "select * from `school` where `id`=\"" + itemID + "\";";
	else if(itemType == "language")		result = "select * from `language` where `id`=\"" + itemID + "\";";
	else if(itemType == "book")			result = "select * from `book` where `id`=\"" + itemID + "\";";
	else if(itemType == "company")		result = "select * from `company` where `id`=\"" + itemID + "\";";
	else if(itemType == "gift")			result = "select * from `gifts` where `id`=\"" + itemID + "\";";
	else if(itemType == "event")		result = "select * from `events` where `id`=\"" + itemID + "\";";
	else
	{
		MESSAGE_ERROR("", "", "itemType [" + itemType + "] unknown");
	}

	MESSAGE_DEBUG("", "", "finish (" + result + ")");
	
	return result;
}

string GetSpecificData_UpdateQueryItemByID(string itemID, string itemType, string folderID, string fileName)
{
	string		result = "";
	string		logo_folder = "";
	string		logo_filename = "";

	MESSAGE_DEBUG("", "", "start");

	logo_folder = GetSpecificData_GetDBCoverPhotoFolderString(itemType);
	logo_filename = GetSpecificData_GetDBCoverPhotoFilenameString(itemType);

	if(logo_folder.length() && logo_filename.length())
	{
		if(itemType == "certification")		result = "update `certification_tracks` set	`" + logo_folder + "`='" + folderID + "', `" + logo_filename + "`='" + fileName + "' where `id`=\"" + itemID + "\";";
		else if(itemType == "course")		result = "update `certification_tracks` set `" + logo_folder + "`='" + folderID + "', `" + logo_filename + "`='" + fileName + "' where `id`=\"" + itemID + "\";";
		else if(itemType == "university")	result = "update `university` set 			`" + logo_folder + "`='" + folderID + "', `" + logo_filename + "`='" + fileName + "' where `id`=\"" + itemID + "\";";
		else if(itemType == "school")		result = "update `school` set 				`" + logo_folder + "`='" + folderID + "', `" + logo_filename + "`='" + fileName + "' where `id`=\"" + itemID + "\";";
		else if(itemType == "language")		result = "update `language` set 			`" + logo_folder + "`='" + folderID + "', `" + logo_filename + "`='" + fileName + "' where `id`=\"" + itemID + "\";";
		else if(itemType == "book")			result = "update `book` set 				`" + logo_folder + "`='" + folderID + "', `" + logo_filename + "`='" + fileName + "' where `id`=\"" + itemID + "\";";
		else if(itemType == "company")		result = "update `company` set 				`" + logo_folder + "`='" + folderID + "', `" + logo_filename + "`='" + fileName + "' where `id`=\"" + itemID + "\";";
		else if(itemType == "gift")			result = "update `gifts` set 				`" + logo_folder + "`='" + folderID + "', `" + logo_filename + "`='" + fileName + "' where `id`=\"" + itemID + "\";";
		else if(itemType == "event")		result = "update `events` set 				`" + logo_folder + "`='" + folderID + "', `" + logo_filename + "`='" + fileName + "' where `id`=\"" + itemID + "\";";
		else
		{
			CLog	log;
			log.Write(ERROR, string(__func__) + "[" + to_string(__LINE__) + "]:ERROR: itemType [" + itemType + "] unknown");
		}
	}
	else
	{
		{
			CLog	log;
			log.Write(ERROR, string(__func__) + "[" + to_string(__LINE__) + "]:ERROR: logo_folder or logo_filename not found for itemType [" + itemType + "]");
		}
	}


	MESSAGE_DEBUG("", "", "finish (" + result + ")");
	
	return result;
}


string GetSpecificData_GetDBCoverPhotoFolderString(string itemType)
{
	MESSAGE_DEBUG("", "", "start");

	map<string, string>	dictionary = 
	{
		{ "certification"					, "logo_folder" },
		{ "course"							, "logo_folder" },
		{ "university"						, "logo_folder" },
		{ "school"							, "logo_folder" },
		{ "language"						, "logo_folder" },
		{ "book"							, "coverPhotoFolder" },
		{ "company"							, "logo_folder" },
		{ "company_profile_logo"			, "logo_folder" },
		{ "gift"							, "logo_folder" },
		{ "event"							, "logo_folder" },
	};

	auto	  result = __GetMapValueByKey(dictionary, itemType);

	MESSAGE_DEBUG("", "", "finish (result = " + result + ")");

	return result;
}
/*
string GetSpecificData_GetDBCoverPhotoFolderString(string itemType)
{
	string	  result = "";

	MESSAGE_DEBUG("", "", "start");

	if(itemType == "certification")	 	result = "logo_folder";
	else if(itemType == "course")	   	result = "logo_folder";
	else if(itemType == "university")   result = "logo_folder";
	else if(itemType == "school")	   	result = "logo_folder";
	else if(itemType == "language")	 	result = "logo_folder";
	else if(itemType == "book")		 	result = "coverPhotoFolder";
	else if(itemType == "company")		result = "logo_folder";
	else if(itemType == "gift")	  		result = "logo_folder";
	else if(itemType == "event")	  	result = "logo_folder";
	else
	{
		MESSAGE_ERROR("", "", "itemType [" + itemType + "] unknown");
	}

	MESSAGE_DEBUG("", "", "finish (" + result + ")");
	
	return result;
}
*/

string GetSpecificData_GetDBCoverPhotoFilenameString(string itemType)
{
	MESSAGE_DEBUG("", "", "start");

	map<string, string>	dictionary = 
	{
		{ "certification"				, "logo_filename" },
		{ "course"						, "logo_filename" },
		{ "university"					, "logo_filename" },
		{ "school"						, "logo_filename" },
		{ "language"					, "logo_filename" },
		{ "book"						, "coverPhotoFilename" },
		{ "company"						, "logo_filename" },
		{ "company_profile_logo"		, "logo_filename" },
		{ "gift"						, "logo_filename" },
		{ "event"						, "logo_filename" },
	};

	auto	  result = __GetMapValueByKey(dictionary, itemType);

	MESSAGE_DEBUG("", "", "finish (result = " + result + ")");

	return result;
}
/*
string GetSpecificData_GetDBCoverPhotoFilenameString(string itemType)
{
	string	  result = "";

	MESSAGE_DEBUG("", "", "start");

	if(itemType == "certification")		result = "logo_filename";
	else if(itemType == "course")		result = "logo_filename";
	else if(itemType == "university")	result = "logo_filename";
	else if(itemType == "school")		result = "logo_filename";
	else if(itemType == "language")		result = "logo_filename";
	else if(itemType == "book")			result = "coverPhotoFilename";
	else if(itemType == "company")		result = "logo_filename";
	else if(itemType == "gift")			result = "logo_filename";
	else if(itemType == "event")		result = "logo_filename";
	else
	{
		MESSAGE_ERROR("", "", "itemType [" + itemType + "] unknown");
	}

	{
		CLog	log;
		log.Write(DEBUG, string(__func__) + "[" + to_string(__LINE__) + "]: finish (result: " + result + ")");
	}
	
	return result;
}
*/
string GetSpecificData_GetFinalFileExtension(string itemType)
{
	MESSAGE_DEBUG("", "", "start");

	map<string, string>	dictionary = 
	{

		{ "certification"				, ".jpg" },
		{ "course"						, ".jpg" },
		{ "university"					, ".jpg" },
		{ "school"						, ".jpg" },
		{ "language"					, ".jpg" },
		{ "book"						, ".jpg" },
		{ "company"						, ".jpg" },
		{ "company_profile_logo"		, ".jpg" },
		{ "gift"						, ".jpg" },
		{ "event"						, ".jpg" },
	};

	auto	  result = __GetMapValueByKey(dictionary, itemType);

	MESSAGE_DEBUG("", "", "finish (" + itemType + " -> " + result + ")");

	return result;
}

/*
string GetSpecificData_GetFinalFileExtension(string itemType)
{
	string	  result = ".jpg";

	MESSAGE_DEBUG("", "", "start");

	if(itemType == "template_sow")						result = ".txt";
	else if(itemType == "template_psow")				result = ".txt";
	else if(itemType == "template_costcenter")			result = ".txt";
	else if(itemType == "template_company")				result = ".txt";
	else if(itemType == "template_agreement_company")	result = ".txt";
	else if(itemType == "template_agreement_sow")		result = ".txt";
	else
	{
		MESSAGE_DEBUG("", "", "default extension(" + result + ") taken");
	}

	MESSAGE_DEBUG("", "", "finish (result = " + result + ")");

	return result;
}
*/


string GetSpecificData_GetDataTypeByItemType(const string &itemType)
{
	map<string, string>	dictionary = 
	{
		{ "certification"				, "image" },
		{ "course"						, "image" },
		{ "university"					, "image" },
		{ "school"						, "image" },
		{ "language"					, "image" },
		{ "book"						, "image" },
		{ "company"						, "image" },
		{ "company_profile_logo"		, "image" },
		{ "gift"						, "image" },
		{ "event"						, "image" },
		{ "template_sow"				, "template" },
		{ "template_psow"				, "template" },
		{ "template_costcenter"			, "template" },
		{ "template_company"			, "template" },
		{ "template_agreement_company"	, "template" },
		{ "template_agreement_sow"		, "template" }
	};

	auto	  result = __GetMapValueByKey(dictionary, itemType);

	MESSAGE_DEBUG("", "", "finish (result = " + result + ")");

	return result;
}
/*
string GetSpecificData_GetDataTypeByItemType(const string &itemType)
{
	auto	result = "image"s;

	MESSAGE_DEBUG("", "", "start");

	if(itemType == "template_sow")					result = "template";
	if(itemType == "template_psow")					result = "template";
	if(itemType == "template_costcenter")			result = "template";
	if(itemType == "template_company")				result = "template";
	if(itemType == "template_agreement_company")	result = "template";
	if(itemType == "template_agreement_sow")		result = "template";

	MESSAGE_DEBUG("", "", "finish (result = " + result + ")");

	return result;
}
*/

// --- Does the owner user allowed to change it ?
// --- For example:
// ---	*) university or school logo can be changed by administrator only.
// ---	*) gift image could be changed by owner
auto GetSpecificData_AllowedToChange(string itemID, string itemType, CMysql *db, CUser *user) -> string
{
	auto	  error_message = ""s;

	MESSAGE_DEBUG("", "", "start");

	if(db->Query(GetSpecificData_SelectQueryItemByID(itemID, itemType))) // --- item itemID exists ?
	{
		if((itemType == "course") || (itemType == "university") || (itemType == "school") || (itemType == "language") || (itemType == "book") || (itemType == "company") || (itemType == "certification"))
		{
			string	  coverPhotoFolder = db->Get(0, GetSpecificData_GetDBCoverPhotoFolderString(itemType).c_str());
			string	  coverPhotoFilename = db->Get(0, GetSpecificData_GetDBCoverPhotoFilenameString(itemType).c_str());

			if(coverPhotoFolder.empty() && coverPhotoFilename.empty()) {}
			else
			{
				error_message = "logo already uploaded";

				MESSAGE_DEBUG("", "", "access to " + itemType + "(" + itemID + ") denied, because logo already uploaded");
			}
		}
		else if(itemType == "event")
		{
			if(user)
			{
				if(db->Query("SELECT `id` FROM `event_hosts` WHERE `event_id`=\"" + itemID + "\" AND `user_id`=\"" + user->GetID() + "\";")) {}
				else
				{
					error_message = "you are not the event host";

					MESSAGE_DEBUG("", "", "access to " + itemType + "(" + itemID + ") denied, you are not the event host");
				}
			}
			else
			{
				error_message = "user object is NULL";

				MESSAGE_ERROR("", "", error_message);
			}
		}
		else if(itemType == "gift")
		{
			string		user_id = db->Get(0, "user_id");

			if(user)
			{
				if(user_id == user->GetID()) {}
				else
				{
					error_message = "you are not the gift owner";

					MESSAGE_DEBUG("", "", "access to " + itemType + "(" + itemID + ") denied, you are not the gift owner");
				}
			}
			else
			{
				error_message = "user object is NULL";

				MESSAGE_ERROR("", "", error_message);
			}
		}
		else
		{
			error_message = "itemType [" + itemType + "] unknown";

			MESSAGE_ERROR("", "", error_message);
		}
	}
	else
	{
		error_message = itemType + "(" + itemID + ") not found";

		MESSAGE_ERROR("", "", error_message);
	}

	MESSAGE_DEBUG("", "", "finish (error_message: " + error_message + ")");
	
	return error_message;
}

