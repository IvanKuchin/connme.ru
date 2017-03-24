#include <sys/time.h>
#include <fstream>
//#include <magick/api.h>

#include "ccgi.h"
#include "cvars.h"
#include "clog.h"
#include "cmysql.h"
#include "localy.h"
#include "cexception.h"
#include "cmenu.h"
#include "cforum.h"
#include "cmail.h"
#include "ccatalog.h"

string GetRandom(int len)
{
	string	result;
	int	i;

	for(i = 0; i < len; i++)
	{
		result += (char)('0' + (int)(rand()/(RAND_MAX + 1.0) * 10));
	}

	return result;
}

string DeleteHTML(string src)
{
    string 		result;
    string::size_type	firstPos, lastPos;

    result = src;
    firstPos = result.find("<");
    if(firstPos == string::npos) return result;
    lastPos = result.find(">", firstPos);
    if(lastPos == string::npos) lastPos = result.length();

    while(firstPos != string::npos)
    {
	result.erase(firstPos, lastPos - firstPos + 1);

        firstPos = result.find("<");
	if(firstPos == string::npos) break;
	lastPos = result.find(">", firstPos);
	if(lastPos == string::npos) lastPos = result.length();
    }

    return result;
}

string removeQuotas(string src)
{
    string		result = src;
    string::size_type	pos = 0;

    while((pos = result.find("\"", pos)) != string::npos)
    {
	result.replace(pos, 1, "\\\"");
	pos += 2;
    }

    return result;
}

string GetMenu(Menu *m, int pID)
{
	ostringstream		ost;
	MenuItem		*tmpMI;
	
	ost << "<ul>";
	tmpMI = m->GetFirstItem(pID);
	while(tmpMI != NULL)
	{
		ost << "<li>";
		{
			CLog	log;
			log.Write(DEBUG, tmpMI->GetContent());
		}
		ost << "<a href=" << getenv("SCRIPT_NAME") << "?act=editmenu&id=" << tmpMI->GetID() << "&rnd=" << GetRandom(10) << ">" << tmpMI->GetID() << " " << tmpMI->GetContent() << "</a>";
		ost << GetMenu(m, tmpMI->GetID());
		ost << "</li>";
		tmpMI = m->GetNextSibling(tmpMI->GetOrder());
	}
	ost << "</ul>";
	return ost.str();
}


int main()
{
    CCgi	indexPage(EXTERNAL_TEMPLATE);
    string	act, id, content;
    char	query[102400];
    int		affected;
    struct	timeval	tv;
// 
    gettimeofday(&tv, NULL);
    srand(tv.tv_sec * tv.tv_usec * 100000);

    try
    {

    indexPage.ParseURL();

    if(!indexPage.SetTemplateFile("templates/adminindex.htmlt"))
    {
	CLog	log;

	log.Write(ERROR, "template file was missing");
	throw CException("Template file was missing");
    }


    CMysql	db;
    if(db.Connect(DB_NAME, DB_LOGIN, DB_PASSWORD) < 0)
    {
        CLog	log;

        log.Write(ERROR, "Can not connect to mysql database");
        return(1);
    }

    db.Query("set names cp1251");
    act = indexPage.GetVarsHandler()->Get("act");

    if(act.empty())
    {
	act = "list_parts_main";
    }

    indexPage.RegisterVariable("rand", GetRandom(10).c_str());

//-------------------------- Offers editing
    if(act == "addoffers")
    {
	indexPage.RegisterVariableForce("isshow", "<select name=isshow><option value='n'>нет</option><option value='y'>да</option>");
	indexPage.SetTemplateFile("templates/adminoffersadd.htmlt");
    }
    if(act == "offersaddsubmit")
    {
	string		fileName = indexPage.GetFilesHandler()->GetName(0);

	memset(query, 0, sizeof(query));

	snprintf(query, sizeof(query) - 2, "insert into `offers` (`price`, `title`, `content_brief`, `content`, `isshow`, `file`) values(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")", removeQuotas(indexPage.GetVarsHandler()->Get("price")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("title")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content_brief")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content")). c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("isshow")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("file")).c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "Спец. предложение добавлено");
    }
    if(act == "listoffers")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select * from offers;");
	affected = db.Query(query);

	content = "<table>\n";
	for(int i = 0; i < affected; i++)
	{
	    content += "<tr><td>";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=deloffers&id=";
            content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += ">";
	    content += "<img src=/images/button_drop.png border=0></a>";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=editoffers&id=";
	    content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += "><img src=/images/button_edit.png border=0> ";
	    content += DeleteHTML(db.Get(i, "title"));
	    content += " (";
	    content += DeleteHTML(db.Get(i, "price"));
	    content += "$)</a>";


	    content += "</td>";
	    content += "</tr>\n";
	}
	content += "</table>\n";
        indexPage.RegisterVariableForce("content", content.c_str());
    }
    if(act == "editoffers")
    {
	ostringstream	ost;
	string		content, content_brief, title, price, id, image, isshow, file, isshowStr;
    
	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "select * from offers where id=%s", id.c_str());
	affected = db.Query(query);
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting offers from db");
		throw CException("error getting offers from db");
	}

	content = db.Get(0, "content");
	content_brief = db.Get(0, "content_brief");
	title = db.Get(0, "title");
	price = db.Get(0, "price");
	id = db.Get(0, "id");
	file = db.Get(0, "file");
	isshow = db.Get(0, "isshow");

	isshowStr = "<select name=isshow><option value='n'>нет</option><option value='y' ";
	if(isshow == "y") isshowStr += " selected ";
	isshowStr += ">да</option>";

	indexPage.RegisterVariableForce("id", id);
        indexPage.RegisterVariableForce("price", price);
        indexPage.RegisterVariableForce("title", title);
        indexPage.RegisterVariableForce("content_brief", content_brief);
        indexPage.RegisterVariableForce("content", content);
        indexPage.RegisterVariableForce("file", file);
        indexPage.RegisterVariableForce("isshow", isshowStr);
	
	indexPage.SetTemplateFile("templates/adminoffersedit.htmlt");
    }
    if(act == "offerseditsubmit")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "update `offers` set `price`=\"%s\", `title`=\"%s\", `content_brief`=\"%s\", `content`=\"%s\", `file`=\"%s\", `isshow`=\"%s\" where `id`='%s'", removeQuotas(indexPage.GetVarsHandler()->Get("price")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("title")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content_brief")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("file")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("isshow")).c_str(), indexPage.GetVarsHandler()->Get("id").c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "Специальное предложение обновлено");
    }
    if(act == "deloffers")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "delete from offers where `id`=\"%s\"", removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "Специальное предложение удалено.");
    } 
//-------------------------- Newthings editing
    if(act == "addnewthings")
    {
	indexPage.RegisterVariableForce("isshow", "<select name=isshow><option value='n'>нет</option><option value='y'>да</option>");
	indexPage.SetTemplateFile("templates/adminnewthingsadd.htmlt");
    }
    if(act == "newthingsaddsubmit")
    {
	string		fileName = indexPage.GetFilesHandler()->GetName(0);

	memset(query, 0, sizeof(query));

	snprintf(query, sizeof(query) - 2, "insert into `newthings` (`price`, `title`, `content_brief`, `content`, `isshow`, `file`) values(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")", removeQuotas(indexPage.GetVarsHandler()->Get("price")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("title")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content_brief")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content")). c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("isshow")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("file")).c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "Новинка добавлена");
    }
    if(act == "listnewthings")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select * from newthings;");
	affected = db.Query(query);

	content = "<table>\n";
	for(int i = 0; i < affected; i++)
	{
	    content += "<tr><td>";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=delnewthings&id=";
            content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += ">";
	    content += "<img src=/images/button_drop.png border=0></a>";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=editnewthings&id=";
	    content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += "><img src=/images/button_edit.png border=0> ";
	    content += DeleteHTML(db.Get(i, "title"));
	    content += "</a>";


	    content += "</td>";
	    content += "</tr>\n";
	}
	content += "</table>\n";
        indexPage.RegisterVariableForce("content", content.c_str());
    }
    if(act == "editnewthings")
    {
	ostringstream	ost;
	string		content, content_brief, title, price, id, image, isshow, file, isshowStr;
    
	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "select * from newthings where id=%s", id.c_str());
	affected = db.Query(query);
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting offers from db");
		throw CException("error getting offers from db");
	}

	content = db.Get(0, "content");
	content_brief = db.Get(0, "content_brief");
	title = db.Get(0, "title");
	price = db.Get(0, "price");
	id = db.Get(0, "id");
	file = db.Get(0, "file");
	isshow = db.Get(0, "isshow");

	isshowStr = "<select name=isshow><option value='n'>нет</option><option value='y' ";
	if(isshow == "y") isshowStr += " selected ";
	isshowStr += ">да</option>";

	indexPage.RegisterVariableForce("id", id);
        indexPage.RegisterVariableForce("price", price);
        indexPage.RegisterVariableForce("title", title);
        indexPage.RegisterVariableForce("content_brief", content_brief);
        indexPage.RegisterVariableForce("content", content);
        indexPage.RegisterVariableForce("file", file);
        indexPage.RegisterVariableForce("isshow", isshowStr);
	
	indexPage.SetTemplateFile("templates/adminnewthingsedit.htmlt");
    }
    if(act == "newthingseditsubmit")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "update `newthings` set `price`=\"%s\", `title`=\"%s\", `content_brief`=\"%s\", `content`=\"%s\", `file`=\"%s\", `isshow`=\"%s\" where `id`='%s'", removeQuotas(indexPage.GetVarsHandler()->Get("price")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("title")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content_brief")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("file")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("isshow")).c_str(), indexPage.GetVarsHandler()->Get("id").c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "Новинка обновлена");
    }
    if(act == "delnewthings")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "delete from newthings where `id`=\"%s\"", removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "Новинка удалена.");
    } 

//-------------------------- Competitions editing
    if(act == "add_competit")
    {
	indexPage.RegisterVariableForce("isshow", "<select name=isshow><option value='n'>нет</option><option value='y'>да</option>");
	indexPage.SetTemplateFile("templates/admincompetitadd.htmlt");
    }
    if(act == "competitaddsubmit")
    {
	string		fileName = indexPage.GetFilesHandler()->GetName(0);

	memset(query, 0, sizeof(query));

	snprintf(query, sizeof(query) - 2, "insert into `competitions` (`name`, `isshow`, `content`) values(\"%s\", \"%s\", \"%s\")", removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("isshow")).c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "Конкурс добавлен");
    }
    if(act == "list_competit")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select * from competitions;");
	affected = db.Query(query);

	content = "<table>\n";
	for(int i = 0; i < affected; i++)
	{
	    content += "<tr><td>";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=delcompetit&id=";
            content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += ">";
	    content += "<img src=/images/button_drop.png border=0></a>";
	    content += " <a href=/cgi-bin/admin/parts.cgi?act=editcompetit&id=";
	    content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += "><img src=/images/button_edit.png border=0> ";
	    content += DeleteHTML(db.Get(i, "name"));
	    content += "</a>";
	    content += "</td></tr>\n";
	}
	content += "</table>\n";
        indexPage.RegisterVariableForce("content", content.c_str());
    }
    if(act == "editcompetit")
    {
	ostringstream	ost;
	string		content, brief, title, name, id, image, isshow, file, isshowStr;
    
	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "select * from competitions where id=%s", id.c_str());
	affected = db.Query(query);
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting offers from db");
		throw CException("error getting offers from db");
	}

	name = db.Get(0, "name");
	isshow = db.Get(0, "isshow");
	content = db.Get(0, "content");

	isshowStr = "<select name=isshow><option value='n'>нет</option><option value='y' ";
	if(isshow == "y") isshowStr += " selected ";
	isshowStr += ">да</option>";

	indexPage.RegisterVariableForce("id", id);
	indexPage.RegisterVariableForce("name", name);
	indexPage.RegisterVariableForce("content", content);
	indexPage.RegisterVariableForce("isshow", isshowStr);
	
	indexPage.SetTemplateFile("templates/admincompetitedit.htmlt");
    }
    if(act == "competiteditsubmit")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "update `competitions` set `name`=\"%s\", `content`=\"%s\", `isshow`=\"%s\" where `id`='%s'", removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("isshow")).c_str(), indexPage.GetVarsHandler()->Get("id").c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "Фотоконкурс обновлен");
    }
    if(act == "delnewthings")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "delete from competitions where `id`=\"%s\"", removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "Фотокоркурс удален.");
    } 

// ----------- Parts reserved -----------------------
    if(act == "edit_parts_reserved")
    {
	ostringstream	ost;
	string		content, title, breadcreams, content_brief, title_head, description_head, keywords_head, id, image, menu_link;

	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	ost << "select * from parts_" << act.substr(11) << " where id=" << id;
	affected = db.Query(ost.str());
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting parts from db");
		throw CException("error getting parts from db");
	}
	ost.str("");

	content = db.Get(0, "content");
	title = db.Get(0, "title");
	id = db.Get(0, "id");

	indexPage.SetTemplateFile("templates/adminpartseditreserved.htmlt");

	indexPage.RegisterVariableForce("parts_type", act.substr(11));
	indexPage.RegisterVariableForce("content", content);
	indexPage.RegisterVariableForce("title", title);
	indexPage.RegisterVariableForce("id", id);
    }
    if(act == "submit_parts_reserved")
    {
	ostringstream	ost;

	ost << "update `parts_" << act.substr(13) <<"` set \
	`title`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("title")).c_str() << "\", \
	`content`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str() << "\" \
	where `id`='" << removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str() << "'";
	db.Query(ost.str());
	content = " Редактирование раздела прошло успешно.";
	indexPage.RegisterVariableForce("content", content);
	
//	Makehtaccess(&db);
    }


// ----------- Parts editing (need modify) ------------------
    if(act == "submit_parts_main")
    {
	ostringstream	ost;

	ost << "update `parts_" << act.substr(13) <<"` set `title_head`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("title_head")).c_str() << "\", `description_head`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("description_head")).c_str() << "\", `keywords_head`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("keywords_head")).c_str() << "\", `title`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("title")).c_str() << "\", `content`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str() << "\", `content_brief`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("content_brief")).c_str() << "\", `image`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("image")).c_str() << "\",  `breadcreams`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("breadcreams")).c_str() << "\",  `menu_link`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("menu_link")).c_str() << "\"  where `id`='" << removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str() << "'";
	db.Query(ost.str());
	content = " Редактирование раздела прошло успешно.";
	indexPage.RegisterVariableForce("content", content);
	
//	Makehtaccess(&db);
    }
    if(act == "submitadd_parts_main")
    {
    	ostringstream	ost;

	ost << "insert into `parts_" << act.substr(16) <<"` (`title_head`, `description_head`, `keywords_head`, `title`, `content`, `content_brief`, `breadcreams`,`image`, `menu_link`) VALUES ('" << removeQuotas(indexPage.GetVarsHandler()->Get("title_head")).c_str() << "', '" << removeQuotas(indexPage.GetVarsHandler()->Get("description_head")).c_str() << "', '" << removeQuotas(indexPage.GetVarsHandler()->Get("keywords_head")).c_str() << "', '" << removeQuotas(indexPage.GetVarsHandler()->Get("title")).c_str() << "', '" <<
	removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str() << "', '" << removeQuotas(indexPage.GetVarsHandler()->Get("content_brief")).c_str() << "',  '" << removeQuotas(indexPage.GetVarsHandler()->Get("breadcreams")).c_str() << "',  '" <<
	removeQuotas(indexPage.GetVarsHandler()->Get("image")).c_str() << "',  '" <<
	removeQuotas(indexPage.GetVarsHandler()->Get("menu_link")).c_str() << "')";
	db.Query(ost.str());
	content = " Добавление раздела прошло успешно.";
	indexPage.RegisterVariableForce("content", content);

//	Makehtaccess(&db);
    }
    if((act.substr(0, 10))  == "del_parts_")
    {
	ostringstream	ost;
	
	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	ost << "delete from parts_" << act.substr(10) << " where id=" << id;
	affected = db.Query(ost.str());
	content = "Удаление прошло успешно";
	indexPage.RegisterVariable("content", content);

//	Makehtaccess(&db);
    }
    if((act.substr(0, 11)) == "list_parts_")
    {
	ostringstream	ost;

	ost << "select id,title from parts_" << act.substr(11) << " order by id asc;";
	affected = db.Query(ost.str());

	content = "<table>\n";
	for(int i = 0; i < affected; i++)
	{
	    content += "<tr>";
	    content += "<td class=blue>\n";

	    content += db.Get(i, "id");

	    content += "</td><td>";

	    if((act.substr(11) != "reserved") && (act.substr(11) != "top"))
	    {
		    content += "<a href=/cgi-bin/admin/parts.cgi?act=del_parts_";
		    content += act.substr(11);
		    content += "&id=";
        	    content += db.Get(i, "id");
		    content += "&rnd=";
		    content += GetRandom(10);
		    content += ">";
		    content += "<img src=/images/button_drop.png border=0></a>";
	    }
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=edit_parts_";
	    content += act.substr(11);
	    content += "&id=";
            content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += ">";
	    content += " <img src=/images/button_edit.png border=0> ";
	    content += DeleteHTML(db.Get(i, "title"));
	    content += "</a>";


	    content += "</td>";
	    content += "</tr>\n";
	}
	content += "</table>\n";
        indexPage.RegisterVariableForce("content", content.c_str());
    }
    if(act == "edit_parts_main")
    {
	ostringstream	ost;
	string		content, title, breadcreams, content_brief, title_head, description_head, keywords_head, id, image, menu_link;

	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	ost << "select * from parts_" << act.substr(11) << " where id=" << id;
	affected = db.Query(ost.str());
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting parts from db");
		throw CException("error getting parts from db");
	}
	ost.str("");

	content = db.Get(0, "content");
	content_brief = db.Get(0, "content_brief");
	breadcreams = db.Get(0, "breadcreams");
	title = db.Get(0, "title");
	id = db.Get(0, "id");
	title_head = db.Get(0, "title_head");
	description_head = db.Get(0, "description_head");
	keywords_head = db.Get(0, "keywords_head");
	image = db.Get(0, "image");
	menu_link = db.Get(0, "menu_link");

	if(act.substr(11) == "reserved")
	{
		indexPage.SetTemplateFile("templates/adminpartseditreserved.htmlt");
	}
	else
	{
		indexPage.SetTemplateFile("templates/adminpartsedit.htmlt");
	}

	indexPage.RegisterVariableForce("parts_type", act.substr(11));
	indexPage.RegisterVariableForce("content", content);
	indexPage.RegisterVariableForce("title", title);
	indexPage.RegisterVariableForce("id", id);
	indexPage.RegisterVariableForce("title_head", title_head);
	indexPage.RegisterVariableForce("description_head", description_head);
	indexPage.RegisterVariableForce("keywords_head", keywords_head);
	indexPage.RegisterVariableForce("content_brief", content_brief);
	indexPage.RegisterVariableForce("breadcreams", breadcreams);

	ost.str("");
	ost << "<select name=image> <option value=\"index.htmlt\">Шаблон 1<option value=\"index1.htmlt\" " << ((image == "index1.htmlt") ? "selected" : "") << ">Шаблон2</select>";
	indexPage.RegisterVariable("image", ost.str());

	ost.str("");
	ost << "select * from parts_picture where category=\"" << id << "\" ORDER BY `date` DESC";
	affected = db.Query(ost.str());
	ost.str("");
	for(int i = 0; i < affected; i++)
	{
		ost << "<tr><td>" << db.Get(i, "image") << "</td><td colspan=2><a href='/cgi-bin/admin/parts.cgi?act=edit_partspicture_item&p=" << db.Get(i, "id") << "&rand=" << GetRandom(10) << "'>" << db.Get(i, "title") << "</a> ";
		if(indexPage.GetVarsHandler()->Get("isDate") == "yes")
		{
			ost << " (" << db.Get(i, "date") << ")";
		}
		ost << "&nbsp;\n\
		<a href='/cgi-bin/admin/parts.cgi?act=delete_partspicture_item&p=" << db.Get(i, "id") << "&rand=" << GetRandom(10) << "'><img src=\"/images/button_drop.png\" border=0></a>\
		<br>" << db.Get(i, "brief") << "</td></tr>";
	}
	indexPage.RegisterVariableForce("imageContent", ost.str());

	ost.str("");
	affected = db.Query("select * from menu ORDER BY `content`");
	ost << "<select name=menu_link><option value=\"\"> none";
	for(int i = 0; i < affected; i++)
	{
		ost << "<option value=\"" << db.Get(i, "id") << "\"" << ((menu_link == db.Get(i, "id")) ? " selected " : "") << ">" << db.Get(i, "content");
	}
	ost << "</select>";
	indexPage.RegisterVariableForce("menu_link", ost.str());
	ost.str("");
	ost << "/cgi-bin/admin/parts.cgi?act=add_partspicture_item&id=" << id << "&rand=" << GetRandom(10);
	indexPage.RegisterVariableForce("href_add_item", ost.str());
//	indexPage.SetTemplateFile("templates/adminpartspictureedit.htmlt");
    }
    if(act == "add_parts_main")
    {
	ostringstream	ost;

	indexPage.RegisterVariable("parts_type", act.substr(10));

	ost.str("");
	ost << "<select name=image> <option value=\"index.htmlt\">Шаблон 1<option value=\"index1.htmlt\">Щаблон2</select>";
	indexPage.RegisterVariable("image", ost.str());

	ost.str("");
	affected = db.Query("select * from menu ORDER BY `content`");
	ost << "<select name=menu_link>";
	for(int i = 0; i < affected; i++)
	{
		ost << "<option value=\"" << db.Get(i, "id") << "\">" << db.Get(i, "content");
	}
	ost << "</select>";
	indexPage.RegisterVariableForce("menu_link", ost.str());

	indexPage.SetTemplateFile("templates/adminpartsadd.htmlt");
    }
// ------------------ User Administrating (need modify) --------------
    if((act == "list_users"))
    {
	ostringstream	ost;
	string		isActivated, isBlocked;

	ost << "select * from users order by id;";
	affected = db.Query(ost.str());

	content = "<table>\n";
	for(int i = 0; i < affected; i++)
	{
	    content += "<tr>";
	    content += "<td class=blue>\n";

	    content += db.Get(i, "id");

	    content += "</td><td>";

	    content += "<a href=/cgi-bin/admin/parts.cgi?act=del_user&id=";
            content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += "><img src=/images/button_drop.png border=0></a> ";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=edit_user&id=";
            content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += ">";
	    content += " <img src=/images/button_edit.png border=0> ";
	    content += DeleteHTML(db.Get(i, "name"));
	    content += "</a></td><td>";
	    content += DeleteHTML(db.Get(i, "phone"));
	    content += "</td><td>";
	    content += DeleteHTML(db.Get(i, "email"));
	    content += "</td>";
	    content += "</tr>\n";
	}
	content += "</table>\n";
        indexPage.RegisterVariableForce("content", content.c_str());
    }
    if(act == "edit_user")
    {
	ostringstream	ost, images;
	string		login, passwd, email, isActivated, name, nameLast, address, description, phone, isBlocked;

	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	ost << "select * from users where id=" << id;
	affected = db.Query(ost.str());
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting user from db");
		throw CException("error getting user from db");
	}
	ost.str("");

	login = db.Get(0, "login");
	passwd = db.Get(0, "passwd");
	email = db.Get(0, "email");
	isActivated = db.Get(0, "isactivated");
	isBlocked = db.Get(0, "isblocked");
	name = db.Get(0, "name");
	nameLast = db.Get(0, "nameLast");
	description = db.Get(0, "description");
	phone = db.Get(0, "phone");
	address = db.Get(0, "address");

	indexPage.RegisterVariableForce("login", login);
	indexPage.RegisterVariableForce("passwd", passwd);
	indexPage.RegisterVariableForce("email", email);
	indexPage.RegisterVariableForce("isActivated", ((isActivated == "Y") ? "Активирован" : "Не активирован"));
	indexPage.RegisterVariableForce("name", name);
	indexPage.RegisterVariableForce("nameLast", nameLast);
	indexPage.RegisterVariableForce("description", description);
	indexPage.RegisterVariableForce("phone", phone);
	indexPage.RegisterVariableForce("address", address);

	ost.str("");
	ost << "<select name=isBlocked><option value='N'>Активен<option value='Y' " << ((isBlocked == "Y") ? "selected" : "") << ">Заблокирован</select>";
	indexPage.RegisterVariableForce("admBlock", ost.str());

        indexPage.SetTemplateFile("templates/adminuseredit.htmlt");
    }
    if(act == "usereditsubmit")
    {
    	ostringstream	ost;

	ost << "update `users` set \
`login`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("login")).c_str() << "\", \
`passwd`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("passwd")).c_str() << "\", \
`email`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("email")).c_str() << "\", \
`name`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "\", \
`phone`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("phone")).c_str() << "\", \
`description`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("description")).c_str() << "\",  \
`isblocked`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("isBlocked")).c_str() << "\"  \
where `id`='" << removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str() << "'";
	db.Query(ost.str());
	content = " Пользователь изменен.";
	indexPage.RegisterVariableForce("content", content);
	
    }
    if(act == "useraddsubmit")
    {
    	ostringstream	ost;

	ost << "insert into `users` (`login`, `passwd`, `email`, `name`, `phone`, `description`) values ( \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("login")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("passwd")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("email")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("phone")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("description")).c_str() << "\")";
	db.Query(ost.str());
	content = " Пользователь добавлен.";
	indexPage.RegisterVariableForce("content", content);
	
    }
    if(act == "admin_message")
    {
        CMailLocal      	mail;
        string			message;

	message = indexPage.GetVarsHandler()->Get("message");
	message += "\n\n";
	message += "С уважением, администратор сайта Photomag.ru";
	indexPage.RegisterVariableForce("message", message);
        mail.Send(indexPage.GetVarsHandler()->Get("login"), "admin_message", indexPage.GetVarsHandler(), &db);

	indexPage.RegisterVariableForce("content", "Сообщение послано");
    }
    if(act == "del_user")
    {
	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "delete from `users` where id=%s", id.c_str());
	affected = db.Query(query);
	indexPage.RegisterVariableForce("content", "Салон удален.");
    }
    if(act == "add_user")
    {
	indexPage.SetTemplateFile("templates/adminuseradd.htmlt");
    }
// ------------------ Catalog Administrating
    if(act == "list_catalog")
    {
	Catalog				m;
	Manufacturer			*tmpMI1, *tmpMI2, *tmpMI3, *tmpMI4, *tmpMI5;
	ostringstream			ost;
	string				act1;

	m.SetDB(&db);
	m.Load();

	ost << "<a href=\"/cgi-bin/admin/parts.cgi?act=add_manufacturer&parentid=0&rnd=" << GetRandom(10) << "\">Добавить</a>";
	ost << "<ul>";
	tmpMI1 = m.GetFirstItem(0);
	while(tmpMI1 != NULL)
	{
		ost << "<li>";
		ost << "<a href=" << getenv("SCRIPT_NAME") << "?act=edit_manufacturer" << act1 << "&id=" << tmpMI1->GetID() << "&rnd=" << GetRandom(10) << ">" << tmpMI1->GetOrder() << " " << tmpMI1->GetName() << "<img src=/images/button_edit.png border=0></a>&nbsp;<a href=" << getenv("SCRIPT_NAME") << "?act=del_manufacturer&id=" << tmpMI1->GetID() << "&rnd=" << GetRandom(10) << "><img src=/images/button_drop.png border=0></a>" << "&nbsp;<a href=" << getenv("SCRIPT_NAME") << "?act=add_manufacturer&parentid=" << tmpMI1->GetID() << "&rnd=" << GetRandom(10) << ">Добавить элемент</a>";

		ost << "<ul>";
		tmpMI2 = m.GetFirstItem(tmpMI1->GetID());
		while(tmpMI2 != NULL)
		{
			ost << "<li>";
			ost << "<a href=" << getenv("SCRIPT_NAME") << "?act=edit_manufacturer" << act1 << "&id=" << tmpMI2->GetID() << "&rnd=" << GetRandom(10) << ">" << tmpMI2->GetOrder() << " " << tmpMI2->GetName() << "<img src=/images/button_edit.png border=0></a>&nbsp;<a href=" << getenv("SCRIPT_NAME") << "?act=del_manufacturer&id=" << tmpMI2->GetID() << "&rnd=" << GetRandom(10) << "><img src=/images/button_drop.png border=0></a>" << "&nbsp;<a href=" << getenv("SCRIPT_NAME") << "?act=add_manufacturer&parentid=" << tmpMI2->GetID() << "&rnd=" << GetRandom(10) << ">Добавить элемент</a>";
			
			ost << "<ul>";
			tmpMI3 = m.GetFirstItem(tmpMI2->GetID());
			while(tmpMI3 != NULL)
			{
				ost << "<li>";
				ost << "<a href=" << getenv("SCRIPT_NAME") << "?act=edit_manufacturer" << act1 << "&id=" << tmpMI3->GetID() << "&rnd=" << GetRandom(10) << ">" << tmpMI3->GetOrder() << " " << tmpMI3->GetName() << "<img src=/images/button_edit.png border=0></a>&nbsp;<a href=" << getenv("SCRIPT_NAME") << "?act=del_manufacturer&id=" << tmpMI3->GetID() << "&rnd=" << GetRandom(10) << "><img src=/images/button_drop.png border=0></a>" << "&nbsp;<a href=" << getenv("SCRIPT_NAME") << "?act=add_manufacturer&parentid=" << tmpMI3->GetID() << "&rnd=" << GetRandom(10) << ">Добавить элемент</a>";

				ost << "<ul>";
				tmpMI4 = m.GetFirstItem(tmpMI3->GetID());
				while(tmpMI4 != NULL)
				{
					ost << "<li>";
					ost << "<a href=" << getenv("SCRIPT_NAME") << "?act=edit_manufacturer" << act1 << "&id=" << tmpMI4->GetID() << "&rnd=" << GetRandom(10) << ">" << tmpMI4->GetOrder() << " " << tmpMI4->GetName() << "<img src=/images/button_edit.png border=0></a>&nbsp;<a href=" << getenv("SCRIPT_NAME") << "?act=del_manufacturer&id=" << tmpMI4->GetID() << "&rnd=" << GetRandom(10) << "><img src=/images/button_drop.png border=0></a>" << "&nbsp;<a href=" << getenv("SCRIPT_NAME") << "?act=add_manufacturer&parentid=" << tmpMI4->GetID() << "&rnd=" << GetRandom(10) << ">Добавить элемент</a>";

					ost << "<ul>";
					tmpMI5 = m.GetFirstItem(tmpMI4->GetID());
					while(tmpMI5 != NULL)
					{
						ost << "<li>";
						ost << "<a href=" << getenv("SCRIPT_NAME") << "?act=edit_manufacturer" << act1 << "&id=" << tmpMI5->GetID() << "&rnd=" << GetRandom(10) << ">" << tmpMI5->GetOrder() << " " << tmpMI5->GetName() << "<img src=/images/button_edit.png border=0></a>&nbsp;<a href=" << getenv("SCRIPT_NAME") << "?act=del_manufacturer&id=" << tmpMI5->GetID() << "&rnd=" << GetRandom(10) << "><img src=/images/button_drop.png border=0></a>";

						ost << "</li>";
						tmpMI5 = m.GetNextSibling(tmpMI5->GetOrder());
					}
					ost << "</ul>";

					ost << "</li>";
					tmpMI4 = m.GetNextSibling(tmpMI4->GetOrder());
				}
				ost << "</ul>";

				ost << "</li>";
				tmpMI3 = m.GetNextSibling(tmpMI3->GetOrder());
			}
			ost << "</ul>";

			ost << "</li>";
			tmpMI2 = m.GetNextSibling(tmpMI2->GetOrder());
		}
		ost << "</ul>";

		ost << "</li>";
		tmpMI1 = m.GetNextSibling(tmpMI1->GetOrder());
	}
	ost << "</ul>";

	indexPage.RegisterVariableForce("content", ost.str());
    }

	if(act == "edit_manufacturer")
	{
		string			content, name, parentID, order, id, file, file2, rambler, isShow, isShowStr, style;
		ostringstream		ost, ost1;
	
		id = indexPage.GetVarsHandler()->Get("id");
		if(id.length() == 0)
		{
		throw CException("parameter ID was missing");
		}
		
		memset(query, 0, sizeof(query));
		snprintf(query, sizeof(query), "select * from manufacture where `id`=%s", id.c_str());
		affected = db.Query(query);
		if(affected == 0)
		{
			CLog	log;
			
			log.Write(ERROR, "error getting menu item from db");
			throw CException("error getting menu item from db");
		}
	
		file = db.Get(0, "file");
		file2 = db.Get(0, "file2");
		content = db.Get(0, "content");
		order = db.Get(0, "order");
		parentID = db.Get(0, "parentID");
		name = db.Get(0, "name");
		id = db.Get(0, "id");
		rambler = db.Get(0, "rambler");
		isShow = db.Get(0, "isShow");
		style = db.Get(0, "style");
	
		isShowStr = "<select name=isshow><option value='N'>нет</option><option value='Y' ";
		if(isShow == "Y") isShowStr += " selected ";
		isShowStr += ">да</option>";


		indexPage.SetTemplateFile("templates/adminmanufactureedit.htmlt");
	
		indexPage.RegisterVariable("parentID", parentID);
		indexPage.RegisterVariable("file", file);
		indexPage.RegisterVariable("file2", file2);
		indexPage.RegisterVariable("rambler", rambler);
		indexPage.RegisterVariable("content", content);
		indexPage.RegisterVariable("name", name);
		indexPage.RegisterVariable("order", order);
		indexPage.RegisterVariable("isShow", isShowStr);
		indexPage.RegisterVariable("articul", db.Get(0, "articul"));
		indexPage.RegisterVariable("image1", db.Get(0, "image1"));
		indexPage.RegisterVariable("image2", db.Get(0, "image2"));
		indexPage.RegisterVariable("title_head", db.Get(0, "title_head"));
		indexPage.RegisterVariable("keywords_head", db.Get(0, "keywords_head"));
		indexPage.RegisterVariable("description_head", db.Get(0, "description_head"));
		indexPage.RegisterVariable("id", id);
		indexPage.RegisterVariable("style", style);

		memset(query, 0, sizeof(query));
		snprintf(query, sizeof(query), "select * from `goods` where `parentID`=%s ORDER BY `order` ASC", id.c_str());
		affected = db.Query(query);
		ost1 << "<table>";
		for(int i = 0; i < affected; i++)
		{
			ost1 << "<tr><td>"<< db.Get(i, "order") << " </td><td><a href=" << getenv("SCRIPT_NAME") << "?act=edit_good&id=" << db.Get(i, "id") << "&" << GetRandom(10) << ">" << db.Get(i, "name") << "</a></td><td>" << db.Get(i, "price") << "</td><td><a href=" << getenv("SCRIPT_NAME") << "?act=edit_good&id=" << db.Get(i, "id") << "&" << GetRandom(10) << "><img src=/images/button_edit.png border=0></a> <a href=" << getenv("SCRIPT_NAME") << "?act=del_good&id=" << db.Get(i, "id") << "&" << GetRandom(10) << "><img src=/images/button_drop.png border=0></a>" << "</td></tr>";
		}
		ost1 << "</table>";
		indexPage.RegisterVariableForce("goods", ost1.str());

		ost.str("");
		ost << "/cgi-bin/admin/parts.cgi?act=add_good&parentID=" << id << "&rand=" << GetRandom(10);
		indexPage.RegisterVariableForce("href_add_good", ost.str());
	}
	if(act == "submit_manufacture_edit")
	{
		ostringstream	ost;

		ost << "update `manufacture` set \
`parentID`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("parentID")).c_str() << "\", \
`content`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str() << "\", \
`articul`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("articul")).c_str() << "\", \
`name`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "\", \
`file`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("file")).c_str() << "\", \
`file2`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("file2")).c_str() << "\", \
`isShow`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("isshow")).c_str() << "\", \
`title_head`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("title_head")).c_str() << "\", \
`keywords_head`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("keywords_head")).c_str() << "\", \
`description_head`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("description_head")).c_str() << "\", \
`rambler`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("rambler")).c_str() << "\", \
`style`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("style")).c_str() << "\", \
`image1`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("image1")).c_str() << "\", \
`image2`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("image2")).c_str() << "\", \
`order`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("order")).c_str() << "\"\
where `id`='" << removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str() << "'";
		db.Query(ost.str());
		content = "Изменено.";
		indexPage.RegisterVariableForce("content", content);
	}
	if(act == "add_manufacturer")
	{
		indexPage.SetTemplateFile("templates/adminmanufactureadd.htmlt");
	}
	if(act == "submit_manufacture_add")
	{
		ostringstream	ost;

		ost << "insert into `manufacture` (`parentID`,`articul`,`content`,`name`,`file`,`file2`,`order`,`style`,`image1`,`image2`,`isShow`) VALUES (\
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("parentID")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("articul")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("file")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("file2")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("order")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("style")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("image1")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("image2")).c_str() << "\", \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("isshow")).c_str() << "\")";
	db.Query(ost.str());
	content = "Добавлено.";
	indexPage.RegisterVariableForce("content", content);
	}
	if(act == "del_manufacturer")
	{
		ostringstream	ost;

		ost << "delete from `manufacture` where `id`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str() << "\" or `parentID`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str() << "\"";
		db.Query(ost.str());
		ost.str("");
		ost << "delete from `goods` where `parentID`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str() << "\"";
		db.Query(ost.str());
		content = "Добавлено.";
		indexPage.RegisterVariableForce("content", content);
	}

	if(act == "add_good")
	{
		indexPage.SetTemplateFile("templates/admingoodadd.htmlt");
	}
	if(act == "edit_good")
	{
		string			id, parentID, order, name, articul, price, content, isNew, isTop, isAvailable, isShow, image1, image2, image3, image4, image5, image6, image7, image8;
		ostringstream		ost;
	
		id = indexPage.GetVarsHandler()->Get("id");
		if(id.length() == 0)
		{
			throw CException("deit_good: parameter id was missing");
		}
		
		memset(query, 0, sizeof(query));
		snprintf(query, sizeof(query), "select * from goods where `id`=%s", id.c_str());
		affected = db.Query(query);
		if(affected == 0)
		{
			CLog	log;
			
			log.Write(ERROR, "edit_good: error getting menu item from db");
			throw CException("error getting menu item from db");
		}
	
		id = db.Get(0, "id");
		parentID = db.Get(0, "parentID");
		order = db.Get(0, "order");
		name = db.Get(0, "name");
		articul = db.Get(0, "articul");
		price = db.Get(0, "price");
		content = db.Get(0, "content");
		isNew = db.Get(0, "isNew");
		isTop = db.Get(0, "isTop");
		isAvailable = db.Get(0, "isAvailable");
		isShow = db.Get(0, "isShow");
		image1 = db.Get(0, "image1");
		image2 = db.Get(0, "image2");
		image3 = db.Get(0, "image3");
		image4 = db.Get(0, "image4");
		image5 = db.Get(0, "image5");
		image6 = db.Get(0, "image6");
		image7 = db.Get(0, "image7");
		image8 = db.Get(0, "image8");

		indexPage.SetTemplateFile("templates/admingoodsedit.htmlt");
	
		indexPage.RegisterVariableForce("id", id);
		indexPage.RegisterVariableForce("parentID", parentID);
		indexPage.RegisterVariableForce("order", order);
		indexPage.RegisterVariableForce("name", name);
		indexPage.RegisterVariableForce("articul", articul);
		indexPage.RegisterVariableForce("price", price);
		indexPage.RegisterVariableForce("content", content);

		ost.str("");
		ost << "<select name=\"isNew\"><option value='N'>Нет</option><option value='Y' " << ((isNew == "Y") ? "selected" : "") << ">Да</option></select>";
		indexPage.RegisterVariableForce("isNew", ost.str());
		ost.str("");
		ost << "<select name=\"isTop\"><option value='N'>Нет</option><option value='Y' " << ((isTop == "Y") ? "selected" : "") << ">Да</option></select>";
		indexPage.RegisterVariableForce("isTop", ost.str());
		ost.str("");
		ost << "<select name=\"isAvailable\"><option value='N'>Нет</option><option value='Y' " << ((isAvailable == "Y") ? "selected" : "") << ">Да</option></select>";
		indexPage.RegisterVariableForce("isAvailable", ost.str());
		ost.str("");
		ost << "<select name=\"isShow\"><option value='N'>Нет</option><option value='Y' " << ((isShow == "Y") ? "selected" : "") << ">Да</option></select>";
		indexPage.RegisterVariableForce("isShow", ost.str());

		indexPage.RegisterVariableForce("image1", image1);
		if(image1.length() > 0) indexPage.RegisterVariableForce("image1Pic", image1); else indexPage.RegisterVariableForce("image1Pic", "1.gif");
		indexPage.RegisterVariableForce("image2", image2);
		if(image2.length() > 0) indexPage.RegisterVariableForce("image2Pic", image2); else indexPage.RegisterVariableForce("image2Pic", "1.gif");
		indexPage.RegisterVariableForce("image3", image3);
		if(image3.length() > 0) indexPage.RegisterVariableForce("image3Pic", image3); else indexPage.RegisterVariableForce("image3Pic", "1.gif");
		indexPage.RegisterVariableForce("image4", image4);
		if(image4.length() > 0) indexPage.RegisterVariableForce("image4Pic", image4); else indexPage.RegisterVariableForce("image4Pic", "1.gif");
		indexPage.RegisterVariableForce("image5", image5);
		if(image5.length() > 0) indexPage.RegisterVariableForce("image5Pic", image5); else indexPage.RegisterVariableForce("image5Pic", "1.gif");
		indexPage.RegisterVariableForce("image6", image6);
		if(image6.length() > 0) indexPage.RegisterVariableForce("image6Pic", image5); else indexPage.RegisterVariableForce("image6Pic", "1.gif");
		indexPage.RegisterVariableForce("image7", image7);
		if(image7.length() > 0) indexPage.RegisterVariableForce("image7Pic", image5); else indexPage.RegisterVariableForce("image7Pic", "1.gif");
		indexPage.RegisterVariableForce("image8", image8);
		if(image8.length() > 0) indexPage.RegisterVariableForce("image8Pic", image5); else indexPage.RegisterVariableForce("image8Pic", "1.gif");

		// ----------- good recomend inside
		ost.str("");
		ost << "/cgi-bin/admin/parts.cgi?act=add_good_recomend&id=" << id << "&rand=" << GetRandom(10);
		indexPage.RegisterVariableForce("add_good_recomend", ost.str());

		ost.str("");
		ost << "select goods_recomend.id as gri, goods.id as gi, goods.name as gn, goods.articul as ga  from `goods_recomend`,`goods` where goods.id=goods_recomend.good_recomend and goods_recomend.good_id='" << id << "'";
		affected = db.Query(ost.str());
		ost.str("");
		ost << "<table>";
		for(int i = 0; i < affected; i++)
		{
			ost << "<tr><td><a href=/cgi-bin/admin/parts.cgi?act=edit_good_recomend&id=" << db.Get(i, "gri") << "&rnd=" << GetRandom(10) << "> " << db.Get(i, "gn") << "(" << db.Get(i, "ga") << ") <img src=/images/button_edit.png border=0></a><a href=/cgi-bin/admin/parts.cgi?act=delete_good_recomend&id=" << db.Get(i, "gri") << "&rnd=" << GetRandom(10) << "> <img src=/images/button_drop.png border=0></a>" << "</td></tr>\n";

		}
		ost << "</table>";
		indexPage.RegisterVariableForce("good_recomend", ost.str());
	}
	if(act == "submit_goods_edit")
	{
		ostringstream	ost;

		ost << "update `goods` set \
`parentID`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("parentID")).c_str() << "\", \
`order`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("order")).c_str() << "\", \
`name`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "\", \
`articul`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("articul")).c_str() << "\", \
`price`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("price")).c_str() << "\", \
`content`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str() << "\", \
`isNew`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("isNew")).c_str() << "\", \
`isTop`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("isTop")).c_str() << "\", \
`isAvailable`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("isAvailable")).c_str() << "\", \
`isShow`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("isShow")).c_str() << "\", \
`image1`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("image1")).c_str() << "\", \
`image2`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("image2")).c_str() << "\", \
`image3`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("image3")).c_str() << "\", \
`image4`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("image4")).c_str() << "\", \
`image5`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("image5")).c_str() << "\", \
`image6`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("image6")).c_str() << "\", \
`image7`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("image7")).c_str() << "\", \
`image8`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("image8")).c_str() << "\" \
where `id`='" << removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str() << "'";
		db.Query(ost.str());
		content = "Товар изменен.";
		indexPage.RegisterVariableForce("content", content);
	}
	if(act == "del_good")
	{
		string		id;
		ostringstream	ost;

		id = indexPage.GetVarsHandler()->Get("id");
		ost.str("");
		ost << "delete from goods where `id`='" << id << "'";
		db.Query(ost.str());
		ost.str("");
		ost << "delete from goods_recomend where `good_id`='" << id << "'";
		db.Query(ost.str());
		ost.str("");
		ost << "delete from goods_recomend where `good_recomend`='" << id << "'";
		db.Query(ost.str());
		indexPage.RegisterVariableForce("content", "Товар удален!");
	}
	if(act == "submit_goods_add")
	{
		string          fileName1 = indexPage.GetFilesHandler()->GetName(0);
		string          fileName2 = indexPage.GetFilesHandler()->GetName(1);
		string          fileName3 = indexPage.GetFilesHandler()->GetName(2);
		string          fileName4 = indexPage.GetFilesHandler()->GetName(3);
		string          fileName5 = indexPage.GetFilesHandler()->GetName(4);
		string          fileName6 = indexPage.GetFilesHandler()->GetName(5);
		string          fileName7 = indexPage.GetFilesHandler()->GetName(6);
		string          fileName8 = indexPage.GetFilesHandler()->GetName(7);
                string          fileNameLocal;
                FILE            *f;
                ostringstream   ost, ost1;
                string          id;


                if(!fileName8.empty())
                {
                    fileNameLocal = IMAGE_PHOTO_DIRECTORY + fileName8;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(7) != NULL) && (indexPage.GetFilesHandler()->GetSize(7) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(7), indexPage.GetFilesHandler()->GetSize(7), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName7.empty())
                {
                    fileNameLocal = IMAGE_PHOTO_DIRECTORY + fileName7;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(6) != NULL) && (indexPage.GetFilesHandler()->GetSize(6) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(6), indexPage.GetFilesHandler()->GetSize(6), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName6.empty())
                {
                    fileNameLocal = IMAGE_PHOTO_DIRECTORY + fileName6;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(5) != NULL) && (indexPage.GetFilesHandler()->GetSize(5) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(5), indexPage.GetFilesHandler()->GetSize(5), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName5.empty())
                {
                    fileNameLocal = IMAGE_PHOTO_DIRECTORY + fileName5;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(4) != NULL) && (indexPage.GetFilesHandler()->GetSize(4) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(4), indexPage.GetFilesHandler()->GetSize(4), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName4.empty())
                {
                    fileNameLocal = IMAGE_PHOTO_DIRECTORY + fileName4;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(3) != NULL) && (indexPage.GetFilesHandler()->GetSize(3) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(3), indexPage.GetFilesHandler()->GetSize(3), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName3.empty())
                {
                    fileNameLocal = IMAGE_PHOTO_DIRECTORY + fileName3;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(2) != NULL) && (indexPage.GetFilesHandler()->GetSize(2) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(2), indexPage.GetFilesHandler()->GetSize(2), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName2.empty())
                {
                    fileNameLocal = IMAGE_PHOTO_DIRECTORY + fileName2;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(1) != NULL) && (indexPage.GetFilesHandler()->GetSize(1) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(1), indexPage.GetFilesHandler()->GetSize(1), 1, f);
                    }
                    fclose(f);
                }
		
                if(!fileName1.empty())
                {
                    fileNameLocal = IMAGE_PHOTO_DIRECTORY + fileName1;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(0) != NULL) && (indexPage.GetFilesHandler()->GetSize(0) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(0), indexPage.GetFilesHandler()->GetSize(0), 1, f);
                    }
                    fclose(f);
                }
		
		ost << "insert into `goods` (`parentID`,`order`,`name`,`articul`,`price`,`content`,`isNew`,`isTop`,`isAvailable`,`isShow`";
		if(!fileName1.empty()) ost << ",`image1`";
		if(!fileName2.empty()) ost << ",`image2`";
		if(!fileName3.empty()) ost << ",`image3`";
		if(!fileName4.empty()) ost << ",`image4`";
		if(!fileName5.empty()) ost << ",`image5`";
		if(!fileName6.empty()) ost << ",`image6`";
		if(!fileName7.empty()) ost << ",`image7`";
		if(!fileName8.empty()) ost << ",`image8`";
		ost << ") values ( \
\"" << removeQuotas(indexPage.GetVarsHandler()->Get("parentID")).c_str() << "\" \
,\"" << removeQuotas(indexPage.GetVarsHandler()->Get("order")).c_str() << "\" \
,\"" << removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "\" \
,\"" << removeQuotas(indexPage.GetVarsHandler()->Get("articul")).c_str() << "\" \
,\"" << removeQuotas(indexPage.GetVarsHandler()->Get("price")).c_str() << "\" \
,\"" << removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str() << "\" \
,\"" << removeQuotas(indexPage.GetVarsHandler()->Get("isNew")).c_str() << "\" \
,\"" << removeQuotas(indexPage.GetVarsHandler()->Get("isTop")).c_str() << "\" \
,\"" << removeQuotas(indexPage.GetVarsHandler()->Get("isAvailable")).c_str() << "\" \
,\"" << removeQuotas(indexPage.GetVarsHandler()->Get("isShow")).c_str() << "\" ";
		if(!fileName1.empty()) ost << ",\"" << fileName1 << "\"";
		if(!fileName2.empty()) ost << ",\"" << fileName2 << "\"";
		if(!fileName3.empty()) ost << ",\"" << fileName3 << "\"";
		if(!fileName4.empty()) ost << ",\"" << fileName4 << "\"";
		if(!fileName5.empty()) ost << ",\"" << fileName5 << "\"";
		if(!fileName6.empty()) ost << ",\"" << fileName6 << "\"";
		if(!fileName7.empty()) ost << ",\"" << fileName7 << "\"";
		if(!fileName8.empty()) ost << ",\"" << fileName8 << "\"";
		ost << ")";
		db.Query(ost.str());
		content = "Товар добавлен.";
		indexPage.RegisterVariableForce("content", content);
	}


	if(act == "add_good_recomend")
	{
		ostringstream		ost;
	
		affected = db.Query("select * from goods");
		if(affected > 0)
		{
			ost << "<select name=recomend>";
			for(int i = 0; i < affected; i++)
			{
				ost << "<option value='" << db.Get(i, "id") << "'>" << db.Get(i, "name") << " (" << db.Get(i, "articul") << ")</option>";
			}
			ost << "</select>";
			indexPage.RegisterVariableForce("good_recomend_choose", ost.str());
		}
		indexPage.SetTemplateFile("templates/good_recomend_add.htmlt");
	}
	if(act == "submit_good_recomend_add")
	{
		string		recomend, id;
		ostringstream	ost;
	
		id = indexPage.GetVarsHandler()->Get("id");
		recomend = indexPage.GetVarsHandler()->Get("recomend");

		ost << "insert into `goods_recomend` (`good_id`, `good_recomend`) values('" << removeQuotas(id) << "', '" << removeQuotas(recomend) << "')";
		db.Query(ost.str());
		indexPage.RegisterVariableForce("content", "Рекомендуемые товары добавлены");
	}
	if(act == "edit_good_recomend")
	{
		ostringstream		ost;
		string			recomended, id;

		id = indexPage.GetVarsHandler()->Get("id");
		ost << "select	* from goods_recomend where `id`='" << id << "'";
		if(db.Query(ost.str()) <= 0)
		{
			CLog	log;
		
			log.Write(ERROR, "thereis no such good");
			throw CException("thereis no such good");
		}
		recomended = db.Get(0, "good_recomend");
		affected = db.Query("select * from goods");
		if(affected > 0)
		{
			ost.str("");
			ost << "<select name=recomend>";
			for(int i = 0; i < affected; i++)
			{
				ost << "<option value='" << db.Get(i, "id") << "'" << ((db.Get(i, "id") == recomended) ? " selected " : "") << ">" << db.Get(i, "name") << " (" << db.Get(i, "articul") << ")</option>";
			}
			ost << "</select>";
			indexPage.RegisterVariableForce("good_recomend_choose", ost.str());
		}
		indexPage.SetTemplateFile("templates/good_recomend_edit.htmlt");
	}
	if(act == "submit_good_recomend_edit")
	{
		string		recomend, id;
		ostringstream	ost;
	
		id = indexPage.GetVarsHandler()->Get("id");
		recomend = indexPage.GetVarsHandler()->Get("recomend");
		
		ost << "update `goods_recomend` set `good_recomend`='" << recomend << "'  where `id`='" << id << "'";
		db.Query(ost.str());
		indexPage.RegisterVariableForce("content", "Список рекомендуемых товаров изменен");
	}
	if(act == "delete_good_recomend")
	{
		string		id;
		ostringstream	ost;

		id = indexPage.GetVarsHandler()->Get("id");
		ost << "delete from goods_recomend where `id`='" << id << "'";
		db.Query(ost.str());
		indexPage.RegisterVariableForce("content", "Товар из списка рекомендуемых удален!");
	}


// ------------------ Menu Administrating
    if(act == "listmenu")
    {
	Menu				m;
	MenuItem			*tmpMI1, *tmpMI2, *tmpMI3;
	ostringstream			ost;
	string				act1;

	m.SetDB(&db);
	m.Load();

	act1 = "";
	if(indexPage.GetVarsHandler()->Get("act1") == "parts")
	{
		act1 = "parts";
	}
	
//	ost << GetMenu(&m, 0);

	ost << "<a href=\"/cgi-bin/admin/parts.cgi?act=addmenu&parentid=0&rnd=" << GetRandom(10) << "\">Add root item</a>";
	ost << "<ul>";
	tmpMI1 = m.GetFirstItem(0);
	while((tmpMI1 != NULL))
	{
		ost << "<li>";
		ost << "<a href=" << getenv("SCRIPT_NAME") << "?act=editmenu" << act1 << "&id=" << tmpMI1->GetID() << "&rnd=" << GetRandom(10) << ">" << tmpMI1->GetOrder() << " " << tmpMI1->GetContent() << "<img src=/images/button_edit.png border=0></a>&nbsp;<a href=" << getenv("SCRIPT_NAME") << "?act=delmenu&id=" << tmpMI1->GetID() << "&rnd=" << GetRandom(10) << "><img src=/images/button_drop.png border=0></a>" << "&nbsp;<a href=" << getenv("SCRIPT_NAME") << "?act=addmenu&parentid=" << tmpMI1->GetID() << "&rnd=" << GetRandom(10) << ">add child item</a>";

		ost << "<ul>";
		tmpMI2 = m.GetFirstItem(tmpMI1->GetID());
		while(tmpMI2 != NULL)
		{
			ost << "<li>";
			ost << "<a href=" << getenv("SCRIPT_NAME") << "?act=editmenu" << act1 << "&id=" << tmpMI2->GetID() << "&rnd=" << GetRandom(10) << ">" << tmpMI2->GetOrder() << " " << tmpMI2->GetContent() << "<img src=/images/button_edit.png border=0></a>&nbsp;<a href=" << getenv("SCRIPT_NAME") << "?act=delmenu&id=" << tmpMI2->GetID() << "&rnd=" << GetRandom(10) << "><img src=/images/button_drop.png border=0></a>";
			
			ost << "<ul>";
			tmpMI3 = m.GetFirstItem(tmpMI2->GetID());
			while(tmpMI3 != NULL)
			{
				ost << "<li>";
				ost << "<a href=" << getenv("SCRIPT_NAME") << "?act=editmenu" << act1 << "&id=" << tmpMI3->GetID() << "&rnd=" << GetRandom(10) << ">" << tmpMI3->GetOrder() << " " << tmpMI3->GetContent() << "<img src=/images/button_edit.png border=0></a>&nbsp;<a href=" << getenv("SCRIPT_NAME") << "?act=delmenu&id=" << tmpMI3->GetID() << "&rnd=" << GetRandom(10) << "><img src=/images/button_drop.png border=0></a>";
				ost << "</li>";
				tmpMI3 = m.GetNextSiblingByID(tmpMI3->GetID());
			}
			ost << "</ul>";

			ost << "</li>";
			tmpMI2 = m.GetNextSiblingByID(tmpMI2->GetID());
		}
		ost << "</ul>";

		ost << "</li>";
		tmpMI1 = m.GetNextSiblingByID(tmpMI1->GetID());
	}
	ost << "</ul>";

	indexPage.RegisterVariableForce("content", ost.str());
    }
    if(act == "editmenu")
    {
	string			content, partID, parentID, order, isPrivate;
	ostringstream		ost;

	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "select * from menu where `id`=%s", id.c_str());
	affected = db.Query(query);
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting menu item from db");
		throw CException("error getting menu item from db");
	}

	content = db.Get(0, "content");
	order = db.Get(0, "order");
	parentID = db.Get(0, "parentID");
	partID = db.Get(0, "partID");
	isPrivate = db.Get(0, "isPrivate");
	id = db.Get(0, "id");
	
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "select id, content from menu");
	affected = db.Query(query);
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting menu from db");
		throw CException("error getting menu from db");
	}
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "select id, content from menu");
	affected = db.Query(query);
	if(affected > 0)
	{
		ost << "<select name=parentID>";
		ost << "<option value=0>.... root item ....</option>";
		for(int i = 0; i < affected; i++)
		{
			ost << "<option value='" << db.Get(i, "id") << "'" << ((db.Get(i, "id") == parentID) ? " selected " : "") << ">" << db.Get(i, "content") << "</option>";
		}
		ost << "</select>";
		indexPage.RegisterVariableForce("parentID", ost.str());
	}

	ost.str("");
	ost << "<select name=isPrivate>";
	ost << "<option value='0'>нет</option>";
	ost << "<option value='1' " << ((isPrivate == "1") ? "selected" : "") << ">да</option>";
	indexPage.RegisterVariableForce("privateBox", ost.str());

	ost.str("");
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "select id, title from parts_main");
	affected = db.Query(query);
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting parts_main from db");
		throw CException("error getting parts_main from db");
	}
	ost << "<select name=partID>";
	ost << "<option value=0>.... without menu item ....</option>";
	for(int i = 0; i < affected; i++)
	{
		ost << "<option value='" << db.Get(i, "id") << "'" << ((db.Get(i, "id") == partID) ? " selected " : "") << ">" << db.Get(i, "title") << "</option>";
	}
/*	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "select id, title from category");
	affected = db.Query(query);
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting parts_picture from db");
		throw CException("error getting parts_picture from db");
	}
	for(int i = 0; i < affected; i++)
	{
		int	picID = (atoi(db.Get(i, "id")) + 100000);
		ost << "<option value='" << picID << "'" << ((picID == atoi(partID.c_str())) ? " selected " : "") << ">" << db.Get(i, "title") << " (с картинками)</option>";
	}
*/
	ost << "</select>";
	indexPage.RegisterVariable("partID", ost.str());
	
        indexPage.SetTemplateFile("templates/adminmenuedit.htmlt");

	indexPage.RegisterVariable("content", content);
	indexPage.RegisterVariable("order", order);
	indexPage.RegisterVariable("id", id);
    }
    if(act == "menueditsubmit")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "update `menu` set `order`=\"%s\", `content`=\"%s\", `partID`=\"%s\", `parentID`=\"%s\", `isPrivate`=\"%s\" where `id`='%s'", removeQuotas(indexPage.GetVarsHandler()->Get("order")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("partID")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("parentID")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("isPrivate")).c_str(), indexPage.GetVarsHandler()->Get("id").c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "update menu item success");
    }
    if(act == "addmenu")
    {
	string			content, partID, parentID, order;
	ostringstream		ost;

	parentID = indexPage.GetVarsHandler()->Get("parentid");
	if(parentID.length() == 0) parentID = "0";

	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "select id, content from menu");
	affected = db.Query(query);
	if(affected > 0)
	{
		ost << "<select name=parentID>";
		ost << "<option value=0>.... root item ....</option>";
		for(int i = 0; i < affected; i++)
		{
			ost << "<option value='" << db.Get(i, "id") << "'" << ((db.Get(i, "id") == parentID) ? " selected " : "") << ">" << db.Get(i, "content") << "</option>";
		}
		ost << "</select>";
		indexPage.RegisterVariableForce("parentID", ost.str());
	}

	ost.str("");
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "select id, title from parts_main");
	affected = db.Query(query);
	if(affected > 0)
	{
		ost << "<select name=partID>";
		ost << "<option value=0>.... without menu item ....</option>";
		for(int i = 0; i < affected; i++)
		{
			ost << "<option value='" << db.Get(i, "id") << "'" << ((db.Get(i, "id") == partID) ? " selected " : "") << ">" << db.Get(i, "title") << "</option>";
		}
		ost << "</select>";
		indexPage.RegisterVariable("partID", ost.str());
	}

	ost.str("");
	ost << "<select name=isPrivate>";
	ost << "<option value=0>нет</option>";
	ost << "<option value=1>да</option>";
	indexPage.RegisterVariableForce("privateBox", ost.str());
        
	indexPage.SetTemplateFile("templates/adminmenuadd.htmlt");
    }
    if(act == "menuaddsubmit")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select * from `menu` where `order`=\"%s\"", removeQuotas(indexPage.GetVarsHandler()->Get("order")).c_str());
	if(db.Query(query) > 0)
	{
		indexPage.RegisterVariableForce("content", "<font color=red>Error in ORDER field. Please click BACK button</font>");
	}
	else
	{
		memset(query, 0, sizeof(query));
		snprintf(query, sizeof(query) - 2, "insert into `menu` (`order`, `content`, `partID`, `parentID`) values(\"%s\", \"%s\", \"%s\", \"%s\")", removeQuotas(indexPage.GetVarsHandler()->Get("order")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("partID")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("parentID")).c_str());
		db.Query(query);
		indexPage.RegisterVariableForce("content", "add menu item success");
	}
    }
    if(act == "delmenu")
    {
	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "delete from menu where id=%s", id.c_str());
	affected = db.Query(query);
//	act = "list";
	indexPage.RegisterVariableForce("content", "delete menu item success");
    }

//-------------------------- Enciclopedia edit
    if((act == "list_baners"))
    {
	ostringstream	ost;

	ost << "select * from baners;";
	affected = db.Query(ost.str());

	content = "<table>\n";
	for(int i = 0; i < affected; i++)
	{
	    content += "<tr>";
	    content += "<td class=blue>\n";

	    content += db.Get(i, "id");

	    content += "</td><td>";

		    content += "<a href=/cgi-bin/admin/parts.cgi?act=del_baner&id=";
        	    content += db.Get(i, "id");
		    content += "&rnd=";
		    content += GetRandom(10);
		    content += ">";
		    content += "<img src=/images/button_drop.png border=0></a>";
	    
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=edit_baner&id=";
	    content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += ">";
	    content += " <img src=/images/button_edit.png border=0> <img src=\"/images/";
	    content += DeleteHTML(db.Get(i, "image"));
	    content += "\" border=0>";
	    content += DeleteHTML(db.Get(i, "link"));
	    content += "</a>";


	    content += "</td>";
	    content += "</tr>\n";
	}
	content += "</table>\n";
        indexPage.RegisterVariableForce("content", content.c_str());
    }
    if(act == "edit_baner")
    {
	ostringstream	ost;
	string		image, link, newwindow, id;

	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	ost << "select * from baners where id=" << id;
	affected = db.Query(ost.str());
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting parts from db");
		throw CException("error getting parts from db");
	}
	ost.str("");

	image = db.Get(0, "image");
	link = db.Get(0, "link");
	id = db.Get(0, "id");
	newwindow = db.Get(0, "newwindow");
	
	indexPage.SetTemplateFile("templates/adminbaneredit.htmlt");

	indexPage.RegisterVariable("image", image);
	indexPage.RegisterVariable("link", link);
	indexPage.RegisterVariable("id", id);

	ost.str("");
	ost << "<select name=\"newwindow\"><option value=\"yes\">В новом окне<option value=\"no\" " << ((newwindow == "no") ? "selected" : "") << ">В текущем окне</select>";
	indexPage.RegisterVariable("newwindow", ost.str());

	affected = db.Query("select * from parts_main;");
	ost.str("");
	ost << "<tr><td colspan=2>Расположение банеров<br><table>\n" << "<tr><td>сверху1</td><td>сверху2</td><td>слева1</td><td>слева2</td><td>слева3</td><td>слева4</td><td>слева5</td><td>справа1</td><td>справа2</td><td>справа3</td></tr>";
	for(int i = 0; i < affected; i++)
	{
		ost << "<tr><td>\n<input name=\"place1" << db.Get(i, "id") << "\" type=checkbox " << ((db.Get(i, "baner1") == id) ? "checked" : "") << "></td>";
		ost << "<td><input name=\"place2" << db.Get(i, "id") << "\" type=checkbox " << ((db.Get(i, "baner2") == id) ? "checked" : "") << "></td>\n";
		ost << "<td><input name=\"place3" << db.Get(i, "id") << "\" type=checkbox " << ((db.Get(i, "baner3") == id) ? "checked" : "") << "></td>\n";
		ost << "<td><input name=\"place4" << db.Get(i, "id") << "\" type=checkbox " << ((db.Get(i, "baner4") == id) ? "checked" : "") << "></td>\n";
		ost << "<td><input name=\"place5" << db.Get(i, "id") << "\" type=checkbox " << ((db.Get(i, "baner5") == id) ? "checked" : "") << "></td>\n";
		ost << "<td><input name=\"place6" << db.Get(i, "id") << "\" type=checkbox " << ((db.Get(i, "baner6") == id) ? "checked" : "") << "></td>\n";
		ost << "<td><input name=\"place7" << db.Get(i, "id") << "\" type=checkbox " << ((db.Get(i, "baner7") == id) ? "checked" : "") << "></td>\n";
		ost << "<td><input name=\"place8" << db.Get(i, "id") << "\" type=checkbox " << ((db.Get(i, "baner8") == id) ? "checked" : "") << "></td>\n";
		ost << "<td><input name=\"place9" << db.Get(i, "id") << "\" type=checkbox " << ((db.Get(i, "baner9") == id) ? "checked" : "") << "></td>\n";
		ost << "<td><input name=\"placea" << db.Get(i, "id") << "\" type=checkbox " << ((db.Get(i, "baner10") == id) ? "checked" : "") << "></td>\n";
		ost << "<td>" << db.Get(i, "title") << "</td></tr>\n";
	}
	ost << "</table></td></tr>\n";
	indexPage.RegisterVariable("parts", ost.str());
    }
    if(act == "add_baner")
    {
	ostringstream	ost;

	ost << "<select name=\"newwindow\"><option value=\"yes\">В новом окне<option value=\"no\">В текущем окне</select>";
	indexPage.RegisterVariable("newwindow", ost.str());
	
	indexPage.SetTemplateFile("templates/adminbaneradd.htmlt");
    }
    if(act == "submitadd_baner")
    {
	ostringstream	ost;
	CVars		*v;
	CVars::iterator	itr;


	ost << "insert into `baners` (`image`, `link`, `newwindow`) VALUES ('" << removeQuotas(indexPage.GetVarsHandler()->Get("image")).c_str() << "', '" << removeQuotas(indexPage.GetVarsHandler()->Get("link")).c_str() << "', '" << removeQuotas(indexPage.GetVarsHandler()->Get("newwindow")).c_str() << "')";
	db.Query(ost.str());

	ost.str(""); ost << "update `parts_main` set `baner1`=\"0\" where `baner1`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner2`=\"0\" where `baner2`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner3`=\"0\" where `baner3`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner4`=\"0\" where `baner4`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner5`=\"0\" where `baner5`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner6`=\"0\" where `baner6`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner7`=\"0\" where `baner7`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner8`=\"0\" where `baner8`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner9`=\"0\" where `baner9`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `banera`=\"0\" where `banera`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	
	v = indexPage.GetVarsHandler();
	for(itr = v->begin(); itr != v->end(); itr++)
	{
		if(itr->first.substr(0, 5) == "place")
		{
			string		place, partID;
			ostringstream	query;

			place = itr->first.substr(5, 1);
			partID = itr->first.substr(6);

			query.str("");
			query << "update `parts_main` set `baner" << removeQuotas(place) << "`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\" where `id`=\"" << removeQuotas(partID) << "\"";
			db.Query(query.str());
		}
	}
	
	indexPage.RegisterVariableForce("content", "Добавление банера успешено.");
    }
    if(act == "submitedit_baner")
    {
	ostringstream	ost;
	CVars		*v;
	CVars::iterator	itr;

	ost << "update `baners` set `image`='" << removeQuotas(indexPage.GetVarsHandler()->Get("image")).c_str() << "', `link`='" << removeQuotas(indexPage.GetVarsHandler()->Get("link")).c_str() << "', `newwindow`='" << removeQuotas(indexPage.GetVarsHandler()->Get("newwindow")).c_str() << "' where id='" << removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str() << "'";
	db.Query(ost.str());

	ost.str(""); ost << "update `parts_main` set `baner1`=\"0\" where `baner1`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner2`=\"0\" where `baner2`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner3`=\"0\" where `baner3`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner4`=\"0\" where `baner4`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner5`=\"0\" where `baner5`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner6`=\"0\" where `baner6`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner7`=\"0\" where `baner7`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner8`=\"0\" where `baner8`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner9`=\"0\" where `baner9`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `banera`=\"0\" where `banera`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	
	v = indexPage.GetVarsHandler();
	for(itr = v->begin(); itr != v->end(); itr++)
	{
		if(itr->first.substr(0, 5) == "place")
		{
			string		place, partID;
			ostringstream	query;

			place = itr->first.substr(5, 1);
			partID = itr->first.substr(6);

			query.str("");
			query << "update `parts_main` set `baner" << removeQuotas(place) << "`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\" where `id`=\"" << removeQuotas(partID) << "\"";
			db.Query(query.str());
		}
	}
	
	indexPage.RegisterVariableForce("content", "Редактирование банера успешено.");
    }
    if(act == "del_baner")
    {
	ostringstream	ost;

	ost << "delete from `baner` where `id`='" << indexPage.GetVarsHandler()->Get("id") << "'";
	db.Query(ost.str());
	
	ost.str(""); ost << "update `parts_main` set `baner1`=\"0\" where `baner1`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner2`=\"0\" where `baner2`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner3`=\"0\" where `baner3`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner4`=\"0\" where `baner4`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner5`=\"0\" where `baner5`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner6`=\"0\" where `baner6`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner7`=\"0\" where `baner7`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner8`=\"0\" where `baner8`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `baner9`=\"0\" where `baner9`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());
	ost.str(""); ost << "update `parts_main` set `banera`=\"0\" where `banera`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("id")) << "\"";
	db.Query(ost.str());

	indexPage.RegisterVariableForce("content", "Удаление банера успешено.");
    }


//-------------------------- Edit picture parts
    if(act == "list_parts_picture")
    {
	ostringstream		ost, result;
	string			id;
	int			affected;

	id = indexPage.GetVarsHandler()->Get("p");
	if(id.length() == 0)
	{
		CLog	log;

		log.Write(ERROR, "category must not be zero !!!!");
		throw CExceptionHTML("category must no be zero");
	}

	ost.str("");
	ost << "select * from parts_main where id=\"" << id << "\"";
	affected = db.Query(ost.str());
	if(affected == 0)
	{
		CLog	log;
		log.Write(ERROR, "category does not exist");
		throw CExceptionHTML("category does not exist");
	}
	indexPage.RegisterVariableForce("id", id);
	indexPage.RegisterVariableForce("title", db.Get(0, "title"));
	indexPage.RegisterVariableForce("description", db.Get(0, "content"));
	indexPage.RegisterVariableForce("isDate", db.Get(0, "isDate"));
    
	ost.str("");
	ost << "select * from parts_picture where category=\"" << id << "\" ORDER BY `date` DESC";
	affected = db.Query(ost.str());
	ost.str("");
	for(int i = 0; i < affected; i++)
	{
		ost << "<tr><td>" << db.Get(i, "image") << "</td><td colspan=2><a href='/cgi-bin/admin/parts.cgi?act=edit_partspicture_item&p=" << db.Get(i, "id") << "&rand=" << GetRandom(10) << "'>" << db.Get(i, "title") << "</a> ";
		if(indexPage.GetVarsHandler()->Get("isDate") == "yes")
		{
			ost << " (" << db.Get(i, "date") << ")";
		}
		ost << "&nbsp;\n\
		<a href='/cgi-bin/admin/parts.cgi?act=delete_partspicture_item&p=" << db.Get(i, "id") << "&rand=" << GetRandom(10) << "'><img src=\"/images/button_drop.png\" border=0></a>\
		<br>" << db.Get(i, "brief") << "</td></tr>";
	}
	indexPage.RegisterVariableForce("content", ost.str());

	ost.str("");
	ost << "/cgi-bin/admin/parts.cgi?act=add_partspicture_item&id=" << id << "&rand=" << GetRandom(10);
	indexPage.RegisterVariableForce("href_add_item", ost.str());
	indexPage.SetTemplateFile("templates/adminpartspictureedit.htmlt");
    }

//-------------------------- Parts picture item
    if(act == "edit_partspicture_item")
    {
	ostringstream		ost, result;
	string			id, parentID;
	int			affected;

	id = indexPage.GetVarsHandler()->Get("p");
	if(id.length() == 0)
	{
		CLog	log;

		log.Write(ERROR, "item must not be zero !!!!");
		throw CExceptionHTML("item must no be zero");
	}

	ost.str("");
	ost << "select * from parts_picture where id=\"" << id << "\"";
	affected = db.Query(ost.str());
	if(affected == 0)
	{
		CLog	log;
		log.Write(ERROR, "category does not exist");
		throw CExceptionHTML("category does not exist");
	}
	indexPage.RegisterVariableForce("id", id);
	indexPage.RegisterVariableForce("title", db.Get(0, "title"));
	indexPage.RegisterVariableForce("brief", db.Get(0, "brief"));
	indexPage.RegisterVariableForce("content", db.Get(0, "content"));
	indexPage.RegisterVariableForce("image", db.Get(0, "image"));
	indexPage.RegisterVariableForce("date", db.Get(0, "date"));
	indexPage.RegisterVariableForce("category", db.Get(0, "category"));
    
	ost.str("");
	ost << "select * from parts_main where id=\"" << indexPage.GetVarsHandler()->Get("category") << "\"";
	affected = db.Query(ost.str());
	if(affected <= 0)
	{
		CLog	log;
		log.Write(ERROR, "there is no such category.");

		CException("there is no such category");
	}
	indexPage.RegisterVariableForce("categoryTitle", db.Get(0, "title"));
	indexPage.RegisterVariableForce("isDate", db.Get(0, "isDate"));
	if(indexPage.GetVarsHandler()->Get("isDate") == "yes")
	{
		indexPage.SetTemplateFile("templates/adminpartspictureitemedit_date.htmlt");
	}
	else
	{
		indexPage.SetTemplateFile("templates/adminpartspictureitemedit.htmlt");
	}
    }
    if(act == "add_partspicture_item")
    {
	ostringstream		ost, result;
	string			id, parentID;
	int			affected;

	indexPage.RegisterVariableForce("category", indexPage.GetVarsHandler()->Get("id"));
    
	ost.str("");
	ost << "select * from parts_main where id=\"" << indexPage.GetVarsHandler()->Get("category") << "\"";
	affected = db.Query(ost.str());
	if(affected <= 0)
	{
		CLog	log;
		log.Write(ERROR, "there is no such category.");

		CException("there is no such category");
	}
	indexPage.RegisterVariableForce("categoryTitle", db.Get(0, "title"));
	indexPage.RegisterVariableForce("isDate", db.Get(0, "isDate"));
	if(indexPage.GetVarsHandler()->Get("isDate") == "yes")
	{
		indexPage.SetTemplateFile("templates/adminpartspictureitemadd_date.htmlt");
	}
	else
	{
		indexPage.SetTemplateFile("templates/adminpartspictureitemadd.htmlt");
	}
    }
    if(act == "submit_partspictureitem_edit")
    {
	string		date, title, brief, content, image, category, id;
	ostringstream	ost;
	
	date = indexPage.GetVarsHandler()->Get("date");
	title = indexPage.GetVarsHandler()->Get("title");
	brief = indexPage.GetVarsHandler()->Get("brief");
	content = indexPage.GetVarsHandler()->Get("content");
	image = indexPage.GetVarsHandler()->Get("image");
	category = indexPage.GetVarsHandler()->Get("category");
	id = indexPage.GetVarsHandler()->Get("id");
	
	ost.str("");
	ost << "UPDATE `parts_picture` SET \
	`content` =\"" << removeQuotas(content) << "\", \
	`title`= \"" << removeQuotas(title) << "\", \
	`date`= \"" << removeQuotas(date) << "\", \
	`brief`= \"" << removeQuotas(brief) << "\", \
	`image`= \"" << removeQuotas(image) << "\" \
	WHERE `id` =\"" << removeQuotas(id) << "\"";
	db.Query(ost.str());
	indexPage.RegisterVariableForce("content", "Описание элемента сохранено.");
    }
    if(act == "submit_partspictureitem_add")
    {
	string		date, title, brief, content, image, category, id;
	ostringstream	ost;
	
	date = indexPage.GetVarsHandler()->Get("date");
	title = indexPage.GetVarsHandler()->Get("title");
	brief = indexPage.GetVarsHandler()->Get("brief");
	content = indexPage.GetVarsHandler()->Get("content");
	image = indexPage.GetVarsHandler()->Get("image");
	category = indexPage.GetVarsHandler()->Get("category");
	
	ost.str("");
	ost << "INSERT INTO `parts_picture` (`content`, `title`, `date`, `brief`, `image`, `category`) VALUES (\
	\"" << removeQuotas(content) << "\", \
	\"" << removeQuotas(title) << "\", \
	\"" << removeQuotas(date) << "\", \
	\"" << removeQuotas(brief) << "\", \
	\"" << removeQuotas(image) << "\", \
	\"" << removeQuotas(category) << "\")";
	db.Query(ost.str());
	indexPage.RegisterVariableForce("content", "Добавление элемента успешно.");
    }
    if(act == "delete_partspicture_item")
    {
	string		id;
	ostringstream	ost;
	
	id = indexPage.GetVarsHandler()->Get("p");
	
	ost.str("");
	ost << "delete from `parts_picture` where id=\"" << id << "\"";
	db.Query(ost.str());
	indexPage.RegisterVariableForce("content", "Удаление элемента успешно.");
    }
    
//-------------------------- Gallery editing
//-------------------------- News editing
    if(act == "addnews")
    {
	indexPage.SetTemplateFile("templates/adminnewsadd.htmlt");
    }
    if(act == "newsaddsubmit")
    {
	ostringstream	ramkaName;
	ostringstream	thumbnailName;

	memset(query, 0, sizeof(query));

	snprintf(query, sizeof(query) - 2, "insert into `news` (`data`, `title`, `content_brief`, `content`, `file`) values(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\")", removeQuotas(indexPage.GetVarsHandler()->Get("data")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("title")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content_brief")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("file")).c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "add news success");
    }
    if(act == "listnews")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select `id`,`data`,`title` from news order by `data` desc;");
	affected = db.Query(query);

	content = "<table>\n";
	for(int i = 0; i < affected; i++)
	{
	    content += "<tr>";
	    content += "<td class=blue>\n";

	    content += db.Get(i, "data");

	    content += "</td><td>";

	    content += "<a href=/cgi-bin/admin/parts.cgi?act=delnews&id=";
            content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += ">";
	    content += "<img src=/images/button_drop.png border=0></a>";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=editnews&id=";
	    content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += "><img src=/images/button_edit.png border=0> ";
	    content += DeleteHTML(db.Get(i, "title"));
	    content += "</a>";


	    content += "</td>";
	    content += "</tr>\n";
	}
	content += "</table>\n";
        indexPage.RegisterVariableForce("content", content.c_str());
    }
    if(act == "editnews")
    {
	ostringstream	ost;
	string		content, content_brief, title, data, id, image;
    
	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "select * from news where id=%s", id.c_str());
	affected = db.Query(query);
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting news from db");
		throw CException("error getting news from db");
	}

	content = db.Get(0, "content");
	content_brief = db.Get(0, "content_brief");
	title = db.Get(0, "title");
	data = db.Get(0, "data");
	id = db.Get(0, "id");
	image = db.Get(0, "file");
        
	indexPage.RegisterVariableForce("id", id);
        indexPage.RegisterVariableForce("data", data);
        indexPage.RegisterVariableForce("title", title);
        indexPage.RegisterVariableForce("content_brief", content_brief);
        indexPage.RegisterVariableForce("content", content);
        indexPage.RegisterVariableForce("image", image);
	
	indexPage.SetTemplateFile("templates/adminnewsedit.htmlt");
    }
    if(act == "newseditsubmit")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "update `news` set `data`=\"%s\", `title`=\"%s\", `content_brief`=\"%s\", `content`=\"%s\", `file`=\"%s\" where `id`='%s'", removeQuotas(indexPage.GetVarsHandler()->Get("data")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("title")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content_brief")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("image")).c_str(), indexPage.GetVarsHandler()->Get("id").c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "update news success");
    }
    if(act == "delnews")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "delete from news where `id`=\"%s\"", removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str()); 
	db.Query(query);
	indexPage.RegisterVariableForce("content", "delete news success");
    } 
            
//-------------------------- Forum administrating
	if(act == "listforum")
        {
                string  forum, thID, parentID, firstTh, page, nextTh, prevTh, postMes, submitForumMes, content;
                string  msg, name, email, topic, url, url_name;

                forum = indexPage.GetVarsHandler()->Get("forum");
                thID = indexPage.GetVarsHandler()->Get("thid");
                firstTh = indexPage.GetVarsHandler()->Get("firstth");
                page = indexPage.GetVarsHandler()->Get("page");
                nextTh = indexPage.GetVarsHandler()->Get("nextth");
                prevTh = indexPage.GetVarsHandler()->Get("prevth");

                postMes = indexPage.GetVarsHandler()->Get("postmes");
                submitForumMes = indexPage.GetVarsHandler()->Get("submitforummes");

                parentID = indexPage.GetVarsHandler()->Get("pid");
                name = indexPage.GetVarsHandler()->Get("name");
                email = indexPage.GetVarsHandler()->Get("email");
                topic = indexPage.GetVarsHandler()->Get("topic");
                msg = indexPage.GetVarsHandler()->Get("msg");
                url = indexPage.GetVarsHandler()->Get("url");
                url_name = indexPage.GetVarsHandler()->Get("url_name");


                {
                        CForum  forumInst(&db);

                        if(thID.length() > 0)
                        {
                                if(firstTh.length() == 0) firstTh = "0";
                                forumInst.SetFirstMessage(atoi(firstTh.c_str()));
                                content = forumInst.GetTextMessage(atoi(thID.c_str()));
                        }
                        else
                        {
                                if(firstTh.length() == 0)
                                        firstTh = "0";
                                forumInst.SetCurrentPage(atoi(page.c_str()));
                                content = forumInst.GetTextForumAdmin(atoi(firstTh.c_str()), THREADS_PER_PAGE);
                        }

                        indexPage.RegisterVariable("content", content.c_str());
                }
 	}
	if(act == "editmes")
        {
		ostringstream	ost;

		ost << "select * from forum where rootID=" << indexPage.GetVarsHandler()->Get("thid");
		db.Query(ost.str());
		indexPage.RegisterVariableForce("id", indexPage.GetVarsHandler()->Get("thid"));
		indexPage.RegisterVariableForce("name", db.Get(0, "name"));
		indexPage.RegisterVariableForce("email", db.Get(0, "email"));
		indexPage.RegisterVariableForce("topic", db.Get(0, "topic"));
		indexPage.RegisterVariableForce("msg", db.Get(0, "msg"));
		indexPage.RegisterVariableForce("url", db.Get(0, "url"));
		indexPage.RegisterVariableForce("url_name", db.Get(0, "url_name"));

		indexPage.SetTemplateFile("templates/adminmesedit.htmlt");
 	}
	if(act == "meseditsubmit")
	{
		memset(query, 0, sizeof(query));
		snprintf(query, sizeof(query) - 2, "update `forum` set `name`=\"%s\", `email`=\"%s\", `topic`=\"%s\", `msg`=\"%s\", `url`=\"%s\", `url_name`=\"%s\" where `rootID`='%s'", removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("email")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("topic")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("url")).c_str(), removeQuotas(indexPage.GetVarsHandler()->Get("url_name")).c_str(), indexPage.GetVarsHandler()->Get("id").c_str());
		db.Query(query);
		indexPage.RegisterVariableForce("content", "update message success");
	}
	if(act == "delmes")
	{
		memset(query, 0, sizeof(query));
		snprintf(query, sizeof(query) - 2, "delete from forum where `rootID`=\"%s\"", removeQuotas(indexPage.GetVarsHandler()->Get("thid")).c_str()); 
		db.Query(query);
		indexPage.RegisterVariableForce("content", "delete messsage success");
	} 
//-------------------------- End forum administrating

//-------------------------- client editing
    if(act == "client_list")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select * from `clients` order by `name` asc;");
	affected = db.Query(query);

	content = "<table>\n";
	for(int i = 0; i < affected; i++)
	{
	    content += "<tr>";
	    content += "<td class=blue>\n";
	    content += db.Get(i, "id");
	    content += "</td><td>";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=client_del&id=";
            content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += ">";
	    content += "<img src=/images/button_drop.png border=0></a> ";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=client_edit&id=";
	    content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += "><img src=/images/button_edit.png border=0> ";
	    content += DeleteHTML(db.Get(i, "name"));
	    content += "</a>";


	    content += "</td>";
	    content += "</tr>\n";
	}
	content += "</table>\n";
        indexPage.RegisterVariableForce("content", content.c_str());
    }
    if(act == "client_edit")
    {
	ostringstream	ost;
	string		name, id;

	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	ost << "select * from `clients` where id=" << id;
	affected = db.Query(ost.str());
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting parts from db");
		throw CException("error getting parts from db");
	}
	ost.str("");

	name = db.Get(0, "name");

	indexPage.RegisterVariableForce("name", name);
	indexPage.RegisterVariableForce("description", db.Get(0, "description"));
	indexPage.RegisterVariableForce("id", id);

	indexPage.SetTemplateFile("templates/client_edit.htmlt");
    }
    if(act == "client_edit_submit")
    {
    	ostringstream	ost;

	ost << "update `clients` set \
	`name`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "\" , \
	`description`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("description")).c_str() << "\" \
	where `id`='" << removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str() << "'";
	db.Query(ost.str());
	ost.str("");
	ost << " Редактирование клиента прошло успешно.";
	indexPage.RegisterVariableForce("content", ost.str());
	
//	Makehtaccess(&db);
    }
    if(act  == "client_del")
    {
	ostringstream	ost;
	
	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	ost << "delete from `clients` where id=" << id;
	affected = db.Query(ost.str());
	content = "Удаление клиента прошло успешно";
	indexPage.RegisterVariableForce("content", content);
    }
    if(act == "client_add")
    {
	indexPage.SetTemplateFile("templates/client_add.htmlt");
    }
    if(act == "client_add_submit")
    {
    	ostringstream	ost;

	ost.str("");
	ost << "insert into `clients` (`name`,`description`) VALUES ('" <<
	removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "',\
        '" << removeQuotas(indexPage.GetVarsHandler()->Get("description")).c_str() << "')";
	db.Query(ost.str());
	content = " Добавление клиента успешно.";
	indexPage.RegisterVariableForce("content", content);

	indexPage.SetTemplateFile("templates/adminindex.htmlt");

//	Makehtaccess(&db);
    }

//-------------------------- type editing
    if(act == "type_list")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select * from `types` order by `name` asc;");
	affected = db.Query(query);

	content = "<table>\n";
	for(int i = 0; i < affected; i++)
	{
	    content += "<tr>";
	    content += "<td class=blue>\n";
	    content += db.Get(i, "id");
	    content += "</td><td>";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=type_del&id=";
            content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += ">";
	    content += "<img src=/images/button_drop.png border=0></a> ";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=type_edit&id=";
	    content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += "><img src=/images/button_edit.png border=0> ";
	    content += DeleteHTML(db.Get(i, "name"));
	    content += "</a>";


	    content += "</td>";
	    content += "</tr>\n";
	}
	content += "</table>\n";
        indexPage.RegisterVariableForce("content", content.c_str());
    }
    if(act == "type_edit")
    {
	ostringstream	ost;
	string		name, id;

	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	ost << "select * from `types` where id=" << id;
	affected = db.Query(ost.str());
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting parts from db");
		throw CException("error getting parts from db");
	}
	ost.str("");

	name = db.Get(0, "name");

	indexPage.RegisterVariableForce("name", name);
	indexPage.RegisterVariableForce("id", id);

	indexPage.SetTemplateFile("templates/type_edit.htmlt");
    }
    if(act == "type_edit_submit")
    {
    	ostringstream	ost;

	ost << "update `types` set \
	`name`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "\" \
	where `id`='" << removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str() << "'";
	db.Query(ost.str());
	ost.str("");
	ost << " Редактирование типа прошло успешно.";
	indexPage.RegisterVariableForce("content", ost.str());
	
//	Makehtaccess(&db);
    }
    if(act  == "type_del")
    {
	ostringstream	ost;
	
	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	ost << "delete from `types` where id=" << id;
	affected = db.Query(ost.str());
	content = "Удаление типа прошло успешно";
	indexPage.RegisterVariableForce("content", content);
    }
    if(act == "type_add")
    {
	indexPage.SetTemplateFile("templates/type_add.htmlt");
    }
    if(act == "type_add_submit")
    {
    	ostringstream	ost;

	ost.str("");
	ost << "insert into `types` (`name`) VALUES ('" <<
	removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "')";
	db.Query(ost.str());
	content = " Добавление типа успешно.";
	indexPage.RegisterVariableForce("content", content);

	indexPage.SetTemplateFile("templates/adminindex.htmlt");

//	Makehtaccess(&db);
    }


//---------------------- Work framework
	if(act == "list_work")
	{
		memset(query, 0, sizeof(query));
		snprintf(query, sizeof(query) - 2, "select * from `works` order by `order` asc;");
		affected = db.Query(query);
	
		content = "<table>\n";
		for(int i = 0; i < affected; i++)
		{
		content += "<tr>";
		content += "<td class=blue>\n";
		content += db.Get(i, "order");
		content += "</td><td>";
		content += "<a href=/cgi-bin/admin/parts.cgi?act=del_work&id=";
		content += db.Get(i, "id");
		content += "&rnd=";
		content += GetRandom(10);
		content += ">";
		content += "<img src=/images/button_drop.png border=0></a> ";
		content += "<a href=/cgi-bin/admin/parts.cgi?act=edit_work&id=";
		content += db.Get(i, "id");
		content += "&rnd=";
		content += GetRandom(10);
		content += "><img src=/images/button_edit.png border=0> ";
		content += DeleteHTML(db.Get(i, "name"));
		content += "</a>";
	
	
		content += "</td>";
		content += "</tr>\n";
		}
		content += "</table>\n";
		indexPage.RegisterVariableForce("content", content.c_str());
	}
	if(act == "add_work")
	{
		int 		affected;
		string		client, tmp;
		ostringstream	ost;

		client = "0";

		affected = db.Query("select * from `clients` order by `name` asc");
		if(affected > 0)
		{
			ost.str("");
			ost << "<select name='clients'>";
			ost << "<option value='0'>...</option>";
			for(int i = 0; i < affected; i++)
			{
				tmp = db.Get(i, "id");
				ost << "<option value='" << tmp << "' " << ((tmp == client) ? "selected" : "") << " >" << db.Get(i, "name") << "</option>";
			}
			ost << "</select>";
			indexPage.RegisterVariableForce("clients", ost.str());
		}

		affected = db.Query("select * from `types` order by `name` asc");
		if(affected > 0)
		{
			ost.str("");
			for(int i = 0; i < affected; i++)
			{
				ost << "<input type=checkbox name=type" << db.Get(i, "id") << ">" << db.Get(i, "name") << "<br>";
			}
			indexPage.RegisterVariableForce("types", ost.str());
		}

		indexPage.SetTemplateFile("templates/work_add.htmlt");
	}
	if(act == "edit_work")
	{
		string			id, parentID, order, name, description, clientID, content, dataCreate, isFavorite, isShow, image1, image2, image3, image4, image5, image6, image7, image8, image9;
		ostringstream		ost, ost1;
		string			client, tmp;
		CMysql			db1;
		int			affected, affected1;

		if(db1.Connect(DB_NAME, DB_LOGIN, DB_PASSWORD) < 0)
		{
			CLog	log;
		
			log.Write(ERROR, "Can not connect to mysql database");
			return(1);
		}
		db1.Query("set names cp1251");
	
		id = indexPage.GetVarsHandler()->Get("id");
		if(id.length() == 0)
		{
			throw CException("deit_work: parameter id was missing");
		}
		
		memset(query, 0, sizeof(query));
		snprintf(query, sizeof(query), "select * from works where `id`=%s", id.c_str());
		affected = db.Query(query);
		if(affected == 0)
		{
			CLog	log;
			
			log.Write(ERROR, "edit_work: error getting menu item from db");
			throw CException("error getting menu item from db");
		}
	
		id = db.Get(0, "id");
		parentID = db.Get(0, "parentID");
		order = db.Get(0, "order");
		name = db.Get(0, "name");
		description = db.Get(0, "description");
		clientID = db.Get(0, "clientID");
		content = db.Get(0, "content");
		dataCreate = db.Get(0, "dataCreate");
		isFavorite = db.Get(0, "isFavorite");
		isShow = db.Get(0, "isShow");
		image1 = db.Get(0, "image1");
		image2 = db.Get(0, "image2");
		image3 = db.Get(0, "image3");
		image4 = db.Get(0, "image4");
		image5 = db.Get(0, "image5");
		image6 = db.Get(0, "image6");
		image7 = db.Get(0, "image7");
		image8 = db.Get(0, "image8");
		image9 = db.Get(0, "image9");

		indexPage.SetTemplateFile("templates/work_edit.htmlt");
	
		indexPage.RegisterVariableForce("id", id);
		indexPage.RegisterVariableForce("parentID", parentID);
		indexPage.RegisterVariableForce("order", order);
		indexPage.RegisterVariableForce("name", name);
		indexPage.RegisterVariableForce("description", description);
		indexPage.RegisterVariableForce("clientID", clientID);
		indexPage.RegisterVariableForce("content", content);
		indexPage.RegisterVariableForce("dataCreate", dataCreate);

		ost.str("");
		ost << "<select name=\"isFavorite\"><option value='N'>Нет</option><option value='Y' " << ((isFavorite == "Y") ? "selected" : "") << ">Да</option></select>";
		indexPage.RegisterVariableForce("isFavorite", ost.str());
		ost.str("");
		ost << "<select name=\"isShow\"><option value='N'>Нет</option><option value='Y' " << ((isShow == "Y") ? "selected" : "") << ">Да</option></select>";
		indexPage.RegisterVariableForce("isShow", ost.str());

		indexPage.RegisterVariableForce("image1", image1);
		if(image1.length() > 0) indexPage.RegisterVariableForce("image1Pic", image1); else indexPage.RegisterVariableForce("image1Pic", "1.gif");
		indexPage.RegisterVariableForce("image2", image2);
		if(image2.length() > 0) indexPage.RegisterVariableForce("image2Pic", image2); else indexPage.RegisterVariableForce("image2Pic", "1.gif");
		indexPage.RegisterVariableForce("image3", image3);
		if(image3.length() > 0) indexPage.RegisterVariableForce("image3Pic", image3); else indexPage.RegisterVariableForce("image3Pic", "1.gif");
		indexPage.RegisterVariableForce("image4", image4);
		if(image4.length() > 0) indexPage.RegisterVariableForce("image4Pic", image4); else indexPage.RegisterVariableForce("image4Pic", "1.gif");
		indexPage.RegisterVariableForce("image5", image5);
		if(image5.length() > 0) indexPage.RegisterVariableForce("image5Pic", image5); else indexPage.RegisterVariableForce("image5Pic", "1.gif");
		indexPage.RegisterVariableForce("image6", image6);
		if(image6.length() > 0) indexPage.RegisterVariableForce("image6Pic", image6); else indexPage.RegisterVariableForce("image6Pic", "1.gif");
		indexPage.RegisterVariableForce("image7", image7);
		if(image7.length() > 0) indexPage.RegisterVariableForce("image7Pic", image7); else indexPage.RegisterVariableForce("image7Pic", "1.gif");
		indexPage.RegisterVariableForce("image8", image8);
		if(image8.length() > 0) indexPage.RegisterVariableForce("image8Pic", image8); else indexPage.RegisterVariableForce("image8Pic", "1.gif");
		indexPage.RegisterVariableForce("image9", image9);
		if(image9.length() > 0) indexPage.RegisterVariableForce("image9Pic", image9); else indexPage.RegisterVariableForce("image9Pic", "1.gif");

		affected = db.Query("select * from `clients` order by `name` asc");
		if(affected > 0)
		{
			ost.str("");
			ost << "<select name='clients'>";
			ost << "<option value='0'>...</option>";
			for(int i = 0; i < affected; i++)
			{
				tmp = db.Get(i, "id");
				ost << "<option value='" << tmp << "' " << ((tmp == clientID) ? "selected" : "") << " >" << db.Get(i, "name") << "</option>";
			}
			ost << "</select>";
			indexPage.RegisterVariableForce("clients", ost.str());
		}

		affected = db.Query("select * from `types` order by `name` asc");
		if(affected > 0)
		{
			ost.str("");
			for(int i = 0; i < affected; i++)
			{
				ost1.str("");
				ost1 << "select * from typesWorks where `typeID`='" << db.Get(i, "id") << "' and `workID`='" << id << "'";
				affected1 = db1.Query(ost1.str());
				ost << "<input type=checkbox name=type" << db.Get(i, "id") << " " << ((affected1 > 0) ? "checked" : "") <<  ">" << db.Get(i, "name") << "<br>";
			}
			indexPage.RegisterVariableForce("types", ost.str());
		}


	}
	if(act == "submit_works_edit")
	{
		string          fileName1 = indexPage.GetFilesHandler()->GetName(0);
		string          fileName2 = indexPage.GetFilesHandler()->GetName(1);
		string          fileName3 = indexPage.GetFilesHandler()->GetName(2);
		string          fileName4 = indexPage.GetFilesHandler()->GetName(3);
		string          fileName5 = indexPage.GetFilesHandler()->GetName(4);
		string          fileName6 = indexPage.GetFilesHandler()->GetName(5);
		string          fileName7 = indexPage.GetFilesHandler()->GetName(6);
		string          fileName8 = indexPage.GetFilesHandler()->GetName(7);
		string          fileName9 = indexPage.GetFilesHandler()->GetName(8);
                string          fileNameLocal;
                FILE            *f;
                ostringstream   ost, ost1;
                string          id;

		id = indexPage.GetVarsHandler()->Get("id");
		if(id.length() == 0)
		{
			CLog	log;
			log.Write(ERROR, "parameter id was missing");

			throw CException("parameter id was missing");
		}

                if(!fileName9.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName9;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(8) != NULL) && (indexPage.GetFilesHandler()->GetSize(8) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(8), indexPage.GetFilesHandler()->GetSize(8), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName8.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName8;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(7) != NULL) && (indexPage.GetFilesHandler()->GetSize(7) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(7), indexPage.GetFilesHandler()->GetSize(7), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName7.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName7;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(6) != NULL) && (indexPage.GetFilesHandler()->GetSize(6) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(6), indexPage.GetFilesHandler()->GetSize(6), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName6.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName6;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(5) != NULL) && (indexPage.GetFilesHandler()->GetSize(5) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(5), indexPage.GetFilesHandler()->GetSize(5), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName5.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName5;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(4) != NULL) && (indexPage.GetFilesHandler()->GetSize(4) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(4), indexPage.GetFilesHandler()->GetSize(4), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName4.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName4;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(3) != NULL) && (indexPage.GetFilesHandler()->GetSize(3) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(3), indexPage.GetFilesHandler()->GetSize(3), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName3.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName3;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(2) != NULL) && (indexPage.GetFilesHandler()->GetSize(2) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(2), indexPage.GetFilesHandler()->GetSize(2), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName2.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName2;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(1) != NULL) && (indexPage.GetFilesHandler()->GetSize(1) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(1), indexPage.GetFilesHandler()->GetSize(1), 1, f);
                    }
                    fclose(f);
                }
		
                if(!fileName1.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName1;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(0) != NULL) && (indexPage.GetFilesHandler()->GetSize(0) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(0), indexPage.GetFilesHandler()->GetSize(0), 1, f);
                    }
                    fclose(f);
                }

		ost.str("");
		ost << "update `works` set \
			`parentID` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("parentID")).c_str() << "\", \
			`order` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("order")).c_str() << "\", \
			`name` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "\", \
			`clientID` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("clients")).c_str() << "\", \
			`isFavorite` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("isFavorite")).c_str() << "\", \
			`dataCreate` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("data")).c_str() << "\", \
			`content` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str() << "\", ";
		if(!fileName1.empty()) ost << "`image1` = \"" << fileName1 << "\", ";
		if(!fileName2.empty()) ost << "`image2` = \"" << fileName2 << "\", ";
		if(!fileName3.empty()) ost << "`image3` = \"" << fileName3 << "\", ";
		if(!fileName4.empty()) ost << "`image4` = \"" << fileName4 << "\", ";
		if(!fileName5.empty()) ost << "`image5` = \"" << fileName5 << "\", ";
		if(!fileName6.empty()) ost << "`image6` = \"" << fileName6 << "\", ";
		if(!fileName7.empty()) ost << "`image7` = \"" << fileName7 << "\", ";
		if(!fileName8.empty()) ost << "`image8` = \"" << fileName8 << "\", ";
		if(!fileName9.empty()) ost << "`image9` = \"" << fileName9 << "\", ";
		ost << "`isShow` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("isShow")).c_str() << "\" \
			where id = " << id;
		db.Query(ost.str());
		ost.str("");
		ost << "delete from `typesWorks` where `workID`='" << id << "'";
		affected = db.Query(ost.str());

		for(int i = 0; i < 100000; i++)
		{
			ost.str("");
			ost << "type" << i;
			if((indexPage.GetVarsHandler()->Get(ost.str())).length() > 0)
			{
				ost.str("");
				ost << "insert into `typesWorks` set \
					`typeID` = '" << i << "', \
					`workID` = '" << id << "'";
				db.Query(ost.str());
			}
		}

		content = "Работа изменена.";
		indexPage.RegisterVariableForce("content", content);
	}
	if(act == "del_work")
	{
		string		id;
		ostringstream	ost;

		id = indexPage.GetVarsHandler()->Get("id");
		ost.str("");
		ost << "delete from works where `id`='" << id << "'";
		db.Query(ost.str());
		ost.str("");
		ost << "delete from typesWorks where `workID`='" << id << "'";
		db.Query(ost.str());
		indexPage.RegisterVariableForce("content", "Работа удалена!");
	}
	if(act == "submit_works_add")
	{
		string          fileName1 = indexPage.GetFilesHandler()->GetName(0);
		string          fileName2 = indexPage.GetFilesHandler()->GetName(1);
		string          fileName3 = indexPage.GetFilesHandler()->GetName(2);
		string          fileName4 = indexPage.GetFilesHandler()->GetName(3);
		string          fileName5 = indexPage.GetFilesHandler()->GetName(4);
		string          fileName6 = indexPage.GetFilesHandler()->GetName(5);
		string          fileName7 = indexPage.GetFilesHandler()->GetName(6);
		string          fileName8 = indexPage.GetFilesHandler()->GetName(7);
		string          fileName9 = indexPage.GetFilesHandler()->GetName(8);
                string          fileNameLocal;
                FILE            *f;
                ostringstream   ost, ost1;
                int             affected;
                string          id;


                if(!fileName9.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName9;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(8) != NULL) && (indexPage.GetFilesHandler()->GetSize(8) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(8), indexPage.GetFilesHandler()->GetSize(8), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName8.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName8;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(7) != NULL) && (indexPage.GetFilesHandler()->GetSize(7) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(7), indexPage.GetFilesHandler()->GetSize(7), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName7.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName7;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(6) != NULL) && (indexPage.GetFilesHandler()->GetSize(6) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(6), indexPage.GetFilesHandler()->GetSize(6), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName6.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName6;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(5) != NULL) && (indexPage.GetFilesHandler()->GetSize(5) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(5), indexPage.GetFilesHandler()->GetSize(5), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName5.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName5;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(4) != NULL) && (indexPage.GetFilesHandler()->GetSize(4) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(4), indexPage.GetFilesHandler()->GetSize(4), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName4.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName4;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(3) != NULL) && (indexPage.GetFilesHandler()->GetSize(3) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(3), indexPage.GetFilesHandler()->GetSize(3), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName3.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName3;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(2) != NULL) && (indexPage.GetFilesHandler()->GetSize(2) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(2), indexPage.GetFilesHandler()->GetSize(2), 1, f);
                    }
                    fclose(f);
                }

                if(!fileName2.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName2;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(1) != NULL) && (indexPage.GetFilesHandler()->GetSize(1) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(1), indexPage.GetFilesHandler()->GetSize(1), 1, f);
                    }
                    fclose(f);
                }
		
                if(!fileName1.empty())
                {
                    fileNameLocal = IMAGE_WORK_DIRECTORY + fileName1;
                        {
                            CLog        log;
                            log.Write(ERROR, "writing file:", fileNameLocal.c_str());
                        }
                    f = fopen(fileNameLocal.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog        log;
                            log.Write(ERROR, "error writing file:", fileNameLocal.c_str());
                        }
                        throw CException("error writing file into server");
                    }
                    if((indexPage.GetFilesHandler()->Get(0) != NULL) && (indexPage.GetFilesHandler()->GetSize(0) > 0))
                    {
                        fwrite(indexPage.GetFilesHandler()->Get(0), indexPage.GetFilesHandler()->GetSize(0), 1, f);
                    }
                    fclose(f);
                }

		ost.str("");
		ost << "insert into `works` set \
			`parentID` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("parentID")).c_str() << "\", \
			`order` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("order")).c_str() << "\", \
			`name` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "\", \
			`clientID` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("clients")).c_str() << "\", \
			`isFavorite` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("isFavorite")).c_str() << "\", \
			`dataCreate` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("data")).c_str() << "\", \
			`content` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str() << "\", ";
		if(!fileName1.empty()) ost << "`image1` = \"" << fileName1 << "\", ";
		if(!fileName2.empty()) ost << "`image2` = \"" << fileName2 << "\", ";
		if(!fileName3.empty()) ost << "`image3` = \"" << fileName3 << "\", ";
		if(!fileName4.empty()) ost << "`image4` = \"" << fileName4 << "\", ";
		if(!fileName5.empty()) ost << "`image5` = \"" << fileName5 << "\", ";
		if(!fileName6.empty()) ost << "`image6` = \"" << fileName6 << "\", ";
		if(!fileName7.empty()) ost << "`image7` = \"" << fileName7 << "\", ";
		if(!fileName8.empty()) ost << "`image8` = \"" << fileName8 << "\", ";
		if(!fileName9.empty()) ost << "`image9` = \"" << fileName9 << "\", ";
		ost << "`isShow` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("isShow")).c_str() << "\"";
		db.Query(ost.str());
		ost.str("");
		ost << "select * from `works` where \
			`parentID` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("parentID")).c_str() << "\" and \
			`order` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("order")).c_str() << "\" and \
			`name` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "\" and \
			`clientID` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("clients")).c_str() << "\" and \
			`isFavorite` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("isFavorite")).c_str() << "\" and \
			`dataCreate` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("data")).c_str() << "\" and \
			`content` = \"" << removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str() << "\"";
		affected = db.Query(ost.str());
		if(affected <= 0)
		{
			CLog	log;
		
			log.Write(ERROR, "error in inserting work into DB");
			throw CException("error in inserting work into DB");
		}
		id = db.Get(0, "id");
		for(int i = 0; i < 100000; i++)
		{
			ost.str("");
			ost << "type" << i;
			if((indexPage.GetVarsHandler()->Get(ost.str())).length() > 0)
			{
				ost.str("");
				ost << "insert into `typesWorks` set \
					`typeID` = '" << i << "', \
					`workID` = '" << id << "'";
				db.Query(ost.str());
			}
		}

		content = "Работа добавлена.";
		indexPage.RegisterVariableForce("content", content);
	}


//-------------------------- Book administrating
    if(act == "addbook")
    {
	indexPage.SetTemplateFile("templates/adminbookadd.htmlt");
    }
    if(act == "bookaddsubmit")
    {
	ostringstream	ramkaName;
	ostringstream	thumbnailName;
        string          fileName = indexPage.GetFilesHandler()->GetName(0);
        string          fileName1 = indexPage.GetFilesHandler()->GetName(1);
	FILE		*f;
	
	memset(query, 0, sizeof(query));

	snprintf(query, sizeof(query) - 2, "insert into `books` (`data`, `title`, `content_brief`, `content`, `author`, `rating`, `category`, `file`, `file2`) values(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")", 
	removeQuotas(indexPage.GetVarsHandler()->Get("data")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("title")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("content_brief")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("author")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("rating")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("category")).c_str(), 
	indexPage.GetFilesHandler()->GetName(0).c_str(),
	indexPage.GetFilesHandler()->GetName(1).c_str());
	db.Query(query);
	
        if(!fileName.empty())
	{
	    fileName = IMAGE_BOOK_DIRECTORY + fileName;
	    f = fopen(fileName.c_str(), "w");
	    if(f == NULL)
	    {
	        {
	            CLog    log;
	            log.Write(ERROR, "error writing file:", fileName.c_str());
		}
	        throw CException("error writing file into server");
	    }
	    if((indexPage.GetFilesHandler()->Get(0) != NULL) && (indexPage.GetFilesHandler()->GetSize(0) > 0))
	    {
	        fwrite(indexPage.GetFilesHandler()->Get(0), indexPage.GetFilesHandler()->GetSize(0), 1, f);
	    }
	    fclose(f);
	}
	
        if(!fileName1.empty())
	{
	    fileName = IMAGE_BOOK_DIRECTORY + fileName1;
	    f = fopen(fileName.c_str(), "w");
	    if(f == NULL)
	    {
	        {
	            CLog    log;
	            log.Write(ERROR, "error writing file:", fileName.c_str());
		}
	        throw CException("error writing file into server");
	    }
	    if((indexPage.GetFilesHandler()->Get(1) != NULL) && (indexPage.GetFilesHandler()->GetSize(1) > 0))
	    {
	        fwrite(indexPage.GetFilesHandler()->Get(1), indexPage.GetFilesHandler()->GetSize(1), 1, f);
	    }
	    fclose(f);
	}
	
	indexPage.RegisterVariableForce("content", "add success");
    }
    if(act == "listbooks")
    {
	string		isMain;

	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select `id`,`data`,`title`,`isMain` from books order by `data` desc;");
	affected = db.Query(query);

	content = "<table>\n";
	for(int i = 0; i < affected; i++)
	{
	    content += "<tr>";
	    content += "<td class=blue>\n";

	    content += db.Get(i, "data");

	    content += "</td><td>";

	    content += "<a href=/cgi-bin/admin/parts.cgi?act=delbook&id=";
            content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += ">";
	    content += "<img src=/images/button_drop.png border=0></a>";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=editbook&id=";
	    content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += "><img src=/images/button_edit.png border=0> ";
	    content += DeleteHTML(db.Get(i, "title"));
	    content += "</a>";
	    content += "</td>";
	    content += "</tr>\n";
	}
	content += "</table>\n";
        indexPage.RegisterVariableForce("content", content.c_str());
    }
    if(act == "editbook")
    {
	ostringstream	ost;
	string		content, content_brief, title, data, id, image, isMain, isApproved;

	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "select * from books where id=%s", id.c_str());
	affected = db.Query(query);
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting book from db");
		throw CException("error getting book from db");
	}

	content = db.Get(0, "content");
	content_brief = db.Get(0, "content_brief");
	title = db.Get(0, "title");
	data = db.Get(0, "data");
	id = db.Get(0, "id");
	image = db.Get(0, "file");
	isMain = db.Get(0, "isMain");
        
	indexPage.RegisterVariableForce("id", id);
        indexPage.RegisterVariableForce("data", data);
        indexPage.RegisterVariableForce("title", title);
        indexPage.RegisterVariableForce("content_brief", content_brief);
        indexPage.RegisterVariableForce("content", content);
        indexPage.RegisterVariableForce("image", image);
        indexPage.RegisterVariableForce("author", db.Get(0, "author"));
        indexPage.RegisterVariableForce("rating", db.Get(0, "rating"));
        indexPage.RegisterVariableForce("category", db.Get(0, "category"));
        indexPage.RegisterVariableForce("image2", db.Get(0, "file2"));
	indexPage.RegisterVariableForce("isMain", ((isMain == "Y") ? "checked" : ""));
	
	ost.str("");
	ost << "select * from books_comment where `parentID`=\"" << id << "\" order by `date` ASC";
	affected = db.Query(ost.str());
	if(affected > 0)
	{
	    ost.str("");
	    ost << "<table>";
	    for(int i = 0; i < affected; i++)
	    {
		isApproved = db.Get(i, "isApproved");
		ost << "<tr><td>" << db.Get(i, "date") << " </td>\
		<td><a href=\"/cgi-bin/admin/parts.cgi?act=edit_book_comment&rand=" << GetRandom(10) << "&id=" << db.Get(i, "id") << "\">" << db.Get(i, "name") << "</a></td><td>" << ((isApproved == "N") ? "no" : "") << "</td>\
		<td><a href=/cgi-bin/admin/parts.cgi?act=comment_book_del&id=" << db.Get(i, "id") << "&rnd=" << GetRandom(10) << "><img src=/images/button_drop.png border=0></a></td></tr>";
	    }
	    ost << "</table>";
	    indexPage.RegisterVariableForce("comments", ost.str());
	}
	indexPage.SetTemplateFile("templates/adminbookedit.htmlt");
    }
    if(act == "bookeditsubmit")
    {
	string		isMain;

	isMain = removeQuotas(indexPage.GetVarsHandler()->Get("isMain"));
	if(isMain == "Y")
		db.Query("update `books` set `isMain`='N'");
	else
		isMain = "N";

	memset(query, 0, sizeof(query));

	snprintf(query, sizeof(query) - 2, "update `books` set `data`=\"%s\", `title`=\"%s\", `content_brief`=\"%s\", `content`=\"%s\", `file`=\"%s\", `file2`=\"%s\", `category`=\"%s\", `author`=\"%s\", `rating`=\"%s\", `isMain`=\"%s\" where `id`='%s'", 
	removeQuotas(indexPage.GetVarsHandler()->Get("data")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("title")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("content_brief")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("image")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("image2")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("category")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("author")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("rating")).c_str(), 
	removeQuotas(indexPage.GetVarsHandler()->Get("isMain")).c_str(), 
	indexPage.GetVarsHandler()->Get("id").c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "update success");
    }
    if(act == "delbook")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "delete from books where `id`=\"%s\"", removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str()); 
	db.Query(query);
	indexPage.RegisterVariableForce("content", "delete success");
    }
    if(act == "comment_book_del")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "delete from books_comment where `id`=\"%s\"", removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str()); 
	db.Query(query);
	indexPage.RegisterVariableForce("content", "delete comment success");
    }

    if(act == "edit_book_comment")
    {
	ostringstream	ost;
	string		content, parentID, name, data, id, image, isMain, isApproved;

	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "select * from books_comment where id=%s", id.c_str());
	affected = db.Query(query);
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting book from db");
		throw CException("error getting book from db");
	}

	content = db.Get(0, "content");
	data = db.Get(0, "date");
	id = db.Get(0, "id");
	name = db.Get(0, "name");
	parentID = db.Get(0, "parentID");
	isApproved = db.Get(0, "isApproved");
        
	indexPage.RegisterVariableForce("id", id);
        indexPage.RegisterVariableForce("data", data);
        indexPage.RegisterVariableForce("name", name);
        indexPage.RegisterVariableForce("parentID", parentID);
        indexPage.RegisterVariableForce("content", content);
        indexPage.RegisterVariableForce("isApproved", ((isApproved == "N") ? "" : "checked"));
	
	indexPage.SetTemplateFile("templates/book_comment_edit.htmlt");
    }
    if(act == "book_comment_edit_submit")
    {
	string		isApprove;

	isApprove = removeQuotas(indexPage.GetVarsHandler()->Get("isApproved"));
	if(isApprove != "Y")
		isApprove = "N";

	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "update `books_comment` set `date`=\"%s\", `name`=\"%s\", `content`=\"%s\", `isApproved`=\"%s\" where `id`='%s'", 
	    removeQuotas(indexPage.GetVarsHandler()->Get("data")).c_str(), 
	    removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str(), 
	    removeQuotas(indexPage.GetVarsHandler()->Get("content")).c_str(), 
	    isApprove.c_str(), 
	    indexPage.GetVarsHandler()->Get("id").c_str());
	db.Query(query);
	indexPage.RegisterVariableForce("content", "update success");
    }
//-------------------------- End book administrating

//-------------------------- blog_keyword editing
    if(act == "blog_keyword_list")
    {
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query) - 2, "select * from `blog_keywords` order by `name` asc;");
	affected = db.Query(query);

	content = "<table>\n";
	for(int i = 0; i < affected; i++)
	{
	    content += "<tr>";
	    content += "<td class=blue>\n";
	    content += db.Get(i, "id");
	    content += "</td><td>";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=blog_keyword_del&id=";
            content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += ">";
	    content += "<img src=/images/button_drop.png border=0></a> ";
	    content += "<a href=/cgi-bin/admin/parts.cgi?act=blog_keyword_edit&id=";
	    content += db.Get(i, "id");
	    content += "&rnd=";
	    content += GetRandom(10);
	    content += "><img src=/images/button_edit.png border=0> ";
	    content += DeleteHTML(db.Get(i, "name"));
	    content += "</a>";


	    content += "</td>";
	    content += "</tr>\n";
	}
	content += "</table>\n";
        indexPage.RegisterVariableForce("content", content.c_str());
    }
    if(act == "blog_keyword_edit")
    {
	ostringstream	ost;
	string		name, id;

	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	ost << "select * from `blog_keywords` where id=" << id;
	affected = db.Query(ost.str());
	if(affected == 0)
	{
		CLog	log;
		
		log.Write(ERROR, "error getting parts from db");
		throw CException("error getting parts from db");
	}
	ost.str("");

	name = db.Get(0, "name");

	indexPage.RegisterVariableForce("name", name);
	indexPage.RegisterVariableForce("description", db.Get(0, "description"));
	indexPage.RegisterVariableForce("id", id);

	indexPage.SetTemplateFile("templates/blog_keyword_edit.htmlt");
    }
    if(act == "blog_keyword_edit_submit")
    {
    	ostringstream	ost;

	ost << "update `blog_keywords` set \
	`name`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "\" , \
	`description`=\"" << removeQuotas(indexPage.GetVarsHandler()->Get("description")).c_str() << "\" \
	where `id`='" << removeQuotas(indexPage.GetVarsHandler()->Get("id")).c_str() << "'";
	db.Query(ost.str());
	ost.str("");
	ost << " Редактирование ключевых слов прошло успешно.";
	indexPage.RegisterVariableForce("content", ost.str());
	
//	Makehtaccess(&db);
    }
    if(act  == "blog_keyword_del")
    {
	ostringstream	ost;
	
	id = indexPage.GetVarsHandler()->Get("id");
	if(id.length() == 0)
	{
	    throw CException("parameter ID was missing");
	}
	ost << "delete from `blog_keywords` where id=" << id;
	affected = db.Query(ost.str());
	content = "Удаление ключевых слов прошло успешно";
	indexPage.RegisterVariableForce("content", content);
    }
    if(act == "blog_keyword_add")
    {
	indexPage.SetTemplateFile("templates/blog_keyword_add.htmlt");
    }
    if(act == "blog_keyword_add_submit")
    {
    	ostringstream	ost;

	ost.str("");
	ost << "insert into `blog_keywords` (`name`,`description`) VALUES ('" <<
	removeQuotas(indexPage.GetVarsHandler()->Get("name")).c_str() << "',\
        '" << removeQuotas(indexPage.GetVarsHandler()->Get("description")).c_str() << "')";
	db.Query(ost.str());
	content = " Добавление ключевых слов успешно.";
	indexPage.RegisterVariableForce("content", content);

	indexPage.SetTemplateFile("templates/adminindex.htmlt");

//	Makehtaccess(&db);
    }
//-------------------------- End blog keywords parts



//-------------------------- Picture editing    
    if(act == "addpics")
    {
        indexPage.SetTemplateFile("templates/adminpicsadd.htmlt");
    }
    
    indexPage.RegisterVariableForce("rand", GetRandom(10).c_str());
    indexPage.OutTemplate();

    }
    catch(CException c)
    {
        indexPage.SetTemplateFile("templates/error.htmlt");
        indexPage.RegisterVariable("content", c.GetReason().c_str());
        indexPage.OutTemplate();
	return(-1);
    }

    return(0);
}

