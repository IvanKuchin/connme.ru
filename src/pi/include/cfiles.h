#ifndef __CFILES__H__
#define __CFILES__H__

#include <vector>
#include <string>

using namespace std;

#include "cexception.h"
#include "clog.h"


class CFile
{
	private:
		char*		content;
		int			length;
		string		name;
	public:
					CFile();
	
		string		GetName();
		int			GetSize();
		char*		GetContent();

		void		SetName(string n);
		void		SetSize(int s);
		void		SetContent(char *c);
		void		Set(string n, char* c, int s);

		void		Delete();

					~CFile();
};

class CFiles
{
	private:
		vector<CFile *>	files;
	public:
					CFiles();

		bool		Add(string name, char *content, int size);
		string		GetName(int i);
		char*		Get(int i);
		char*		Get(string name);
		int			GetSize(int i);
		int			GetSize(string name);
		void		Delete(string name);
		int			Count();

					~CFiles();
};

#endif
