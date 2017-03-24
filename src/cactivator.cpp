#include <sys/time.h>

#include <sstream>
#include <string>

#include "cexception.h"
#include "cactivator.h"
#include "localy.h"
#include "cmysql.h"

CActivator::CActivator() : cgi(NULL), db(NULL)
{
	struct	timeval	tv;

	gettimeofday(&tv, NULL);
	srand(tv.tv_sec * tv.tv_usec * 100000);

	actID = GetRandom(ACTIVATOR_LEN);
}

string CActivator::GetRandom(int len)
{
	string	result;
	int	i;

	for(i = 0; i < len; i++)
	{
		result += (char)('0' + (int)(rand()/(RAND_MAX + 1.0) * 10));
	}

	return result;
}

string CActivator::GetID()
{
	return actID;
}

void CActivator::SetID(string id)
{
	actID = id;
}

string CActivator::GetUser()
{
	return user;
}

void CActivator::SetUser(string u)
{
	user = u;
}

void CActivator::SetType(string t)
{
	type = t;
}

string CActivator::GetType()
{
	return type;
}

void CActivator::SetTime(string t)
{
	time = t;
}

string CActivator::GetTime()
{
	return time;
}

void CActivator::SetDB(CMysql *mysql)
{
	ostringstream 	ost;

	db = mysql;

	// --- Clean-up database
	{
		CLog	log;
		log.Write(DEBUG, "CActivator::SetDB: clean-up 'activator' table.");
	}
	ost.str("");
	ost << "delete from activators where  `date`<=(now() - INTERVAL " << ACTIVATOR_SESSION_LEN << " MINUTE)";
	db->Query(ost.str());

	ost.str("");
	ost << "delete from users where `isactivated`='N' and `activator_sent` <= (now() - INTERVAL " << ACTIVATOR_SESSION_LEN << " MINUTE)";
	db->Query(ost.str());

}

void CActivator::SetCgi(CCgi *c)
{
	cgi = c;
}

void CActivator::Save()
{
	ostringstream	ost;

	if(!db)
	{
		CLog	log;
		log.Write(ERROR, "CActivator::Save: error connect to database in CActivator module");

		throw CExceptionHTML("error db");
	}

	if(GetUser().empty())
	{
		CLog	log;
		log.Write(ERROR, "CActivator::Save: user name must be set in activator");

		throw CExceptionHTML("activator error");
	}
	if(GetID().empty())
	{
		CLog	log;
		log.Write(ERROR, "CActivator::Save: id must be set in activator");

		throw CExceptionHTML("activator error");
	}
	if(GetType().empty())
	{
		CLog	log;
		log.Write(ERROR, "CActivator::Save: type must be set in activator");

		throw CExceptionHTML("activator error");
	}


	ost << "INSERT INTO `activators` (`id` , `user` , `type` , `date` ) VALUES ('" << GetID() << "', '" << GetUser() << "', '" << GetType() << "', NOW())";

	db->Query(ost.str());
}

bool CActivator::Load(string id)
{
	ostringstream	ost;

	if(!db)
	{
		CLog	log;
		log.Write(ERROR, "CActivator::Load: error connect to database in CActivator module");

		throw CExceptionHTML("error db");
	}
	if(id.empty())
	{
		CLog	log;
		log.Write(ERROR, "CActivator::Load: id must be set in activator");

		throw CExceptionHTML("activator error");
	}

	ost << "select * from activators where id='" << id << "'";
	if(db->Query(ost.str()) == 0)
	{
		CLog	log;

		log.Write(DEBUG, "CActivator::Load: there is no such activator");

		return false;
	}
	SetID(id);
	SetUser(db->Get(0, "user"));
	SetType(db->Get(0, "type"));
	SetTime(db->Get(0, "date"));

	return true;
}

void CActivator::Delete(string id)
{
	ostringstream	ost;

	if(!db)
	{
		CLog	log;
		log.Write(ERROR, "CActivator::Delete: error connect to database in CActivator module");

		throw CExceptionHTML("error db");
	}
	if(id.empty())
	{
		CLog	log;
		log.Write(ERROR, "CActivator::Delete: id must be set in activator");

		throw CExceptionHTML("activator error");
	}

	ost.str("");
	ost << "delete from activators where id='" << GetID() << "'";
	db->Query(ost.str());
}

void CActivator::DeleteByUser(string userToDelete)
{
	ostringstream	ost;

	if(!db)
	{
		CLog	log;
		log.Write(ERROR, "CActivator::DeleteByUser: error connect to database in CActivator module");

		throw CExceptionHTML("error db");
	}
	if(userToDelete.empty())
	{
		CLog	log;
		log.Write(ERROR, "CActivator::DeleteByUser: userToDelete must be set in activator");

		throw CExceptionHTML("activator error");
	}

	ost.str("");
	ost << "delete from activators where `user`='" << userToDelete << "' and `type`='regNewUser'";
	db->Query(ost.str());
}

void CActivator::Activate()
{
	ostringstream	ost;
	int		affected;
	string		u, t;

	if(GetID().empty())
	{
		CLog	log;
		log.Write(ERROR, "CActivator::Activate: Activator ID is empty while trying activate event");

		throw CExceptionHTML("no activator");
	}

	ost << "select * from activators where `id`='" << GetID() << "' and `date`>=(now() - INTERVAL " << ACTIVATOR_SESSION_LEN << " MINUTE)";

	if(db == NULL)
	{
		CLog	log;
		log.Write(ERROR, "CActivator::Activate: error connect DB in CActivator module");

		throw CExceptionHTML("error db");
	}
	affected = db->Query(ost.str());
	if(affected == 0) throw CExceptionHTML("no activator");

	u = db->Get(0, "user");
	t = db->Get(0, "type");

	ost.str("");
	if(t == "regNewUser")
	{
		ost << "update `users` set `isactivated`='Y',`activated`=NOW() where `login`='" << u << "'";
		db->Query(ost.str());

		if(cgi == NULL)
		{
			CLog	log;
			log.Write(ERROR, "CActivator::Activate: CCgi is empty in module CActivator::ActivateAction");

			throw CExceptionHTML("cgi error");
		}
		if(!cgi->SetTemplate("activate_user_complete.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, "CActivator::Activate: template file was missing");
			throw CException("Template file was missing");
		}

		cgi->RegisterVariableForce("login", u);
		Delete(GetID());
	}
}

void CActivator::Delete()
{
	Delete(GetID());
}

CActivator::~CActivator()
{
}





