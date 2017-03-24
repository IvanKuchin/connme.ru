#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iomanip>
#include <iostream>

#include <sstream>

#include "cactivator.h"
#include "cmail.h"
#include "cuser.h"
#include "cexception.h"
//#include "GeoIP.h"

CUser::CUser() : login("Guest"), db(NULL), vars(NULL)
{
}

CUser::CUser(string log, string pas, string pasConfirm, string em, string l, string i, string agr, string t, string pID, string ph) : db(NULL), vars(NULL)
{
	SetLogin(log);
	SetPasswd(pas);
	passwdConfirm = pasConfirm;
	SetEmail(em);
	SetLng(l);
	SetIP(i);
	agreement = agr;
	SetType(t);
	SetPartnerID(pID);
	SetPhone(ph);
}

void CUser::SetDB(CMysql *mysql)
{
	db = mysql;
}

CMysql* CUser::GetDB()
{
	return db;
}

void CUser::SetVars(CVars *v)
{
	vars = v;
}

CVars* CUser::GetVars()
{
	return vars;
}

bool CUser::isBlock()
{
	ostringstream	ost;

	ost << "select * from users_block where `userid`='" << GetLogin() << "'";
	if(!db)
	{
		CLog	log;
		log.Write(ERROR, "CUser::isBlock: error connecting db in CUser::Block");
		throw CExceptionHTML("error db");
	}
	if(db->Query(ost.str()) > 0)
		return true;
	else
		return false;
}

bool CUser::isActive()
{
	ostringstream	ost;
	string		tmp;

	ost << "select * from users where `login`='" << GetLogin() << "'";
	if(!db)
	{
		CLog	log;
		log.Write(ERROR, "CUser::isActive: error connecting db in CUser::Block");
		throw CExceptionHTML("error db");
	}
	if(db->Query(ost.str()) > 0)
	{
		tmp = db->Get(0, "isactivated");
		if(tmp == "Y") return true;
	}
	return false;
}

void CUser::Block(string reason)
{
	ostringstream	ost;

	if(GetLogin().empty())
	{
		CLog	log;
		log.Write(ERROR, "CUser::Block: it is require to set user_login before block in CUser::Block");
		throw CExceptionHTML("no user");
	}
	ost << "insert into users_block(`userid`,`date`,`notes`) value('" << GetLogin() << "',now(),'" << reason << "')";
	if(!db)
	{
		CLog	log;
		log.Write(ERROR, "CUser::Block: error connecting db in CUser::Block");
		throw CExceptionHTML("error db");
	}
	db->Query(ost.str());
}

bool CUser::isLoginExist()
{
	ostringstream	ost;

	ost << "select type from users where login='" << login << "'";
	if(!db)
	{
		CLog	log;
		log.Write(ERROR, "CUser::isLoginExist: error connecting db");
		throw CExceptionHTML("error db");
	}
	if(db->Query(ost.str()) <= 0)
		return false;

	return true;
}

bool CUser::isPasswdError()
{
	if(passwd.empty()) return true;
	if(passwd != passwdConfirm)
		return true;
	else
		return false;
}

bool CUser::isEmailCorrect()
{
	string::size_type	atPos, dotPos;
	ostringstream	ost;

	if(email.empty()) return false;

	atPos = email.find("@");
	if((atPos == 0) || (atPos == string::npos)) return false;

	dotPos = email.rfind(".");
	if((dotPos == 0) || (dotPos == string::npos)) return false;
	if(dotPos < email.rfind("@")) return false;
	return true;
}

bool CUser::isEmailDuplicate()
{
	ostringstream	ost;

	ost << "select type from users where email='" << email << "'";
	if(!db) throw CExceptionHTML("error db");
	if(db->Query(ost.str()) > 0) return true;

	return false;
}

bool CUser::isPhoneCorrect()
{
	string		ph;
	ostringstream	ost;

	ph = GetPhone();
	if(ph.find_first_not_of("01234567890-()") != string::npos) return false;
	while(ph.find_first_not_of("0123456789") != string::npos)
	{
		ph.replace(ph.find_first_not_of("0123456789"), 1, "");
	}
	if(ph.length() != 10) return false;
	return true;
}

bool CUser::isPhoneDuplicate()
{
	string		ph;
	ostringstream	ost;

	ph = GetPhone();
	ost.str("");
	ost << "select * from `users` where `phone`='" << ph << "'";
	if(!db)
	{
		CLog	log;
		log.Write(ERROR, "CUser::isPhoneDuplicate: error connecting db in CUser::isPhoneCorrect");
		throw CExceptionHTML("error db");
	}
	if(db->Query(ost.str()) > 0) return true;
	
	return false;
}

bool CUser::isTypeCorrect()
{
	if(GetType().empty()) return false;

	if(GetType() == "user") return true;
	if(GetType() == "partner") return true;
	if(GetType() == "investor") return true;

	return false;
}

bool CUser::isAgree()
{
	if((agreement[0] == 'n') || (agreement[0] == 'N')) return false;

	return true;
}

string CUser::GetID()
{
	return id;
}

string CUser::GetLogin()
{
	return login;
}

string CUser::GetName()
{
	return name;
}

string CUser::GetNameLast()
{
	return nameLast;
}

string CUser::GetPasswd()
{
	return passwd;
}

string CUser::GetEmail()
{
	return email;
}

string CUser::GetLng()
{
	return lng;
}

string CUser::GetIP()
{
	return ip;
}

string CUser::GetPhone()
{
	return phone;
}

string CUser::GetCV()
{
	return cv;
}

string CUser::GetType()
{
	return type;
}

string CUser::GetPartnerID()
{
	return partnerID;
}

string CUser::GetAvatar()
{
	return avatar;
}

string CUser::GetLastOnline()
{
	return lastOnline;
}

void CUser::SetPartnerID(string p)
{
	partnerID = p;
}

void CUser::SetID(string p)
{
	id = p;
}

void CUser::SetLogin(string p)
{
	login = p;
}

void CUser::SetName(string p)
{
	name = p;
}

void CUser::SetNameLast(string p)
{
	nameLast = p;
}

void CUser::SetPasswd(string p)
{
	passwd = p;
}

void CUser::SetEmail(string p)
{
	email = p;
}

void CUser::SetAvatar(string p)
{
	avatar = p;
}

void CUser::SetLastOnline(string p)
{
	lastOnline = p;
}

bool CUser::isAllowedLng(string p)
{
	struct	stat	buf;
	ostringstream	ost;

	ost << TEMPLATE_PATH << p << "/";

	if(stat(ost.str().c_str(), &buf) == 0)
		return true;
	else
		return false;
}

void CUser::SetLng(string p)
{
	if(!isAllowedLng(p))
	{
		ostringstream	ost;
		CLog		log;

		ost << "CUser::SetLng: language for user " << GetLogin() << " changed to default, because " << p << "is not supported yet.";
		log.Write(ERROR, ost.str());

		lng = DEFAULT_LANGUAGE;
	}
	else
		lng = p;
}

void CUser::SetIP(string p)
{
	ip = p;
}

void CUser::SetType(string p)
{
	type = p;
}

void CUser::SetPhone(string p)
{
	phone = p;
}

void CUser::SetCV(string p)
{
	cv = p;
}

void CUser::Create()
{
	ostringstream	ost;
	string		countryName;

	ost << "insert into `users` (`login`, `email`, `type`, `isactivated`, `lng`, `regdate`, `partnerid`, `country`, `ip`, `phone`, `activator_sent`, `cv`) values (\"" << GetLogin() << "\", \"" << GetEmail() << "\", \"" << GetType() << "\", 'N', \"" << GetLng() << "\", now(), \"" << (GetPartnerID().length() ? GetPartnerID() : "0") << "\", \"0\", \"" << GetIP() << "\", \"" << GetPhone() << "\", NOW(), \"" << GetCV() << "\")";
	if(db == NULL)
	{
		CLog	log;

		log.Write(ERROR, "CUser::Create(): ERROR: can't insert user information into `users` table");
		throw CExceptionHTML("error db in User Create");
	} 
	db->Query(ost.str());

	ost.str("");
	ost << "select `id`, `type` from `users` where `login`=\"" << GetLogin() << "\"";
	if(db->Query(ost.str()) == 0) 
	{
		CLog	log;

		log.Write(ERROR, "CUser::Create(): ERROR: can't find new user in table `users` with type 'user'");
		throw CExceptionHTML("error db");
	}
	SetID(db->Get(0, "id"));

	ost.str("");
	ost << "INSERT INTO `users_passwd` \
				(`userID`, `passwd`, `isActive`, `eventTimestamp`) \
			VALUES \
				(\"" << GetID() << "\", \"" << GetPasswd() << "\", \"true\", NOW())";
	if(db == NULL)
	{
		CLog	log;

		log.Write(ERROR, "CUser::Create(): ERROR: can't update passwd table with user password");
		throw CExceptionHTML("error db in User Create");	
	} 
	db->Query(ost.str());

	// Email("registered");
}

void CUser::Email(string messageID)
{
	CMailLocal	mail;
	CActivator	activator;

	if(GetVars() == NULL)
	{
		CLog	log;

		log.Write(ERROR, "CUser::Email: Vars array must be send to CUser()");
		throw CExceptionHTML("vars array");
	}

	if(GetDB() == NULL)
	{
		CLog	log;

		log.Write(ERROR, "CUser::Email: DB-connection must be send to CUser()");
		throw CExceptionHTML("db error parameter");
	}

	if(GetLogin().empty())
	{
		CLog	log;

		log.Write(ERROR, "CUser::Email: Login must be set in CUser()");
		throw CExceptionHTML("recipient before template");
	}

	if(messageID.empty())
	{
		CLog	log;

		log.Write(ERROR, "CUser::Email: Type of mail message is unknown");
		throw CExceptionHTML("mail template");
	}

	activator.SetDB(GetDB());
	activator.SetUser(GetLogin());
	activator.SetType("reg_user");
	GetVars()->Set("actid", activator.GetID());
	GetVars()->Set("host", getenv("HTTP_HOST"));
	activator.Save();
	mail.Send(GetLogin(), messageID, GetVars(), GetDB());
}

bool CUser::GetFromDBbyLogin(string log)
{
	ostringstream		ost;
	bool				result;

	if(db == NULL) throw CExceptionHTML("error db");

	ost << "SELECT * FROM `users`  \
			INNER JOIN `users_passwd` ON `users`.`id`=`users_passwd`.`userID` \
			WHERE `users`.`login`=\"" << log << "\" AND `users_passwd`.`isActive`='true';";
	if(db->Query(ost.str()) == 0) { 
		result = false; 
	}
	else{
		SetID(db->Get(0, "id"));
		SetLogin(db->Get(0, "login"));
		SetPasswd(db->Get(0, "passwd"));
		SetEmail(db->Get(0, "email"));
		SetLng(db->Get(0, "lng"));
		SetIP(db->Get(0, "ip"));
		SetType(db->Get(0, "type"));
		// SetPartnerID(db->Get(0, "partnerid"));
		SetCV(db->Get(0, "cv"));
		SetName(db->Get(0, "name"));
		SetNameLast(db->Get(0, "nameLast"));

		result = true;
	}

	return result;
}

bool CUser::GetFromDBbyID(string id)
{
	ostringstream		ost;
	bool				result;

	if(db == NULL) throw CExceptionHTML("error db");

	ost << "SELECT * FROM `users`  \
			INNER JOIN `users_passwd` ON `users`.`id`=`users_passwd`.`userID` \
			WHERE `users`.`id`=\"" << id << "\" AND `users_passwd`.`isActive`='true';";
	if(db->Query(ost.str()) == 0) {
		result =  false;
	}
	else {
		SetID(db->Get(0, "id"));
		SetLogin(db->Get(0, "login"));
		SetPasswd(db->Get(0, "passwd"));
		SetEmail(db->Get(0, "email"));
		SetLng(db->Get(0, "lng"));
		SetIP(db->Get(0, "ip"));
		SetType(db->Get(0, "type"));
		// SetPartnerID(db->Get(0, "partnerid"));
		SetCV(db->Get(0, "cv"));
		SetName(db->Get(0, "name"));
		SetNameLast(db->Get(0, "nameLast"));

		result = true;
	}

	return result;
}

// --- Get `user`.`id` from DB using `sessions`.`user`
// --- basically checking if users still exists in DB if session has been stored on client device
// --- This function differ from FullVersion that did not get the password from `users_passwd`
bool CUser::GetFromDBbyEmailNoPassword(string email)
{
	ostringstream		ost;
	bool				result = false;

	{
		CLog	log;
		log.Write(DEBUG, "CUser::GetFromDBbyEmailNoPassword: start");
	}
	if(db == NULL)
	{
		CLog	log;
		log.Write(DEBUG, "CUser::GetFromDBbyEmailNoPassword: ERROR: db must be initialized");

		throw CExceptionHTML("error db");
	} 

	ost << "SELECT * FROM `users`  \
			WHERE `email`=\"" << email << "\";";
	if(db->Query(ost.str()) == 0) {
		{
			CLog	log;

			log.Write(DEBUG, "CUser::GetFromDBbyEmailNoPassword: user with email [", email, "] not found");
		}
	}
	else 
	{
		SetID(db->Get(0, "id"));
		SetLogin(db->Get(0, "login"));
		SetEmail(db->Get(0, "email"));
		SetLng(db->Get(0, "lng"));
		SetIP(db->Get(0, "ip"));
		SetType(db->Get(0, "type"));
		SetCV(db->Get(0, "cv"));
		SetName(db->Get(0, "name"));
		SetNameLast(db->Get(0, "nameLast"));
		result = true;
	}
	{
		CLog			log;
		ostringstream	ost;

		ost.str("");
		ost << "CUser::GetFromDBbyEmailNoPassword: end (return " << (result ? "true" : "false") << ")";
		log.Write(DEBUG, ost.str());
	}
	return result;
}

// --- Get `user`.`id` from DB using `sessions`.`user`
// --- basically checking if users still exists in DB if session has been stored on client device
bool CUser::GetFromDBbyEmail(string email)
{
	ostringstream		ost;
	bool				result = false;

	{
		CLog	log;
		log.Write(DEBUG, "CUser::GetFromDBbyEmail: start");
	}

	if(db == NULL)
	{
		CLog	log;
		log.Write(DEBUG, "CUser::GetFromDBbyEmail: ERROR: db must be initialized");

		throw CExceptionHTML("error db");
	} 

	if(GetFromDBbyEmailNoPassword(email))
	{

		ost << "SELECT * FROM `users_passwd`  \
				WHERE `userID`=\"" << GetID() << "\" AND `users_passwd`.`isActive`='true';";
		if(db->Query(ost.str()) == 0) 
		{
			{
				CLog	log;

				log.Write(DEBUG, "CUser::GetFromDBbyEmail: user with email [", email, "] not found");
			}
		}
		else 
		{
			SetPasswd(db->Get(0, "passwd"));
			result = true;
		}
	}
	else
	{
		
		{
			CLog	log;
			ostringstream	ost;
			ost.str("");
			ost << "CUser::GetFromDBbyEmail: ERROR: fail to receive username from GetFromDBbyEmailNoPassword(" << email << ")";
			log.Write(ERROR, ost.str());
		}
	}


	{
		CLog	log;
		ostringstream	ost;
		ost.str("");
		ost << "CUser::GetFromDBbyEmail: end (" << (result ? "true" : "false") << ")";
		log.Write(DEBUG, ost.str());
	}
	return result;
}

bool CUser::LoadAvatar()
{
	ostringstream	ost1;
	bool			result = false;
	string			avatarPath;

	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CUser::LoadAvatar: begin (" << GetID() << ")";

		log.Write(DEBUG, ost.str());
	}

	// --- Get user avatars
	ost1.str("");
	ost1 << "select * from `users_avatars` where `userid`='" << GetID() << "' and `isActive`='1';";
	avatarPath = "empty";
	if(db->Query(ost1.str()))
	{
		ost1.str("");
		ost1 << "/images/avatars/avatars" << db->Get(0, "folder") << "/" << db->Get(0, "filename");
		avatarPath = ost1.str();
		result = true;
	}
	SetAvatar(ost1.str());

	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CUser::LoadAvatar: end";

		log.Write(DEBUG, ost.str());
	}
	
	return result;
}

bool CUser::UpdatePresence()
{
	bool 			result = true; 
	ostringstream	ost;

	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CUser::UpdatePresense: start";

		log.Write(DEBUG, ost.str());
	}

	ost.str("");
	ost << "update `users` set `last_online`=now(), `last_onlineSecondsSinceY2k`='" << to_string(GetSecondsSinceY2k()) << "' where `id`='" << GetID() << "';";

	if(db->Query(ost.str())) {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CUser::UpdatePresense(): ERROR: update presense SQL-query must return 0. It is not zero, means userID [" << GetID() << "] having more than 1 users.";

		log.Write(ERROR, ost.str());

		result = false;
	}

	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CUser::UpdatePresense: end";

		log.Write(DEBUG, ost.str());
	}

	return result;
}

CUser::~CUser()
{
}











