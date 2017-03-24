#ifndef __CSESSION__H__
#define __CSESSION__H__

using namespace std;

#include <maxminddb.h>
#include <string>
#include <sstream>
#include "cexception.h"
#include "localy.h"
#include "cmysql.h"
#include "ccookie.h"


#define	SESSION_LEN	60	// --- session duration in minutes
#define	MMDB_fname "/home/httpd/dev.connme.ru/cgi-bin/GeoLite2-City.mmdb"

class CSession
{
	private:

// --- UTF8 to CP1251 converter
		int 		convert_utf8_to_windows1251(const char* utf8, char* windows1251, size_t n);
// --- UTF8 to CP1251 converter


		CMysql		*db;
		CCookie		*cookies;
		string		sessID;
		string		user;
		string		lng;
		string		ip;
		int			expire;

		// --- Possible to convert to "static" to reduce memory usage (MMDB is using about 9K per instance)
		bool					MMDB_usage;
		MMDB_s					mmdb;
		MMDB_entry_data_list_s *MMDB_entry_data_list;
		MMDB_lookup_result_s	MMDB_result;

		string		GetRandom(int len);
		bool		Save();
		string		DetectItem(string MMDB_itemName);

	public:
				CSession();

		void		SetUser(string u);
		string		GetUser();

		void		GenerateID();
		void		SetID(string id);
		string		GetID();
		void		SetIP(string i);
		string		GetIP();
		void		SetLng(string l);
		string		GetLng();
		void		SetExpire(int l);
		int			GetExpire();

		string		DetectCountry();
		string		DetectCity();

		void		SetDB(CMysql *mysql);

		bool		Save(string u, string i, string l);
		bool		Load(string id);
		bool		Update();
		bool		UpdateIP(string newIP);


		bool		isExist(string id);
		bool		CheckConsistency();
		bool		SessID_ExpireSession();


				~CSession();
};

#endif
