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
		enum state_enum { boundary, multiline_start };

		string					config_folder				=	""s;

		c_config_cache			cache;

		tuple<string, string>	ExtractSingleValue(const string &line);
		string					RemoveComment(string line);
		map<string, string>		ReadFileContent(const string &file);

	public:
								c_config(const string &folder) : config_folder {folder} {};

		map<string, string>		GetFromFile(const string &file, const vector<string> &params);

		void					SetConfigFolder(const string &param)					{ config_folder = param; }
		string					GetConfigFolder()										{ return config_folder; }

};

ostream&	operator<<(ostream& os, const c_config &);

#endif
