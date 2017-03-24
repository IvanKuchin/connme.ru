#include <ctime>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

//#include "GeoIP.h"
#include "localy.h"
#include "ccgi.h"

CCgi::CCgi() :  templateFile(NULL), initHeadersFlag(1)
{
}

CCgi::CCgi(const char *fileName, int initFlag = 1) : initHeadersFlag(initFlag)
{
    SetTemplateFile(fileName);
}

CCgi::CCgi(const char *fileName, CVars v) : initHeadersFlag(1)
{
    SetTemplateFile(fileName);
    vars = v;
}

CCgi::CCgi(const char *fileName, CVars v, CFiles f) : initHeadersFlag(1)
{
    SetTemplateFile(fileName);
    vars = v;
    files = f;
}

CCgi::CCgi(int typeTemplate) : templateFile(NULL), initHeadersFlag(typeTemplate)
{
}

void	CCgi::SetDB(CMysql *mysql) {
	db = mysql;
	sessionDB.SetDB(mysql);
}

string CCgi::ReplaceHTMLTags(string src)
{
    string	result;

    result = src;
    result = GlobalMessageReplace(result, "<", "&lt;");
    result = GlobalMessageReplace(result, ">", "&gt;");

    return result;
}

string	CCgi::FindLanguageByIP(string ip)
{
//	GeoIP			*gi;
	char		*lngTemp = NULL;
	string			lng;
	string::iterator	iter;

//	gi = GeoIP_new(GEOIP_STANDARD);
//	lngTemp = GeoIP_country_code_by_addr(gi, getenv("REMOTE_ADDR"));
//	GeoIP_delete(gi);

	if(lngTemp == NULL) lng = ""; else lng = lngTemp;

	for(iter = lng.begin(); iter < lng.end(); ++iter) 
		*iter = tolower(*iter);

	return lng;
}

bool CCgi::isAcceptedLanguage(string lang)
{
	struct	stat	buf;
	ostringstream	ost;

	ost << TEMPLATE_PATH << lang << "/";

	if(stat(ost.str().c_str(), &buf) == 0)
		return true;
	else
		return false;
}

string CCgi::GetLanguage()
{
	string	tryLng;

	tryLng = GetVarsHandler()->Get("lng");
	if(tryLng.empty())
		tryLng = GetCookie("lng");
	if(tryLng.empty())
		tryLng = FindLanguageByIP(getenv("REMOTE_ADDR"));
	if(tryLng.empty())
		tryLng = DEFAULT_LANGUAGE;

	if(!isAcceptedLanguage(tryLng))
		tryLng = DEFAULT_LANGUAGE;

	return tryLng;
}

string CCgi::GetEncoding()
{
	string	result, lng;

	lng = GetLanguage();
	if(lng == "ru")
		return "windows-1251";
	if(lng == "en")
		return "windows-1251";

	return "windows-1251";
}



bool CCgi::SetTemplate(string templ)
{
	ostringstream	ost;

	ost << TEMPLATE_PATH << GetLanguage() << "/pages/" << templ;

	return SetTemplateFile(ost.str());
}

bool CCgi::SetTemplateFile(string fileName)
{

	{
		CLog logFile;
		logFile.Write(DEBUG, "CCgi::SetTemplateFile: setting up template [", fileName, "]");
	}

    templateFile = fopen(fileName.c_str(), "r");
    if(!templateFile)
    {
		CLog logFile;
		logFile.Write(ERROR, "CCgi::SetTemplateFile: Template error: can't open file", fileName);
		templateFile = NULL;

		return FALSE;
    }

    return TRUE;
}

void CCgi::OutString(const char *str, bool endlFlag = true)
{
    if(!str) return;

	cout << str;
    if(endlFlag) cout << endl;
/*
	// --- debug warning too much output
    {
		CLog logFile;
		logFile.Write(DEBUG, "----- ", str);
    }
*/
}

void CCgi::OutString(const string str, bool endlFlag = true)
{
    if(str.empty()) return;

	cout << str;
    if(endlFlag) cout << endl;
/*
	// --- debug warning too much output
    {
		CLog logFile;
		logFile.Write(DEBUG, "----- ", str);
    }
*/
}

string CCgi::StringEncode(string str)
{
    if(str.empty())
	return str;

    return ReplaceHTMLTags(str);
}

void CCgi::OutStringEncode(const char *str)
{
    if(!str)
	return;

    cout << StringEncode(str) << endl;
}


string CCgi::RenderLine(string rLine)
{
	string result = rLine;
	string::size_type findB, findE;

	// {
	//     CLog log;
	//     log.Write(DEBUG, "CCgi::RenderLine: rendered string: ", result.c_str());
	// }

	findB = result.find("<<vars:");
	while(findB != string::npos)
	{
	    findE = result.find(">>");
	    if(findE == string::npos)
	    {
			CLog log;
			log.Write(WARNING, "CCgi::RenderLine: error in template compiling found '<<vars:' witout '>>'");
			break;
	    }

	    string tag = result.substr(findB + 7, findE - findB - 7);
	    string varValue = vars.Get(tag);

	    if(varValue.length() == 0)
	    {
			CLog log;
			log.Write(WARNING, "CCgi::RenderLine: value of the variable ", tag, " is empty.");
	    }

	    result.replace(findB, findE + 2 - findB, varValue);

/*
		// --- commented to reduce log flooding
	    {
			CLog log;
			log.Write(DEBUG, "CCgi::RenderLine: rendered to: ", (result.length() > LOG_FILE_MAX_LENGTH ? (result.substr(0, LOG_FILE_MAX_LENGTH) + " ... cut due to size ...") : result));
	    }
*/
	    findB = result.find("<<vars:");

	}

	return result;
}

void CCgi::InitHeaders()
{
    if(!initHeadersFlag)
    {
	char		date[100];
	time_t		t = time(0);
	string		cook;
	ostringstream	ost;

	memset(date, 0, sizeof(date));
	strftime(date, sizeof(date) - 2, "Last-Modified: %a, %d %b %Y %X %Z", localtime(&t));
	OutString(date);

	OutString("Status-text: powered by Ivan Kuchin");

	cook = cookie.GetAll();
	OutString(cook);

	ost.str("");
	ost << "Content-Type: text/html; charset=" << GetEncoding() << ";" << endl << endl;
	OutString(ost.str());
    }
}

string CCgi::GetCookie(string name)
{
	return cookie.Get(name);
}

void CCgi::ModifyCookie(string name, string value, string cma, string cp, string cd, string cs)
{
	cookie.Modify(name, value, cma, cd, cp, cs);
}

bool CCgi::CookieUpdateTS(string name, int deltaTimeStamp) {
	return cookie.UpdateTS(name, deltaTimeStamp);
}

void CCgi::DeleteCookie(string name, string cd, string cp, string cs)
{
	cookie.Delete(name, cd, cp, cs);
}

string CCgi::GetHttpHost()
{
    return getenv("HTTP_HOST");
}

string CCgi::GetRequestURI()
{
    return getenv("REQUEST_URI");
}

void CCgi::AddCookie(string cn, string cv, string ce, int cma, string cd, string cp, string cs)
{
    cookie.Add(cn, cv, ce, cma, cd, cp, cs, TRUE);
}

void CCgi::AddCookie(string cn, string cv, string ce, string cd, string cp, string cs)
{
    cookie.Add(cn, cv, ce, cd, cp, cs, TRUE);
}

string CCgi::RecvLine(FILE *s)
{
    int i = 0;
    int ch;
    string result;

    result = "";

    for (; (ch = fgetc(s)) != EOF; i++)
    {
        result += ch;

        if (result[i] == '\n') break;
    }

    return result;
}

void CCgi::OutTemplate()
{
    string	fLine;

    {
    	CLog	log;
    	log.Write(DEBUG, "CCgi::OutTemplate(): start.");
    }

    if(templateFile == NULL) return;

    InitHeaders();

    fLine = RecvLine(templateFile);

    while(fLine.length() > 0)
    {
		string::size_type	bPos;

		if(fLine.find("<<template:") != string::npos)
		{
		    string::size_type	ePos;

		    bPos = fLine.find("<<template:");
		    ePos = fLine.find(">>");
		    if(ePos != string::npos)
		    {
				string	templateName = fLine.substr(bPos + 11, ePos - bPos - 11);
				CCgi	nextTempl(templateName.c_str(), vars);

				string	before = fLine.substr(0, bPos);
				string	after = fLine.substr(ePos + 2, fLine.length() - ePos - 2);

				OutString(before, WITHOUT_ENDL);
				nextTempl.OutTemplate();
				OutString(after, WITHOUT_ENDL);
		    }
		    else
		    {
				OutString(fLine, WITHOUT_ENDL);
		    }
		}
		else if(fLine.find("<<vars:") != string::npos)
		{
			OutString(RenderLine(fLine), WITHOUT_ENDL);
			
	/*
			// Comment it for testing purpose !
		    string::size_type	ePos;

		    bPos = fLine.find("<<vars:");
		    ePos = fLine.find(">>");
		    if(ePos != string::npos)
			{
				string		varName = fLine.substr(bPos + 7, ePos - bPos - 7);
				string		varValue = vars.Get(varName);

				string		before = fLine.substr(0, bPos);
				string		after = fLine.substr(ePos + 2, fLine.length() - ePos - 2);

				OutString(before, WITHOUT_ENDL);

				if(varValue.length() > 0)
				    OutString(varValue, WITHOUT_ENDL);
				else
				{
				    CLog log;
				    log.Write(WARNING, "value of the variable ", varName, " is empty.");
				}

				OutString(after, WITHOUT_ENDL);
			}
		    else
		    {
			OutString(fLine, WITHOUT_ENDL);
		    }
	*/
		}
		else
		{
		    OutString(fLine, WITHOUT_ENDL);
		}

		fLine = RecvLine(templateFile);
    }

    {
    	CLog	log;
    	log.Write(DEBUG, "CCgi::OutTemplate(): end");
    }
}

void CCgi::RegisterVariable(string name, string value)
{
    vars.Add(name, value);
}

void CCgi::RegisterVariableForce(string name, string value)
{
    vars.Redefine(name, value);
}

void CCgi::ParseURL()
{
    CRequest	req;
    char	*c = getenv("HTTP_COOKIE");

    req.RegisterURLVariables(&vars, &files);
    if(c)
    {
		cookie.ParseString(c);
		cookie.RegisterCookieVariables(&vars);
    }
}

CVars *CCgi::GetVarsHandler()
{
    return(&vars);
}

CFiles *CCgi::GetFilesHandler()
{
    return(&files);
}

void CCgi::Redirect(const char *cUrl)
{
    string	result;

	{
		ostringstream	ost1;
		CLog	log;

		ost1.str("");
		ost1 << "void CCgi::Redirect(" << cUrl << "): redirecting";
		log.Write(ERROR, ost1.str());
	}

/*    result = "Location: ";
    result += cUrl;
    result += "\n\n";
    OutString(result);
*/
    RegisterVariableForce("redirect_url", cUrl);

    throw CExceptionHTML("redirect");
}

string CCgi::GlobalMessageReplace(string where, string src, string dst)
{
        string                  result;
        string::size_type       pos;

        result = where;

        pos = result.find(src);
        while(pos != string::npos)
        {
                result.replace(pos, src.length(), dst);
                pos = result.find(src);
        }

        return result;
}

// --- Session part
string CCgi::SessID_Get_FromHTTP (void) {
	string sessid;

	sessid = "";
	if (cookie.IsExist("sessid")) {
		sessid = cookie.Get("sessid");
	};

	return sessid;
}

string CCgi::SessID_Create_HTTP_DB (int max_age) {

	if(!sessionDB.Save("Guest", getenv("REMOTE_ADDR"), GetLanguage())) {
		ostringstream	ost1;
		CLog	log;

		ost1.str("");
		ost1 << "CCgi::SessID_Create_HTTP_DB (" << max_age << "): ERROR, unable to save session in DB";
		log.Write(ERROR, ost1.str());

		return "";
	}

	// --- AddCookie(name, value, expiration, max-age, domain, path)
	AddCookie("sessid", sessionDB.GetID(), "", max_age, "", "/");

	{
		CLog	log;
		log.Write(DEBUG, "CCgi::SessID_Create (void): Generate session: create session for user 'Guest' (", sessionDB.GetID(), ")");
	}

	return sessionDB.GetID();
}

bool CCgi::SessID_Load_FromDB(string sessidHTTP) {
	return sessionDB.Load(sessidHTTP);
}

string CCgi::SessID_Get_UserFromDB() {
	return sessionDB.GetUser();
}

bool CCgi::SessID_CheckConsistency(void) {
	return sessionDB.CheckConsistency();
}

bool CCgi::SessID_Update_HTTP_DB() 
{

	if(!sessionDB.Update()) 
	{
			CLog	log;

			log.Write(ERROR, "bool CCgi::SessID_Update_HTTP_DB(void): ERROR in updating DB session");
			throw CExceptionHTML("SQL error");		
	}

	if(cookie.IsExist("sessid")) 
	{
		if(!CookieUpdateTS("sessid", (sessionDB.GetExpire() == 0 ? 0 : SESSION_LEN)))
		{
			CLog	log;

			log.Write(ERROR, "bool CCgi::SessID_Update_HTTP_DB(void): ERROR in updating 'sessid' cookie TS(timestamp)");
		}
	}
	else 
	{
		CLog	log;

		log.Write(ERROR, "bool CCgi::SessID_Update_HTTP_DB(void): ERROR cookie sessid is not exists");

		return false;
	}

	return true;
}

bool CCgi::Cookie_Expire() {
	if(cookie.IsExist("sessid_ever")) {
		cookie.Delete("sessid_ever", "", "", "");
	} else if (cookie.IsExist("sessid")) {
		cookie.Delete("sessid", "", "", "");
	}

	return true;
}

CCgi::~CCgi()
{
    if(templateFile)
    {
	fclose(templateFile);
	templateFile = NULL;
    }
}


