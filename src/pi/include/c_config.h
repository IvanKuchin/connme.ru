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

class c_config
{
	protected:
		string					config_folder				=	""s;

		string					secret_file					=	"secret"s;
		string					number_of_folders_file		=	"number_of_folders"s;

		tuple<string, string>	ExtractSingleValue(const string &line, const vector<string> &params);
		map<string, string>		Read(const string &file, const vector<string> &params);

	public:
								c_config(const string &folder) : config_folder {folder} {};

		map<string, string>		ReadFromSecret(const vector<string> &params)			{ 	return Read(GetConfigFolder() + GetSecretFilename(), params); }
		map<string, string>		ReadFromNumberOfFolders(const vector<string> &params)	{ 	return Read(GetConfigFolder() + GetNumberOfFoldersFilename(), params); }

		void					SetConfigFolder(const string &param)					{ config_folder = param; }
		string					GetConfigFolder()										{ return config_folder; }

		void					SetSecretFilename(const string &param)					{ secret_file = param; }
		string					GetSecretFilename()										{ return secret_file; }

		void					SetNumberOfFoldersFilename(const string &param)			{ number_of_folders_file = param; }
		string					GetNumberOfFoldersFilename()							{ return number_of_folders_file; }

};

ostream&	operator<<(ostream& os, const c_config &);

#endif
