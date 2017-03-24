#ifndef __UTILITIES__H__
#define __UTILITIES__H__

#include <fstream>  //used for CopyFile function
#include <map>
#include <vector>
#include <list>
#include <unordered_set>

using namespace std;

inline std::string rtrim(std::string& str)
{
	str.erase(str.find_last_not_of(' ')+1);         //suffixing spaces
	return str;
}

inline std::string ltrim(std::string& str)
{
	str.erase(0, str.find_first_not_of(' '));       //prefixing spaces
	return str;
}

inline std::string trim(std::string& str)
{
	rtrim(str);
	ltrim(str);
	return str;
}

string	quoted(string src)
{
	return '"' + src + '"';
}

string GetRandom(int len)
{
	string	result;
	int	i;

	for(i = 0; i < len; i++)
	{
		result += (char)('0' + (int)(rand()/(RAND_MAX + 1.0) * 10));
	}

	return result;
}

string DeleteHTML(string src)
{
    string 		result;
    string::size_type	firstPos, lastPos;

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "DeleteHTML(" << src << "): start";
		log.Write(DEBUG, ost.str());
    }

    result = src;
    firstPos = result.find("<");
    if(firstPos == string::npos) return result;
    lastPos = result.find(">", firstPos);
    if(lastPos == string::npos) lastPos = result.length();

    while(firstPos != string::npos)
    {
	result.erase(firstPos, lastPos - firstPos + 1);

        firstPos = result.find("<");
	if(firstPos == string::npos) break;
	lastPos = result.find(">", firstPos);
	if(lastPos == string::npos) lastPos = result.length();
    }

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "DeleteHTML(" << ost.str() << "): start";
		log.Write(DEBUG, ost.str());
    }

    return result;
}


/*
	Delete symbol " from string src
*/
string RemoveQuotas(string src)
{
    string		result = src;
    string::size_type	pos = 0;

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "RemoveQuotas(" << src << "): start";
		log.Write(DEBUG, ost.str());
    }

    while((pos = result.find("\"", pos)) != string::npos)
    {
        result.replace(pos, 1, "\\\"");
        pos += 2;
    }

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "RemoveQuotas(" << result << "): start";
		log.Write(DEBUG, ost.str());
    }

    return result;
}

/*
	Delete special symbols like \t \\ \<
*/
string RemoveSpecialSymbols(string src)
{
    string		result = src;
    string::size_type	pos = 0;

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "RemoveSpecialSymbols(" << src << "): start";
		log.Write(DEBUG, ost.str());
    }

    while ((result.length()) && (result.at(result.length() - 1) == '\\')) result.replace(result.length() - 1, 1, "");

    pos = 0;
    while((pos = result.find("\\", pos)) != string::npos)
    {
        result.replace(pos, 2, "");
        pos += 2;
    }


    pos = 0;
    while((pos = result.find("\t", pos)) != string::npos)
    {
        result.replace(pos, 1, " ");
    }

    pos = 0;
    while((pos = result.find("є", pos)) != string::npos)
    {
        result.replace(pos, 1, "N");
        pos += 1;
    }

    pos = 0;
    while((pos = result.find("Ч", pos)) != string::npos)
    {
        result.replace(pos, 1, "-");
        pos += 1;
    }


    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "RemoveSpecialSymbols(): end (" << result << ")";
		log.Write(DEBUG, ost.str());
    }

    return result;
}

/*
	Delete special symbols like \t \\ \<
*/
string RemoveSpecialHTMLSymbols(string src)
{
    string		result = src;
    string::size_type	pos = 0;

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "RemoveSpecialHTMLSymbols(" << src << "): start";
		log.Write(DEBUG, ost.str());
    }

    while ((result.length()) && (result.at(result.length() - 1) == '\\')) result.replace(result.length() - 1, 1, "");

    pos = 0;
    while((pos = result.find("\\", pos)) != string::npos)
    {
        result.replace(pos, 1, "&#92;");
    }


    pos = 0;
    while((pos = result.find("\t", pos)) != string::npos)
    {
        result.replace(pos, 1, " ");
    }

    pos = 0;
    while((pos = result.find("<", pos)) != string::npos)
    {
        result.replace(pos, 1, "&lt;");
    }

    pos = 0;
    while((pos = result.find(">", pos)) != string::npos)
    {
        result.replace(pos, 1, "&gt;");
    }

    pos = 0;
    while((pos = result.find("є", pos)) != string::npos)
    {
        result.replace(pos, 1, "N");
    }

    pos = 0;
    while((pos = result.find("Ч", pos)) != string::npos)
    {
        result.replace(pos, 1, "-");
    }

    pos = 0;
    while((pos = result.find("\"", pos)) != string::npos)
    {
        result.replace(pos, 1, "&quot;");
    }


    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "RemoveSpecialHTMLSymbols(): end (" << result << ")";
		log.Write(DEBUG, ost.str());
    }

    return result;
}

/*
	Change " symbol to " from string src
*/
string ReplaceDoubleQuoteToQuote(string src)
{
    string		result = src;
    string::size_type	pos = 0;

/*
    // --- commented to avoid logs flooding
    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "ReplaceDoubleQuoteToQuote(): start";
		log.Write(DEBUG, ost.str());
    }
*/
    while((pos = result.find("\"", pos)) != string::npos)
    {
        result.replace(pos, 1, "'");
        // pos += 2;
    }

/*
    // --- commented to avoid logs flooding
    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "ReplaceDoubleQuoteToQuote(): end";
		log.Write(DEBUG, ost.str());
    }
*/
    return result;
}

/*
	Change CR/CRLF symbol to <BR> from string src
*/
string ReplaceCRtoHTML(string src)
{
    string		result = src;
    string::size_type	pos = 0;

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "ReplaceCRtoHTML(): start";
		log.Write(DEBUG, ost.str());
    }


    pos = 0;
    while((pos = result.find("\r\n", pos)) != string::npos)
    {
        result.replace(pos, 2, "<br>");
        // pos += 1;
    }

    pos = 0;
    while((pos = result.find("\n", pos)) != string::npos)
    {
        result.replace(pos, 1, "<bR>");
    }

    pos = 0;
    while((pos = result.find("\r", pos)) != string::npos)
    {
        result.replace(pos, 1, "<Br>");
    }


    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "ReplaceCRtoHTML(): end";
		log.Write(DEBUG, ost.str());
    }

    return result;
}

string CleanUPText(const string messageBody)
{
	string 		result = messageBody;

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CleanUPText(): start";
		log.Write(DEBUG, ost.str());
    }

	result = DeleteHTML(result);
	result = ReplaceDoubleQuoteToQuote(result);
	result = ReplaceCRtoHTML(result);
	result = RemoveSpecialSymbols(result);
	trim(result);

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CleanUPText(): end";
		log.Write(DEBUG, ost.str());
    }

	return result;
}

/*
	Delete any special symbols
	Used only for matching duplicates
*/
string RemoveAllNonAlphabetSymbols(string src)
{
    string		result = src;
    string::size_type	pos = 0;

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "RemoveAllNonAlphabetSymbols(" << src << "): start";
		log.Write(DEBUG, ost.str());
    }

    while ((result.length()) && (result.at(result.length() - 1) == '\\')) result.replace(result.length() - 1, 1, "");

    pos = 0;
    while((pos = result.find("&lt;", pos)) != string::npos)
    {
        result.replace(pos, 4, "");
    }
    pos = 0;
    while((pos = result.find("&gt;", pos)) != string::npos)
    {
        result.replace(pos, 4, "");
    }
    pos = 0;
    while((pos = result.find("&quot;", pos)) != string::npos)
    {
        result.replace(pos, 6, "");
    }
    pos = 0;
    while((pos = result.find("&#92;", pos)) != string::npos)
    {
        result.replace(pos, 5, "");
    }
    pos = 0;
    while((pos = result.find(" ", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }
    pos = 0;
    while((pos = result.find("\\", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }
    pos = 0;
    while((pos = result.find("/", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }
    pos = 0;
    while((pos = result.find("\t", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }

    pos = 0;
    while((pos = result.find("<", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }

    pos = 0;
    while((pos = result.find(">", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }

    pos = 0;
    while((pos = result.find("є", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }

    pos = 0;
    while((pos = result.find("Ч", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }

    pos = 0;
    while((pos = result.find("\"", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }

	pos = 0;
    while((pos = result.find("'", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }
	pos = 0;
    while((pos = result.find(";", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }
	pos = 0;
    while((pos = result.find(":", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }


    pos = 0;
    while((pos = result.find("`", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }

    pos = 0;
    while((pos = result.find(".", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }

    pos = 0;
    while((pos = result.find(",", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }

    pos = 0;
    while((pos = result.find("%", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }

    pos = 0;
    while((pos = result.find("-", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }
    pos = 0;
    while((pos = result.find("N", pos)) != string::npos)
    {
        result.replace(pos, 1, "");
    }


    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "RemoveAllNonAlphabetSymbols(): end (" << result << ")";
		log.Write(DEBUG, ost.str());
    }

    return result;
}


string ConvertTextToHTML(const string messageBody)
{
	string 		result = messageBody;

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "ConvertTextToHTML(): start";
		log.Write(DEBUG, ost.str());
    }

	result = RemoveSpecialHTMLSymbols(result);
	result = DeleteHTML(result);
	result = ReplaceCRtoHTML(result);
	trim(result);

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "ConvertTextToHTML(): end";
		log.Write(DEBUG, ost.str());
    }

	return result;
}


string CheckIfFurtherThanNow(string occupationStart_cp1251) 
{
	time_t      now_t, checked_t;
    // char        utc_str[100];
    struct tm   *local_tm, check_tm;
    ostringstream	ost;

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CheckIfFurtherThanNow(" << occupationStart_cp1251 << "): start";
		log.Write(DEBUG, ost.str());
    }


    now_t = time(NULL);
    local_tm = localtime(&now_t);
    if(local_tm == NULL) 
    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CheckIfFurtherThanNow(now): ERROR in running localtime(&t)";
		log.Write(ERROR, ost.str());
    }

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CheckIfFurtherThanNow(now): now_t = " << now_t;
		log.Write(DEBUG, ost.str());
    }


    // now2_t = time(NULL);
    // check_tm = localtime(&now2_t);
    sscanf(occupationStart_cp1251.c_str(), "%4d-%2d-%2d", &check_tm.tm_year, &check_tm.tm_mon, &check_tm.tm_mday);
    check_tm.tm_year -= 1900;
    check_tm.tm_mon -= 1;
    check_tm.tm_hour = 23;
    check_tm.tm_min = 59;
    check_tm.tm_isdst = 0;	// --- Summer time is OFF. Be carefull with it.

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CheckIfFurtherThanNow(" << occupationStart_cp1251 << "): checked year = " << check_tm.tm_year << " checked month = " << check_tm.tm_mon << " checked day = " << check_tm.tm_mday << "";
		log.Write(DEBUG, ost.str());
    }

    checked_t = mktime(&check_tm);

    {
		CLog	log;
		ostringstream	ost;
		char	buffer[80];

		ost.str("");
		strftime(buffer,80,"check_tm: date regenerated: %02d-%b-%Y %T %Z  %I:%M%p.", &check_tm);
		ost << "CheckIfFurtherThanNow(" << occupationStart_cp1251 << "): " << buffer << "";
		log.Write(DEBUG, ost.str());

		memset(buffer, 0, 80);
		strftime(buffer,80,"local_tm: date regenerated: %02d-%b-%Y %T %Z  %I:%M%p.", local_tm);
		ost.str("");
		ost << "CheckIfFurtherThanNow(" << occupationStart_cp1251 << "): " << buffer << "";
		log.Write(DEBUG, ost.str());

		ost.str("");
		ost << "CheckIfFurtherThanNow(" << occupationStart_cp1251 << "): difftime( now_t=" << now_t << ", checked_t=" << checked_t << ")";
		log.Write(DEBUG, ost.str());

		ost.str("");
		ost << "CheckIfFurtherThanNow(" << occupationStart_cp1251 << "): difference = " << difftime(now_t, checked_t);
		log.Write(DEBUG, ost.str());
    }

    if(difftime(now_t, checked_t) <= 0)
    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CheckIfFurtherThanNow(" << occupationStart_cp1251 << "): clicked date further in futer than now, therefore considered as a 0000-00-00";
		log.Write(DEBUG, ost.str());

    	return "0000-00-00";
    }

    return occupationStart_cp1251;
}

double GetSecondsSinceY2k()
{
	time_t      timer;
    // struct tm   y2k = {0};
	double		seconds;
	double		secondsY2kUTC = 946684800;

	// y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
	// y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

	timer = time(NULL);  /* get current time; same as: timer = time(NULL)  */
	// seconds = difftime(timer,mktime(&y2k));
	seconds = timer - secondsY2kUTC;

    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "GetSecondsSinceY2k(): end (seconds since Y2k" << std::to_string(seconds) << ")";
		log.Write(DEBUG, ost.str());
    }

    return seconds;
}

string GetLocalFormattedTimestamp()
{
	time_t      now_t;
    struct tm   *local_tm;
    char        buffer[80];
    string		result = "";

    now_t = time(NULL);
    local_tm = localtime(&now_t);
    if(local_tm == NULL) 
    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "GetLocalFormatedTimestamp(): ERROR in running localtime(&t)";
		log.Write(ERROR, ost.str());
    }
    
    memset(buffer, 0, 80);
    strftime(buffer,80,"%Y-%m-%02d %T", local_tm);
    result = buffer;


    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "GetLocalFormatedTimestamp(): end (" << result << ")";
		log.Write(DEBUG, ost.str());
    }

    return result;

}

// --- input format: "2015-06-10 00:00:00"
// --- return: number of second difference from now 
double GetTimeDifferenceFromNow(const string timeAgo)
{
	time_t      now_t, checked_t;
    // char        utc_str[100];
    struct tm   *local_tm, check_tm;
    ostringstream	ost;
/*
    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "GetTimeDifferenceFromNow(" << timeAgo << "): start";
		log.Write(DEBUG, ost.str());
    }
*/

    now_t = time(NULL);
    local_tm = localtime(&now_t);
    if(local_tm == NULL) 
    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "GetTimeDifferenceFromNow(now): ERROR in running localtime(&t)";
		log.Write(ERROR, ost.str());
    }
/*
    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "GetTimeDifferenceFromNow(now): now_t = " << now_t;
		log.Write(DEBUG, ost.str());
    }
*/

    // now2_t = time(NULL);
    // check_tm = localtime(&now2_t);
    sscanf(timeAgo.c_str(), "%4d-%2d-%2d %2d:%2d:%2d", &check_tm.tm_year, &check_tm.tm_mon, &check_tm.tm_mday, &check_tm.tm_hour, &check_tm.tm_min, &check_tm.tm_sec);
    check_tm.tm_year -= 1900;
    check_tm.tm_mon -= 1;
    check_tm.tm_isdst = 0;	// --- "Summer time" is OFF. Be carefull with it.
							// --- Russia is not using Daylight saving
    						// --- USA is

    // --- For testing purposes try to use the same Daylight saving as in local clock
    // --- Test starts on 9/11. If there is no side effect, comment-out previous line.
    if(local_tm) check_tm.tm_isdst = local_tm->tm_isdst;	// --- "Summer time" is the same as locak clock.

/*
    {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "GetTimeDifferenceFromNow(" << timeAgo << "): checked year = " << check_tm.tm_year << " checked month = " << check_tm.tm_mon << " checked day = " << check_tm.tm_mday << " checked hour = " << check_tm.tm_hour << " checked min = " << check_tm.tm_min << " checked sec = " << check_tm.tm_sec;
		log.Write(DEBUG, ost.str());
    }
*/
    checked_t = mktime(&check_tm);
/*
    {
		CLog	log;
		ostringstream	ost;
		char	buffer[80];

		ost.str("");
		strftime(buffer,80,"check_tm: date regenerated: %02d-%b-%Y %T %Z  %I:%M%p.", &check_tm);
		ost << "GetTimeDifferenceFromNow(" << timeAgo << "): " << buffer << "";
		log.Write(DEBUG, ost.str());

		memset(buffer, 0, 80);
		strftime(buffer,80,"local_tm: date regenerated: %02d-%b-%Y %T %Z  %I:%M%p.", local_tm);
		ost.str("");
		ost << "GetTimeDifferenceFromNow(" << timeAgo << "): " << buffer << "";
		log.Write(DEBUG, ost.str());

		ost.str("");
		ost << "GetTimeDifferenceFromNow(" << timeAgo << "): difftime( now_t=" << now_t << ", checked_t=" << checked_t << ")";
		log.Write(DEBUG, ost.str());
	}
    {
		ost.str("");
		ost << "GetTimeDifferenceFromNow(" << timeAgo << "): end (difference = " << difftime(now_t, checked_t) << ")";
		log.Write(DEBUG, ost.str());
    }
*/
    return difftime(now_t, checked_t);
}

string GetMinutesDeclension(const int value)
{
	map<int, string> 	mapDeclension = {
		{1, "минуту"},
		{2, "минуты"},
		{3, "минут"}
	};

	string				result;

	if(value % 10 == 0) 						{ result = mapDeclension.at(3); };
	if(value % 10 == 1) 						{ result = mapDeclension.at(1); };
	if((value % 10 >= 2) and (value % 10 <= 4))	{ result = mapDeclension.at(2); };
	if((value % 10 >= 5) and (value % 10 <= 9))	{ result = mapDeclension.at(3); };
	if((value >= 5) and (value <= 20))			{ result = mapDeclension.at(3); };
	return result;
}


string GetHoursDeclension(const int value)
{
	map<int, string> 	mapDeclension = {
		{1, "час"},
		{2, "часа"},
		{3, "часов"}
	};
	string				result;

	if(value % 10 == 0) 						{ result = mapDeclension.at(3); };
	if(value % 10 == 1) 						{ result = mapDeclension.at(1); };
	if((value % 10 >= 2) and (value % 10 <= 4))	{ result = mapDeclension.at(2); };
	if((value % 10 >= 5) and (value % 10 <= 9))	{ result = mapDeclension.at(3); };
	if((value >= 5) and (value <= 20))			{ result = mapDeclension.at(3); };
	return result;
}

string GetDaysDeclension(const int value)
{
	map<int, string> 	mapDeclension = {
		{1, "день"},
		{2, "дн€"},
		{3, "дней"}
	};
	string				result;

	if(value % 10 == 0) 						{ result = mapDeclension.at(3); };
	if(value % 10 == 1) 						{ result = mapDeclension.at(1); };
	if((value % 10 >= 2) and (value % 10 <= 4))	{ result = mapDeclension.at(2); };
	if((value % 10 >= 5) and (value % 10 <= 9))	{ result = mapDeclension.at(3); };
	if((value >= 5) and (value <= 20))			{ result = mapDeclension.at(3); };
	return result;
}

string GetMonthsDeclension(const int value)
{
	map<int, string> 	mapDeclension = {
		{1, "мес€ц"},
		{2, "мес€ца"},
		{3, "мес€цев"}
	};
	string				result;

	if(value % 10 == 0) 						{ result = mapDeclension.at(3); };
	if(value % 10 == 1) 						{ result = mapDeclension.at(1); };
	if((value % 10 >= 2) and (value % 10 <= 4))	{ result = mapDeclension.at(2); };
	if((value % 10 >= 5) and (value % 10 <= 9))	{ result = mapDeclension.at(3); };
	if((value >= 5) and (value <= 20))			{ result = mapDeclension.at(3); };
	return result;
}

string GetYearsDeclension(const int value)
{
	map<int, string> 	mapDeclension = {
		{1, "год"},
		{2, "года"},
		{3, "лет"}
	};
	string				result;

	if(value % 10 == 0) 						{ result = mapDeclension.at(3); };
	if(value % 10 == 1) 						{ result = mapDeclension.at(1); };
	if((value % 10 >= 2) and (value % 10 <= 4))	{ result = mapDeclension.at(2); };
	if((value % 10 >= 5) and (value % 10 <= 9))	{ result = mapDeclension.at(3); };
	if((value >= 5) and (value <= 20))			{ result = mapDeclension.at(3); };
	return result;
}

// --- input format: "2015-06-10 00:00:00"
// --- return: human readable format
string GetHumanReadableTimeDifferenceFromNow (const string timeAgo)
{
	double			seconds = GetTimeDifferenceFromNow(timeAgo);
	double			minutes = seconds / 60;
	double			hours = minutes / 60;
	double			days = hours / 24;
	double			months = days / 30;
	double			years = months / 12;
	ostringstream	ost;

	ost.str("");
	if(years >= 1)
	{
		ost << (int)years << " " << GetYearsDeclension(years);
	} 
	else if(months >= 1)
	{
		ost << (int)months << " " << GetMonthsDeclension(months);
	}
	else if(days >= 1)
	{
		ost << (int)days << " " << GetDaysDeclension(days);
	}
	else if(hours >= 1)
	{
		ost << (int)hours << " " << GetHoursDeclension(hours);
	}
	else
	{
		ost << (int)minutes << " " << GetMinutesDeclension(minutes);
	}

	ost << " назад.";

/*
    // --- commented to reduce logs flooding
    {
		CLog	log;
		ostringstream	ost1;

		ost1.str("");
		ost1 << "string GetHumanReadableTimeDifferenceFromNow (" << timeAgo << "): sec difference (" << seconds << ") human format (" << ost.str() << ")" ;
		log.Write(DEBUG, ost1.str());
    }
*/

	return ost.str();
}


/*
Copyright (c) <YEAR>, <OWNER>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions a
re met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in th
e documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from t
his software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT L
IMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIG
HT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AN
D ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
int convert_utf8_to_windows1251(const char* utf8, char* windows1251, size_t n)
{
        int i = 0;
        int j = 0;
        int k = 0;

		typedef struct ConvLetter {
		        char    win1251;
		        int             unicode;
		} Letter;

	 	Letter	g_letters[] = {
        {(char)0x82, 0x201A}, // SINGLE LOW-9 QUOTATION MARK
        {(char)0x83, 0x0453}, // CYRILLIC SMALL LETTER GJE
        {(char)0x84, 0x201E}, // DOUBLE LOW-9 QUOTATION MARK
        {(char)0x85, 0x2026}, // HORIZONTAL ELLIPSIS
        {(char)0x86, 0x2020}, // DAGGER
        {(char)0x87, 0x2021}, // DOUBLE DAGGER
        {(char)0x88, 0x20AC}, // EURO SIGN
        {(char)0x89, 0x2030}, // PER MILLE SIGN
        {(char)0x8A, 0x0409}, // CYRILLIC CAPITAL LETTER LJE
        {(char)0x8B, 0x2039}, // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
        {(char)0x8C, 0x040A}, // CYRILLIC CAPITAL LETTER NJE
        {(char)0x8D, 0x040C}, // CYRILLIC CAPITAL LETTER KJE
        {(char)0x8E, 0x040B}, // CYRILLIC CAPITAL LETTER TSHE
        {(char)0x8F, 0x040F}, // CYRILLIC CAPITAL LETTER DZHE
        {(char)0x90, 0x0452}, // CYRILLIC SMALL LETTER DJE
        {(char)0x91, 0x2018}, // LEFT SINGLE QUOTATION MARK
        {(char)0x92, 0x2019}, // RIGHT SINGLE QUOTATION MARK
        {(char)0x93, 0x201C}, // LEFT DOUBLE QUOTATION MARK
        {(char)0x94, 0x201D}, // RIGHT DOUBLE QUOTATION MARK
        {(char)0x95, 0x2022}, // BULLET
        {(char)0x96, 0x2013}, // EN DASH
        {(char)0x97, 0x2014}, // EM DASH
        {(char)0x99, 0x2122}, // TRADE MARK SIGN
        {(char)0x9A, 0x0459}, // CYRILLIC SMALL LETTER LJE
        {(char)0x9B, 0x203A}, // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
        {(char)0x9C, 0x045A}, // CYRILLIC SMALL LETTER NJE
        {(char)0x9D, 0x045C}, // CYRILLIC SMALL LETTER KJE
        {(char)0x9E, 0x045B}, // CYRILLIC SMALL LETTER TSHE
        {(char)0x9F, 0x045F}, // CYRILLIC SMALL LETTER DZHE
        {(char)0xA0, 0x00A0}, // NO-BREAK SPACE
        {(char)0xA1, 0x040E}, // CYRILLIC CAPITAL LETTER SHORT U
        {(char)0xA2, 0x045E}, // CYRILLIC SMALL LETTER SHORT U
        {(char)0xA3, 0x0408}, // CYRILLIC CAPITAL LETTER JE
        {(char)0xA4, 0x00A4}, // CURRENCY SIGN
        {(char)0xA5, 0x0490}, // CYRILLIC CAPITAL LETTER GHE WITH UPTURN
        {(char)0xA6, 0x00A6}, // BROKEN BAR
        {(char)0xA7, 0x00A7}, // SECTION SIGN
        {(char)0xA8, 0x0401}, // CYRILLIC CAPITAL LETTER IO
        {(char)0xA9, 0x00A9}, // COPYRIGHT SIGN
        {(char)0xAA, 0x0404}, // CYRILLIC CAPITAL LETTER UKRAINIAN IE
        {(char)0xAB, 0x00AB}, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
        {(char)0xAC, 0x00AC}, // NOT SIGN
        {(char)0xAD, 0x00AD}, // SOFT HYPHEN
        {(char)0xAE, 0x00AE}, // REGISTERED SIGN
        {(char)0xAF, 0x0407}, // CYRILLIC CAPITAL LETTER YI
        {(char)0xB0, 0x00B0}, // DEGREE SIGN
        {(char)0xB1, 0x00B1}, // PLUS-MINUS SIGN
        {(char)0xB2, 0x0406}, // CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I
        {(char)0xB3, 0x0456}, // CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I
        {(char)0xB4, 0x0491}, // CYRILLIC SMALL LETTER GHE WITH UPTURN
        {(char)0xB5, 0x00B5}, // MICRO SIGN
        {(char)0xB6, 0x00B6}, // PILCROW SIGN
        {(char)0xB7, 0x00B7}, // MIDDLE DOT
        {(char)0xB8, 0x0451}, // CYRILLIC SMALL LETTER IO
        {(char)0xB9, 0x2116}, // NUMERO SIGN
        {(char)0xBA, 0x0454}, // CYRILLIC SMALL LETTER UKRAINIAN IE
        {(char)0xBB, 0x00BB}, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
        {(char)0xBC, 0x0458}, // CYRILLIC SMALL LETTER JE
        {(char)0xBD, 0x0405}, // CYRILLIC CAPITAL LETTER DZE
        {(char)0xBE, 0x0455}, // CYRILLIC SMALL LETTER DZE
        {(char)0xBF, 0x0457} // CYRILLIC SMALL LETTER YI
	};


    {
        CLog    log;
        log.Write(DEBUG, "convert_utf8_to_windows1251: start");
    }


        for(; i < (int)n && utf8[i] != 0; ++i) {
                char prefix = utf8[i];
                char suffix = utf8[i+1];
                if ((prefix & 0x80) == 0) {
                        windows1251[j] = (char)prefix;
                        ++j;
                } else if ((~prefix) & 0x20) {
                        int first5bit = prefix & 0x1F;
                        first5bit <<= 6;
                        int sec6bit = suffix & 0x3F;
                        int unicode_char = first5bit + sec6bit;


                        if ( unicode_char >= 0x410 && unicode_char <= 0x44F ) {
                                windows1251[j] = (char)(unicode_char - 0x350);
                        } else if (unicode_char >= 0x80 && unicode_char <= 0xFF) {
                                windows1251[j] = (char)(unicode_char);
                        } else if (unicode_char >= 0x402 && unicode_char <= 0x403) {
                                windows1251[j] = (char)(unicode_char - 0x382);
                        } else {
                                int count = sizeof(g_letters) / sizeof(Letter);
                                for (k = 0; k < count; ++k) {
                                        if (unicode_char == g_letters[k].unicode) {
                                                windows1251[j] = g_letters[k].win1251;
                                                goto NEXT_LETTER;
                                        }
                                }
                                // can't convert this char
							    {
							        CLog    log;
							        ostringstream	ost;

							        ost.str("");
							        ost << "convert_utf8_to_windows1251: ERROR (1) can't convert this " << i << " char " << hex << (int)prefix;
							        log.Write(ERROR, ost.str());
							    }
                                return 0;
                        }
NEXT_LETTER:
                        ++i;
                        ++j;
                } else {
                        // can't convert this chars
					    {
					        CLog    log;
					        ostringstream	ost;

					        ost.str("");
					        ost << "convert_utf8_to_windows1251: ERROR (2) can't convert this " << i << " char " << hex << (int)prefix;
					        log.Write(ERROR, ost.str());
					    }
                        return 0;
                }
        }
        windows1251[j] = 0;

    {
        CLog    log;
        log.Write(DEBUG, "convert_utf8_to_windows1251: end");
    }

        return 1;
}

bool convert_cp1251_to_utf8(const char *in, char *out, int size) {
    const char table[129*3] = {                 
        "\320\202 \320\203 \342\200\232\321\223 \342\200\236\342\200\246\342\200\240\342\200\241"
        "\342\202\254\342\200\260\320\211 \342\200\271\320\212 \320\214 \320\213 \320\217 "      
        "\321\222 \342\200\230\342\200\231\342\200\234\342\200\235\342\200\242\342\200\223\342\200\224"
        "   \342\204\242\321\231 \342\200\272\321\232 \321\234 \321\233 \321\237 "                     
        "\302\240 \320\216 \321\236 \320\210 \302\244 \322\220 \302\246 \302\247 "                     
        "\320\201 \302\251 \320\204 \302\253 \302\254 \302\255 \302\256 \320\207 "                     
        "\302\260 \302\261 \320\206 \321\226 \322\221 \302\265 \302\266 \302\267 "
        "\321\221 \342\204\226\321\224 \302\273 \321\230 \320\205 \321\225 \321\227 "
        "\320\220 \320\221 \320\222 \320\223 \320\224 \320\225 \320\226 \320\227 "
        "\320\230 \320\231 \320\232 \320\233 \320\234 \320\235 \320\236 \320\237 "
        "\320\240 \320\241 \320\242 \320\243 \320\244 \320\245 \320\246 \320\247 "
        "\320\250 \320\251 \320\252 \320\253 \320\254 \320\255 \320\256 \320\257 "
        "\320\260 \320\261 \320\262 \320\263 \320\264 \320\265 \320\266 \320\267 "
        "\320\270 \320\271 \320\272 \320\273 \320\274 \320\275 \320\276 \320\277 "
        "\321\200 \321\201 \321\202 \321\203 \321\204 \321\205 \321\206 \321\207 "
        "\321\210 \321\211 \321\212 \321\213 \321\214 \321\215 \321\216 \321\217 "
    };
    int	counter = 0;
    bool result = true;

    {
        CLog    log;
        log.Write(DEBUG, "convert_cp1251_to_utf8: start");
    }

    while (*in)
        if (*in & 0x80) {
            const char *p = &table[3 * (0x7f & *in++)];
            if (*p == ' ')
                continue;
            counter += 2;
            if(counter > size) 
            {
                result = false;
                break;
            }
            *out++ = *p++;
            *out++ = *p++;

            if (*p == ' ')
                continue;

            counter++;
            if(counter > size) 
            {
                result = false;
                break;
            }
            *out++ = *p++;
        }
        else
        {
            counter++;
            if(counter > size) 
            {
                result = false;
                break;
            }
            *out++ = *in++;
        }
    *out = 0;

    if(!result)
    {
        CLog    log;
        log.Write(ERROR, "convert_cp1251_to_utf8: ERROR: buffer size is not enough");
    }

	{
		CLog	log;
		log.Write(DEBUG, "convert_cp1251_to_utf8: end");
	}
    return result;
}

inline bool isFileExists(const std::string& name) {
	struct stat buffer;
	return (stat (name.c_str(), &buffer) == 0);
}

string SymbolReplace(const string where, const string src, const string dst)
{
    string                  result;
    string::size_type       pos;
		
    result = where;
		
    pos = result.find(src);
    while(pos != string::npos)
    {
        result.replace(pos, src.length(), dst);
        pos = result.find(src, pos + 1);
    }
    return result;
}

bool CheckUserEmailExisting(string userNameToCheck, CMysql *db) {
	CUser		user;

	{
		CLog	log;
		log.Write(DEBUG, "CheckUserEmailExisting: start");
	}

	user.SetDB(db);
	user.SetLogin(userNameToCheck);
	user.SetEmail(userNameToCheck);

	if(user.isLoginExist() or user.isEmailDuplicate()) {
		{
			CLog	log;
			log.Write(DEBUG, "CheckUserEmailExisting: login or email already registered");
		}
		return true;
	}
	else {
		{
			CLog	log;
			log.Write(DEBUG, "CheckUserEmailExisting: login or email not yet exists");
		}
		return false;
	}
}

// --- Quote Words: split string into vector<string>
// --- input: string, reference to vector
// --- output: success status
//				1 - success
//				0 - fail
int	qw(const string src, vector<string> &dst)
{
	std::size_t	prevPointer = 0, nextPointer;
	string		trimmedStr = src;
	int			wordCounter = 0;

	trim(trimmedStr);

	prevPointer = 0, wordCounter = 0;
	do
	{
		nextPointer = trimmedStr.find(" ", prevPointer);
		if(nextPointer == string::npos)
		{
			dst.push_back(trimmedStr.substr(prevPointer));
		}
		else
		{
			dst.push_back(trimmedStr.substr(prevPointer, nextPointer - prevPointer));
		}
		prevPointer = nextPointer + 1;
		wordCounter++;
	} while( (nextPointer != string::npos) );

	return 1;
}

auto UniqueUserIDInUserIDLine(string userIDLine) //-> decltype(static_cast<string>("123"))
{
	list<long int>	listUserID;
	string			result {""};
	std::size_t		prevPointer {0}, nextPointer;

	{
		CLog	log;
		log.Write(DEBUG, "UniqueUserIDInUserIDLine: start (", userIDLine, ")");
	}

	do
	{
		nextPointer = userIDLine.find(",", prevPointer);
		if(nextPointer == string::npos)
		{
			listUserID.push_back(atol(userIDLine.substr(prevPointer).c_str()));
		}
		else
		{
			listUserID.push_back(atol(userIDLine.substr(prevPointer, nextPointer - prevPointer).c_str()));
		}
		prevPointer = nextPointer + 1;
	} while( (nextPointer != string::npos) );

	listUserID.sort();
	listUserID.unique();
	for(auto it: listUserID)
	{
		result += (result.length() ? "," : "") + to_string(it);
	}

	{
		CLog	log;
		log.Write(DEBUG, "UniqueUserIDInUserIDLine: end (result ", result, ")");
	}

	return result;	
}

string GetChatMessagesInJSONFormat(string dbQuery, CMysql *db)
{
	ostringstream	result, ost;
	int				affected;

	{
		CLog	log;
		log.Write(DEBUG, "GetChatMessagesInJSONFormat: start");
	}
	result.str("");

	affected = db->Query(dbQuery);
	if(affected)
	{
		for(int i = 0; i < affected; i++)
		{
			result << (i ? "," : "") << "{\
				\"id\": \""						<< db->Get(i, "id") << "\", \
				\"message\": \"" 				<< ReplaceDoubleQuoteToQuote(db->Get(i, "message")) << "\", \
				\"fromType\": \"" 				<< db->Get(i, "fromType") << "\",\
				\"fromID\": \""					<< db->Get(i, "fromID") << "\",\
				\"toType\": \""			 		<< db->Get(i, "toType") << "\",\
				\"toID\": \""	 				<< db->Get(i, "toID") << "\",\
				\"messageStatus\": \""			<< db->Get(i, "messageStatus") << "\",\
				\"eventTimestampDelta\": \""	<< GetHumanReadableTimeDifferenceFromNow(db->Get(i, "eventTimestamp")) << "\",\
				\"secondsSinceY2k\": \""		<< db->Get(i, "secondsSinceY2k") << "\",\
				\"eventTimestamp\": \""			<< db->Get(i, "eventTimestamp") << "\"\
			}";
		}
	}
	
	{
		CLog	log;
		log.Write(DEBUG, "GetChatMessagesInJSONFormat: end");
	}

	return	result.str();
}


// --- Returns user list in JSON format grabbed from DB
// --- Input: dbQuery - SQL format returns users
//            db      - DB connection
//            user    - current user object
string GetUserListInJSONFormat(string dbQuery, CMysql *db, CUser *user)
{
	ostringstream	                 ost, ost1;
	string			                 sessid, lookForKey;
	CMysql			                 db1;
	int				                 affected, affected1;
    unordered_set<unsigned long>     setOfUserID;



	{
		CLog	log;
		log.Write(DEBUG, "GetUserListInJSONFormat: start");
	}

	if((affected = db->Query(dbQuery)) > 0)
	{
		if(db1.Connect(DB_NAME, DB_LOGIN, DB_PASSWORD) < 0)
		{
			CLog	log;
	
			log.Write(ERROR, "GetUserListInJSONFormat: Can't connect to mysql database");
			throw CExceptionHTML("MySql connection");
		}

#ifdef MYSQL_4
		db1.Query("set names cp1251");
#endif


		ost.str("");
		for(int i = 0; i < affected; i++) 
		{

            if(setOfUserID.find(atol(db->Get(i, "id"))) == setOfUserID.end())
            {
                string                           userID, userLogin, userName, userNameLast, userCurrentEmployment, userCurrentCity, avatarPath;
                string                           userLastOnline, numberUreadMessages, userLastOnlineSecondSinceY2k;
                string                           userFriendship;
                ostringstream                    ost1;

                userID = db->Get(i, "id");
                userLogin = db->Get(i, "login");
                userName = db->Get(i, "name");
                userNameLast = db->Get(i, "nameLast");
                userCurrentCity = "не определено";
                userLastOnline = db->Get(i, "last_online");
                userLastOnlineSecondSinceY2k = db->Get(i, "last_onlineSecondsSinceY2k");

                setOfUserID.insert(atol(userID.c_str()));

                // --- Defining title and company of user
                ost1.str("");
    			ost1 << "SELECT `users_company_position`.`title` as `users_company_position_title`,  \
    					`company`.`name` as `company_name`, `company`.`id` as `company_id`  \
    					FROM `users_company` \
    					LEFT JOIN  `users_company_position` ON `users_company_position`.`id`=`users_company`.`position_title_id` \
    					LEFT JOIN  `company` 				ON `company`.`id`=`users_company`.`company_id` \
    					WHERE `users_company`.`user_id`=\"" << userID << "\" and `users_company`.`current_company`='1' \
    					ORDER BY  `users_company`.`occupation_start` DESC ";

    			affected1 = db1.Query(ost1.str());
    			ost1.str("");
    			ost1 << "[";
    			if(affected1 > 0)
    			{
    				for(int j = 0; j < affected1; j++)
    				{
    					ost1 << "{ \
    							\"companyID\": \"" << db1.Get(j, "company_id") << "\", \
    							\"company\": \"" << db1.Get(j, "company_name") << "\", \
    							\"title\": \"" << db1.Get(j, "users_company_position_title") << "\" \
    							}";
    					if(j < (affected1 - 1)) ost1 << ", ";
    				}
    			}
    			ost1 << "]";
    			userCurrentEmployment = ost1.str(); 

    			{
    				CLog	log;

    				log.Write(DEBUG, "GetUserListInJSONFormat: done with building employment list ", userCurrentEmployment);
    			}

    			// --- Get user avatars
    			ost1.str("");
    			ost1 << "select * from `users_avatars` where `userid`='" << userID << "' and `isActive`='1';";
    			avatarPath = "empty";
    			if(db1.Query(ost1.str()))
    			{
    				ost1.str("");
    				ost1 << "/images/avatars/avatars" << db1.Get(0, "folder") << "/" << db1.Get(0, "filename");
    				avatarPath = ost1.str();
    			}

    			// --- Get friendship status
    			ost1.str("");
    			ost1 << "select * from `users_friends` where `userid`='" << user->GetID() << "' and `friendID`='" << userID << "';";
    			userFriendship = "empty";
    			if(db1.Query(ost1.str()))
    			{
    				userFriendship = db1.Get(0, "state");
    			}

    			// --- Get presense status for chat purposes
    			ost1.str("");
    			ost1 << "select COUNT(*) as `number_unread_messages` from `chat_messages` where `fromType`='fromUser' and `fromID`='" << userID << "' and (`messageStatus`='unread' or `messageStatus`='sent' or `messageStatus`='delivered');";
    			if(db1.Query(ost1.str()))
    			{
    				numberUreadMessages = db1.Get(0, "number_unread_messages");
    			}

    			if(ost.str().length()) ost << ", ";

                ost << "{ \"id\": \""                           << userID << "\", \
                          \"name\": \""                         << userName << "\", \
                          \"nameLast\": \""                     << userNameLast << "\",\
                          \"last_online\": \""                  << userLastOnline << "\",\
                          \"last_online_diff\": \""             << GetTimeDifferenceFromNow(userLastOnline) << "\",\
                          \"last_onlineSecondsSinceY2k\": \""   << userLastOnlineSecondSinceY2k << "\",\
                          \"userFriendship\": \""               << userFriendship << "\",\
                          \"avatar\": \""                       << avatarPath << "\",\
                          \"currentEmployment\": "              << userCurrentEmployment << ", \
                          \"currentCity\": \""                  << userCurrentCity << "\", \
                          \"numberUnreadMessages\": \""         << numberUreadMessages << "\", \
                          \"isMe\": \""                         << ((userID == user->GetID()) ? "yes" : "no") << "\" \
                        }";
    		} // --- if user is not dupicated
		} // --- for loop through user list
	} // --- if sql-query on user selection success
	else
	{
		CLog	log;

		ost.str("");
		log.Write(DEBUG, "GetUserListInJSONFormat: there are users returned by request [", dbQuery, "]");
	}

	{
		CLog	log;
		ostringstream	ostTemp;

		ostTemp.str("");
		ostTemp << "GetUserListInJSONFormat: end. returning string length " << ost.str().length();
		log.Write(DEBUG, ostTemp.str());
	}

	return ost.str();
}

// --- Returns company list in JSON format grabbed from DB
// --- Input: dbQuery - SQL format returns users
//            db      - DB connection
//            user    - current user object
string GetCompanyListInJSONFormat(string dbQuery, CMysql *db, CUser *user, bool quickSearch = true)
{
    struct CompanyClass {
        string    id, name, foundationDate, numberOfEmployee, admin_userID, webSite, description;
        string    type, logo_folder, logo_filename;
    };

    ostringstream                   ost, ostFinal;
    string                          sessid, lookForKey;
    int                             affected;
    vector<CompanyClass>            companiesList;
    int                             companyCounter = 0;

    {
        CLog    log;
        log.Write(DEBUG, "GetCompanyListInJSONFormat: start");
    }

    ostFinal.str("");

    if((affected = db->Query(dbQuery)) > 0)
    {
        companyCounter = affected;
        companiesList.reserve(companyCounter);  // --- reserving allows avoid moving vector in future
                                                // --- to fit vector into continous memory piece

        for(int i = 0; i < affected; i++)
        {
            CompanyClass    company;

            company.id = db->Get(i, "id");
            company.name = db->Get(i, "name");
            company.admin_userID = db->Get(i, "admin_userID");
            company.foundationDate = db->Get(i, "foundationDate");
            company.numberOfEmployee = db->Get(i, "numberOfEmployee");
            company.webSite = db->Get(i, "webSite");
            company.description = db->Get(i, "description");
            company.type = db->Get(i, "type");
            company.logo_folder = db->Get(i, "logo_folder");
            company.logo_filename = db->Get(i, "logo_filename");

            companiesList.push_back(company);
        }

        for(int i = 0; i < companyCounter; i++)
        {
                string              companyOwners = "";
                string              companyFounders = "";
                string              companyIndustry = "";


                if(!quickSearch)
                {
                    ost.str("");
                    ost << "select `company_industry_ref`.`id` as `company_industry_ref_id`, `company_industry`.`name` as `company_industry_name` from `company_industry_ref` \
    right join `company_industry` on `company_industry_ref`.`profile_id`=`company_industry`.`id`\
    where `company_industry_ref`.`company_id`=\"" << companiesList[i].id << "\"";
                    affected = db->Query(ost.str());
                    if(affected)
                        for(int i = 0; i < affected; i++) 
                        {
                            if(i) companyIndustry += ",";
                            companyIndustry += "{\"company_industry_ref_id\":\"";
                            companyIndustry += db->Get(i, "company_industry_ref_id");
                            companyIndustry += "\",\"name\":\"";
                            companyIndustry += db->Get(i, "company_industry_name");
                            companyIndustry += "\"}";
                        }

                    ost.str("");
                    ost << "select * from `company_owner`  where `companyID`=\"" << companiesList[i].id << "\"";
                    affected = db->Query(ost.str());
                    if(affected)
                        for(int i = 0; i < affected; i++) 
                        {
                            if(i) companyOwners += ",";
                            companyOwners += "{\"id\":\"";
                            companyOwners += db->Get(i, "id");
                            companyOwners += "\",\"name\":\"";
                            companyOwners += db->Get(i, "owner_name");
                            companyOwners += "\"}";
                        }

                    ost.str("");
                    ost << "select * from `company_founder`  where `companyID`=\"" << companiesList[i].id << "\"";
                    affected = db->Query(ost.str());
                    if(affected)
                    {
                        struct  CompanyFounderType 
                        {
                            string  id, name, userID;
                        };
                        vector<CompanyFounderType>    tempVector;
                        int                           vectorSize = affected;

                        tempVector.reserve(vectorSize);

                        for(int i = 0; i < vectorSize; i++) 
                        {
                            CompanyFounderType  tempObj;

                            tempObj.id = db->Get(i, "id");
                            tempObj.name = db->Get(i, "founder_name");
                            tempObj.userID = db->Get(i, "founder_userID");

                            tempVector.push_back(tempObj);
                        }

                        for(int i = 0; i < vectorSize; i++) 
                        {
                            if(i) companyFounders += ",";
                            companyFounders += "{\"id\":\"";
                            companyFounders += tempVector.at(i).id;
                            companyFounders += "\",\"name\":\"";
                            if(tempVector.at(i).userID != "0")
                            {
                                ost.str("");
                                ost << "select * from `users` where `id`=\"" << tempVector.at(i).userID << "\";";
                                if(db->Query(ost.str()))
                                {
                                    companyFounders += db->Get(0, "name");
                                    companyFounders += " ";
                                    companyFounders += db->Get(0, "nameLast");
                                }
                                else
                                {
                                    CLog    log;
                                    log.Write(DEBUG, "GetCompanyListInJSONFormat: there are no user with ID [", tempVector.at(i).userID, "]");
                                }
                            }
                            else
                                companyFounders += tempVector.at(i).name;
                            companyFounders += "\",\"userid\":\"";
                            companyFounders += tempVector.at(i).userID;
                            companyFounders += "\"}";
                        }
                    }
                }

                if(ostFinal.str().length()) ostFinal << ", ";

                ostFinal << "{ \"id\": \""                           << companiesList[i].id << "\",";
                ostFinal <<   "\"name\": \""                         << companiesList[i].name << "\", ";
                ostFinal <<   "\"foundationDate\": \""               << companiesList[i].foundationDate << "\",";
                ostFinal <<   "\"numberOfEmployee\": \""             << companiesList[i].numberOfEmployee << "\",";
                ostFinal <<   "\"webSite\": \""                      << companiesList[i].webSite << "\",";
                ostFinal <<   "\"description\": \""                  << companiesList[i].description << "\",";
                ostFinal <<   "\"type\": \""                         << companiesList[i].type << "\",";
                ostFinal <<   "\"logo_folder\": \""                  << companiesList[i].logo_folder << "\",";
                ostFinal <<   "\"logo_filename\": \""                << companiesList[i].logo_filename << "\",";
                ostFinal <<   "\"industries\": ["                    << companyIndustry << "],";
                ostFinal <<   "\"owners\": ["                        << companyOwners << "],";
                ostFinal <<   "\"founders\": ["                      << companyFounders << "],";
                ostFinal <<   "\"isMine\": \""                       << (companiesList[i].admin_userID == user->GetID()) << "\",";
                ostFinal <<   "\"isFree\": \""                       << (companiesList[i].admin_userID == "0") << "\"";
                ostFinal << "}";
        } // --- for loop through company list
    } // --- if sql-query on company selection success
    else
    {
        CLog    log;

        log.Write(DEBUG, "GetCompanyListInJSONFormat: there are no companies returned by request [", dbQuery, "]");
    }

    {
        CLog    log;
        ostringstream   ostTemp;

        ostTemp.str("");
        ostTemp << "GetCompanyListInJSONFormat: end. returning number of companies is " << companyCounter;
        log.Write(DEBUG, ostTemp.str());
    }

    return ostFinal.str();
}


string GetUreadChatMessagesInJSONFormat(CUser *user, CMysql *db)
{
	ostringstream	result, ost;
	int				affected;

	{
		CLog	log;
		log.Write(DEBUG, "GetUreadChatMessagesInJSONFormat: start");
	}

	result.str("");

	ost.str("");
	ost << "select * from `chat_messages` where `toID`='" << user->GetID() << "' and (`messageStatus`='unread' or `messageStatus`='delivered' or `messageStatus`='sent');";
	affected = db->Query(ost.str());
	if(affected)
	{
		for(int i = 0; i < affected; i++)
		{
			result << (i ? "," : "") << "{\
				\"id\": \""					<< db->Get(i, "id") << "\", \
				\"message\": \"" 			<< ReplaceDoubleQuoteToQuote(db->Get(i, "message")) << "\", \
				\"fromType\": \"" 			<< db->Get(i, "fromType") << "\",\
				\"fromID\": \""				<< db->Get(i, "fromID") << "\",\
				\"toType\": \""			 	<< db->Get(i, "toType") << "\",\
				\"toID\": \""	 			<< db->Get(i, "toID") << "\",\
				\"messageStatus\": \""		<< db->Get(i, "messageStatus") << "\",\
				\"eventTimestamp\": \""		<< db->Get(i, "eventTimestamp") << "\"\
			}";
		}
	}
	
	{
		CLog	log;
		log.Write(DEBUG, "GetUreadChatMessagesInJSONFormat: end");
	}

	return	result.str();
}


// --- Function returns list of images belongs to imageSet
// --- input: imageSetID, db
// --- output: list of image objects
string GetMessageImageList(string imageSetID, CMysql *db)
{
	ostringstream	ost;
    string          result = "";

    {
        CLog    log;
        log.Write(DEBUG, "GetMessageImageList: start");
    }

    if(imageSetID != "0")
    {
    	int				affected;
        
		ost.str("");
		ost << "select * from `feed_images` where `setID`='" << imageSetID << "';";
		affected = db->Query(ost.str());
		if(affected > 0) 
		{
			ost.str("");
			for(int i = 0; i < affected; i++)
			{
				if(i > 0) ost << "," << std::endl;
				ost << "{";
				ost << "\"id\" : \"" << db->Get(i, "id") << "\"," << std::endl;
                ost << "\"folder\" : \"" << db->Get(i, "folder") << "\"," << std::endl;
				ost << "\"filename\" : \"" << db->Get(i, "filename") << "\"," << std::endl;
				ost << "\"isActive\" : \"" << db->Get(i, "isActive") << "\"" << std::endl;
				ost << "}";
			}

			result = ost.str();
		}
	}

	{
		CLog			log;
		ostringstream	ost;

		ost.str();
		ost <<  "GetMessageImageList: end. returning string length " << result.length();
		log.Write(DEBUG, ost.str());
	}

	return result;
}

// --- Function returns list of users "liked" messageID in JSON-format
// --- input: messageID, user, db
//            user - used for friendship status definition
// --- output: was this message "liked" by particular user or not
string GetMessageLikesUsersList(string messageID, CUser *user, CMysql *db)
{
	ostringstream	ost;
	int				affected;
	string			result = "";

	{
		CLog	log;
		log.Write(DEBUG, "GetMessageLikesUsersList: start");
	}

	ost.str("");
	ost << "select * from `feed_message_params` where `parameter`='like' and `messageID`='" << messageID << "';";
	affected = db->Query(ost.str());
	ost.str("");
	if(affected > 0) 
	{
		ost << "select * from users where id in (";
		for(int i = 0; i < affected; i++)
		{
			if(i > 0) ost << ",";
			ost << db->Get(i, "userID");
		}
		ost << ") and `isactivated`='Y' and `isblocked`='N';";
		result = GetUserListInJSONFormat(ost.str(), db, user);
	}

	{
		CLog			log;
		ostringstream	ost;

		ost.str();
		ost <<  "GetMessageLikesUsersList: end. returning string length " << result.length();
		log.Write(DEBUG, ost.str());
	}

	return result;
}

string GetMessageCommentsCount(string messageID, CMysql *db)
{
	ostringstream	ost;
	int				affected;
	string			result = "0";

	{
		CLog	log;
		log.Write(DEBUG, "GetMessageCommentsCount: start");
	}

	ost.str("");
	ost << "select count(*) as `counter` from `feed_message_comment` where `messageID`='" << messageID << "';";
	affected = db->Query(ost.str());
	if(affected > 0) 
	{
		result = db->Get(0, "counter");
	}

	{
		CLog	log;
		log.Write(DEBUG, "GetMessageCommentsCount: end");
	}

	return result;
}

string GetMessageSpam(string messageID, CMysql *db)
{
	ostringstream	ost;
	int				affected;
	string			result = "0";

	{
		CLog	log;
		log.Write(DEBUG, "GetMessageSpam: start");
	}

	ost.str("");
	ost << "select count(*) as `counter` from `feed_message_params` where `parameter`='spam' and messageID='" << messageID << "';";
	affected = db->Query(ost.str());


	if(affected > 0) 
	{
		result = db->Get(0, "counter");
	}

	{
		CLog	log;
		log.Write(DEBUG, "GetMessageSpam: end");
	}

	return result;
}

// --- Function returns true or false depends on userID "spamed" it or not
// --- input: messageID, userID
// --- output: was this message "spamed" by particular user or not
string GetMessageSpamUser(string messageID, string userID, CMysql *db)
{
	ostringstream	ost;
	int				affected;
	string			result = "0";

	{
		CLog	log;
		log.Write(DEBUG, "GetMessageSpamUser: start");
	}

	ost.str("");
	ost << "select count(*) as `counter` from `feed_message_params` where `parameter`='spam' and `messageID`='" << messageID << "' and `userID`='" << userID << "' ;";
	affected = db->Query(ost.str());
	if(affected > 0) 
	{
		result = db->Get(0, "counter");
	}

	
	{
		CLog	log;
		log.Write(DEBUG, "GetMessageSpamUser: end");
	}

	return result;
}

bool AllowMessageInNewsFeed(CUser *me, const string messageOwnerID, const string messageAccessRights, vector<int> *messageFriendList)
{

	{
		ostringstream	ost;
		CLog			log;

		ost.str("");
		ost << "AllowMessageInNewsFeed(user [" << me->GetID() << "], messageOwnerID [" << messageOwnerID << "], messageAccessRights [" << messageAccessRights << "]): start";
		log.Write(DEBUG, ost.str());
	}

	// --- messages belons to yourself must be shown unconditionally
	if(me->GetID() == messageOwnerID)
		return true;

	if(messageAccessRights == "public") return true;
	if(messageAccessRights == "private")
	{
		if(me->GetID() == messageOwnerID)
			return true;
		else
			return false;
	}

	// --- require to check friendship list;
	if(messageAccessRights == "friends")
	{
		vector<int>::iterator	it;

		for(it = messageFriendList->begin(); it != messageFriendList->end(); ++it)
		{
			ostringstream	ost;

			ost.str("");
			ost << *it;
			if(ost.str() == messageOwnerID) return true;
		}
		return false;
	}

	return true;
}

// --- rate-limit on sessid persistence		
// --- input: REMOTE_ADDR
// --- output: true, if rate-limit required
// ---         false, if rate-limit not required
bool isPersistenceRateLimited(string REMOTE_ADDR, CMysql *db)
{
	int 			maxTime = 60, maxAttempts = 3;
	ostringstream	ost;
	string			rateLimitID = "";
	int				affected, attempts;
	bool			result = false;

	{
		CLog	log;
		ostringstream	ostTemp;

		ostTemp.str("");
		ostTemp << "isPersistenceRateLimited: start. REMOTE_ADDR [" << REMOTE_ADDR << "]";
		log.Write(DEBUG, ostTemp.str());
	}

	// --- cleanup rate-limit table
	ost.str("");
	ost << "delete from `sessions_persistence_ratelimit` where `eventTimestamp` < (NOW() - interval " << maxTime << " second);";
	affected = db->Query(ost.str());

	ost.str("");
	ost << "select `id`, `attempts` from `sessions_persistence_ratelimit` where `ip`='" << REMOTE_ADDR << "';";
	affected = db->Query(ost.str());
	if(affected)
	{
		{
			CLog	log;
			ostringstream	ostTemp;

			ostTemp.str("");
			ostTemp << "isPersistenceRateLimited: REMOTE_ADDR in rate-limit table";
			log.Write(DEBUG, ostTemp.str());
		}
		rateLimitID = db->Get(0, "id");
		attempts = atoi(db->Get(0, "attempts"));
		ost.str("");
		ost << "update `sessions_persistence_ratelimit` set `attempts`=`attempts`+1 where `id`='" << rateLimitID << "';";
		db->Query(ost.str());

		if(attempts > maxAttempts)
		{
			{
				CLog	log;
				ostringstream	ostTemp;

				ostTemp.str("");
				ostTemp << "isPersistenceRateLimited: REMOTE_ADDR has tryed " << attempts << " times during the last " << maxTime << "sec. Needed to be rate-limited.";
				log.Write(DEBUG, ostTemp.str());
			}
			result = true;
		}
		else
		{
			{
				CLog	log;
				ostringstream	ostTemp;

				ostTemp.str("");
				ostTemp << "isPersistenceRateLimited: REMOTE_ADDR has tryed " << attempts << " times during the last " << maxTime << "sec. No need to rate-limit.";
				log.Write(DEBUG, ostTemp.str());
			}
			result = false;
		}
	}
	else
	{
		{
			CLog	log;
			ostringstream	ostTemp;

			ostTemp.str("");
			ostTemp << "isPersistenceRateLimited: REMOTE_ADDR not in rate-limit table";
			log.Write(DEBUG, ostTemp.str());
		}
		ost.str("");
		ost << "insert into `sessions_persistence_ratelimit` set `attempts`='1', `ip`='" << REMOTE_ADDR << "', `eventTimestamp`=NOW();";
		db->Query(ost.str());

		result = false;
	}

	{
		CLog	log;
		ostringstream	ostTemp;

		ostTemp.str("");
		ostTemp << "isPersistenceRateLimited: end. " << (result ? "rate-limit" : "no need rate-limit");
		log.Write(DEBUG, ostTemp.str());
	}

	return result;
}

void CopyFile(const string src, const string dst) 
{
    clock_t start, end;
    {
        CLog    log;
        ostringstream   ost;

        ost.str("");
        ost << "CopyFile (" << src << ", " << dst << "): enter";
        log.Write(DEBUG, ost.str());
    }

    start = clock();

    ifstream source(src.c_str(), ios::binary);
    ofstream dest(dst.c_str(), ios::binary);

    dest << source.rdbuf();

    source.close();
    dest.close();


    end = clock();

    {
        CLog    log;
        ostringstream   ost;

        ost.str("");
        ost << "CopyFile (" << src << ", " << dst << "): time of file copying is " << (end - start) / CLOCKS_PER_SEC << " second";
        log.Write(DEBUG, ost.str());
    }

}

// --- admin function
string GetCompanyDuplicates(CMysql *db)
{
	// --- map<companyName, companyID>
	map<string, string>		companyMap;

	// --- map<companyID, companyName>
	map<string, string>		duplicatesMap;
	ostringstream			ost, ostResult;

	{
		CLog	log;
		ostringstream	ostTemp;

		ostTemp.str("");
		ostTemp << "GetCompanyDuplicates: start ";
		log.Write(DEBUG, ostTemp.str());
	}

	ostResult.str("");
	ost.str("");
	ost << "select * from `company`;";
	for(int i = 0; i < db->Query(ost.str()); i++)
	{
		string		companyToTestName, companyToTestID;

		companyToTestID = db->Get(i, "id");
		companyToTestName = RemoveAllNonAlphabetSymbols(db->Get(i, "name"));

		auto it = companyMap.find(companyToTestName);
		if(it != companyMap.end())
		{
			// --- duplicate found
			duplicatesMap[companyToTestID] = companyToTestName;
			duplicatesMap[it->second] = it->first;
		}
		companyMap[companyToTestName] = companyToTestID;
	}

	for (const auto& it : duplicatesMap) 
	{
		if(ostResult.str().length()) ostResult << ",";
		ost.str("");
		ost << "select * from `company` where `id`='" << it.first << "';";
		db->Query(ost.str());
		ostResult << "{\"id\":\"" << it.first << "\", \"companyName\":\"" << db->Get(0, "name") << "\"";

		ost.str("");
		ost << "select count(*) as `number_of_users` from `users_company` where `company_id`='" << it.first << "';";
		db->Query(ost.str());
		ostResult << ", \"usersInCompany\":\"" << db->Get(0, "number_of_users") << "\"}";
	}

	{
		CLog	log;
		ostringstream	ostTemp;

		ostTemp.str("");
		ostTemp << "GetCompanyDuplicates: end (number of duplicates: " << duplicatesMap.size() << ")";
		log.Write(DEBUG, ostTemp.str());
	}

	return ostResult.str();
}

// --- admin function
string GetPicturesWithEmptySet(CMysql *db)
{
    ostringstream           ost, ostResult;
    int                     affected;

    {
        CLog    log;
        ostringstream   ostTemp;

        ostTemp.str("");
        ostTemp << "GetPicturesWithEmptySet: start ";
        log.Write(DEBUG, ostTemp.str());
    }

    ostResult.str("");
    ost.str("");
    ost << "SELECT * FROM `feed_images` where `setID`='0';";
    affected = db->Query(ost.str());
    for(int i = 0; i < affected; i++)
    {
        if(ostResult.str().length()) ostResult << ",";
        ostResult << "{\"id\":\"" << db->Get(i, "id") << "\",\"folder\":\"" << db->Get(i, "folder") << "\",\"filename\":\"" << db->Get(i, "filename") << "\"}";
    }

    {
        CLog    log;
        ostringstream   ostTemp;

        ostTemp.str("");
        ostTemp << "GetPicturesWithEmptySet: end (# of lost pictures: " << affected << ")";
        log.Write(DEBUG, ostTemp.str());
    }

    return ostResult.str();
}

// --- admin function
string GetPicturesWithUnknownMessage(CMysql *db)
{
    ostringstream                       ost, ostResult;
    int                                 affected, lostCount = 0;
    unordered_set<unsigned long>        allImageSets;
    unordered_set<unsigned long>        lostImageSets;

    {
        CLog    log;
        ostringstream   ostTemp;

        ostTemp.str("");
        ostTemp << "GetPicturesWithUnknownMessage: start ";
        log.Write(DEBUG, ostTemp.str());
    }

    ostResult.str("");
    ost.str("");
    ost << "SELECT `setID` FROM `feed_images`;";
    affected = db->Query(ost.str());
    for(int i = 0; i < affected; i++)
    {
        allImageSets.insert(atol(db->Get(i, "setID")));
    }

    for(const unsigned long& id: allImageSets)
    {
        ost.str("");
        ost << "select count(*) as count from `feed_message` where `imageSetID`=\"" << id << "\";";
        db->Query(ost.str());
        if(!atoi(db->Get(0, "count")))
        {
            lostImageSets.insert(id);
        }
    }

    ostResult.str("");
    for(const unsigned long& id: lostImageSets)
    {
        ost.str("");
        ost << "select * from `feed_images` where `setID`=\"" << id << "\";";
        for(int i = 0; i < db->Query(ost.str()); i++, lostCount++)
        {
            if(ostResult.str().length()) ostResult << ",";
            ostResult << "{\"id\":\"" << db->Get(i, "id") << "\",\"setID\":\"" << db->Get(i, "setID") << "\",\"folder\":\"" << db->Get(i, "folder") << "\",\"filename\":\"" << db->Get(i, "filename") << "\"}";
        }
    }

    {
        CLog    log;
        ostringstream   ostTemp;

        ostTemp.str("");
        ostTemp << "GetPicturesWithUnknownMessage: end (# of lost pictures: " << affected << ")";
        log.Write(DEBUG, ostTemp.str());
    }

    return ostResult.str();
}

// --- admin function
string GetPicturesWithUnknownUser(CMysql *db)
{
    ostringstream                       ost, ostResult;
    int                                 affected, lostCount = 0;
    unordered_set<unsigned long>        allImageUser;
    unordered_set<unsigned long>        lostImages;

    {
        CLog    log;
        ostringstream   ostTemp;

        ostTemp.str("");
        ostTemp << "GetPicturesWithUnknownUser: start ";
        log.Write(DEBUG, ostTemp.str());
    }

    ostResult.str("");
    ost.str("");
    ost << "SELECT `userID` FROM `feed_images`;";
    affected = db->Query(ost.str());
    for(int i = 0; i < affected; i++)
    {
        allImageUser.insert(atol(db->Get(i, "userID")));
    }

    for(const unsigned long& id: allImageUser)
    {
        ost.str("");
        ost << "select `id` from `users` where `id`=\"" << id << "\";";
        if(!db->Query(ost.str()))
        {
            lostImages.insert(id);
        }
    }

    ostResult.str("");
    for(const unsigned long& id: lostImages)
    {
        ost.str("");
        ost << "select * from `feed_images` where `userID`=\"" << id << "\";";
        for(int i = 0; i < db->Query(ost.str()); i++, lostCount++)
        {
            if(ostResult.str().length()) ostResult << ",";
            ostResult << "{\"id\":\"" << db->Get(i, "id") << "\",\"setID\":\"" << db->Get(i, "setID") << "\",\"folder\":\"" << db->Get(i, "folder") << "\",\"filename\":\"" << db->Get(i, "filename") << "\"}";
        }
    }

    {
        CLog    log;
        ostringstream   ostTemp;

        ostTemp.str("");
        ostTemp << "GetPicturesWithUnknownUser: end (# of lost pictures: " << affected << ")";
        log.Write(DEBUG, ostTemp.str());
    }

    return ostResult.str();
}

// --- admin function
string GetRecommendationAdverse(CMysql *db)
{
    ostringstream                       ost, ostResult, dictionaryStatement;
    int                                 affected;

    dictionaryStatement.str("");
    ostResult.str("");
    ost.str("");
    ost << "SELECT * FROM `dictionary_adverse`;";
    affected = db->Query(ost.str());
    if(affected)
    {
        for(int i = 0; i < affected; i++)
        {
            if(i) dictionaryStatement << " or ";
            dictionaryStatement << "(`title` like \"%" << db->Get(i, "word") << "%\")";
        }
    }

    ost.str("");
    ost << "select * from `users_recommendation` where `state` = 'unknown' and (" << dictionaryStatement.str() << ");";
    affected = db->Query(ost.str());
    if(affected)
    {
        for(int i = 0; i < affected; i++)
        {
            if(i) ostResult << ",";
            ostResult << "{";
            ostResult << "\"recommendationId\":\"" << db->Get(i, "id") <<"\",";
            ostResult << "\"recommendationRecommended_userID\":\"" << db->Get(i, "recommended_userID") <<"\",";
            ostResult << "\"recommendationRecommending_userID\":\"" << db->Get(i, "recommending_userID") <<"\",";
            ostResult << "\"recommendationTitle\":\"" << db->Get(i, "title") <<"\",";
            ostResult << "\"recommendationEventTimestamp\":\"" << db->Get(i, "eventTimestamp") <<"\",";
            ostResult << "\"recommendationState\":\"" << db->Get(i, "state") <<"\"";
            ostResult << "}";
        }
    }

    return ostResult.str();
}

// --- User notification part
string GetUserAvatarByUserID(string userID, CMysql *db)
{
    ostringstream   ost;
    string          userAvatar = "";

    ost.str("");
    ost << "select * from `users_avatars` where `userid`='" << userID << "' and `isActive`='1';";
    if(db->Query(ost.str()))
    {
        ost.str("");
        ost << "/images/avatars/avatars" << db->Get(0, "folder") << "/" << db->Get(0, "filename");
        userAvatar = ost.str();
    }

    return userAvatar;
}

string  GetUserNotificationSpecificDataByType(unsigned long typeID, unsigned long actionID, CMysql *db)
{
    ostringstream   ostResult, ost;

    {
        CLog    log;
        ostringstream   ost;

        ost.str("");
        ost << "GetUserNotificationSpecificDataByType(typeID=" << typeID << ", actionID=" << actionID << "): start";
        log.Write(DEBUG, ost.str());
    }


    ostResult.str("");

    // --- comment provided
    if(typeID == 19)
    {
        unsigned long   comment_id = actionID;

        ost.str("");
        ost << "select * from  `feed_message_comment` where `id`=\"" << comment_id << "\";";
        if(db->Query(ost.str()))
        {
            string  friend_userID = db->Get(0, "userID");
            string  commentTitle = db->Get(0, "comment");
            string  commentTimestamp = db->Get(0, "eventTimestamp");
            string  messageID = db->Get(0, "messageID");

            ost.str("");
            ost << "select * from  `feed_message` where `id`=\"" << messageID << "\";";
            if(db->Query(ost.str()))
            {
                string  messageTitle = db->Get(0, "title");
                string  messageBody = db->Get(0, "message");
                string  messageImageSetID = db->Get(0, "imageSetID");

                ost.str("");
                ost << "select * from `feed_images` where `setID`=\"" << messageImageSetID << "\";";
                if(db->Query(ost.str()))
                {
                    string      imageSetFolder = db->Get(0, "folder");
                    string      imageSetPic = db->Get(0, "filename");

                    ost.str("");
                    ost << "select * from `users` where `id`='" << friend_userID << "';";
                    if(db->Query(ost.str()))
                    {
                        string  friend_userName = db->Get(0, "name");
                        string  friend_userNameLast = db->Get(0, "nameLast");
 
                        ostResult << "\"notificationMessageID\":\"" << messageID << "\",";
                        ostResult << "\"notificationMessageTitle\":\"" << messageTitle << "\",";
                        ostResult << "\"notificationMessageBody\":\"" << messageBody << "\",";
                        ostResult << "\"notificationMessageImageFolder\":\"" << imageSetFolder << "\",";
                        ostResult << "\"notificationMessageImageName\":\"" << imageSetPic << "\",";
                        ostResult << "\"notificationCommentID\":\"" << comment_id << "\",";
                        ostResult << "\"notificationCommentTitle\":\"" << commentTitle << "\",";
                        ostResult << "\"notificationCommentEventTimestamp\":\"" << commentTimestamp << "\",";
                        ostResult << "\"notificationFriendUserID\":\"" << friend_userID << "\",";
                        ostResult << "\"notificationFriendUserName\":\"" << friend_userName << "\",";
                        ostResult << "\"notificationFriendUserNameLast\":\"" << friend_userNameLast << "\",";
                        ostResult << "\"notificationFriendUserAvatar\":\"" << GetUserAvatarByUserID(friend_userID, db) << "\"";

                    }
                    else
                    {
                        CLog log;
                        log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=19: ERROR selecting from users");
                    }

                }
            }
            else
            {
                CLog log;
                log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=19: ERROR finding message int feed_message");
            }
        }
        else
        {
            CLog log;
            log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=19: ERROR selecting from users_recommendation");
        }
    }


    // --- skill confirmed
    if(typeID == 43)
    {
        unsigned long   skill_confirmed_id = actionID;

        ost.str("");
        ost << "select * from `skill_confirmed` where `id`=\"" << skill_confirmed_id << "\";";
        if(db->Query(ost.str()))
        {
            string  users_skill_id = db->Get(0, "users_skill_id");
            string  approver_userID = db->Get(0, "approver_userID");

            ost.str("");
            ost << "select * from `users` where `id`='" << approver_userID << "';";
            if(db->Query(ost.str()))
            {
                string  approver_userName = db->Get(0, "name");
                string  approver_userNameLast = db->Get(0, "nameLast");

                ost.str("");
                ost << "select * from `users_skill` where `id`='" << users_skill_id << "';";
                if(db->Query(ost.str()))
                {
                    string  skillID = db->Get(0, "skill_id");

                    ost.str("");
                    ost << "select * from `skill` where `id`='" << skillID << "';";
                    if(db->Query(ost.str()))
                    {
                        string  skillTitle = db->Get(0, "title");

                        ostResult << "\"notificationFriendUserID\":\"" << approver_userID << "\",";
                        ostResult << "\"notificationFriendUserName\":\"" << approver_userName << "\",";
                        ostResult << "\"notificationFriendUserNameLast\":\"" << approver_userNameLast << "\",";
                        ostResult << "\"notificationFriendUserAvatar\":\"" << GetUserAvatarByUserID(approver_userID, db) << "\",";
                        ostResult << "\"notificationSkillTitle\":\"" << skillTitle << "\"";

                    }
                    else
                    {
                        CLog log;
                        log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=43: ERROR selecting from skill");
                    }

                }
                else
                {
                    CLog log;
                    log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=43: ERROR selecting from users_skill");
                }

            }
            else
            {
                CLog log;
                log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=43: ERROR selecting from users");
            }

        }
        else
        {
            CLog log;
            log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=43: ERROR selecting from skill_confirmed");
        }
    }

    // --- skill removed
    if(typeID == 44)
    {
        ost.str("");
        {
            unsigned long   users_skill_id = actionID;

            {
                ost.str("");
                ost << "select * from `users_skill` where `id`='" << users_skill_id << "';";
                if(db->Query(ost.str()))
                {
                    string  skillID = db->Get(0, "skill_id");

                    ost.str("");
                    ost << "select * from `skill` where `id`='" << skillID << "';";
                    if(db->Query(ost.str()))
                    {
                        string  skillTitle = db->Get(0, "title");

                        ostResult << "\"notificationSkillTitle\":\"" << skillTitle << "\"";

                    }
                    else
                    {
                        CLog log;
                        log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=44: ERROR selecting from skill");
                    }

                }
                else
                {
                    CLog log;
                    log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=44: ERROR selecting from users_skill");
                }

            }

        }
    }

    // --- recommendation provided
    if(typeID == 45)
    {
        ost.str("");
        {
            unsigned long   users_recommendation_id = actionID;

            {
                ost.str("");
                ost << "select * from `users_recommendation` where `id`=\"" << users_recommendation_id << "\";";
                if(db->Query(ost.str()))
                {
                    string  recommended_userID = db->Get(0, "recommended_userID");
                    string  recommending_userID = db->Get(0, "recommending_userID");
                    string  title = db->Get(0, "title");
                    string  eventTimestamp = db->Get(0, "eventTimestamp");

                    ost.str("");
                    ost << "select * from `users` where `id`='" << recommending_userID << "';";
                    if(db->Query(ost.str()))
                    {
                        string  recommending_userName = db->Get(0, "name");
                        string  recommending_userNameLast = db->Get(0, "nameLast");

                        ostResult << "\"notificationFriendUserID\":\"" << recommending_userID << "\",";
                        ostResult << "\"notificationFriendUserName\":\"" << recommending_userName << "\",";
                        ostResult << "\"notificationFriendUserNameLast\":\"" << recommending_userNameLast << "\",";
                        ostResult << "\"notificationFriendUserAvatar\":\"" << GetUserAvatarByUserID(recommending_userID, db) << "\",";
                        ostResult << "\"notificationRecommendationID\":\"" << users_recommendation_id << "\",";
                        ostResult << "\"notificationRecommendationTitle\":\"" << title << "\",";
                        ostResult << "\"notificationRecommendationEventTimestamp\":\"" << eventTimestamp << "\"";

                    }
                    else
                    {
                        CLog log;
                        log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=45: ERROR selecting from skill");
                    }

                }
                else
                {
                    CLog log;
                    log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=45: ERROR selecting from users_recommendation (probably deleted already)");
                }

            }

        }
    }

    // --- 46/47 recommendation deleted by benefit-owner
    if((typeID == 46) || (typeID == 47))
    {
        ost.str("");
        {
            unsigned long   friend_userID = actionID;

            {
                {
                    ost.str("");
                    ost << "select * from `users` where `id`='" << friend_userID << "';";
                    if(db->Query(ost.str()))
                    {
                        string  friend_userName = db->Get(0, "name");
                        string  friend_userNameLast = db->Get(0, "nameLast");

                        ostResult << "\"notificationFriendUserID\":\"" << friend_userID << "\",";
                        ostResult << "\"notificationFriendUserName\":\"" << friend_userName << "\",";
                        ostResult << "\"notificationFriendUserNameLast\":\"" << friend_userNameLast << "\",";
                        ostResult << "\"notificationFriendUserAvatar\":\"" << GetUserAvatarByUserID(to_string(friend_userID), db) << "\"";

                    }
                    else
                    {
                        CLog log;
                        log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=(46 or 47): ERROR selecting from skill");
                    }

                }
            }
        }
    }

    // --- recommendation modified
    if(typeID == 48)
    {
        unsigned long   user_recommendation_id = actionID;

        ost.str("");
        ost << "select * from  `users_recommendation` where `id`=\"" << user_recommendation_id << "\";";
        if(db->Query(ost.str()))
        {
            string  recommended_userID = db->Get(0, "recommended_userID");
            string  friend_userID = db->Get(0, "recommending_userID");
            string  recommendationTitle = db->Get(0, "title");
            string  recommendationEventTimestamp = db->Get(0, "eventTimestamp");

            {
                {
                    ost.str("");
                    ost << "select * from `users` where `id`='" << friend_userID << "';";
                    if(db->Query(ost.str()))
                    {
                        string  friend_userName = db->Get(0, "name");
                        string  friend_userNameLast = db->Get(0, "nameLast");

                        ostResult << "\"notificationRecommendationID\":\"" << user_recommendation_id << "\",";
                        ostResult << "\"notificationRecommendationTitle\":\"" << recommendationTitle << "\",";
                        ostResult << "\"notificationRecommendationEventTimestamp\":\"" << recommendationEventTimestamp << "\",";
                        ostResult << "\"notificationFriendUserID\":\"" << friend_userID << "\",";
                        ostResult << "\"notificationFriendUserName\":\"" << friend_userName << "\",";
                        ostResult << "\"notificationFriendUserNameLast\":\"" << friend_userNameLast << "\",";
                        ostResult << "\"notificationFriendUserAvatar\":\"" << GetUserAvatarByUserID(friend_userID, db) << "\"";

                    }
                    else
                    {
                        CLog log;
                        log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=48: ERROR selecting from skill");
                    }

                }
            }
        }
        else
        {
            CLog log;
            log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=48: ERROR selecting from users_recommendation");
        }
    }

    // --- message liked
    if(typeID == 49)
    {
        unsigned long   feed_message_params_id = actionID;

        ost.str("");
        ost << "select * from  `feed_message_params` where `id`=\"" << feed_message_params_id << "\";";
        if(db->Query(ost.str()))
        {
            string  friend_userID = db->Get(0, "userID");
            string  messageID = db->Get(0, "messageID");

            ost.str("");
            ost << "select * from  `feed_message` where `id`=\"" << messageID << "\";";
            if(db->Query(ost.str()))
            {
                string  messageTitle = db->Get(0, "title");
                string  messageBody = db->Get(0, "message");
                string  messageImageSetID = db->Get(0, "imageSetID");

                ost.str("");
                ost << "select * from `feed_images` where `setID`=\"" << messageImageSetID << "\";";
                if(db->Query(ost.str()))
                {
                    string      imageSetFolder = db->Get(0, "folder");
                    string      imageSetPic = db->Get(0, "filename");

                    ost.str("");
                    ost << "select * from `users` where `id`='" << friend_userID << "';";
                    if(db->Query(ost.str()))
                    {
                        string  friend_userName = db->Get(0, "name");
                        string  friend_userNameLast = db->Get(0, "nameLast");
 
                        ostResult << "\"notificationMessageID\":\"" << messageID << "\",";
                        ostResult << "\"notificationMessageTitle\":\"" << messageTitle << "\",";
                        ostResult << "\"notificationMessageBody\":\"" << messageBody << "\",";
                        ostResult << "\"notificationMessageImageFolder\":\"" << imageSetFolder << "\",";
                        ostResult << "\"notificationMessageImageName\":\"" << imageSetPic << "\",";
                        ostResult << "\"notificationFriendUserID\":\"" << friend_userID << "\",";
                        ostResult << "\"notificationFriendUserName\":\"" << friend_userName << "\",";
                        ostResult << "\"notificationFriendUserNameLast\":\"" << friend_userNameLast << "\",";
                        ostResult << "\"notificationFriendUserAvatar\":\"" << GetUserAvatarByUserID(friend_userID, db) << "\"";

                    }
                    else
                    {
                        CLog log;
                        log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=49: ERROR selecting from users");
                    }

                }
                else
                {
                    CLog log;
                    log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=49: ERROR selecting from feed_images");
                }
            }
            else
            {
                CLog log;
                log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=49: ERROR finding message int feed_message");
            }
        }
        else
        {
            CLog log;
            log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=49: ERROR selecting from feed_message_params");
        }
    }

    // --- message disliked
    if(typeID == 50)
    {
        unsigned long   messageID = actionID;

        {

            ost.str("");
            ost << "select * from  `feed_message` where `id`=\"" << messageID << "\";";
            if(db->Query(ost.str()))
            {
                string  messageTitle = db->Get(0, "title");
                string  messageBody = db->Get(0, "message");
                string  messageImageSetID = db->Get(0, "imageSetID");

                ost.str("");
                ost << "select * from `feed_images` where `setID`=\"" << messageImageSetID << "\";";
                if(db->Query(ost.str()))
                {
                    string      imageSetFolder = db->Get(0, "folder");
                    string      imageSetPic = db->Get(0, "filename");

                    {
 
                        ostResult << "\"notificationMessageID\":\"" << messageID << "\",";
                        ostResult << "\"notificationMessageTitle\":\"" << messageTitle << "\",";
                        ostResult << "\"notificationMessageBody\":\"" << messageBody << "\",";
                        ostResult << "\"notificationMessageImageFolder\":\"" << imageSetFolder << "\",";
                        ostResult << "\"notificationMessageImageName\":\"" << imageSetPic << "\"";

                    }

                }
                else
                {
                    CLog log;
                    log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=49: ERROR selecting from feed_images");
                }
            }
            else
            {
                CLog log;
                log.Write(ERROR, "GetUserNotificationSpecificDataByType: typeID=49: ERROR finding message int feed_message");
            }
        }
    }

    {
        CLog    log;
        ostringstream   ost;

        ost.str("");
        ost << "GetUserNotificationSpecificDataByType(typeID=" << typeID << ", actionID=" << actionID << "): end (return strlen=" << ostResult.str().length() << ")";
        log.Write(DEBUG, ost.str());
    }

    return  ostResult.str();
}

string  GetUserNotificationInJSONFormat(string sqlRequest, CMysql *db)
{
    int             affected;
    ostringstream   ostUserNotifications;

    {
        CLog    log;
        ostringstream   ost;

        ost.str("");
        ost << "GetUserNotificationInJSONFormat: start";
        log.Write(DEBUG, ost.str());
    }

    ostUserNotifications.str("");
    ostUserNotifications << "[";

    affected = db->Query(sqlRequest);
    if(affected)
    {
        class DBUserNotification
        {
            public:
                string      notificationID;
                string      feed_eventTimestamp;
                string      feed_actionId;
                string      feed_actionTypeId;
                string      action_types_title;
                string      user_id;
                string      user_name;
                string      user_nameLast;
                string      user_email;
                string      action_category_title;
                string      action_category_id;
        };

        vector<DBUserNotification>  dbResult;

        for(int i = 0; i < affected; ++i)
        {
            DBUserNotification      tmpObj;

            tmpObj.notificationID = db->Get(i, "users_notification_id");
            tmpObj.feed_actionTypeId = db->Get(i, "feed_actionTypeId");
            tmpObj.action_types_title = db->Get(i, "action_types_title");
            tmpObj.feed_eventTimestamp = db->Get(i, "feed_eventTimestamp");
            tmpObj.user_id = db->Get(i, "user_id");
            tmpObj.user_name = db->Get(i, "user_name");
            tmpObj.user_nameLast = db->Get(i, "user_nameLast");
            tmpObj.user_email = db->Get(i, "user_email");
            tmpObj.action_category_title = db->Get(i, "action_category_title");
            tmpObj.action_category_id = db->Get(i, "action_category_id");
            tmpObj.feed_actionId = db->Get(i, "feed_actionId");

            dbResult.push_back(tmpObj);
        }


        for(auto &it: dbResult)
        {
            string      userNotificationEnrichment = "";

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

            userNotificationEnrichment = GetUserNotificationSpecificDataByType(atol(it.feed_actionTypeId.c_str()), atol(it.feed_actionId.c_str()), db);
            if(userNotificationEnrichment.length()) ostUserNotifications << "," << userNotificationEnrichment;

            ostUserNotifications << "}";
        }
    }
    ostUserNotifications << "]";

    {
        CLog    log;
        ostringstream   ost;

        ost.str("");
        ost << "GetUserNotificationInJSONFormat: end";
        log.Write(DEBUG, ost.str());
    }
    
    return ostUserNotifications.str();
}

// --- function removes message image from FileSystems and cleanup DB
// --- as input require SWL WHERE clause (because of using SELECT and DELETE statements)
// --- input params:
// --- 1) SQL WHERE statement
// --- 2) db reference  
void    RemoveMessageImages(string sqlWhereStatement, CMysql *db)
{
    int             affected;
    ostringstream   ost;

    {
        CLog    log;

        ost.str("");
        ost << "int main(void): action == AJAX_updateNewsFeedMessage: start (sqlWhereStatement: " << sqlWhereStatement << ")";
        log.Write(DEBUG, ost.str());
    }

    ost.str("");
    ost << "select * from `feed_images` where " << sqlWhereStatement;
    affected = db->Query(ost.str());
    if(affected)
    {
        for(int i = 0; i < affected; i++)
        {
            string  filename = "";

            filename = IMAGE_FEED_DIRECTORY;
            filename +=  "/";
            filename +=  db->Get(i, "folder");
            filename +=  "/";
            filename +=  db->Get(i, "filename");

            {
                CLog    log;
                ostringstream   ost;

                ost.str("");
                ost << "int main(): action == AJAX_updateNewsFeedMessage: file must be deleted [" << filename << "]";
                log.Write(DEBUG, ost.str());
            }

            if(isFileExists(filename))
            {
                unlink(filename.c_str());
            }
            else
            {
                CLog    log;

                ost.str("");
                ost << "int main(void): action == AJAX_updateNewsFeedMessage: ERROR: file is not exists  [filename=" << filename << "]";
                log.Write(ERROR, ost.str());
            }

        }
        // --- cleanup DB with images pre-populated for posted message
        ost.str("");
        ost << "delete from `feed_images` where " << sqlWhereStatement;
        db->Query(ost.str());
    }

    {
        CLog    log;

        ost.str("");
        ost << "int main(void): action == AJAX_updateNewsFeedMessage: finish";
        log.Write(DEBUG, ost.str());
    }
}


// --- base64 encoding/decoding
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";
typedef unsigned char BYTE;

static inline bool is_base64(BYTE c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(BYTE const* buf, unsigned int bufLen) {
  std::string ret;
  int i = 0;
  int j = 0;
  BYTE char_array_3[3];
  BYTE char_array_4[4];

  while (bufLen--) {
    char_array_3[i++] = *(buf++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';
  }

  return ret;
}


std::string base64_decode(std::string const& encoded_string) {
  size_t in_len = encoded_string.size();
  size_t i = 0;
  size_t j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}

/*std::vector<BYTE> base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  BYTE char_array_4[4], char_array_3[3];
  std::vector<BYTE> ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
          ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
  }

  return ret;
}
*/

#endif