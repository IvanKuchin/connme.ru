#include "imageuploader.h"	 
#include "utilities.h"

bool ImageSaveAsJpg (const string src, const string dst, struct ExifInfo &exifInfo)
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

        if((imageGeometry.width() > IMAGE_MAX_WIDTH) || (imageGeometry.height() > IMAGE_MAX_HEIGHT))
        {
            int   newHeight, newWidth;
            if(imageGeometry.width() >= imageGeometry.height())
            {
                newWidth = IMAGE_MAX_WIDTH;
                newHeight = newWidth * imageGeometry.height() / imageGeometry.width();
            }
            else
            {
                newHeight = IMAGE_MAX_HEIGHT;
                newWidth = newHeight * imageGeometry.width() / imageGeometry.height();
            }

            image.resize(Magick::Geometry(newWidth, newHeight, 0, 0));
        }

        {
            // --- Exif information save
            exifInfo.DateTime = "";
            exifInfo.DateTime = image.attribute("exif:DateTime");
            exifInfo.GPSAltitude = "";
            exifInfo.GPSAltitude = image.attribute("exif:GPSAltitude");
            exifInfo.GPSLatitude = "";
            exifInfo.GPSLatitude = image.attribute("exif:GPSLatitude");
            exifInfo.GPSLongitude = "";
            exifInfo.GPSLongitude = image.attribute("exif:GPSLongitude");
            exifInfo.GPSSpeed = "";
            exifInfo.GPSSpeed = image.attribute("exif:GPSSpeed");
            exifInfo.Model = "";
            exifInfo.Model = image.attribute("exif:Model");
            exifInfo.Authors = "";
            exifInfo.Authors = image.attribute("exif:Authors");
            exifInfo.ApertureValue = "";
            exifInfo.ApertureValue = image.attribute("exif:ApertureValue");
            exifInfo.BrightnessValue = "";
            exifInfo.BrightnessValue = image.attribute("exif:BrightnessValue");
            exifInfo.ColorSpace = "";
            exifInfo.ColorSpace = image.attribute("exif:ColorSpace");
            exifInfo.ComponentsConfiguration = "";
            exifInfo.ComponentsConfiguration = image.attribute("exif:ComponentsConfiguration");
            exifInfo.Compression = "";
            exifInfo.Compression = image.attribute("exif:Compression");
            exifInfo.DateTimeDigitized = "";
            exifInfo.DateTimeDigitized = image.attribute("exif:DateTimeDigitized");
            exifInfo.DateTimeOriginal = "";
            exifInfo.DateTimeOriginal = image.attribute("exif:DateTimeOriginal");
            exifInfo.ExifImageLength = "";
            exifInfo.ExifImageLength = image.attribute("exif:ExifImageLength");
            exifInfo.ExifImageWidth = "";
            exifInfo.ExifImageWidth = image.attribute("exif:ExifImageWidth");
            exifInfo.ExifOffset = "";
            exifInfo.ExifOffset = image.attribute("exif:ExifOffset");
            exifInfo.ExifVersion = "";
            exifInfo.ExifVersion = image.attribute("exif:ExifVersion");
            exifInfo.ExposureBiasValue = "";
            exifInfo.ExposureBiasValue = image.attribute("exif:ExposureBiasValue");
            exifInfo.ExposureMode = "";
            exifInfo.ExposureMode = image.attribute("exif:ExposureMode");
            exifInfo.ExposureProgram = "";
            exifInfo.ExposureProgram = image.attribute("exif:ExposureProgram");
            exifInfo.ExposureTime = "";
            exifInfo.ExposureTime = image.attribute("exif:ExposureTime");
            exifInfo.Flash = "";
            exifInfo.Flash = image.attribute("exif:Flash");
            exifInfo.FlashPixVersion = "";
            exifInfo.FlashPixVersion = image.attribute("exif:FlashPixVersion");
            exifInfo.FNumber = "";
            exifInfo.FNumber = image.attribute("exif:FNumber");
            exifInfo.FocalLength = "";
            exifInfo.FocalLength = image.attribute("exif:FocalLength");
            exifInfo.FocalLengthIn35mmFilm = "";
            exifInfo.FocalLengthIn35mmFilm = image.attribute("exif:FocalLengthIn35mmFilm");
            exifInfo.GPSDateStamp = "";
            exifInfo.GPSDateStamp = image.attribute("exif:GPSDateStamp");
            exifInfo.GPSDestBearing = "";
            exifInfo.GPSDestBearing = image.attribute("exif:GPSDestBearing");
            exifInfo.GPSDestBearingRef = "";
            exifInfo.GPSDestBearingRef = image.attribute("exif:GPSDestBearingRef");
            exifInfo.GPSImgDirection = "";
            exifInfo.GPSImgDirection = image.attribute("exif:GPSImgDirection");
            exifInfo.GPSImgDirectionRef = "";
            exifInfo.GPSImgDirectionRef = image.attribute("exif:GPSImgDirectionRef");
            exifInfo.GPSInfo = "";
            exifInfo.GPSInfo = image.attribute("exif:GPSInfo");
            exifInfo.GPSTimeStamp = "";
            exifInfo.GPSTimeStamp = image.attribute("exif:GPSTimeStamp");
            exifInfo.ISOSpeedRatings = "";
            exifInfo.ISOSpeedRatings = image.attribute("exif:ISOSpeedRatings");
            exifInfo.JPEGInterchangeFormat = "";
            exifInfo.JPEGInterchangeFormat = image.attribute("exif:JPEGInterchangeFormat");
            exifInfo.JPEGInterchangeFormatLength = "";
            exifInfo.JPEGInterchangeFormatLength = image.attribute("exif:JPEGInterchangeFormatLength");
            exifInfo.Make = "";
            exifInfo.Make = image.attribute("exif:Make");
            exifInfo.MeteringMode = "";
            exifInfo.MeteringMode = image.attribute("exif:MeteringMode");
            exifInfo.Orientation = "";
            exifInfo.Orientation = image.attribute("exif:Orientation");
            exifInfo.ResolutionUnit = "";
            exifInfo.ResolutionUnit = image.attribute("exif:ResolutionUnit");
            exifInfo.SceneCaptureType = "";
            exifInfo.SceneCaptureType = image.attribute("exif:SceneCaptureType");
            exifInfo.SceneType = "";
            exifInfo.SceneType = image.attribute("exif:SceneType");
            exifInfo.SensingMethod = "";
            exifInfo.SensingMethod = image.attribute("exif:SensingMethod");
            exifInfo.ShutterSpeedValue = "";
            exifInfo.ShutterSpeedValue = image.attribute("exif:ShutterSpeedValue");
            exifInfo.Software = "";
            exifInfo.Software = image.attribute("exif:Software");
            exifInfo.SubjectArea = "";
            exifInfo.SubjectArea = image.attribute("exif:SubjectArea");
            exifInfo.SubSecTimeDigitized = "";
            exifInfo.SubSecTimeDigitized = image.attribute("exif:SubSecTimeDigitized");
            exifInfo.SubSecTimeOriginal = "";
            exifInfo.SubSecTimeOriginal = image.attribute("exif:SubSecTimeOriginal");
            exifInfo.WhiteBalance = "";
            exifInfo.WhiteBalance = image.attribute("exif:WhiteBalance");
            exifInfo.XResolution = "";
            exifInfo.XResolution = image.attribute("exif:XResolution");
            exifInfo.YCbCrPositioning = "";
            exifInfo.YCbCrPositioning = image.attribute("exif:YCbCrPositioning");
            exifInfo.YResolution = "";
            exifInfo.YResolution = image.attribute("exif:YResolution");

            exifInfo.GPSAltitude = image.attribute("exif:GPSAltitudeRef") + ": " + exifInfo.GPSAltitude;
            exifInfo.GPSLatitude = image.attribute("exif:GPSLatitudeRef") + ": " + exifInfo.GPSLatitude;
            exifInfo.GPSLongitude = image.attribute("exif:GPSLongitudeRef") + ": " + exifInfo.GPSLongitude;
            exifInfo.GPSSpeed = image.attribute("exif:GPSSpeedRef") + ": " + exifInfo.GPSSpeed;

            image.strip();
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
    string          action, partnerID, imageTempSet;
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
    	imageTempSet = indexPage.GetVarsHandler()->Get("imageTempSet");
    	{
    		CLog	log;
    		ostringstream	ost;

    		ost.str("");
    		ost << "main(): action = " << action << ", imageTempSet = " << imageTempSet;

    		log.Write(DEBUG, ost.str());
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
    			else 
    			{
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
    						

    						}
    						else
    						{
    							CLog	log;
    							log.Write(ERROR, "main(): ERROR update session in HTTP or DB failed");
    						}
    					}
    					else {
    						CLog	log;
    						log.Write(ERROR, "main(): ERROR session consistency check failed");
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

    			// --- check the imageTempSet is exists
    			if(imageTempSet.length() == 0)
                {
                    {
                        CLog    log;

                        ost.str("");
                        ost << "main(): ERROR imageTempSet[" << imageTempSet << "] is empty";
                        log.Write(ERROR, ost.str());
                    }

                    throw CExceptionHTML("news_feed: error in image set");
                } // --- end check the imageTempSet is exists
    		} // --- end generate session
    	} // --- end generate common parts

    	for(int filesCounter = 0; filesCounter < indexPage.GetFilesHandler()->Count(); filesCounter++)
    	{
    		FILE	        *f;
    		int		        folderID = (int)(rand()/(RAND_MAX + 1.0) * FEEDIMAGE_NUMBER_OF_FOLDERS) + 1;
    		string	        filePrefix = GetRandom(20);
    		string	        file2Check, tmpFile2Check, tmpImageJPG, fileName, fileExtention;
            ostringstream   ost;
            struct ExifInfo exifInfo;

    		{
    			CLog	log;
    			ostringstream	ost;

    			ost << "int main(void): number of files POST'ed = " << indexPage.GetFilesHandler()->Count();
    			log.Write(DEBUG, ost.str());
    		}

    		if(indexPage.GetFilesHandler()->GetSize(filesCounter) > FEEDIMAGE_MAX_FILE_SIZE) 
    		{
				CLog        	log;
				ostringstream	ost;

				ost.str("");
				ost << "int main(void): ERROR feed image file [" << indexPage.GetFilesHandler()->GetName(filesCounter) << "] size exceed permited maximum: " << indexPage.GetFilesHandler()->GetSize(filesCounter) << " > " << FEEDIMAGE_MAX_FILE_SIZE;

				log.Write(ERROR, ost.str());
                throw CExceptionHTML("image file size exceed", indexPage.GetFilesHandler()->GetName(filesCounter));
    		}

    		//--- check image file existing
    		do
    		{
    			ostringstream	ost;
                string          tmp;
                std::size_t  	foundPos;

    			folderID = (int)(rand()/(RAND_MAX + 1.0) * FEEDIMAGE_NUMBER_OF_FOLDERS) + 1;
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
    			ost << IMAGE_FEED_DIRECTORY << "/" << folderID << "/" << filePrefix << ".jpg";
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
                    throw CExceptionHTML("image file write error", indexPage.GetFilesHandler()->GetName(filesCounter));
                }
            }
            fwrite(indexPage.GetFilesHandler()->Get(filesCounter), indexPage.GetFilesHandler()->GetSize(filesCounter), 1, f);
            fclose(f);

            if(ImageSaveAsJpg(tmpFile2Check, tmpImageJPG, exifInfo))
            {
                unsigned long      feed_imagesID = 0;

                {
                    CLog    log;
                    ostringstream   ost;

                    ost << "int main(void): choosen filename for feed image [" << file2Check << "]";
                    log.Write(DEBUG, ost.str());
                }

                CopyFile(tmpImageJPG, file2Check);

                ost.str("");
                ost << "insert into `feed_images` set \
                `tempSet`='" << imageTempSet << "', \
                `userID`='" << user.GetID() << "',  \
                `folder`='" << folderID << "', \
                `filename`='" << filePrefix << ".jpg', "
                 << ((exifInfo.DateTime.length() && exifInfo.DateTime != "unknown") ? (string)("`exifDateTime`='") + exifInfo.DateTime + "', " : "") <<
                "`exifGPSAltitude`='" << exifInfo.GPSAltitude << "', \
                `exifGPSLatitude`='" << exifInfo.GPSLatitude << "', \
                `exifGPSLongitude`='" << exifInfo.GPSLongitude << "', \
                `exifGPSSpeed`='" << exifInfo.GPSSpeed << "', \
                `exifModel`='" << exifInfo.Model << "', \
                `exifAuthors`='" << exifInfo.Authors << "', \
                `exifApertureValue`='" << exifInfo.ApertureValue << "', \
                `exifBrightnessValue`='" << exifInfo.BrightnessValue << "', \
                `exifColorSpace`='" << exifInfo.ColorSpace << "', \
                `exifComponentsConfiguration`='" << exifInfo.ComponentsConfiguration << "', \
                `exifCompression`='" << exifInfo.Compression << "', \
                `exifDateTimeDigitized`='" << exifInfo.DateTimeDigitized << "', \
                `exifDateTimeOriginal`='" << exifInfo.DateTimeOriginal << "', \
                `exifExifImageLength`='" << exifInfo.ExifImageLength << "', \
                `exifExifImageWidth`='" << exifInfo.ExifImageWidth << "', \
                `exifExifOffset`='" << exifInfo.ExifOffset << "', \
                `exifExifVersion`='" << exifInfo.ExifVersion << "', \
                `exifExposureBiasValue`='" << exifInfo.ExposureBiasValue << "', \
                `exifExposureMode`='" << exifInfo.ExposureMode << "', \
                `exifExposureProgram`='" << exifInfo.ExposureProgram << "', \
                `exifExposureTime`='" << exifInfo.ExposureTime << "', \
                `exifFlash`='" << exifInfo.Flash << "', \
                `exifFlashPixVersion`='" << exifInfo.FlashPixVersion << "', \
                `exifFNumber`='" << exifInfo.FNumber << "', \
                `exifFocalLength`='" << exifInfo.FocalLength << "', \
                `exifFocalLengthIn35mmFilm`='" << exifInfo.FocalLengthIn35mmFilm << "', \
                `exifGPSDateStamp`='" << exifInfo.GPSDateStamp << "', \
                `exifGPSDestBearing`='" << exifInfo.GPSDestBearing << "', \
                `exifGPSDestBearingRef`='" << exifInfo.GPSDestBearingRef << "', \
                `exifGPSImgDirection`='" << exifInfo.GPSImgDirection << "', \
                `exifGPSImgDirectionRef`='" << exifInfo.GPSImgDirectionRef << "', \
                `exifGPSInfo`='" << exifInfo.GPSInfo << "', \
                `exifGPSTimeStamp`='" << exifInfo.GPSTimeStamp << "', \
                `exifISOSpeedRatings`='" << exifInfo.ISOSpeedRatings << "', \
                `exifJPEGInterchangeFormat`='" << exifInfo.JPEGInterchangeFormat << "', \
                `exifJPEGInterchangeFormatLength`='" << exifInfo.JPEGInterchangeFormatLength << "', \
                `exifMake`='" << exifInfo.Make << "', \
                `exifMeteringMode`='" << exifInfo.MeteringMode << "', \
                `exifOrientation`='" << exifInfo.Orientation << "', \
                `exifResolutionUnit`='" << exifInfo.ResolutionUnit << "', \
                `exifSceneCaptureType`='" << exifInfo.SceneCaptureType << "', \
                `exifSceneType`='" << exifInfo.SceneType << "', \
                `exifSensingMethod`='" << exifInfo.SensingMethod << "', \
                `exifShutterSpeedValue`='" << exifInfo.ShutterSpeedValue << "', \
                `exifSoftware`='" << exifInfo.Software << "', \
                `exifSubjectArea`='" << exifInfo.SubjectArea << "', \
                `exifSubSecTimeDigitized`='" << exifInfo.SubSecTimeDigitized << "', \
                `exifSubSecTimeOriginal`='" << exifInfo.SubSecTimeOriginal << "', \
                `exifWhiteBalance`='" << exifInfo.WhiteBalance << "', \
                `exifXResolution`='" << exifInfo.XResolution << "', \
                `exifYCbCrPositioning`='" << exifInfo.YCbCrPositioning << "', \
                `exifYResolution`='" << exifInfo.YResolution << "';";
                feed_imagesID = db.InsertQuery(ost.str());

                if(filesCounter == 0) ostJSONResult << "[" << std::endl;
                if(filesCounter  > 0) ostJSONResult << ",";
                if(feed_imagesID)
                {
                    ostJSONResult << "{" << std::endl;
        	        ostJSONResult << "\"result\": \"success\"," << std::endl;
        	        ostJSONResult << "\"textStatus\": \"\"," << std::endl;
                    ostJSONResult << "\"fileName\": \"" << indexPage.GetFilesHandler()->GetName(filesCounter) << "\" ," << std::endl;
                    ostJSONResult << "\"imageID\": \"" << feed_imagesID << "\" ," << std::endl;
        	        ostJSONResult << "\"jqXHR\": \"\"" << std::endl;
                    ostJSONResult << "}" << std::endl;
                }
                else
                {
                    {
                        CLog    log;
                        log.Write(ERROR, "int main(void): ERROR: inserting image info into feed_images table");
                    }
                    ostJSONResult << "{" << std::endl;
                    ostJSONResult << "\"result\": \"error\"," << std::endl;
                    ostJSONResult << "\"textStatus\": \"error inserting into table\"," << std::endl;
                    ostJSONResult << "\"fileName\": \"" << indexPage.GetFilesHandler()->GetName(filesCounter) << "\" ," << std::endl;
                    ostJSONResult << "\"imageID\": \"" << feed_imagesID << "\" ," << std::endl;
                    ostJSONResult << "\"jqXHR\": \"\"" << std::endl;
                    ostJSONResult << "}" << std::endl;
                }
                if(filesCounter == (indexPage.GetFilesHandler()->Count() - 1)) ostJSONResult << "]";

            }
            else
            {
                {
                    ostringstream   ost;
                    CLog            log;

                    ost.clear();
                    ost << "int main(void): feed image [" << indexPage.GetFilesHandler()->GetName(filesCounter) << "] is not valid image";
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

        } // --- Loop through all uploaded files

        indexPage.RegisterVariableForce("result", ostJSONResult.str());


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
        ostringstream   ost;

        c.SetLanguage(indexPage.GetLanguage());
        c.SetDB(&db);

        log.Write(ERROR, "catch CExceptionHTML: exception reason: [", c.GetReason(), "]");

        ost.str("");
        ost << "[{" << std::endl;
        ost << "\"result\": \"error\"," << std::endl;
        ost << "\"textStatus\": \"" << c.GetReason();
        if(c.GetParam1().length() > 0) ost << " (" << c.GetParam1() << ")";
        ost << "\"," << std::endl;
        ost << "\"jqXHR\": \"\"" << std::endl;
        ost << "}]" << std::endl;
        indexPage.RegisterVariableForce("result", ost.str());

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

