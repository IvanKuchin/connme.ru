#ifndef __CVARS__H__
#define __CVARS__H__

#include <map>
#include <string>

using namespace std;

#include "clog.h"


class CNocaseCmp
{
    public:
	bool operator()(const string &x, const string &y) const
	{
	    return(strcasecmp(x.c_str(), y.c_str()) < 0);
	}
};

class CVars : public map<string, string, CNocaseCmp>
{
    public:
		    CVars();

		    bool Add(string name, string value);
                    bool Redefine(string name, string value);
		    string Get(string name);
		    bool Set(string name, string val);
		    bool Delete(string name);
		    int	Count();

		    ~CVars();
};

#endif
