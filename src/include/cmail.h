#ifndef __CMAIL__H__
#define __CMAIL__H__

#include <string>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include "cmysql.h"
#include "cvars.h"

using namespace std;

#define	CONNECT_TIMEOUT	10
#define	RECEIVE_TIMEOUT	1

class CMail
{
	private:
		string			rcptto, mailfrom, subject, message;
		string			smtpServer;
		int				smtpPort;
		bool			smtpFlag;
		string			uniqFilePostfix;
		string			stdoutFname, stdinFname, stderrFname;

		unsigned long 	Addr2Num (const char *addr);
		int				WaitSocket(int s, int sec);
		string			SocketReceive(int s);
		void			SocketSend(int s, string data);

		int				SendMailExec(const char *progr, char **av, int inp);
		int				MakeMailFile(string text);
		int				Input(int fd);

		void			RedirStdoutToFile();
		void			RedirStderrToFile();

		long long		getFileSize(string fname);
		bool			isFileExist(string fname);

		string			GetRandom(int len);
		string			GetTimeStamp();
		void			BuildUniqPostfix();
		
		bool			SendLocal();
		bool			SendSMTP();

		void			SetUniqPostfix(string tmp) { uniqFilePostfix = tmp; }
		string			GetUniqPostfix() { return uniqFilePostfix; }

		void			SetStdinFname(string tmp) { stdinFname = tmp; }
		string			GetStdinFname() { return stdinFname; }
		void			SetStdoutFname(string tmp) { stdoutFname = tmp; }
		string			GetStdoutFname() { return stdoutFname; }
		void			SetStderrFname(string tmp) { stderrFname = tmp; }
		string			GetStderrFname() { return stderrFname; }

	public:
						CMail();

		void			To(string to);
		void			From(string from);
		void			Subject(string subj);
		void			Message(string mess);

		// setting SMTP-server like "127.0.0.1" or "65.25.0.4"
		// in: string
		// out: none
		void			SetSMTPServer(string server);
		void			SetSMTPPort(int port);

		bool			Send();
		bool			Send(string to, string subj, string mess);

		void			SetSMTP();
		void			SetLocal();

						~CMail();
};

class CMailLocal : public CMail
{
	private:
		CMysql		*db;
		CVars		*vars;
		string		userLogin, userID;

	public:
				CMailLocal();

		void		SetDB(CMysql *mysql);
		void		SetVars(CVars *v);

		void		UserLogin(string log);
		string		GetUserLogin();
		void		UserID(string id);
		string		GetUserID();

		string		GetUserLng();

		string		SetTemplate(string templID);
		string		SetTemplateFile(string fileName);

		// mainly usage function.
		void		Send(string login, string templID, CVars *v, CMysql *mysql);

				~CMailLocal();
};

#endif
