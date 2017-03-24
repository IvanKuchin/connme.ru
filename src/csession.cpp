#include <sys/time.h>

#include "csession.h"

CSession::CSession() : db(NULL), cookies(NULL)
{
	struct	timeval	tv;

	gettimeofday(&tv, NULL);
	srand(tv.tv_sec * tv.tv_usec * 100000);

	sessID = GetRandom(SESSION_LEN);

	{
			CLog	log;
			ostringstream	ost;

			ost.str("");
			ost << "CSession::CSession(): start";
			log.Write(DEBUG, ost.str());
	}


#ifdef MAXMIND_ACTIVATE
	if(getenv("REMOTE_ADDR"))
	{
		int		status;

	    status = MMDB_open(MMDB_fname, MMDB_MODE_MMAP, &mmdb);
	    if (status == MMDB_SUCCESS) 
	    {
	    	int gai_error, mmdb_error;

			MMDB_usage = true;
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "CSession::CSession(): MMDB_open(" << MMDB_fname << ") opened succesfully. ";
				log.Write(DEBUG, ost.str());
			}

		    MMDB_result = MMDB_lookup_string(&mmdb, getenv("REMOTE_ADDR"), &gai_error, &mmdb_error);

		    if (0 != gai_error) {
		    	{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "CSession::CSession(): ERROR MMDB_open(" << MMDB_fname << ") Error from getaddrinfo for " << getenv("REMOTE_ADDR") << ", " << gai_strerror(gai_error) << ". ";
					log.Write(ERROR, ost.str());
				}

				MMDB_usage = false;
				MMDB_close(&mmdb);
		    }

		    if (MMDB_SUCCESS != mmdb_error) {
		    	{
					CLog	log;
					ostringstream	ost;

					ost.str("");
					ost << "CSession::CSession(): ERROR MMDB_open(" << MMDB_fname << ") Got an error from libmaxminddb: " << MMDB_strerror(mmdb_error);
					log.Write(ERROR, ost.str());
				}

				MMDB_usage = false;
				MMDB_close(&mmdb);
		    }

		    MMDB_entry_data_list = NULL;
		    if (MMDB_result.found_entry) {
		        int status = MMDB_get_entry_data_list(&MMDB_result.entry, &MMDB_entry_data_list);

		        if (MMDB_SUCCESS != status) {
			    	{
						CLog	log;
						ostringstream	ost;

						ost.str("");
						ost << "CSession::CSession(): ERROR MMDB_open(" << MMDB_fname << ") : Got an error looking up the entry data -" << MMDB_strerror(status);
						log.Write(ERROR, ost.str());
					}

					MMDB_usage = false;
					MMDB_close(&mmdb);
		        }

		    }


	    }
	    else 
	    {
			CLog	log;
			ostringstream	ost;

			ost.str("");
			ost << "CSession::CSession(): ERROR in reading GeoInfo DB MMDB_open(" << MMDB_fname << ") error is: " << MMDB_strerror(status) << ". ";
			log.Write(ERROR, ost.str());

			MMDB_usage = false;

	        if (MMDB_IO_ERROR == status) 
	        {
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "CSession::CSession(): ERROR MMDB_open(" << MMDB_fname << ") returned MMDB_IO_ERROR: " << strerror(status) << ". ";
				log.Write(ERROR, ost.str());
	        }
	    }

	}
	else
	{
	    {
			CLog	log;
			ostringstream	ost;

			ost.str("");
			ost << "CSession::CSession(): REMOTE_ADDR is empty, no way to determine remote IP";
			log.Write(DEBUG, ost.str());
		}
	}
#endif

	{
			CLog	log;
			ostringstream	ost;

			ost.str("");
			ost << "CSession::CSession(): end";
			log.Write(DEBUG, ost.str());
	}
}

string CSession::GetRandom(int len)
{
	string	result;
	int	i;

	for(i = 0; i < len; i++)
	{
		result += (char)('0' + (int)(rand()/(RAND_MAX + 1.0) * 10));
	}

	return result;
}

string CSession::GetID()
{
	return sessID;
}

void CSession::SetID(string id)
{
	sessID = id;
}

string CSession::GetUser()
{
	return user;
}

void CSession::SetUser(string u)
{
	user = u;
}

string CSession::GetIP()
{
	return ip;
}

void CSession::SetIP(string i)
{
	ip = i;
}

string CSession::GetLng()
{
	return lng;
}

void CSession::SetLng(string l)
{
	lng = l;
}

void CSession::SetDB(CMysql *mysql)
{
	db = mysql;
}

int	 CSession::GetExpire()
{
	return expire;
}

void CSession::SetExpire(int i)
{
	expire = i;
}

string CSession::DetectItem(string MMDB_itemName) {
	string		item = "";

#ifdef MAXMIND_ACTIVATE
	if(MMDB_usage) {
        MMDB_entry_data_s       MMDB_entryDataS;

		if(MMDB_get_value(&MMDB_result.entry, &MMDB_entryDataS, MMDB_itemName.c_str(), "names", "ru", NULL) == MMDB_SUCCESS) {
		        if(MMDB_entryDataS.has_data) {
		                char    itemName[1024] = {0};

		                // int convert_utf8_to_windows1251(const char* utf8, char* windows1251, size_t n)
		                if(convert_utf8_to_windows1251(MMDB_entryDataS.utf8_string, itemName, MMDB_entryDataS.data_size)) { 
								CLog	log;
								ostringstream	ost;

								ost.str("");
								ost << "void CSession::DetectItem(" << MMDB_itemName << "): result is " << itemName << ". ";

								item = itemName; 
		                }
		                else {
								CLog	log;
								ostringstream	ost;

								ost.str("");
								ost << "void CSession::DetectItem(" << MMDB_itemName << "): ERROR in converting from UTF8 to CP1251. ";
								log.Write(ERROR, ost.str());
					    }
				}
		}
	}

#endif
	return	item;
}

string CSession::DetectCountry() {
	string country;

	country = DetectItem("country");
	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "void CSession::DetectCountry: auto detection country by IP  is " << country << ".";
		log.Write(DEBUG, ost.str());
    }		                

	return country;
}

string CSession::DetectCity() {
	string city;

	city = DetectItem("city");
	{
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "void CSession::DetectCity: auto detection city by IP is " << city << ".";
		log.Write(DEBUG, ost.str());
    }		                

	return city;
}

bool CSession::Save(string u, string i, string l)
{
	SetUser(u);
	SetIP(i);
	SetLng(l);
	return Save();
}

bool CSession::Save()
{
	ostringstream	ost;

	if(!db)
	{
		CLog	log;
		log.Write(ERROR, "CSession::Save(): error connect to database in CSession module");

		throw CExceptionHTML("error db");
	}

	if(GetUser().empty())
	{
		CLog	log;
		log.Write(ERROR, "CSession::Save(): user name must be set in session::Save");

		throw CExceptionHTML("session error");
	}
	if(GetID().empty())
	{
		CLog	log;
		log.Write(ERROR, "CSession::Save(): id must be set in session::Save");

		throw CExceptionHTML("session error");
	}
	if(GetIP().empty())
	{
		CLog	log;
		log.Write(ERROR, "CSession::Save(): ip must be set in session::Save");

		throw CExceptionHTML("session error");
	}
	if(GetLng().empty())
	{
		CLog	log;
		log.Write(ERROR, "CSession::Save(): lng must be set in session::Save");

		throw CExceptionHTML("session error");
	}

	ost << "INSERT INTO `sessions` (`id`, `user`, `country_auto`, `city_auto`, `lng`, `ip`, `time`,`expire` ) VALUES ('" << GetID() << "', '" << GetUser() << "', '" << DetectCountry() << "', '" << DetectCity() << "', '" << GetLng() << "', '" << GetIP() << "', NOW(), '" << SESSION_LEN * 60 << "')";

	if(db->Query(ost.str()) != 0) {
		CLog	log;
		log.Write(ERROR, "CSession::Save(): error in insert SQL-query");

		return false;
	}

	return true;
}



bool CSession::Load(string id)
{
	ostringstream	ost;
	string		currIP;

	{
		CLog	log;
		log.Write(DEBUG, "CSession::Load(", id, "): start");
	}


	if(!isExist(id))
	{
		CLog	log;

		log.Write(ERROR, "CSession::Load(", id, "): ERROR there is no session in DB, session has not been loaded from DB");

/*		cgi->ModifyCookie("sessid", "", "", "", "/");

		throw CExceptionHTML("session relogin");
*/
		return false;
	}

	SetID(id);
	SetUser(db->Get(0, "user"));
	SetIP(db->Get(0, "ip"));
	SetLng(db->Get(0, "lng"));
	SetExpire(stoi(db->Get(0, "expire")));

	{
		CLog	log;

		log.Write(DEBUG, "CSession::Load(", id, "): session has been loaded succesfully");
	}

	return true;
}

bool CSession::CheckConsistency() {
	bool		result = true;

	{
		CLog	log;
		log.Write(DEBUG, "CSession::CheckConsistency(): start");
	}
	// --- Check IP address changing
	if(getenv("REMOTE_ADDR"))
	{
		string	currIP = getenv("REMOTE_ADDR");
		string	oldIP = GetIP();

		if(currIP != oldIP)
		{
			if(UpdateIP(currIP))
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "CSession::CheckConsistency(): user (" << GetUser() << ") IP was changed (" << oldIP << " -> " << currIP << ") during the session.";
				log.Write(DEBUG, ost.str());
			}
			else
			{
				CLog	log;
				ostringstream	ost;

				ost.str("");
				ost << "CSession::CheckConsistency(): ERROR: fail to update IP (" << oldIP << " -> " << currIP << ")";
				log.Write(DEBUG, ost.str());
			}
		}
	}
	else
	{
		CLog	log;

		log.Write(DEBUG, "CSession::CheckConsistency(): environment variable 'REMOTE_ADDR' not defined");
	}

	{
		CLog	log;
		log.Write(DEBUG, "CSession::CheckConsistency(): end");
	}
	return result;
}

bool CSession::Update() {
	ostringstream	ost;

	if(GetUser() == "") {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CSession::Update(): session [" << GetID() << "], user not defined on Update function";

		log.Write(ERROR, ost.str());

		return false;		
	}

	if(GetID() == "") {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CSession::Update(): session ID not defined on Update function";

		log.Write(ERROR, ost.str());

		return false;		
	}

	ost.str("");
	ost << "update `sessions` set `time`=now() where id='" << GetID() << "'";
	if(db->Query(ost.str())) {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CSession::Update(): ERROR, session [" << GetID() << "] there are more than one session with the same session ID.";

		log.Write(ERROR, ost.str());

		return false;
	}

	return true;
}

bool CSession::UpdateIP(string newIP) {
	ostringstream	ost;

	if(GetUser() == "") {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CSession::UpdateIP(): session [" << GetID() << "], user not defined on Update function";

		log.Write(ERROR, ost.str());

		return false;		
	}

	if(GetID() == "") {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CSession::UpdateIP(): session ID not defined on Update function";

		log.Write(ERROR, ost.str());

		return false;		
	}

	ost.str("");
	ost << "UPDATE `sessions` SET `ip`='" << newIP << "' WHERE `id`='" << GetID() << "'";
	if(db->Query(ost.str())) {
		CLog	log;
		ostringstream	ost;

		ost.str("");
		ost << "CSession::UpdateIP(): ERROR, session [" << GetID() << "] there are more than one session with the same session ID.";

		log.Write(ERROR, ost.str());

		return false;
	}
	SetIP(newIP);

	return true;
}

bool CSession::isExist(string id)
{
	ostringstream	ost;
	string		currIP;

	{ 
		CLog	log;
		log.Write(DEBUG, "bool CSession::isExist(string ", id, "): start");
	}

	if(!db)
	{ 
		CLog	log;
		log.Write(ERROR, "bool CSession::isExist(string ", id, "): ERROR error connect to database in CSession::Load module");

		throw CExceptionHTML("error db");
	}
	if(id.empty())
	{
		CLog	log;
		log.Write(ERROR, "bool CSession::isExist(string ", id, "): ERROR id must be set in session::Load");

		throw CExceptionHTML("activator error");
	}

	ost << "delete from `sessions` where `time`<=(NOW()-interval `expire` second) and `expire`>'0'";
	db->Query(ost.str());
	ost.str("");
	ost << "select * from `sessions` where id='" << id << "'";
	if(db->Query(ost.str()) == 0)
	{
		CLog	log;
		log.Write(DEBUG, "bool CSession::isExist(string ", id, "): session is not exists");

		return false;
	}
	
	{
		CLog	log;
		log.Write(DEBUG, "bool CSession::isExist(string ", id, "): session exists");
	}

	return true;
}




/*
Copyright (c) <YEAR>, <OWNER>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions a
re met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in th
e documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from t
his software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT L
IMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIG
HT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AN
D ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
int CSession::convert_utf8_to_windows1251(const char* utf8, char* windows1251, size_t n)
{
        int i = 0;
        int j = 0;
        int k = 0;

		typedef struct ConvLetter {
		        char    win1251;
		        int             unicode;
		} Letter;

	 	Letter	g_letters[] = {
        {(char)0x82, 0x201A}, // SINGLE LOW-9 QUOTATION MARK
        {(char)0x83, 0x0453}, // CYRILLIC SMALL LETTER GJE
        {(char)0x84, 0x201E}, // DOUBLE LOW-9 QUOTATION MARK
        {(char)0x85, 0x2026}, // HORIZONTAL ELLIPSIS
        {(char)0x86, 0x2020}, // DAGGER
        {(char)0x87, 0x2021}, // DOUBLE DAGGER
        {(char)0x88, 0x20AC}, // EURO SIGN
        {(char)0x89, 0x2030}, // PER MILLE SIGN
        {(char)0x8A, 0x0409}, // CYRILLIC CAPITAL LETTER LJE
        {(char)0x8B, 0x2039}, // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
        {(char)0x8C, 0x040A}, // CYRILLIC CAPITAL LETTER NJE
        {(char)0x8D, 0x040C}, // CYRILLIC CAPITAL LETTER KJE
        {(char)0x8E, 0x040B}, // CYRILLIC CAPITAL LETTER TSHE
        {(char)0x8F, 0x040F}, // CYRILLIC CAPITAL LETTER DZHE
        {(char)0x90, 0x0452}, // CYRILLIC SMALL LETTER DJE
        {(char)0x91, 0x2018}, // LEFT SINGLE QUOTATION MARK
        {(char)0x92, 0x2019}, // RIGHT SINGLE QUOTATION MARK
        {(char)0x93, 0x201C}, // LEFT DOUBLE QUOTATION MARK
        {(char)0x94, 0x201D}, // RIGHT DOUBLE QUOTATION MARK
        {(char)0x95, 0x2022}, // BULLET
        {(char)0x96, 0x2013}, // EN DASH
        {(char)0x97, 0x2014}, // EM DASH
        {(char)0x99, 0x2122}, // TRADE MARK SIGN
        {(char)0x9A, 0x0459}, // CYRILLIC SMALL LETTER LJE
        {(char)0x9B, 0x203A}, // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
        {(char)0x9C, 0x045A}, // CYRILLIC SMALL LETTER NJE
        {(char)0x9D, 0x045C}, // CYRILLIC SMALL LETTER KJE
        {(char)0x9E, 0x045B}, // CYRILLIC SMALL LETTER TSHE
        {(char)0x9F, 0x045F}, // CYRILLIC SMALL LETTER DZHE
        {(char)0xA0, 0x00A0}, // NO-BREAK SPACE
        {(char)0xA1, 0x040E}, // CYRILLIC CAPITAL LETTER SHORT U
        {(char)0xA2, 0x045E}, // CYRILLIC SMALL LETTER SHORT U
        {(char)0xA3, 0x0408}, // CYRILLIC CAPITAL LETTER JE
        {(char)0xA4, 0x00A4}, // CURRENCY SIGN
        {(char)0xA5, 0x0490}, // CYRILLIC CAPITAL LETTER GHE WITH UPTURN
        {(char)0xA6, 0x00A6}, // BROKEN BAR
        {(char)0xA7, 0x00A7}, // SECTION SIGN
        {(char)0xA8, 0x0401}, // CYRILLIC CAPITAL LETTER IO
        {(char)0xA9, 0x00A9}, // COPYRIGHT SIGN
        {(char)0xAA, 0x0404}, // CYRILLIC CAPITAL LETTER UKRAINIAN IE
        {(char)0xAB, 0x00AB}, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
        {(char)0xAC, 0x00AC}, // NOT SIGN
        {(char)0xAD, 0x00AD}, // SOFT HYPHEN
        {(char)0xAE, 0x00AE}, // REGISTERED SIGN
        {(char)0xAF, 0x0407}, // CYRILLIC CAPITAL LETTER YI
        {(char)0xB0, 0x00B0}, // DEGREE SIGN
        {(char)0xB1, 0x00B1}, // PLUS-MINUS SIGN
        {(char)0xB2, 0x0406}, // CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I
        {(char)0xB3, 0x0456}, // CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I
        {(char)0xB4, 0x0491}, // CYRILLIC SMALL LETTER GHE WITH UPTURN
        {(char)0xB5, 0x00B5}, // MICRO SIGN
        {(char)0xB6, 0x00B6}, // PILCROW SIGN
        {(char)0xB7, 0x00B7}, // MIDDLE DOT
        {(char)0xB8, 0x0451}, // CYRILLIC SMALL LETTER IO
        {(char)0xB9, 0x2116}, // NUMERO SIGN
        {(char)0xBA, 0x0454}, // CYRILLIC SMALL LETTER UKRAINIAN IE
        {(char)0xBB, 0x00BB}, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
        {(char)0xBC, 0x0458}, // CYRILLIC SMALL LETTER JE
        {(char)0xBD, 0x0405}, // CYRILLIC CAPITAL LETTER DZE
        {(char)0xBE, 0x0455}, // CYRILLIC SMALL LETTER DZE
        {(char)0xBF, 0x0457} // CYRILLIC SMALL LETTER YI
	};



        for(; i < (int)n && utf8[i] != 0; ++i) {
                char prefix = utf8[i];
                char suffix = utf8[i+1];
                if ((prefix & 0x80) == 0) {
                        windows1251[j] = (char)prefix;
                        ++j;
                } else if ((~prefix) & 0x20) {
                        int first5bit = prefix & 0x1F;
                        first5bit <<= 6;
                        int sec6bit = suffix & 0x3F;
                        int unicode_char = first5bit + sec6bit;


                        if ( unicode_char >= 0x410 && unicode_char <= 0x44F ) {
                                windows1251[j] = (char)(unicode_char - 0x350);
                        } else if (unicode_char >= 0x80 && unicode_char <= 0xFF) {
                                windows1251[j] = (char)(unicode_char);
                        } else if (unicode_char >= 0x402 && unicode_char <= 0x403) {
                                windows1251[j] = (char)(unicode_char - 0x382);
                        } else {
                                int count = sizeof(g_letters) / sizeof(Letter);
                                for (k = 0; k < count; ++k) {
                                        if (unicode_char == g_letters[k].unicode) {
                                                windows1251[j] = g_letters[k].win1251;
                                                goto NEXT_LETTER;
                                        }
                                }
                                // can't convert this char
                                return 0;
                        }
NEXT_LETTER:
                        ++i;
                        ++j;
                } else {
                        // can't convert this chars
                        return 0;
                }
        }
        windows1251[j] = 0;
        return 1;
}











CSession::~CSession()
{
	{
			CLog	log;
			log.Write(DEBUG, "CSession::~CSession(): start");
	}

#ifdef MAXMIND_ACTIVATE

	if(MMDB_usage) {
		MMDB_close(&mmdb);
	}

#endif

	{
			CLog	log;
			log.Write(DEBUG, "CSession::~CSession(): end");
	}
}





