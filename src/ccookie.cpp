#include "ccookie.h"
#include "clog.h"
#include "cexception.h"

CCookie::CCookie()
{
	{
		CLog	log;
		log.Write(DEBUG, "CCookie::CCookie(): ");
	}
}

CCookie::CCookie(string n, string v) : name(n), value(v)
{
	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CCookie::CCookie(" << n << ", " << v << "): ";
		log.Write(DEBUG, ost.str());
	}
}

void	CCookie::SetName(string s) { 
	{
		CLog	log;
		log.Write(DEBUG, "CCookie::SetName(", s, "): ");
	}

	name = s; 
};
void	CCookie::SetValue(string s) {
	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CCookie::SetValue(" << s << "): name = " << GetName() << "";
		log.Write(DEBUG, ost.str());
	}
	value = s;
};
void	CCookie::SetDomain(string s) { domain = s; };
void	CCookie::SetPath(string s) { path = s; };

/*
Header mismatch:
    Expected: Set-Cookie: name=value; expires=Thu, 24-Apr-2014 22:36:43 GMT; Max-Age=5
    Received: Set-Cookie: name=value; expires=Thu, 24-Apr-2014 22:36:43 GMT
*/
void	CCookie::SetExpiration(string s) { expiration = s; };
void	CCookie::SetMaxAge(int s) { maxAge = s; };
void	CCookie::SetSecure(string s) { secure = s; };
void	CCookie::SetNew(bool s) {
	{
		CLog	log;
		ostringstream ost;
		ost.str("");
		ost << "CCookie::SetNew(" << s << "): name = " << GetName();
		log.Write(DEBUG, ost.str());
	}
	isNew = s;
};

string	CCookie::GetName() { return name; };
string	CCookie::GetValue() { return value; };
string	CCookie::GetDomain() { return domain; };
string	CCookie::GetPath() { return path; };
string	CCookie::GetExpiration() { return expiration; };
int		CCookie::GetMaxAge() { return maxAge; };
string	CCookie::GetSecure() { return secure; };
bool	CCookie::GetNew() { return isNew; };


CCookie::~CCookie()
{
	{
		CLog	log;
		log.Write(DEBUG, "CCookie::~CCookie(): destructor for ", GetName());
	}
}

// ------------------------ Main Cookie class ------------------------

CCookies::CCookies()
{
	{
		CLog	log;
		log.Write(DEBUG, "CCookies::CCookies():");
	}

	cookies.reserve(8);

	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CCookies::CCookies(): size of cookies=" << cookies.size() << " object after reservation";
		log.Write(DEBUG, ost.str());
	}

}

void CCookies::Add(string name, string value, string expiration, string domain, string path, string secure, bool newCookie)
{
	CCookie		*c;

	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CCookies::Add(w/o maxAge): start (size of cookies=" << cookies.size() << " object)";
		log.Write(DEBUG, ost.str());
	}

	if((cookies.capacity() - cookies.size()) <= 0)
	{
		CLog	log;
		log.Write(ERROR, "Size of cookie vector is overflow. May be reallocating memory.");
	}

	c = new(CCookie);
	if(!c)
	{
		CLog	log;
		log.Write(PANIC, "CCookies::Add(): ERROR: allocating memory (cookie module)");
		throw CException("error allocating memory (cookie module)");
	}

	c->SetName(name);
	c->SetValue(value);
	c->SetExpiration(expiration);
	c->SetMaxAge(0);
	c->SetDomain(domain);
	c->SetPath(path);
	c->SetSecure(secure);
	c->SetNew(newCookie);

	cookies.push_back(c);

	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CCookies::Add(w/o maxAge): end (size of cookies=" << cookies.size() << " object)";
		log.Write(DEBUG, ost.str());
	}

}

void CCookies::Add(string name, string value, string expiration, int maxAge, string domain, string path, string secure, bool newCookie)
{
	CCookie		*c;

	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "name=" << name << " value=" << value << " exp=" << expiration << " maxAge=" << maxAge << " domain=" << domain << " path=" << path << " secure=" << secure << " newCookie=" << newCookie;

		log.Write(DEBUG, "CCookies::Add(): start ", ost.str());
	}

	if((cookies.capacity() - cookies.size()) <= 0)
	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CCookies::Add(): ERROR Size of cookie vector is overflow. May be reallocating memory. (cookies.capacity()=" << cookies.capacity() << " cookies.size()=" << cookies.size() << " )";
		log.Write(ERROR, ost.str());
	}

	c = new(CCookie);
	if(!c)
	{
		CLog	log;
		log.Write(PANIC, "CCookies::Add(): error allocating memory (cookie module)");
		throw CException("error allocating memory (cookie module)");
	}

	c->SetName(name);
	c->SetValue(value);
	c->SetExpiration((expiration == "") ? GetTimestampShifted(maxAge) : expiration);
	c->SetMaxAge(maxAge);
	c->SetDomain(domain);
	c->SetPath(path);
	c->SetSecure(secure);
	c->SetNew(newCookie);

	cookies.push_back(c);

	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CCookies::Add(): end (size of cookies=" << cookies.size() << " object)";
		log.Write(DEBUG, ost.str());
	}
}

bool CCookies::IsExist(string name)
{
	bool	result;
	vector<CCookie *>::iterator	im;

	result = false;
	for(im = cookies.begin(); im < cookies.end(); im++)
	{
		if((*im)->GetName() == name)
		{
			result = true;
			break;
		}
	}

	return result;
}

// --- input: deltaTimeStamp in seconds
// --- return: string with shifted timestamp
string CCookies::GetTimestampShifted(int deltaTimeStamp) {
	time_t      t;
    char        utc_str[100];
    struct tm   *utc_tm;
    ostringstream	ost;

    t = time(NULL) + deltaTimeStamp;
    utc_tm = gmtime(&t);
    if(utc_tm == NULL) {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CCookies::GetTimestampShifted(" << deltaTimeStamp << "): error in running gmtime(&t)  (possible problem with cookie expiration in IE)";
		log.Write(ERROR, ost.str());
    }

    if(strftime(utc_str, sizeof(utc_str), "%a, %02d-%b-%Y %T %Z", utc_tm) == 0) {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CCookies::GetTimestampShifted(" << deltaTimeStamp << "): error in running strftime)(utc_str, sizeof(utc_str), '', utc_tm) (possible problem with cookie expiration in IE)";
		log.Write(ERROR, ost.str());
    }

    ost.str("");
    ost << utc_str;

	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CCookies::GetTimestampShifted(" << deltaTimeStamp << "): return value '" << utc_str << "'";
		log.Write(ERROR, ost.str());
    }

	return ost.str();
}

bool CCookies::UpdateTS(string name, int deltaTimeStamp)
{
	string	value, path, timestamp, domain, secure;
	vector<CCookie *>::iterator	im;

	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CCookies::UpdateTS(" << name << ", " << deltaTimeStamp << "): ";
		log.Write(DEBUG, ost.str());
	}

	if(IsExist(name)) {
		value = Get(name);
		timestamp = GetTimestampShifted(deltaTimeStamp);
		path = "/";
		domain = "";
		secure = "";

		for(im = cookies.begin(); im < cookies.end(); im++)
		{
			if((*im)->GetName() == name)
			{
				(*im)->SetValue(value);
				(*im)->SetExpiration(deltaTimeStamp == 0 ? "" : timestamp);
				(*im)->SetMaxAge(deltaTimeStamp * 60);
				(*im)->SetDomain(domain);
				(*im)->SetPath(path);
				(*im)->SetSecure(secure);
				(*im)->SetNew(true);
				break;
			}
		}

	}
	else {
		{
		    CLog	log;
	   	    log.Write(ERROR, "CCookies::UpdateTS(): ERROR, unable to find cookie: ", name);
		}

		return false;
	} // if(IsExist(name)) 

	return true;
}

void CCookies::Modify(string name, string value, string expiration, string domain, string path, string secure)
{
	vector<CCookie *>::iterator	im;

	for(im = cookies.begin(); im < cookies.end(); im++)
	{
		if((*im)->GetName() == name)
		{
			(*im)->SetValue(value);
			(*im)->SetExpiration(expiration);
			(*im)->SetDomain(domain);
			(*im)->SetPath(path);
			(*im)->SetSecure(secure);
			(*im)->SetNew(true);
			break;
		}
	}
}

void CCookies::Delete(string name, string domain, string path, string secure)
{
	vector<CCookie *>::iterator	im;
	time_t		t = time(NULL) - 3600 * 24 * 30 * 12;
	char		date[100];

	memset(date, 0, sizeof(date));
	strftime(date, sizeof(date) - 2, "%a, %d %b %Y %X %Z", localtime(&t));

	for(im = cookies.begin(); im < cookies.end(); im++)
	{
		if((*im)->GetName() == name)
		{
			(*im)->SetValue("");
			(*im)->SetExpiration(date);
			(*im)->SetMaxAge(1);
			(*im)->SetNew(TRUE);
//			break;
		}
	}
}

string CCookies::Get(string name)
{
	string result;
	vector<CCookie *>::iterator	im;

	result = "";
	for(im = cookies.begin(); im < cookies.end(); im++)
	{
		if((*im)->GetName() == name)
		{
			result = (*im)->GetValue();
			break;
		}
	}


	return result;
}

string CCookies::GetAll()
{
	string 						result;
	vector<CCookie *>::iterator	im;
	ostringstream				ost;

	result = "";
	for(im = cookies.begin(); im < cookies.end(); im++)
	{
		if((*im)->GetNew())   // does this cookie needs to be sendback to server ?
		{
			result += "Set-Cookie: ";
			result += (*im)->GetName();
			result += "=";
			result += (*im)->GetValue();
			if((*im)->GetExpiration().length() > 0)
			{
				result += "; ";
				result += "expires= ";
				result += (*im)->GetExpiration();
			}
			if((*im)->GetMaxAge() > 0)
			{
				ost.str("");
				ost << "; max-age=" << (*im)->GetMaxAge();
				result += ost.str();
			}
			if((*im)->GetPath().length() > 0)
			{
				result += "; ";
				result += "path=";
				result += (*im)->GetPath();
			}
			result += "\n";
		}
	}
	if(result.length() > 1)	result.erase(result.length() - 1);
	{
	    CLog	log;
   	    log.Write(DEBUG, "CCookies::GetAll(): list all cookie: ", result);
	}
	return result;
}

void CCookies::RegisterCookieVariables(CVars *v)
{
	vector<CCookie *>::iterator	im;

	for(im = cookies.begin(); im < cookies.end(); im++)
	{
		v->Add((*im)->GetName(), (*im)->GetValue());
	}
}

void CCookies::ParseString(string str)
{
	string			result, expr, cookName, cookValue;
	string::size_type	exprPos, eqPos;

	{
	    CLog	log;
   	    log.Write(DEBUG, "CCookies::ParseString(", str, "): start ");
	}

	result = str;
	exprPos = result.find(";");

	while((exprPos != string::npos) || (result.length() > 2))
	{
		if(exprPos != string::npos)
		{
			expr = result.substr(0, exprPos);
			result = result.erase(0, exprPos + 2);
		}
		else
		{
			expr = result;
			result.erase(0, result.length());
		}

		eqPos = expr.find("=");
		if(eqPos == string::npos)
		{
			exprPos = result.find(";");
			continue;
		}

		cookName = expr.substr(0, eqPos);
		cookValue = expr.substr(eqPos + 1, expr.length() - eqPos - 1);

		{
			CLog	log;
			ostringstream ost;

			ost.str("");
			ost << "CCookies::ParseString(" << str << "): adding cookie: " << cookName << ", " << cookValue;
			log.Write(DEBUG, ost.str());
		}

		Add(cookName, cookValue, "", "", "", "", FALSE);

		exprPos = result.find(";");
	}

	{
	    CLog	log;
   	    log.Write(DEBUG, "CCookies::ParseString(): end ");
	}
}

CCookies::~CCookies()
{
	vector<CCookie *>::iterator	iv;

	{
		CLog	log;
		log.Write(DEBUG, "CCookies::~CCookies(): start");
	}

	for(iv = cookies.begin(); iv < cookies.end(); iv++)
	{
		delete(*iv);
	}

	{
		CLog	log;
		log.Write(DEBUG, "CCookies::~CCookies(): finish");
	}
}
