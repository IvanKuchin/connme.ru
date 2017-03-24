#ifndef __CATALOG__H__
#define __CATALOG__H__

#include <algorithm>
#include <vector>
#include <string>
#include <map>

#include "cmysql.h"
#include "cvars.h"

using namespace std;

class Manufacturer
{
	private:
		int		id;
		int		parentID;
		int		order;
		string		name;
		string		content;
		string		file;
		string		isShow;
		string		image1;
		string		image2;
	public:
		void		SetID(int s);
		void		SetParentID(int s);
		void		SetOrder(int s);
		void		SetContent(string s);
		void		SetName(string s);
		void		SetFile(string s);
		void		SetIsShow(string s);
		void		SetImage1(string s);
		void		SetImage2(string s);

		int		GetID();
		int		GetParentID();
		int		GetOrder();
		string		GetContent();
		string		GetName();
		string		GetFile();
		string		IsShow();
		string		GetImage1();
		string		GetImage2();
};

class Catalog
{
	private:
		vector<Manufacturer *>	catalog;
		CMysql			*db;
	public:
				Catalog();

		void		SetDB(CMysql *d);

		void		Load();
		Manufacturer*	GetItem(int id);
		Manufacturer*	GetItemByOrder(int order);
		
		// get first item in menu by parentID
		// IN: parent id
		// OUT: first menu item or NULL
		Manufacturer*	GetFirstItem(int parentID);
		
		// get next sibling
		// IN: current order id
		// OUT: next sibling or NULL
		Manufacturer*	GetNextSibling(int order);

		// get one menu item
		// IN: parent ID and order
		// OUT: menu item or NULL
		Manufacturer*	GetManufacturer(int parentID, int order);

		// get count menu item for some level
		// IN: parent ID
		// OUT: count menu item
		unsigned int	GetCountItem(int parentID);
				
				~Catalog();
};


#endif


