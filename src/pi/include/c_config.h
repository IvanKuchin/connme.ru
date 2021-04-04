#ifndef __C_CONFIG__H__
#define __C_CONFIG__H__

#include <iostream>
#include <fstream>
#include <regex>
#include <tuple>
#include <vector>
#include <map>

#include "clog.h"

using namespace std;

class c_config_cache
{
	private:
		const string			DEFAULT_KEY	= "__default__";
		map<string, map<string, string>>	content;

	public:
		void					Add(const string &file, const map<string, string> &file_content) { content[file] = file_content; };
		bool					IsInCache(const string &file);
		bool					IsInCache(const string &file, const vector<string> &entry);
		map<string, string>		Get(const string &file, const vector<string> &entries);
};

class c_config
{
	private:
		string					config_folder				=	""s;

		c_config_cache			cache;

		// --- multiline related definitions
		enum state_enum 		{ KEYVALUE_PAIR, MULTILINE_VALUE };
		const string			MULTILINE_MARKER = "\'\'\'";
		bool					IsMultilineMarkerPresent(const string &line);
		bool		 			IsStartFromMultilineMarker(const string &line);
		bool		 			IsLineEndsWithMultilineMarker(const string &line);
		string					StripMultilineMarker(string line);

		// --- support class methods
		tuple<string, string>	ExtractKeyValue(const string &line);
		string					RemoveComment(string line);
		map<string, string>		ReadFileContent(const string &file);
		string					trim(string line);

	public:
								c_config(const string &folder) : config_folder {folder} {};

		map<string, string>		GetFromFile(const string &file, const vector<string> &params);
		string					GetFromFile(const string &file, const string &params);

		void					SetConfigFolder(const string &param)					{ config_folder = param; }
		string					GetConfigFolder()										{ return config_folder; }

};

ostream&	operator<<(ostream& os, const c_config &);

#endif
