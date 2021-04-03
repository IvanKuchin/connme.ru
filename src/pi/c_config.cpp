#include "c_config.h"

bool					c_config_cache::IsInCache(const string &file)
{
	MESSAGE_DEBUG("", "", "start");

	auto	result = (content.find(file) != content.end());

	MESSAGE_DEBUG("", "", "finish (" + to_string(result) + ")");

	return result;
}

bool					c_config_cache::IsInCache(const string &file, const vector<string> &entries)
{
	MESSAGE_DEBUG("", "", "start");

	auto	result				= true;

	if(IsInCache(file))
	{
		auto	file_content = content.find(file)->second;

		for(auto &entry: entries)
		{
			result &= (file_content.find(entry) != file_content.end());
		}
	}
	else
	{
		MESSAGE_DEBUG("", "", "file " + file + " not in cache");
	}

	MESSAGE_DEBUG("", "", "finish (" +  to_string(result) + ")");

	return result;
}

map<string, string>		c_config_cache::Get(const string &file, const vector<string> &entries)
{
	MESSAGE_DEBUG("", "", "start");

	map<string, string>	result;

	if(IsInCache(file))
	{
		auto	file_content = content.find(file)->second;

		for(auto &entry: entries)
		{
			if(file_content.find(entry) != file_content.end()) result[file_content.find(entry)->first] = file_content.find(entry)->second;
		}
	}
	else
	{
		MESSAGE_DEBUG("", "", "file " + file + " not in cache");
	}

	MESSAGE_DEBUG("", "", "finish (" + to_string(result.size()) + ")");

	return result;
}


string	c_config::trim(string line)
{
	line.erase(0, line.find_first_not_of(" \t\f\v\n\r"));	   //prefixing spaces
	line.erase(line.find_last_not_of(" \t\f\v\n\r") + 1);		 //suffixing spaces

	return line;
}

tuple<string, string> c_config::ExtractSingleValue(const string &line)
{
	MESSAGE_DEBUG("", "", "start (" + line + ")");

	tuple<string, string>	result;

	regex	e("^\\s*(.*)\\s*=\\s*\"(.*)\"");
	smatch	m;

	if(regex_search(line, m, e))
	{
		result = make_tuple(trim(m[1]), trim(m[2]));
	}

	MESSAGE_DEBUG("", "", "finish (" + get<0>(result) + ", " + get<1>(result) + ")");

	return result;
}
/*
string				c_config::GetKeyValue(ifstream &f)
{
}
*/

string				c_config::RemoveComment(string line)
{
	MESSAGE_DEBUG("", "", "start (" + line + ")");

	auto	comment_pos = line.find('#');

	if(comment_pos != string::npos)
		line.resize(comment_pos);

	MESSAGE_DEBUG("", "", "finish (" + line + ")");

	return line;
}



map<string, string>	c_config::ReadFileContent(const string &file)
{
	MESSAGE_DEBUG("", "", "start (" + file + ")");

	map<string, string>		result;
	ifstream				f(file);

	if(f.is_open())
	{
		auto	line = ""s;
		auto	state = BOUNDARY;

		while( getline (f,line) )
		{
			line = RemoveComment(line);
			line = trim(line);
			
			if(state = BOUNDARY)
			auto	value = ExtractSingleValue(line);

			if(get<0>(value).length())
				result[get<0>(value)] = get<1>(value);
		}
		f.close();
	}
	else
	{
		MESSAGE_ERROR("", "", "can't open " + file)
	}

	MESSAGE_DEBUG("", "", "finish (result size: " + to_string(result.size()) + ")");

	return result;
}

map<string, string>		c_config::GetFromFile(const string &file, const vector<string> &params)
{
	MESSAGE_DEBUG("", "", "start");

	map<string, string>		result;

	if(!cache.IsInCache(file))
	{
		auto file_content = ReadFileContent(GetConfigFolder() + file);
		cache.Add(file, file_content);
	}

	result = cache.Get(file, params);

	MESSAGE_DEBUG("", "", "finish (result size: " + to_string(result.size()) + ")");

	return result;
}


ostream& operator<<(ostream& os, const c_config &var)
{
	os << string("test line");
	return os;
}

