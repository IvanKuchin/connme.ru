#ifndef __CCOOKIE__H__
#define __CCOOKIE__H__

#include <string>
#include <vector>
#include "cvars.h"

using namespace std;

class CCookie
{
	private:
		string	name, value, domain, path, expiration, secure;
		int		maxAge;
		bool	isNew;		// this parameter control sentback coockie to server
	public:
				CCookie();
				CCookie(string n, string v);

		void	SetName(string s);
		void	SetValue(string s);
		void	SetDomain(string s);
		void	SetPath(string s);
		void	SetExpiration(string s);
		void	SetMaxAge(int s);
		void	SetSecure(string s);
		void	SetNew(bool s);

		string	GetName();
		string	GetValue();
		string	GetDomain();
		string	GetPath();
		string	GetExpiration();
		int		GetMaxAge();
		string	GetSecure();
		bool	GetNew();

			~CCookie();
};

class CCookies
{
	private:
		vector<CCookie *>	cookies;
	public:
			CCookies();

		void	Add(string name, string value, string expiration = "", string domain = "", string path = "", string secure = "", bool newCookie = TRUE);
		void	Add(string name, string value, string expiration = "", int maxage = 0, string domain = "", string path = "", string secure = "", bool newCookie = TRUE);
		string	GetTimestampShifted(int deltaTimeStamp);
		bool	UpdateTS(string name, int deltaTimeStamp);
		void	Modify(string name, string value, string expiration = "", string domain = "", string path = "", string secure = "");
		void	Delete(string name, string domain = "", string path = "", string secure = "");
		string	Get(string name);
		bool	IsExist(string name);
		string	GetAll();
		void	ParseString(string str);
		void	RegisterCookieVariables(CVars *v);

			~CCookies();
};

#endif
