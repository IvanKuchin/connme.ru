#ifndef	__CFORUM__H__
#define	__CFORUM__H__

#include <vector>
#include "cexception.h"
#include "cmysql.h"

using	namespace std;

class	CForumUser
{
	private:
		string			name, password;
	public:
					CForumUser() {};

		string			GetName(void) { return name; };

					~CForumUser() {};
};

class	CMessage
{
	private:
		CMysql			*db;
		string			message, subj, date, name, email, ip, getmail, url, url_text;
		int			id, page, nextThread, prevThread, firstMessageOnPage;
		vector <CMessage *>	childs;

		string	 		GlobalMessageReplace(string where, string src, string dst);
	public:
					CMessage();
					CMessage(int i);

		int			GetID() { return id; };
		void			SetID(int i) { id = i; };
		string			GetMessage();
		void			SetMessage(string mes) { message = mes; };
		string			GetSubject() { return subj; };
		void			SetSubject(string subject) { subj = subject; };
		string			GetDate() { return date; };
		void			SetDate(string d) { date = d; };
		string			GetName() { return name; };
		void			SetName(string n) { name = n; };
		string			GetEMail() { return email; };
		void			SetEMail(string em) { email = em; };
		string			GetIP() { return ip; };
		void			SetIP(string addr) { ip = addr; };
		string			GetSendMail() { return getmail; };
		void			SetSendMail(string rcpt) { getmail = rcpt; };
		string			GetUrl() { return url; };
		void			SetUrl(string u) { url = u; };
		string			GetUrlText() { return url_text; };
		void			SetUrlText(string ut) { url_text = ut; };
		int			GetPage() { return page; };
		void			SetPage(int p) { page = p; };
		int			GetNextThread() { return nextThread; };
		void			SetNextThread(int thread) { nextThread = thread; };
		int			GetPrevThread() { return prevThread; };
		void			SetPrevThread(int thread) { prevThread = thread; };
		int			GetFirstMessageOnPage() { return firstMessageOnPage; };
		void			SetFirstMessageOnPage(int fm) { firstMessageOnPage = fm; };

		void			SetDB(CMysql *dbConnect) { db = dbConnect; };
		void			Fill();
		string			GetText();
		string			GetTextAdmin();
		string			GetFullText();

		void			Erase();

					~CMessage() {};
};

class	CThread
{
	private:
		CMysql			*db;
		CMessage		*rootMessage;
		int			rootMessageID, page, firstMessageOnPage;
	public:
					CThread() : rootMessage(NULL), rootMessageID(-1), page(0), firstMessageOnPage(0) {};
					CThread(CMysql *dbConnect) : db(dbConnect), rootMessage(NULL), rootMessageID(-1), page(0), firstMessageOnPage(0) {};

		void			SetDB(CMysql *dbConnect) { db = dbConnect; };
		void			FillThread();

		void			SetRootMessageID(int id) { rootMessageID = id; };
		int			GetRootMessageID(void) { return rootMessageID; };

		void			SetPage(int p) { page = p; }
		int			GetPage() {return page; };
		int			GetFirstMessageOnPage() { return firstMessageOnPage; };
		void			SetFirstMessageOnPage(int fm) { firstMessageOnPage = fm; };

		string			GetTextThread();
		string			GetTextThreadAdmin();
		string			GetFullTextThread();

// action befor thread will erased
		void			Erase();
					~CThread() {};
};

class	CForum
{
	private:
		bool			adminFlag;
		vector <CThread* >	threads;
		int			maxMessage, firstMessage, page;
		CMysql			*db;

		int			AddThread();
	public:
					CForum();
					CForum(CMysql *dbConnect);

		void			SetDB(CMysql *dbConnect) { db = dbConnect; };

		void			SetCurrentPage(int p);
		int			GetCurrentPage();

		string			GetTextForum(int numThread = 0, int numFirstThread = THREADS_PER_PAGE);
		string			GetTextMessage(int id);

		string			GetTextForumAdmin(int numThread = 0, int numFirstThread = THREADS_PER_PAGE);

		int			GetMaxMessage() { return maxMessage; };
		void			SetMaxMessage(int mm) { maxMessage = mm; };
		int			GetFirstMessage() { return firstMessage; };
		void			SetFirstMessage(int fm) { firstMessage = fm; };

					~CForum();
};

#endif
