#ifndef __CACTIVATOR__H__
#define __CACTIVATOR__H__

#include <string>
#include <sstream>
#include "localy.h"
#include "cmysql.h"
#include "ccgi.h"

using namespace std;

#define	ACTIVATOR_LEN			20
#define ACTIVATOR_SESSION_LEN	24*60

class CActivator
{
	private:
		CCgi		*cgi;
		CMysql		*db;
		string		actID;
		string		type;
		string		time;
		string		user;

		string		GetRandom(int len);
	public:
				CActivator();

		void		SetUser(string u);
		string		GetUser();

		void		GenerateID();
		void		SetID(string id);
		string		GetID();

		void		SetType(string t);
		string		GetType();

		void		SetTime(string t);
		string		GetTime();

		void		SetDB(CMysql *mysql);
		void		SetCgi(CCgi *cgi);

		void		Save();
		bool		Load(string id);
		void		Delete(string id);
		void		DeleteByUser(string userToDelete);
		void		Delete();

		void		Activate();

				~CActivator();
};

#endif
