#include <utility>
#include "cmenu.h"
#include "clog.h"
#include "localy.h"
#include "cvars.h"
#include "cexception.h"
#include "ctemplate.h"

void MenuItem::SetID(int s)
{
	id = s;
}

void MenuItem::SetOrder(int s)
{
	order = s;
}

void MenuItem::SetParentID(int s)
{
	parentID = s;
}

void MenuItem::SetContent(string s)
{
	content = s;
}

void MenuItem::SetPrivate(bool s)
{
	if(s) priv = "1"; else priv = "0";
}

void MenuItem::SetPartID(string s)
{
	partID = s;
}

int MenuItem::GetID()
{
	return id;
}

int MenuItem::GetOrder()
{
	return order;
}

int MenuItem::GetParentID()
{
	return parentID;
}

string MenuItem::GetContent()
{
	return content;
}

string MenuItem::GetPartID()
{
	return partID;
}

bool MenuItem::isPrivate()
{
	if(priv == "1") return true;
	return false;
}

Menu::Menu() : db(NULL)
{
}

void Menu::SetDB(CMysql *d)
{
	db = d;
}

void Menu::Load()
{
	int		affected;
	int		i;
	MenuItem	*mi;
	
	if(db == NULL)
	{	
		CLog	log;

		log.Write(ERROR, "error db-object inside Menu");
		throw CExceptionHTML("error db");
	}

	affected = db->Query("select * from menu");
	if(affected <= 0)
	{
		CLog	log;

		log.Write(ERROR, "error select from 'menu' table");
		throw CExceptionHTML("error db");
	}

	for(i = 0;  i < affected; i++)
	{
		string	isPrivate;

		mi = new MenuItem();
		mi->SetID	(atoi(db->Get(i, "id")));
		mi->SetOrder	(atoi(db->Get(i, "order")));
		mi->SetParentID	(atoi(db->Get(i, "parentID")));
		mi->SetPartID	(db->Get(i, "partID"));
		mi->SetContent	(db->Get(i, "content"));
		isPrivate = db->Get(i, "isPrivate");
		mi->SetPrivate	(((isPrivate == "0") ? false : true));
		
		menu.push_back(mi);
	}
}

MenuItem *Menu::GetItem(int id)
{
	vector<MenuItem *>::iterator	mi;
	for(mi = menu.begin(); mi < menu.end(); mi++) if((*mi)->GetID() == id) return(*mi);
	
	return(NULL);
}

MenuItem *Menu::GetItemByOrder(int order)
{
	vector<MenuItem *>::iterator	mi;
	for(mi = menu.begin(); mi < menu.end(); mi++) if((*mi)->GetOrder() == order) return(*mi);
	
	return(NULL);
}

MenuItem *Menu::GetNextSiblingByID(int id)
{
	vector<MenuItem *>::iterator	mii;
	MenuItem			*mi;
	int				parentID = 0;
	vector< pair<int, int> >			ids;
	vector< pair<int, int> >::iterator		idsii;
	
	mi = GetItem(id);
	if(mi != NULL) parentID = mi->GetParentID();

	for(mii = menu.begin(); mii < menu.end(); mii++)
	{
		if((*mii)->GetParentID() == parentID) ids.push_back(pair<int, int>((*mii)->GetOrder(), (*mii)->GetID()));
	}
	sort(ids.begin(), ids.end(), Pair_Lt());
	for(idsii = ids.begin(); idsii < ids.end(); idsii++)
		if(idsii->second == id) break;
	idsii++;
	if(idsii < ids.end()) return(GetItem((*idsii).second));
	return(NULL);
}

MenuItem *Menu::GetNextSibling(int order)
{
	vector<MenuItem *>::iterator	mii;
	MenuItem			*mi;
	int				parentID = 0;
	vector< pair<int, int> >			ids;
	vector< pair<int, int> >::iterator		idsii;
	
	mi = GetItemByOrder(order);
	if(mi != NULL) parentID = mi->GetParentID();

	for(mii = menu.begin(); mii < menu.end(); mii++)
	{
		if((*mii)->GetParentID() == parentID) ids.push_back(pair<int, int>((*mii)->GetOrder(), (*mii)->GetID()));
	}
	sort(ids.begin(), ids.end());
//	idsii = find(ids.begin(), ids.end(), order);
	for(idsii = ids.begin(); idsii < ids.end(); idsii++)
	{
		if(idsii->first == order) break;
	}
	idsii++;
	if(idsii < ids.end()) return(GetItem((*idsii).second));
	return(NULL);
}

MenuItem *Menu::GetFirstItem(int parentID)
{
	vector<MenuItem *>::iterator	mii;
	// MenuItem			*mi;
	vector<int>			ids;
	vector<int>::iterator		idsii;
	
	for(mii = menu.begin(); mii < menu.end(); mii++)
	{
		if((*mii)->GetParentID() == parentID) ids.push_back((*mii)->GetOrder());
	}
	sort(ids.begin(), ids.end());
	if(ids.begin() != ids.end()) return(GetItemByOrder(*(ids.begin())));
	return(NULL);
}

MenuItem *Menu::GetMenuItem(int parentID, int order)
{
	vector<MenuItem *>::iterator	mii;
	// MenuItem			*mi;
	vector<int>			ids;
	vector<int>::iterator		idsii;
	
	for(mii = menu.begin(); mii < menu.end(); mii++)
	{
		if((*mii)->GetParentID() == parentID) ids.push_back((*mii)->GetOrder());
	}
	sort(ids.begin(), ids.end());
	idsii = ids.begin() + order;
	if(idsii < ids.end()) return(GetItemByOrder(*idsii));
	return(NULL);
}

unsigned int Menu::GetCountItem(int parentID)
{
	vector<MenuItem *>::iterator	mii;
	int				count = 0;

	for(mii = menu.begin(); mii < menu.end(); mii++)
	{
		if((*mii)->GetParentID() == parentID) count++;
	}
	return(count);
}

Menu::~Menu()
{
	vector<MenuItem *>::iterator	mi;
	for(mi = menu.begin(); mi < menu.end(); mi++) free(*mi);
}









