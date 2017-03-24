#include "ccatalog.h"
#include "clog.h"
#include "localy.h"
#include "cvars.h"
#include "cexception.h"
#include "ctemplate.h"

void Manufacturer::SetID(int s)
{
	id = s;
}

void Manufacturer::SetOrder(int s)
{
	order = s;
}

void Manufacturer::SetParentID(int s)
{
	parentID = s;
}

void Manufacturer::SetContent(string s)
{
	content = s;
}

void Manufacturer::SetName(string s)
{
	name = s;
}

void Manufacturer::SetFile(string s)
{
	file = s;
}

void Manufacturer::SetIsShow(string s)
{
	isShow = s;
}

void Manufacturer::SetImage1(string s)
{
	image1 = s;
}

void Manufacturer::SetImage2(string s)
{
	image2 = s;
}

int Manufacturer::GetID()
{
	return id;
}

int Manufacturer::GetOrder()
{
	return order;
}

int Manufacturer::GetParentID()
{
	return parentID;
}

string Manufacturer::GetContent()
{
	return content;
}

string Manufacturer::GetFile()
{
	return file;
}

string Manufacturer::GetName()
{
	return name;
}

string Manufacturer::GetImage1()
{
	return image1;
}

string Manufacturer::GetImage2()
{
	return image2;
}

string Manufacturer::IsShow()
{
	return isShow;
}

Catalog::Catalog() : db(NULL)
{
}

void Catalog::SetDB(CMysql *d)
{
	db = d;
}

void Catalog::Load()
{
	int		affected;
	int		i;
	Manufacturer	*mi;
	
	if(db == NULL)
	{	
		CLog	log;

		log.Write(ERROR, "error db-object inside Catalog");
		throw CExceptionHTML("error db");
	}

	affected = db->Query("select * from `manufacture`");
	if(affected <= 0)
	{
		CLog	log;

		log.Write(ERROR, "error select from 'Manufacture' table");
		throw CExceptionHTML("error db");
	}

	for(i = 0;  i < affected; i++)
	{
		string	isPrivate;

		mi = new Manufacturer();
		mi->SetID	(atoi(db->Get(i, "id")));
		mi->SetOrder	(atoi(db->Get(i, "order")));
		mi->SetParentID	(atoi(db->Get(i, "parentID")));
		mi->SetName	(db->Get(i, "name"));
		mi->SetContent	(db->Get(i, "content"));
		mi->SetFile	(db->Get(i, "file"));
		mi->SetImage1	(db->Get(i, "image1"));
		mi->SetImage2	(db->Get(i, "image2"));
		mi->SetIsShow	(db->Get(i, "isShow"));
		
		catalog.push_back(mi);
	}
}

Manufacturer *Catalog::GetItem(int id)
{
	vector<Manufacturer *>::iterator	mi;
	for(mi = catalog.begin(); mi < catalog.end(); mi++) if((*mi)->GetID() == id) return(*mi);
	
	return(NULL);
}

Manufacturer *Catalog::GetItemByOrder(int order)
{
	vector<Manufacturer *>::iterator	mi;
	for(mi = catalog.begin(); mi < catalog.end(); mi++) if((*mi)->GetOrder() == order) return(*mi);
	
	return(NULL);
}

Manufacturer *Catalog::GetNextSibling(int order)
{
	vector<Manufacturer *>::iterator	mii;
	Manufacturer			*mi;
	int				parentID = 0;
	vector<int>			ids;
	vector<int>::iterator		idsii;
	
	mi = GetItemByOrder(order);
	if(mi != NULL) parentID = mi->GetParentID();

	for(mii = catalog.begin(); mii < catalog.end(); mii++)
	{
		if((*mii)->GetParentID() == parentID) ids.push_back((*mii)->GetOrder());
	}
	sort(ids.begin(), ids.end());
	idsii = find(ids.begin(), ids.end(), order);
	idsii++;
	if(idsii < ids.end()) return(GetItemByOrder(*idsii));
	return(NULL);
}

Manufacturer *Catalog::GetFirstItem(int parentID)
{
	vector<Manufacturer *>::iterator	mii;
	// Manufacturer			*mi;
	vector<int>			ids;
	vector<int>::iterator		idsii;
	
	for(mii = catalog.begin(); mii < catalog.end(); mii++)
	{
		if((*mii)->GetParentID() == parentID) ids.push_back((*mii)->GetOrder());
	}
	sort(ids.begin(), ids.end());
	if(ids.begin() != ids.end()) return(GetItemByOrder(*(ids.begin())));
	return(NULL);
}

Manufacturer *Catalog::GetManufacturer(int parentID, int order)
{
	vector<Manufacturer *>::iterator	mii;
	// Manufacturer			*mi;
	vector<int>			ids;
	vector<int>::iterator		idsii;
	
	for(mii = catalog.begin(); mii < catalog.end(); mii++)
	{
		if((*mii)->GetParentID() == parentID) ids.push_back((*mii)->GetOrder());
	}
	sort(ids.begin(), ids.end());
	idsii = ids.begin() + order;
	if(idsii < ids.end()) return(GetItemByOrder(*idsii));
	return(NULL);
}

unsigned int Catalog::GetCountItem(int parentID)
{
	vector<Manufacturer *>::iterator	mii;
	int				count = 0;

	for(mii = catalog.begin(); mii < catalog.end(); mii++)
	{
		if((*mii)->GetParentID() == parentID) count++;
	}
	return(count);
}

Catalog::~Catalog()
{
	vector<Manufacturer *>::iterator	mi;
	for(mi = catalog.begin(); mi < catalog.end(); mi++) free(*mi);
}









