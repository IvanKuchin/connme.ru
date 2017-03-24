#include <iomanip>
#include <sstream>
#include <time.h>

#include "cforum.h"
#include "clog.h"
#include "cexception.h"

// ---------------------- CMessage

CMessage::CMessage() : id(-1), page(0), firstMessageOnPage(0)
{
	childs.reserve(10);
}

CMessage::CMessage(int i) : id(i), page(0), firstMessageOnPage(0)
{
	childs.reserve(10);
}

string CMessage::GlobalMessageReplace(string where, string src, string dst)
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

string CMessage::GetMessage()
{
	message = GlobalMessageReplace(message, "\n", "<br>");
	message = GlobalMessageReplace(message, ":D", "<img src=/images/icons/icon_biggrin.gif>");
	message = GlobalMessageReplace(message, ":)", "<img src=/images/icons/icon_smile.gif>");
	message = GlobalMessageReplace(message, ":(", "<img src=/images/icons/icon_sad.gif>");
	message = GlobalMessageReplace(message, "80", "<img src=/images/icons/icon_eek.gif>");
	message = GlobalMessageReplace(message, "8)", "<img src=/images/icons/icon_cool.gif>");
	message = GlobalMessageReplace(message, ":lol:", "<img src=/images/icons/icon_lol.gif>");
	message = GlobalMessageReplace(message, ":x", "<img src=/images/icons/icon_mad.gif>");
	message = GlobalMessageReplace(message, ":P", "<img src=/images/icons/icon_razz.gif>");
	message = GlobalMessageReplace(message, ":oops:", "<img src=/images/icons/icon_redface.gif>");
	message = GlobalMessageReplace(message, ":o", "<img src=/images/icons/icon_surprised.gif>");
	message = GlobalMessageReplace(message, ":cry:", "<img src=/images/icons/icon_cry.gif>");
	message = GlobalMessageReplace(message, ":evil:", "<img src=/images/icons/icon_evil.gif>");
	message = GlobalMessageReplace(message, ":twisted:", "<img src=/images/icons/icon_twisted.gif>");
	message = GlobalMessageReplace(message, ":roll:", "<img src=/images/icons/icon_rolleyes.gif>");
	message = GlobalMessageReplace(message, ":wink:", "<img src=/images/icons/icon_wink.gif>");
	message = GlobalMessageReplace(message, ":!:", "<img src=/images/icons/icon_exclaim.gif>");
	message = GlobalMessageReplace(message, ":?:", "<img src=/images/icons/icon_question.gif>");
	message = GlobalMessageReplace(message, ":?", "<img src=/images/icons/icon_confused.gif>");
	message = GlobalMessageReplace(message, ":idea:", "<img src=/images/icons/icon_idea.gif>");
	message = GlobalMessageReplace(message, ":arrow:", "<img src=/images/icons/icon_arrow.gif>");
	message = GlobalMessageReplace(message, "[b]", "<b>");
	message = GlobalMessageReplace(message, "[/b]", "</b>");
	message = GlobalMessageReplace(message, "[i]", "<i>");
	message = GlobalMessageReplace(message, "[/i]", "</i>");
	message = GlobalMessageReplace(message, "[u]", "<u>");
	message = GlobalMessageReplace(message, "[/u]", "</u>");
	message = GlobalMessageReplace(message, "[size=7]", "<font style=\"font-size: 7pt;\">");
	message = GlobalMessageReplace(message, "[size=9]", "<font style=\"font-size: 9pt;\">");
	message = GlobalMessageReplace(message, "[size=12]", "<font style=\"font-size: 12pt;\">");
	message = GlobalMessageReplace(message, "[size=18]", "<font style=\"font-size: 18pt;\">");
	message = GlobalMessageReplace(message, "[size=24]", "<font style=\"font-size: 24pt;\">");
	message = GlobalMessageReplace(message, "[/size]", "</font>");
	return message;
}

void CMessage::Fill()
{
	int				affected, i;
	CMessage *			mess = NULL;
	vector<CMessage *>::iterator	iter;
	string				query = "";

	if(GetID() < 0)
	{
		CLog	log;
	log.Write(ERROR, "before fill message you need to set it's ID");

		throw CException("before fill message you need to set it's ID");
	}

	{
		char	query[300];

		memset(query, 0, sizeof(query));
		snprintf(query, sizeof(query) - 2,  "select * from forum where rootID='%d'", GetID());
		affected = db->Query(query);

		if(affected == 0)
		{
			CLog	log;
			log.Write(ERROR, "error in query:", query);
			throw CException("error in db talking");
		}
	}

	SetMessage(db->Get(0, "msg"));
	SetSubject(db->Get(0, "topic"));
	SetDate(db->Get(0, "time"));
	SetName(db->Get(0, "name"));
	SetEMail(db->Get(0, "email"));
	SetIP(db->Get(0, "ip"));
	SetSendMail(db->Get(0, "getmail"));
	SetUrl(db->Get(0, "url"));
	SetUrlText(db->Get(0, "url_name"));

	{
	char	query[300];

	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2,  "select rootID from forum where parentID='%d'", GetID());
	affected = db->Query(query);
	}

	if(affected > 0)
	{
		for(i = 0; i < affected; i++)
		{
			string	childIDStr = db->Get(i, "rootID");
			int	childID = atoi(childIDStr.c_str());

			try
			{
				mess = new CMessage();
			}
			catch(bad_alloc)
			{
				CLog	log;
				log.Write(ERROR, "not enough memory");
				throw CException("not enough memory");
			}

			if(mess == NULL)
			{
				CLog	log;

				log.Write(ERROR, "error allocating memory");
				throw CException("error allocating memory");
			}
			mess->SetID(childID);
			mess->SetDB(db);
			mess->SetPage(GetPage());
			mess->SetFirstMessageOnPage(GetFirstMessageOnPage());


			if(childs.capacity() - childs.size() <= 0)
			{
				CLog	log;

				log.Write(ERROR, "error in childs vector. May be overflow.");
				throw CException("error in childs vector. May be overflow.");
			}
			childs.push_back(mess);
		}
		for(iter = childs.begin(); iter < childs.end(); iter++)
			(*iter)->Fill();
	}

	return;
}

string CMessage::GetText()
{
	ostringstream			os;
	time_t				t;
	struct tm *			ts;
	vector<CMessage *>::iterator	iter;

	if(GetID() < 0)
	{
		CLog	log;
		log.Write(ERROR, "before get text thread you need to fill the thread");

		throw CException("before get text thread you need to fill the thread");
	}

	t = atoi(GetDate().c_str());
	ts = localtime(&t);

	os << "<ul>" << endl;
	os << "<li>" << endl;
	os << "<a class=forum_link href=/cgi-bin/index.cgi?forum=y&thid=" << GetID() << "&firstth=" << GetFirstMessageOnPage() << ">" << GetSubject() << "</a> <span class='forum_gray'>oт</span><span class=forum_name> " << endl;
//	if(GetEMail().length() > 0)
//		os << "<a href=mailto:" << GetEMail() << ">" << GetName() << "</a>" << endl;
//	else
		os << GetName() << "</span><span class=forum_gray>" << endl;

	os << "  " << setw(2) << setfill('0') << ts->tm_mday << "." << setw(2) << setfill('0') << ts->tm_mon + 1 << "." << ts->tm_year + 1900 << " " << setw(2) << setfill('0') << ts->tm_hour << ":" << setw(2) << setfill('0') << ts->tm_min << /*" (" << GetIP() << ")" <<*/  "</span>" << endl;
	os << "</li>" << endl;

	for(iter = childs.begin(); iter < childs.end(); iter++)
	{
		os << (*iter)->GetText();
	}
	os << "</ul>" << endl;

	return os.str();
}

string CMessage::GetTextAdmin()
{
	ostringstream			os;
	time_t				t;
	struct tm *			ts;
	vector<CMessage *>::iterator	iter;

	if(GetID() < 0)
	{
		CLog	log;
		log.Write(ERROR, "before get text thread you need to fill the thread");

		throw CException("before get text thread you need to fill the thread");
	}

	t = atoi(GetDate().c_str());
	ts = localtime(&t);

	os << "<ul>" << endl;
	os << "<li>" << endl;
	os << "<a href=" << getenv("SCRIPT_NAME") << "?act=editmes&forum=y&thid=" << GetID() << "&firstth=" << GetFirstMessageOnPage() << ">" << GetSubject() << "</a> posted by " << endl;
	if(GetEMail().length() > 0)
		os << "<a href=mailto:" << GetEMail() << ">" << GetName() << "</a>" << endl;
	else
		os << GetName() << endl;

	os << " at " << ts->tm_year + 1900 << "-" << setw(2) << setfill('0') << ts->tm_mon + 1 << "-" << setw(2) << setfill('0') << ts->tm_mday << " " << setw(2) << setfill('0') << ts->tm_hour << ":" << setw(2) << setfill('0') << ts->tm_min << /* " (" << GetIP() << ")" <<*/ endl;
	os << "<a href=" << getenv("SCRIPT_NAME") << "?forum=y&thid=" << GetID() << "&firstth=" << GetFirstMessageOnPage() << "&act=delmes><img src=/images/button_drop.png border=0></a>" << endl;
	os << "</li>" << endl;

	for(iter = childs.begin(); iter < childs.end(); iter++)
	{
		os << (*iter)->GetTextAdmin();
	}
	os << "</ul>" << endl;

	return os.str();
}

string CMessage::GetFullText()
{
	ostringstream			os;
	time_t				t;
	struct tm *			ts;
	vector<CMessage *>::iterator	iter;

	if(GetID() < 0)
	{
		CLog	log;
		log.Write(ERROR, "before get text thread you need to fill the thread");

		throw CException("before get text thread you need to fill the thread");
	}

	t = atoi(GetDate().c_str());
	ts = localtime(&t);

	os << "<span class='forum_subj'>" <<  GetSubject() << "</span> <span class='forum_name'>(" << endl;
//	if(GetEMail().length() > 0)
//		os << "<a href=mailto:" << GetEMail() << ">" << GetName() << "</a>" << endl;
//	else
		os << GetName() << endl;

	os << ")</span><span class='forum_gray'> " << ts->tm_year + 1900 << "-" << setw(2) << setfill('0') << ts->tm_mon + 1 << "-" << setw(2) << setfill('0') << ts->tm_mday << " " << setw(2) << setfill('0') << ts->tm_hour << ":" << setw(2) << setfill('0') << ts->tm_min << /* " (" << GetIP() << ")" <<*/ "</span>" << endl;

	os << "<br><br>" << GetMessage() << "<br><br><a href=\"" << GetUrl() << "\">" << GetUrlText() << "</a>";

	os << "<br><br><img src=\"/images/line.gif\">" << GetText();

	return os.str();
}


void CMessage::Erase()
{
	vector<CMessage *>::iterator	iter;

	for(iter = childs.begin(); iter < childs.end(); iter++)
	{
		(*iter)->Erase();
		delete(*iter);
	}
}

// ---------------------- CThread

void CThread::FillThread()
{
	CMessage *	msg;

	msg = new CMessage(GetRootMessageID());
	msg->SetDB(db);
	msg->SetPage(GetPage());
	msg->SetFirstMessageOnPage(GetFirstMessageOnPage());
	msg->Fill();
	rootMessage = msg;
}

string CThread::GetTextThread()
{
	string		result;
	// time_t		t;
	// struct tm *	ts;

	if(GetRootMessageID() < 0)
	{
		CLog	log;
		log.Write(ERROR, "before get text thread you need to fill the thread");
		throw CException("before get text thread you need to fill the thread");
	}
	if(rootMessage == NULL)
	{
		CLog	log;
		log.Write(ERROR, "before get text thread you need to fill the thread");
		throw CException("root message is NULL. please fill root message.");
	}

	result =  rootMessage->GetText();

	return result;
}

string CThread::GetTextThreadAdmin()
{
	string		result;
	// time_t		t;
	// struct tm *	ts;

	if(GetRootMessageID() < 0)
	{
		CLog	log;
		log.Write(ERROR, "before get text thread you need to fill the thread");
		throw CException("before get text thread you need to fill the thread");
	}
	if(rootMessage == NULL)
	{
		CLog	log;
		log.Write(ERROR, "before get text thread you need to fill the thread");
		throw CException("root message is NULL. please fill root message.");
	}

	result =  rootMessage->GetTextAdmin();

	return result;
}

string CThread::GetFullTextThread()
{
	ostringstream	ost;
	// time_t		t;
	// struct tm *	ts;

	if(GetRootMessageID() < 0)
	{
		CLog	log;
		log.Write(ERROR, "before get text thread you need to fill the thread");
		throw CException("before get text thread you need to fill the thread");
	}
	if(rootMessage == NULL)
	{
		CLog	log;
		log.Write(ERROR, "before get text thread you need to fill the thread");
		throw CException("root message is NULL. please fill root message.");
	}

	ost << "<table align=right width=100% class=plain><tr><td width=17%>";
	ost << "<a href=/cgi-bin/index.cgi?postmes=y&pid=" << GetRootMessageID() << ">[ ответить ]</a>";
	ost << "</td><td width=17%>";
	ost << "<a href=/cgi-bin/index.cgi?forum=y&firstth=" << GetFirstMessageOnPage() << ">[ основная страница ]</a>";
	ost << "</td><td width=17%>";
	ost << "<a href=/cgi-bin/index.cgi?postmes=y&pid=0>[ новая ветка ]</a>";
	ost << "</td></tr></table><br><br>";
	ost << rootMessage->GetFullText();

	return ost.str();
}

void CThread::Erase()
{
	if(rootMessage != NULL)
	{
		rootMessage->Erase();
		delete(rootMessage);
		rootMessage = NULL;
	}
}

// ----------------------- CForum

CForum::CForum() : adminFlag(false), maxMessage(THREADS_PER_PAGE)
{
	threads.reserve(THREADS_PER_PAGE * 6);
}

CForum::CForum(CMysql *dbConnect) : maxMessage(THREADS_PER_PAGE), db(dbConnect)
{
	threads.reserve(THREADS_PER_PAGE * 6);
}

void CForum::SetCurrentPage(int p)
{
	page = p;
}

int CForum::GetCurrentPage()
{
	return page;
}

int CForum::AddThread()
{
	if(threads.capacity() - threads.size() <= 0)
	{
		CLog	log;

		log.Write(ERROR, "error in threads number per page");
		throw CException("error in threads number per page");
	}
	threads.push_back(new CThread);
	return threads.size();
}

string	CForum::GetTextForum(int numFirstThread, int numThreads)
{
	ostringstream		ost;
	string			result;
	int			affected, i;
	char			query[300];
	CThread *		thread;
	vector<CThread *>::iterator	thIter;

	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select `rootID`, `parentID` from forum where parentID='0' order by time DESC LIMIT %d,%d", numFirstThread, numThreads);

	affected = db->Query(query);
	if(affected == 0)
	{
		CLog	log;
		log.Write(WARNING, "forum is empty");
	}
	for(i = 0; i < affected; i++)
	{
		thread = new CThread;
		if(thread == NULL)
		{
			CLog	log;
			log.Write(ERROR, "memory allocation error");

			throw CException("memory allocation error");
		}
		thread->SetDB(db);
		thread->SetRootMessageID( atoi(db->Get(i, "rootID")) );
		thread->SetPage(GetCurrentPage());
		thread->SetFirstMessageOnPage(numFirstThread);
		if(threads.capacity() - threads.size() <= 0)
		{
			CLog	log;

			log.Write(ERROR, "error in threads vector. May be overflow.");
			throw CException("error in threads vector. May be overflow.");
		}
		threads.push_back(thread);
	}

	ost << "<div align=right><a href=/cgi-bin/index.cgi?postmes=y&pid=0>новая ветка</a></div><br>" << endl;
	ost << "<table class=plain>" << endl;
	for(thIter = threads.begin(); thIter < threads.end(); thIter++)
	{
		{
			CLog	log;
			char	mess[300];

			memset(mess, 0, sizeof(mess));
			sprintf(mess, "root = %d", (*thIter)->GetRootMessageID());
			log.Write(DEBUG, mess);
		}
		(*thIter)->FillThread();

		ost << "<tr><td>" << (*thIter)->GetTextThread() << "</td></tr>";
	}
	ost << "</table>";

	ost << "<br><table width=100% class=plain><tr><td width=50% align=center>";
	if(numFirstThread - THREADS_PER_PAGE >= 0)
		ost << "<a href=/cgi-bin/index.cgi?forum=y&firstth=" << numFirstThread-numThreads << ">предидущая страница</a>";

	ost << "</td><td width=50% align=center>";
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select count(*) from forum where parentID='0'");
	affected = db->Query(query);
	if(affected == 0)
	{
		CLog	log;

		log.Write(ERROR, "can't calculate count entries in forum");
	}
	else
	{
		string	count;

		count = db->Get(0, 0);
		if(atoi(count.c_str()) > numFirstThread + numThreads)
			ost << "<a href=/cgi-bin/index.cgi?forum=y&firstth=" << numFirstThread+numThreads << ">следующая страница</a>";
	}

	ost << "</td></tr></table>";

	return ost.str();
}

string	CForum::GetTextForumAdmin(int numFirstThread, int numThreads)
{
	ostringstream		ost;
	string			result;
	int			affected, i;
	char			query[300];
	CThread *		thread;
	vector<CThread *>::iterator	thIter;

	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select `rootID`, `parentID` from forum where parentID='0' order by time DESC LIMIT %d,%d", numFirstThread, numThreads);

	affected = db->Query(query);
	if(affected == 0)
	{
		CLog	log;
		log.Write(WARNING, "forum is empty");
	}
	for(i = 0; i < affected; i++)
	{
		thread = new CThread;
		if(thread == NULL)
		{
			CLog	log;
			log.Write(ERROR, "memory allocation error");

			throw CException("memory allocation error");
		}
		thread->SetDB(db);
		thread->SetRootMessageID( atoi(db->Get(i, "rootID")) );
		thread->SetPage(GetCurrentPage());
		thread->SetFirstMessageOnPage(numFirstThread);
		if(threads.capacity() - threads.size() <= 0)
		{
			CLog	log;

			log.Write(ERROR, "error in threads vector. May be overflow.");
			throw CException("error in threads vector. May be overflow.");
		}
		threads.push_back(thread);
	}

	ost << "<div align=right><a href=/cgi-bin/index.cgi?postmes=y&pid=0>новая ветка</a></div><br>" << endl;
	ost << "<table class=plain>" << endl;
	for(thIter = threads.begin(); thIter < threads.end(); thIter++)
	{
		{
			CLog	log;
			char	mess[300];

			memset(mess, 0, sizeof(mess));
			sprintf(mess, "root = %d", (*thIter)->GetRootMessageID());
			log.Write(DEBUG, mess);
		}
		(*thIter)->FillThread();

		ost << "<tr><td>" << (*thIter)->GetTextThreadAdmin() << "</td></tr>";
	}
	ost << "</table>";

	ost << "<br><table width=100% class=plain><tr><td width=50% align=center>";
	if(numFirstThread - THREADS_PER_PAGE >= 0)
		ost << "<a href=" << getenv("SCRIPT_NAME") << "?forum=y&firstth=" << numFirstThread-numThreads << ">предидущая страница</a>";

	ost << "</td><td width=50% align=center>";
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select count(*) from forum where parentID='0'");
	affected = db->Query(query);
	if(affected == 0)
	{
		CLog	log;

		log.Write(ERROR, "can't calculate count entries in forum");
	}
	else
	{
		string	count;

		count = db->Get(0, 0);
		if(atoi(count.c_str()) > numFirstThread + numThreads)
			ost << "<a href=" << getenv("SCRIPT_NAME") << "?forum=y&firstth=" << numFirstThread+numThreads << ">следующая страница</a>";
	}

	ost << "</td></tr></table>";

	return ost.str();
}

string CForum::GetTextMessage(int id)
{
	string			result;
	int			affected;
	char			query[300];
	CThread *		thread;

	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select * from forum where rootID='%d'", id);

	affected = db->Query(query);
	if(affected == 0)
	{
		CLog	log;
		log.Write(WARNING, "message is missing");

		return("message is missing");
	}

	thread = new CThread;
	if(thread == NULL)
	{
		CLog	log;
		log.Write(ERROR, "memory allocation error");

		throw CException("memory allocation error");
	}
	thread->SetDB(db);
	thread->SetRootMessageID( atoi(db->Get(0, "rootID")) );
	thread->SetPage(GetCurrentPage());
	thread->SetFirstMessageOnPage(firstMessage);

	{
		CLog	log;
		char	mess[300];

		memset(mess, 0, sizeof(mess));
		sprintf(mess, "root = %d", thread->GetRootMessageID());
		log.Write(DEBUG, mess);
	}
	thread->FillThread();

	result = thread->GetFullTextThread();

	return result;
}

CForum::~CForum()
{
	vector<CThread *>::iterator	i;

	for(i = threads.begin(); i < threads.end(); i++)
	{
		(*i)->Erase();
		delete(*i);
	}
}
