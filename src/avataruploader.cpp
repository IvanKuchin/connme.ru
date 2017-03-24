#include <sstream>
#include <stdexcept>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <Magick++.h>

#include "avataruploader.h"	 
#include "localy.h"
#include "ccgi.h"
#include "cvars.h"
#include "clog.h"
#include "cmysql.h"
#include "cexception.h"
#include "cactivator.h"
#include "cforum.h"
#include "cmenu.h"
#include "cuser.h"
#include "cmail.h"
#include "ccatalog.h"
#include "cstatistics.h"
#include "utilities.h"
/*
bool ImageSaveAsJpg (const string src, const string dst)
{
    {
        CLog    log;
        ostringstream   ost;

        ost.str("");
        ost << "ImageSaveAsJpg (" << src << ", " << dst << "): enter";
        log.Write(DEBUG, ost.str());
    }

#ifdef IMAGEMAGICK_ACTIVATE
    // Construct the image object. Seperating image construction from the
    // the read operation ensures that a failure to read the image file
    // doesn't render the image object useless.
    Magick::Image image;
    try {
        // Read a file into image object
        image.read( src );

        // Write the image to a file
        image.write( dst );
    }
    catch( Magick::Exception &error_ )
    {
        {
            CLog    log;
            ostringstream   ost;

            ost.str("");
            ost << "ImageSaveAsJpg (" << src << ", " << dst << "): exception in read/write operation [" << error_.what() << "]";
            log.Write(DEBUG, ost.str());
        }
        return false;
    }
    {
        CLog    log;
        ostringstream   ost;

        ost.str("");
        ost << "ImageSaveAsJpg (" << src << ", " << dst << "): image has been successfully converted to .jpg format";
        log.Write(DEBUG, ost.str());
    }
    return true;
#endif

#ifndef IMAGEMAGICK_ACTIVATE
    {
        CLog    log;
        ostringstream   ost;

        ost.str("");
        ost << "ImageSaveAsJpg (" << src << ", " << dst << "): simple file coping cause ImageMagick++ is not activated";
        log.Write(DEBUG, ost.str());
    }
    CopyFile(src, dst);
    return  true;
#endif
}
*/
bool ImageSaveAsJpg (const string src, const string dst)
{
    {
        CLog    log;
        ostringstream   ost;

        ost.str("");
        ost << "ImageSaveAsJpg (" << src << ", " << dst << "): enter";
        log.Write(DEBUG, ost.str());
    }

#ifdef IMAGEMAGICK_ACTIVATE
    // Construct the image object. Seperating image construction from the
    // the read operation ensures that a failure to read the image file
    // doesn't render the image object useless.
    try {
        Magick::Image           image;
        Magick::OrientationType imageOrientation;
        Magick::Geometry        imageGeometry;

        // Read a file into image object
        image.read( src );

        imageGeometry = image.size();
        imageOrientation = image.orientation();

        {
            CLog    log;
            ostringstream   ost;

            ost.str("");
            ost << "ImageSaveAsJpg (" << src << ", " << dst << "): imageOrientation = " << imageOrientation << ", xRes = " << imageGeometry.width() << ", yRes = " << imageGeometry.height();
            log.Write(DEBUG, ost.str());
        }

        if(imageOrientation == Magick::TopRightOrientation) image.flop();
        if(imageOrientation == Magick::BottomRightOrientation) image.rotate(180);
        if(imageOrientation == Magick::BottomLeftOrientation) { image.flop(); image.rotate(180); }
        if(imageOrientation == Magick::LeftTopOrientation) { image.flop(); image.rotate(-90); }
        if(imageOrientation == Magick::RightTopOrientation) image.rotate(90);
        if(imageOrientation == Magick::RightBottomOrientation) { image.flop(); image.rotate(90); }
        if(imageOrientation == Magick::LeftBottomOrientation) image.rotate(-90);

        if((imageGeometry.width() > AVATAR_MAX_WIDTH) || (imageGeometry.height() > AVATAR_MAX_HEIGHT))
        {
            int   newHeight, newWidth;
            if(imageGeometry.width() >= imageGeometry.height())
            {
                newWidth = AVATAR_MAX_WIDTH;
                newHeight = newWidth * imageGeometry.height() / imageGeometry.width();
            }
            else
            {
                newHeight = AVATAR_MAX_HEIGHT;
                newWidth = newHeight * imageGeometry.width() / imageGeometry.height();
            }

            image.resize(Magick::Geometry(newWidth, newHeight, 0, 0));
        }

        // Write the image to a file
        image.write( dst );
    }
    catch( Magick::Exception &error_ )
    {
        {
            CLog    log;
            ostringstream   ost;

            ost.str("");
            ost << "ImageSaveAsJpg (" << src << ", " << dst << "): exception in read/write operation [" << error_.what() << "]";
            log.Write(DEBUG, ost.str());
        }
        return false;
    }
    {
        CLog    log;
        ostringstream   ost;

        ost.str("");
        ost << "ImageSaveAsJpg (" << src << ", " << dst << "): image has been successfully converted to .jpg format";
        log.Write(DEBUG, ost.str());
    }
    return true;
#endif

#ifndef IMAGEMAGICK_ACTIVATE
    {
        CLog    log;
        ostringstream   ost;

        ost.str("");
        ost << "ImageSaveAsJpg (" << src << ", " << dst << "): simple file coping cause ImageMagick++ is not activated";
        log.Write(DEBUG, ost.str());
    }
    CopyFile(src, dst);
    return  true;
#endif
}

int main()
{
    CStatistics     appStat;  // --- CStatistics must be firts statement to measure end2end param's
    CCgi            indexPage(EXTERNAL_TEMPLATE);
    CUser           user;
    string          action, partnerID;
    CMysql          db;
    struct timeval  tv;
    ostringstream   ostJSONResult/*(static_cast<ostringstream&&>(ostringstream() << "["))*/;

    gettimeofday(&tv, NULL);
    srand(tv.tv_sec * tv.tv_usec * 100000);
    ostJSONResult.clear();
	    
    try
    {
    	indexPage.ParseURL();
    	indexPage.AddCookie("lng", "ru", "", "", "/");

    	if(!indexPage.SetTemplate("index.htmlt"))
    	{
    		CLog    log;

    		log.Write(ERROR, "template file was missing");
    		throw CException("Template file was missing");
    	}

    	if(db.Connect(DB_NAME, DB_LOGIN, DB_PASSWORD) < 0)
    	{
    		CLog	log;

    		log.Write(ERROR, "Can not connect to mysql database");
    		throw CExceptionHTML("MySql connection");
    	}

    	indexPage.SetDB(&db);

    #ifdef MYSQL_4
    	db.Query("set names cp1251");
    #endif
    	action = indexPage.GetVarsHandler()->Get("action");
    	{
    		CLog	log;

    		log.Write(DEBUG, "avataruploader: main(): action = ", action);
    	}



    // ------------ generate common parts
    	{
    		ostringstream			query, ost1, ost2;
    		string				partNum;
    		map<string, string>		menuHeader;
    		map<string, string>::iterator	iterMenuHeader;
    		string				content;
    		Menu				m;

    		indexPage.RegisterVariableForce("rand", GetRandom(10));
    		indexPage.RegisterVariableForce("random", GetRandom(10));
    		indexPage.RegisterVariableForce("style", "style.css");


#ifdef IMAGEMAGICK_ACTIVATE
            Magick::InitializeMagick(NULL);
#endif
    /*
    		m.SetDB(&db);
    		m.Load();
    		GenerateAndRegisterMenuV("1", &m, &db, &indexPage);
    */

    //------- Cleanup data
    		{
    			ostringstream	ost;

    			ost.str("");
    			ost << "delete from captcha where `timestamp`<=(NOW()-INTERVAL " << SESSION_LEN << " MINUTE)";
    			db.Query(ost.str());
    		}

    //------- Generate session
    		{
    			string			lng, sessidHTTP;
    			ostringstream	ost;


    			sessidHTTP = indexPage.SessID_Get_FromHTTP();
    			if(sessidHTTP.length() < 5) {
    				{
    					CLog	log;
    					log.Write(DEBUG, "main(): session cookie is not exist, generating new session.");
    				}
    				sessidHTTP = indexPage.SessID_Create_HTTP_DB();
    				if(sessidHTTP.length() < 5) {
    					CLog	log;
    					log.Write(ERROR, "main(): error in generating session ID");
    					throw CExceptionHTML("session can't be created");
    				}
    			} 
    			else {
    				if(indexPage.SessID_Load_FromDB(sessidHTTP)) 
    				{
    					if(indexPage.SessID_CheckConsistency()) 
    					{
    						if(indexPage.SessID_Update_HTTP_DB()) 
    						{
    							indexPage.RegisterVariableForce("loginUser", "");

    							if(indexPage.SessID_Get_UserFromDB() != "Guest") {
    								user.SetDB(&db);
    								user.GetFromDBbyEmail(indexPage.SessID_Get_UserFromDB());
    								indexPage.RegisterVariableForce("loginUser", indexPage.SessID_Get_UserFromDB());
    								{
    									CLog	log;
    									ostringstream	ost;

    									ost << "int main(void): user [" << user.GetLogin() << "] logged in";
    									log.Write(DEBUG, ost.str());
    								}
    							}
                                else
                                {
                                    indexPage.RegisterVariableForce("ErrorDescription", "ERROR user not signed in (Guest user)");

                                    {
                                        CLog    log;
                                        log.Write(ERROR, "main(): ERROR user not signed in (Guest user)");
                                    }
                                }
                            }
                            else
                            {
                                indexPage.RegisterVariableForce("ErrorDescription", "ERROR update session in HTTP or DB failed");

                                {
                                    CLog    log;
                                    log.Write(ERROR, "main(): ERROR update session in HTTP or DB failed");
                                }
                            }
                        }
                        else {
                            indexPage.RegisterVariableForce("ErrorDescription", "ERROR session consistency check failed");

							{
        						CLog	log;
        						log.Write(ERROR, "main(): ERROR session consistency check failed");
                            }
    					}
    				}
    				else 
    				{
    					ostringstream	ost;

    					{
    						CLog	log;
    						log.Write(DEBUG, "main(): cookie session and DB session is not equal. Need to recreate session");
    					}

    					ost.str("");
    					ost << "/?rand=" << GetRandom(10);

    					if(!indexPage.Cookie_Expire()) {
    						CLog	log;
    						log.Write(ERROR, "main(): Error in session expiration");
    					} // --- if(!indexPage.Cookie_Expire())
    					indexPage.Redirect(ost.str().c_str());
    				} // --- if(indexPage.SessID_Load_FromDB(sessidHTTP)) 
    			} // --- if(sessidHTTP.length() < 5)




    		}
    //------- End generate session

    	}
    // ------------ end generate common parts


        if((user.GetID().length()) && (user.GetName() != "Guest"))
        {

        	for(int filesCounter = 0; filesCounter < indexPage.GetFilesHandler()->Count(); filesCounter++)
        	{
        		FILE	        *f;
        		int		        folderID = (int)(rand()/(RAND_MAX + 1.0) * AVATAR_NUMBER_OF_FOLDERS) + 1;
        		string	        filePrefix = GetRandom(20);
        		string	        file2Check, tmpFile2Check, tmpImageJPG, fileName, fileExtention;
                ostringstream   ost;
                int             affected;

                // --- Check the number of files for that user
                ost.clear();
                ost << "select id from `users_avatars` where `userid`='" << user.GetID() << "';";
                affected = db.Query(ost.str());
                if(affected < 4) // --- 1 text avatar and 3 image avatar
                { 
            		{
            			CLog	log;
            			ostringstream	ost;

            			ost << "int main(void): number of files POST'ed = " << indexPage.GetFilesHandler()->Count();
            			log.Write(DEBUG, ost.str());
            		}

            		if(indexPage.GetFilesHandler()->GetSize(filesCounter) > AVATAR_MAX_FILE_SIZE) 
            		{
        				CLog        	log;
        				ostringstream	ost;

        				ost.str("");
        				ost << "int main(void): ERROR avatar file [" << indexPage.GetFilesHandler()->GetName(filesCounter) << "] size exceed permited maximum: " << indexPage.GetFilesHandler()->GetSize(filesCounter) << " > " << AVATAR_MAX_FILE_SIZE;

        				log.Write(ERROR, ost.str());
                        throw CExceptionHTML("avatar file size exceed", indexPage.GetFilesHandler()->GetName(filesCounter));
            		}

            		//--- check avatar file existing
            		do
            		{
            			ostringstream	ost;
                        string          tmp;
                        std::size_t  foundPos;

            			folderID = (int)(rand()/(RAND_MAX + 1.0) * AVATAR_NUMBER_OF_FOLDERS) + 1;
            			filePrefix = GetRandom(20);
                        tmp = indexPage.GetFilesHandler()->GetName(filesCounter);

                        if((foundPos = tmp.rfind(".")) != string::npos) 
                        {
                            fileExtention = tmp.substr(foundPos, tmp.length() - foundPos);
                        }
                        else
                        {
                            fileExtention = ".jpg";
                        }

            			ost.str("");
            			ost << IMAGE_AVATAR_DIRECTORY << "/avatars" << folderID << "/" << filePrefix << ".jpg";
                        file2Check = ost.str();

                        ost.str("");
                        ost << "/tmp/tmp_" << filePrefix << fileExtention;
                        tmpFile2Check = ost.str();

                        ost.str("");
                        ost << "/tmp/" << filePrefix << ".jpg";
                        tmpImageJPG = ost.str();
            		} while(isFileExists(file2Check) || isFileExists(tmpFile2Check) || isFileExists(tmpImageJPG));



            		{
            			CLog	log;
            			ostringstream	ost;

            			ost << "int main(void): Save file to /tmp for checking of image validity [" << tmpFile2Check << "]";
            			log.Write(DEBUG, ost.str());
            		}

                    // --- Save file to "/tmp/" for checking of image validity
                    f = fopen(tmpFile2Check.c_str(), "w");
                    if(f == NULL)
                    {
                        {
                            CLog            log;

                            log.Write(ERROR, "int main(void): ERROR writing file:", tmpFile2Check.c_str());
                            throw CExceptionHTML("avatar file write error", indexPage.GetFilesHandler()->GetName(filesCounter));
                        }
                    }
                    fwrite(indexPage.GetFilesHandler()->Get(filesCounter), indexPage.GetFilesHandler()->GetSize(filesCounter), 1, f);
                    fclose(f);

                    if(ImageSaveAsJpg(tmpFile2Check, tmpImageJPG))
                    {

                        {
                            CLog    log;
                            ostringstream   ost;

                            ost << "int main(void): choosen filename for avatar [" << file2Check << "]";
                            log.Write(DEBUG, ost.str());
                        }


                        CopyFile(tmpImageJPG, file2Check);

                        ost.str("");
                        ost << "insert into `users_avatars` set `userid`='" << user.GetID() << "', `folder`='" << folderID << "', `filename`='" << filePrefix << ".jpg'";
                        if(unsigned long avatarID = db.InsertQuery(ost.str()))
                        {
                            if(filesCounter == 0) ostJSONResult << "[" << std::endl;
                            if(filesCounter  > 0) ostJSONResult << ",";
                            ostJSONResult << "{" << std::endl;
                	        ostJSONResult << "\"result\": \"success\"," << std::endl;
                	        ostJSONResult << "\"textStatus\": \"\"," << std::endl;
                            ostJSONResult << "\"fileName\": \"" << indexPage.GetFilesHandler()->GetName(filesCounter) << "\" ," << std::endl;
                	        ostJSONResult << "\"jqXHR\": \"\"" << std::endl;
                            ostJSONResult << "}" << std::endl;
                            if(filesCounter == (indexPage.GetFilesHandler()->Count() - 1)) ostJSONResult << "]";

                            // --- Update live feed
                            ost.str("");
                            ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << user.GetID() << "\", \"10\", \"" << avatarID << "\", NOW())";
                            if(!db.InsertQuery(ost.str()))
                            {
                                {
                                    ostringstream   ostTemp;
                                    CLog            log;

                                    ostTemp.str("");
                                    ostTemp << "int main(void): ERROR inserting into `feed` table (" << ost.str() << ")";
                                    log.Write(ERROR, ostTemp.str());
                                }
                            }

                            
                        }
                        else
                        {

                            {
                                ostringstream   ostTemp;
                                CLog            log;

                                ostTemp.str("");
                                ostTemp << "int main(void): ERROR inserting into `user_avatars` table (" << ost.str() << ")";
                                log.Write(ERROR, ostTemp.str());
                            }

                            if(filesCounter == 0) ostJSONResult << "[" << std::endl;
                            if(filesCounter  > 0) ostJSONResult << ",";
                            ostJSONResult << "{" << std::endl;
                            ostJSONResult << "\"result\": \"error\"," << std::endl;
                            ostJSONResult << "\"textStatus\": \"ERROR inserting into `user_avatars` table\"," << std::endl;
                            ostJSONResult << "\"fileName\": \"\" ," << std::endl;
                            ostJSONResult << "\"jqXHR\": \"\"" << std::endl;
                            ostJSONResult << "}" << std::endl;
                            if(filesCounter == (indexPage.GetFilesHandler()->Count() - 1)) ostJSONResult << "]";

                            // --- Delete temporarily files
                            unlink(tmpFile2Check.c_str());
                            unlink(tmpImageJPG.c_str());
                        }

                    }
                    else
                    {
                        {
                            ostringstream   ost;
                            CLog            log;

                            ost.clear();
                            ost << "int main(void): avatar [" << indexPage.GetFilesHandler()->GetName(filesCounter) << "] is not valid image";
                            log.Write(DEBUG, ost.str());
                        }

                        if(filesCounter == 0) ostJSONResult << "[" << std::endl;
                        if(filesCounter  > 0) ostJSONResult << ",";
                        ostJSONResult << "{" << std::endl;
                        ostJSONResult << "\"result\": \"error\"," << std::endl;
                        ostJSONResult << "\"textStatus\": \"wrong format\"," << std::endl;
                        ostJSONResult << "\"fileName\": \"" << indexPage.GetFilesHandler()->GetName(filesCounter) << "\" ," << std::endl;
                        ostJSONResult << "\"jqXHR\": \"\"" << std::endl;
                        ostJSONResult << "}" << std::endl;
                        if(filesCounter == (indexPage.GetFilesHandler()->Count() - 1)) ostJSONResult << "]";
                    }
                    // --- Delete temporarily files
                    unlink(tmpFile2Check.c_str());
                    unlink(tmpImageJPG.c_str());

                } //--- Check that number of avatars <= 4
                else
                {
                    {
                        ostringstream   ost;
                        CLog            log;

                        ost.clear();
                        ost << "int main(void): number of avatars for user " << user.GetLogin() << " exceeds 4";
                        log.Write(DEBUG, ost.str());
                    }

                    if(filesCounter == 0) ostJSONResult << "[" << std::endl;
                    if(filesCounter  > 0) ostJSONResult << ",";
                    ostJSONResult << "{" << std::endl;
                    ostJSONResult << "\"result\": \"error\"," << std::endl;
                    ostJSONResult << "\"textStatus\": \"number of avatars exceeds\"," << std::endl;
                    ostJSONResult << "\"fileName\": \"" << indexPage.GetFilesHandler()->GetName(filesCounter) << "\" ," << std::endl;
                    ostJSONResult << "\"jqXHR\": \"\"" << std::endl;
                    ostJSONResult << "}" << std::endl;
                    if(filesCounter == (indexPage.GetFilesHandler()->Count() - 1)) ostJSONResult << "]";
                }

            } // --- Loop through all uploaded files

            indexPage.RegisterVariableForce("result", ostJSONResult.str());

        }
        else
        {
            ostringstream   ost;
            
            ost.str("");
            ost << "{" << std::endl;
            ost << "\"result\": \"error\"," << std::endl;
            ost << "\"textStatus\": \"" << indexPage.GetVarsHandler()->Get("ErrorDescription") << "\"," << std::endl;
            ost << "\"fileName\": \"\" ," << std::endl;
            ost << "\"jqXHR\": \"\"" << std::endl;
            ost << "}" << std::endl;

            indexPage.RegisterVariableForce("result", ost.str());
        }

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog    log;

			log.Write(ERROR, "int main(void): ERROR template file was missing: ", "json_response_with_braces.htmlt");
			throw CException("Template file was missing");
		}

    	indexPage.OutTemplate();

    }
    catch(CExceptionHTML &c)
    {
		CLog	log;

		c.SetLanguage(indexPage.GetLanguage());
		c.SetDB(&db);

		log.Write(ERROR, "catch CExceptionHTML: exception reason: [", c.GetReason(), "]");

        if(c.GetReason() == "avatar file write error")
        {
            ostringstream   ost;

            ost.str("");
            ost << "\"result\": \"error\"," << std::endl;
            ost << "\"textStatus\": \"" << c.GetReason() << ", file: (" << c.GetParam1() << ")\"," << std::endl;
            ost << "\"jqXHR\": \"\"" << std::endl;
            indexPage.RegisterVariableForce("result", ost.str());
        }
        if(c.GetReason() == "avatar file size exceed")
        {
            ostringstream   ost;

            ost.str("");
            ost << "\"result\": \"error\"," << std::endl;
            ost << "\"textStatus\": \"" << c.GetReason() << ", file: (" << c.GetParam1() << ")\"," << std::endl;
            ost << "\"jqXHR\": \"\"" << std::endl;
            indexPage.RegisterVariableForce("result", ost.str());
        }


		if(!indexPage.SetTemplate(c.GetTemplate()))
		{
			return(-1);
		}
		indexPage.RegisterVariable("content", c.GetReason());
		indexPage.OutTemplate();
		return(0);
    }
    catch(CException &c)
    {
    	CLog 	log;

		if(!indexPage.SetTemplateFile("templates/error.htmlt"))
		{
			return(-1);
		}

		log.Write(ERROR, "catch CException: exception: ", c.GetReason());

		indexPage.RegisterVariable("content", c.GetReason());
		indexPage.OutTemplate();
		return(-1);
    }

    return(0);
}

