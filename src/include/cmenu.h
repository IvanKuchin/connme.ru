#ifndef __MENU__H__
#define __MENU__H__

#include <algorithm>
#include <vector>
#include <string>
#include <map>

#include "cmysql.h"
#include "cvars.h"

using namespace std;

class Pair_Lt
{
    public:
        int operator()(pair<int, int> o1, pair<int, int> o2)
        {
            return o1.first < o2.first;
        }
};

class MenuItem
{
	private:
		int		id;
		int		parentID;
		int		order;
		string		priv;
		string		content;
		string		partID;
	public:
		void		SetID(int s);
		void		SetParentID(int s);
		void		SetOrder(int s);
		void		SetPartID(string s);
		void		SetContent(string s);
		void		SetPrivate(bool s);

		int		GetID();
		int		GetParentID();
		int		GetOrder();
		string		GetPartID();
		string		GetContent();
		bool		isPrivate();
};

class Menu
{
	private:
		vector<MenuItem *>	menu;
		CMysql			*db;
	public:
				Menu();

		void		SetDB(CMysql *d);

		void		Load();
		MenuItem*	GetItem(int id);
		MenuItem*	GetItemByOrder(int order);
		
		// get first item in menu by parentID
		// IN: parent id
		// OUT: first menu item or NULL
		MenuItem*	GetFirstItem(int parentID);
		
		// get next sibling
		// IN: current order id
		// OUT: next sibling or NULL
		MenuItem*	GetNextSibling(int order);
		MenuItem*	GetNextSiblingByID(int id);

		// get one menu item
		// IN: parent ID and order
		// OUT: menu item or NULL
		MenuItem*	GetMenuItem(int parentID, int order);

		// get count menu item for some level
		// IN: parent ID
		// OUT: count menu item
		unsigned int	GetCountItem(int parentID);
				
				~Menu();
};


#endif


