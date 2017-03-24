#ifndef __CUSER__H__
#define __CUSER__H__

#include <stdio.h>

using namespace std;

#include "clog.h"
#include "cmysql.h"
#include "cvars.h"

extern double GetSecondsSinceY2k();

class CUser
{
	private:
		string		id, login, passwd, passwdConfirm, email, lng, ip, agreement, type, partnerID, phone, cv, name, nameLast, avatar, lastOnline;
		CMysql		*db;
		CVars		*vars;

		bool		LoadAvatar();
	public:
					CUser();
					CUser(string log, string pas, string pasConfirm, string em, string l, string i, string agr, string t, string pID, string ph);

		void		SetDB(CMysql *mysql);
		CMysql*		GetDB();

		void		SetVars(CVars *v);
		CVars*		GetVars();

		bool		isAllowedLng(string p);
		bool		isLoginExist();
		bool		isPasswdError();
		bool		isEmailCorrect();
		bool		isEmailDuplicate();
		bool		isPhoneCorrect();
		bool		isPhoneDuplicate();
		bool		isAgree();
		bool		isTypeCorrect();
		bool		isBlock();
		bool		isActive();

		string		GetID();
		void		SetID(string p);
		string		GetLogin();
		void		SetLogin(string p);
		string		GetName();
		void		SetName(string p);
		string		GetNameLast();
		void		SetNameLast(string p);
		string		GetPasswd();
		void		SetPasswd(string p);
		string		GetEmail();
		void		SetEmail(string p);
		string		GetLng();
		void		SetLng(string p);
		string		GetIP();
		void		SetIP(string p);
		string		GetPhone();
		void		SetPhone(string p);
		string		GetType();
		void		SetType(string p);
		string		GetPartnerID();
		void		SetPartnerID(string p);
		string		GetCV();
		void		SetCV(string p);
		string		GetAvatar();
		void		SetAvatar(string p);
		string		GetLastOnline();
		void		SetLastOnline(string p);

		void		Create();
		void		Block(string reason);
		void		Email(string message);
		bool		UpdatePresence();

		bool		GetFromDBbyLogin(string log);
		bool		GetFromDBbyID(string id);
		bool		GetFromDBbyEmail(string email);
		bool		GetFromDBbyEmailNoPassword(string email);

					~CUser();
};

#endif

