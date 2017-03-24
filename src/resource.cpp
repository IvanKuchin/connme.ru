#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <list>
#include <map>
#include <magick/api.h>

#include "ccgi.h"
#include "cvars.h"
#include "clog.h"
#include "cmysql.h"
#include "localy.h"
#include "cexception.h"

string GetRandom(int len)
{
        string  result;
        int     i;

        for(i = 0; i < len; i++)
        {
                result += (char)('0' + (int)(rand()/(RAND_MAX + 1.0) * 10));
        }

        return result;
}

string GetRealPath(string path)
{
    map<string, string>		appender;
    string			result, first_cat;
    
    appender["images"] = LIST_DIRECTORY;
    
    if(path.find("..") != string::npos)
    {
	path = "images";
    }

    if(path.find("/") != string::npos)
        first_cat = path.substr(0, path.find("/"));

    result = appender[first_cat];

    if(!result.empty())
        result += path;
    else
    	throw CException("there is no shuch resources");

    {
        CLog    log;
        log.Write(ERROR, "result path: ", result.c_str());
    }

    return result;
}

bool ReservedDir(string dirName)
{
    if(dirName[0] == '.') return true;

    return false;
}

string GetFormattedListingFiles(string path, string execPrefix, string readPrefix)
{
    DIR*		hDir;
    struct dirent*	dirEnt;
    string		result, parentDir;
    list<string>	files;
    map<string, string>	files2Links;
    list<string>::iterator	i;


    {
        CLog    log;

        log.Write(ERROR, "open directory: ", path.c_str());
    }



    hDir = opendir(path.c_str());
    if(hDir == NULL)
    {
	CLog	log;

	log.Write(ERROR, "error open directory: ", path.c_str());

	throw CException("error open directory");
    }

    result = "<table>";
    while((dirEnt = readdir(hDir)) != NULL)
    {
	files.push_back(dirEnt->d_name);

	if((string)(dirEnt->d_name) == "..")
	{
	    string temp(execPrefix);
	    temp.erase(temp.length() - 1);

	    if(temp.rfind("/") != string::npos)
	    {
	        result = "<tr><td>";
		parentDir = temp.erase(temp.rfind("/"));
		result += "<img src=/images/folder.gif>";
		result += "</td><td>";
		result += "<a href=\"/cgi-bin/admin/resource.cgi?path=";
		result += parentDir;
		result += "/\">..</a>";
		result += "</td></tr>";
		
		files2Links[dirEnt->d_name] = result;
	    }
	    else
	    {
		result += "";
	    }
	}
	if(ReservedDir(dirEnt->d_name)) continue;

	if(dirEnt->d_type == DT_DIR)
	{
	    result = "<tr><td><img src=/images/folder.gif>";
	    result += "</td><td>";
	    result += "<a href=\"/cgi-bin/admin/resource.cgi?path=";
	    result += execPrefix;
	    result += dirEnt->d_name;
	    result += "/\">";
	    result += dirEnt->d_name;
	    result += "</a></td></tr>";
	    
	    files2Links[dirEnt->d_name] = result;

	}
	else
	{
	    result = "<tr><td></td><td>";
	    result += "<a href=\"";
	    result += readPrefix;
	    result += dirEnt->d_name;
	    result += "\">";
	    result += dirEnt->d_name;
	    result += "</a></td></tr>";

	    files2Links[dirEnt->d_name] = result;
	}
    }
    result += "</table>";

    closedir(hDir);

    files.sort();

    result = "<table>";
    
    for(i = files.begin(); i != files.end(); ++i)
    {
	result += "<tr>";
	result += files2Links[*i];
	result += "</tr>";
    }
    result += "</table>";

    return result;
}

string RebuildPrefix(string str1, string str2)
{
    string	result;

    result = str1;
    result += str2;

//    while(result.find("/") != string::npos)
//	result.replace(result.find("/"), 1, "\\");

    return result;
}

int main()
{
    CCgi	indexPage(EXTERNAL_TEMPLATE);
    string	part, path, fsPath;
    string	content, act;
    struct	timeval	tv;

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

	path = indexPage.GetVarsHandler()->Get("path");
	act = indexPage.GetVarsHandler()->Get("act");

	if(act.empty())
		act = "list";

	if(act == "list")
	{
		if(!path.empty())
		{
			fsPath = GetRealPath(path);
			if(!fsPath.empty())
			{
				if(RebuildPrefix(BROWSER_FILE_PREFIX, path) != "")
					content = GetFormattedListingFiles(fsPath, path, RebuildPrefix(BROWSER_FILE_PREFIX, path));

			}
			indexPage.RegisterVariable("content", content.c_str());
		}
	}
	if(act == "submitpic")
	{
		string	fileName = indexPage.GetFilesHandler()->GetName(0);
		string	waterFlag = indexPage.GetVarsHandler()->Get("watermark");
		ExceptionInfo	exception;
		FILE	*f;

		if(!fileName.empty())
		{
			fileName = IMAGE_DIRECTORY + fileName;
			f = fopen(fileName.c_str(), "w");
			if(f == NULL)
			{
				{
					CLog	log;
					log.Write(ERROR, "error writing file:", fileName.c_str());
				}
				throw CException("error writing file into server");
			}
			if((indexPage.GetFilesHandler()->Get(0) != NULL) && (indexPage.GetFilesHandler()->GetSize(0) > 0))
			{
				fwrite(indexPage.GetFilesHandler()->Get(0), indexPage.GetFilesHandler()->GetSize(0), 1, f);
			}
			fclose(f);
			indexPage.RegisterVariable("content", "upload picture complete");
			if(waterFlag.length() > 0)
			{
				string		fileNameWater;
				Image		*image1, *image2;
				ImageInfo	*image1_info, *image2_info;

				fileNameWater = IMAGE_DIRECTORY;
				fileNameWater += "water.png";

				InitializeMagick(NULL);
				GetExceptionInfo(&exception);
				image1_info = CloneImageInfo((ImageInfo *) NULL);
				image2_info = CloneImageInfo((ImageInfo *) NULL);
				(void) strcpy(image1_info->filename, fileName.c_str());
				(void) strcpy(image2_info->filename, fileNameWater.c_str());
				image1 = ReadImage(image1_info, &exception);
				if (exception.severity != UndefinedException)
				{
					CLog	log;
		
					CatchException(&exception);
					log.Write(ERROR, "error in managing original picture");
					throw CExceptionHTML("image error");
				}
				image2 = ReadImage(image2_info, &exception);
				if (exception.severity != UndefinedException)
				{
					CLog	log;
		
					CatchException(&exception);
					log.Write(ERROR, "error in managing watermakr picture");
					throw CExceptionHTML("image error");
				}
				if(image1 == (Image *) NULL)
				{
					CLog	log;
		
					log.Write(ERROR, "error in managing original picture");
					throw CExceptionHTML("image error");
				}
				if(image2 == (Image *) NULL)
				{
					CLog	log;
		
					log.Write(ERROR, "error in managing watermark picture");
					throw CExceptionHTML("image error");
				}

				CompositeImage(image1, OverCompositeOp, image2, image1->columns - image2->columns, image1->rows - image2->rows);
		
				WriteImage(image1_info, image1);
				DestroyImage(image1);
				DestroyImage(image2);
				DestroyImageInfo(image1_info);
				DestroyImageInfo(image2_info);
				DestroyExceptionInfo(&exception);
				DestroyMagick();

			}
		}
		else
		{
			{
				CLog	log;
				log.Write(ERROR, "You forget to fill the field 'name of picture on server'");
			}
			throw CException("You forget to fill the field 'name of picture on server'");
		}
	}
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

