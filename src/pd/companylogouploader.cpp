#include "companylogouploader.h"	 

bool ImageSaveAsJpg (const string src, const string dst)
{
	{
		CLog	log;

		log.Write(DEBUG, string(__func__) + "(" + src + ", " + dst + ")[" + to_string(__LINE__) + "]: start");
	}

#ifndef IMAGEMAGICK_DISABLE
	// Construct the image object. Seperating image construction from the
	// the read operation ensures that a failure to read the image file
	// doesn't render the image object useless.
	try {
		Magick::Image		   image;
		Magick::OrientationType imageOrientation;
		Magick::Geometry		imageGeometry;

		// Read a file into image object
		image.read( src );

		imageGeometry = image.size();
		imageOrientation = image.orientation();

		{
			CLog	log;
			ostringstream   ost;

			ost.str("");
			ost << "imageOrientation = " << imageOrientation << ", xRes = " << imageGeometry.width() << ", yRes = " << imageGeometry.height();
			log.Write(DEBUG, string(__func__) + "(" + src + ", " + dst + ")[" + to_string(__LINE__) + "]: " + ost.str());
		}

		if(imageOrientation == Magick::TopRightOrientation) image.flop();
		if(imageOrientation == Magick::BottomRightOrientation) image.rotate(180);
		if(imageOrientation == Magick::BottomLeftOrientation) { image.flop(); image.rotate(180); }
		if(imageOrientation == Magick::LeftTopOrientation) { image.flop(); image.rotate(-90); }
		if(imageOrientation == Magick::RightTopOrientation) image.rotate(90);
		if(imageOrientation == Magick::RightBottomOrientation) { image.flop(); image.rotate(90); }
		if(imageOrientation == Magick::LeftBottomOrientation) image.rotate(-90);

		if((imageGeometry.width() > COMPANYLOGO_MAX_WIDTH) || (imageGeometry.height() > COMPANYLOGO_MAX_HEIGHT))
		{
			int   newHeight, newWidth;
			if(imageGeometry.width() >= imageGeometry.height())
			{
				newWidth = COMPANYLOGO_MAX_WIDTH;
				newHeight = newWidth * imageGeometry.height() / imageGeometry.width();
			}
			else
			{
				newHeight = COMPANYLOGO_MAX_HEIGHT;
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
			CLog	log;
			log.Write(DEBUG, string(__func__) + "(" + src + ", " + dst + ")[" + to_string(__LINE__) + "]: exception in read/write operation [" + error_.what() + "]");
		}
		return false;
	}
	{
		CLog	log;
		log.Write(DEBUG, string(__func__) + "(" + src + ", " + dst + ")[" + to_string(__LINE__) + "]: finish (image has been successfully converted to .jpg format)");
	}
	return true;
#else
	{
		CLog	log;
		log.Write(DEBUG, string(__func__) + "(" + src + ", " + dst + ")[" + to_string(__LINE__) + "]: simple file coping, because ImageMagick++ is not activated");
	}
	CopyFile(src, dst);
	{
		CLog	log;
		log.Write(DEBUG, string(__func__) + "(" + src + ", " + dst + ")[" + to_string(__LINE__) + "]: finish");
	}
	return  true;
#endif
}

int main()
{
	CStatistics	 appStat;  // --- CStatistics must be firts statement to measure end2end param's
	CCgi			indexPage(EXTERNAL_TEMPLATE);
	CUser		   user;
	string		  action;
	CMysql		  db;
	struct timeval  tv;
	ostringstream   ostJSONResult/*(static_cast<ostringstream&&>(ostringstream() << "["))*/;

	{
		CLog	log;
		log.Write(DEBUG, string(__func__) + string("[") + to_string(__LINE__) + "]: " + __FILE__);
	}

	signal(SIGSEGV, crash_handler); 

	gettimeofday(&tv, NULL);
	srand(tv.tv_sec * tv.tv_usec * 100000);
	ostJSONResult.clear();
		
	try
	{
		indexPage.ParseURL();

		if(!indexPage.SetTemplate("index.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, string(__func__) + "[" + to_string(__LINE__) + "]:ERROR: template file was missing");
			throw CException("Template file was missing");
		}

		if(db.Connect() < 0)
		{
			CLog	log;

			log.Write(ERROR, string(__func__) + "[" + to_string(__LINE__) + "]:ERROR: Can not connect to mysql database");
			throw CExceptionHTML("MySql connection");
		}

		indexPage.SetDB(&db);

#ifndef IMAGEMAGICK_DISABLE
		Magick::InitializeMagick(NULL);
#endif

		action = CheckHTTPParam_Text(indexPage.GetVarsHandler()->Get("action"));

		MESSAGE_DEBUG("", "", "action taken from HTTP is " + action);

		// ------------ generate common parts
		{
			if(RegisterInitialVariables(&indexPage, &db, &user))
			{
			}
			else
			{
				MESSAGE_ERROR("", "", "RegisterInitialVariables failed, throwing exception");
				throw CExceptionHTML("environment variable error");
			}

			action = GenerateSession(action, &indexPage, &db, &user);
		}
		// ------------ end generate common parts

		MESSAGE_DEBUG("", "", "pre-condition if(action == \"" + action + "\")");


		if((user.GetID().length()) && (user.GetName() != "Guest"))
		{
			string	  companyID = CheckHTTPParam_Number(indexPage.GetVarsHandler()->Get("companyid"));

			if(companyID.length())
			{
				// --- parameter companyID exists and not empty

				if(db.Query("select * from `company` where `id`=\"" + companyID + "\" and `admin_userID`=\"" + user.GetID() + "\";"))
				{
					// --- user.GetID() is the company admin

					if(indexPage.GetFilesHandler()->Count() == 1)
					{
						// --- number uploaded files = 1

						for(int filesCounter = 0; filesCounter < indexPage.GetFilesHandler()->Count(); filesCounter++)
						{
							FILE			*f;
							int			 folderID = (int)(rand()/(RAND_MAX + 1.0) * COMPANYLOGO_NUMBER_OF_FOLDERS) + 1;
							string		  filePrefix = GetRandom(20);
							string		  file2Check, tmpFile2Check, tmpImageJPG, fileName, fileExtention;
							ostringstream   ost;

							if(indexPage.GetFilesHandler()->GetSize(filesCounter) > COMPANYLOGO_MAX_FILE_SIZE) 
							{
								CLog			log;
								ostringstream   ost;

								ost.str("");
								ost << string(__func__) << "[" << to_string(__LINE__) << "]:ERROR: avatar file [" << indexPage.GetFilesHandler()->GetName(filesCounter) << "] size exceed permited maximum: " << indexPage.GetFilesHandler()->GetSize(filesCounter) << " > " << COMPANYLOGO_MAX_FILE_SIZE;

								log.Write(ERROR, ost.str());
								throw CExceptionHTML("avatar file size exceed", indexPage.GetFilesHandler()->GetName(filesCounter));
							}

							//--- check avatar file existing
							do
							{
								ostringstream   ost;
								string		  tmp;
								std::size_t  foundPos;

								folderID = (int)(rand()/(RAND_MAX + 1.0) * COMPANYLOGO_NUMBER_OF_FOLDERS) + 1;
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
								ost << IMAGE_COMPANIES_DIRECTORY << "/" << folderID << "/" << filePrefix << ".jpg";
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
								ostringstream   ost;

								ost << __func__ << "[" << __LINE__ << "]: Save file to /tmp for checking of image validity [" << tmpFile2Check << "]";
								log.Write(DEBUG, ost.str());
							}

							// --- Save file to "/tmp/" for checking of image validity
							f = fopen(tmpFile2Check.c_str(), "w");
							if(f == NULL)
							{
								{
									CLog			log;

									log.Write(ERROR, string(__func__) + "[" + to_string(__LINE__) + "]:ERROR: writing file:", tmpFile2Check.c_str());
									throw CExceptionHTML("avatar file write error", indexPage.GetFilesHandler()->GetName(filesCounter));
								}
							}
							fwrite(indexPage.GetFilesHandler()->Get(filesCounter), indexPage.GetFilesHandler()->GetSize(filesCounter), 1, f);
							fclose(f);

							if(ImageSaveAsJpg(tmpFile2Check, tmpImageJPG))
							{

								{
									CLog	log;
									ostringstream   ost;

									ost << string(__func__) << "[" << to_string(__LINE__) << "]: choosen filename for avatar [" << file2Check << "]";
									log.Write(DEBUG, ost.str());
								}

								// --- remove previous logo
								if(db.Query("select `logo_folder`, `logo_filename` from `company` where `id`=\"" + companyID + "\";"))
								{
									string  currLogo = string(IMAGE_COMPANIES_DIRECTORY) + "/" + db.Get(0, "logo_folder") + "/" + db.Get(0, "logo_filename");

									if(isFileExists(currLogo)) 
									{
										{
											CLog			log;
											log.Write(DEBUG, string(__func__) + "[" + to_string(__LINE__) + "]: remove current logo (" + currLogo + ")");
										}
										unlink(currLogo.c_str());
									}
								}

								CopyFile(tmpImageJPG, file2Check);

								db.Query("update `company` set `logo_folder`='" + to_string(folderID) + "', `logo_filename`='" + filePrefix + ".jpg',`lastActivity`=UNIX_TIMESTAMP() where `id`=\"" + companyID + "\";");
								{
									if(filesCounter == 0) ostJSONResult << "[" << std::endl;
									if(filesCounter  > 0) ostJSONResult << ",";
									ostJSONResult	<< "{"
													<< "\"result\": \"success\","
													<< "\"textStatus\": \"\","
													<< "\"fileName\": \"" << indexPage.GetFilesHandler()->GetName(filesCounter) << "\" ,"
													<< "\"logo_folder\": \"" << folderID << "\","
													<< "\"logo_filename\": \"" << filePrefix << ".jpg\","
													<< "\"jqXHR\": \"\""
													<< "}";
									if(filesCounter == (indexPage.GetFilesHandler()->Count() - 1)) ostJSONResult << "]";

									// --- Update live feed
									ost.str("");
									ost << "insert into `feed` (`title`, `userId`, `actionTypeId`, `actionId`, `eventTimestamp`) values(\"\",\"" << companyID << "\", \"1009\", \"0\", NOW())";
									if(!db.InsertQuery(ost.str()))
									{
										{
											ostringstream   ostTemp;
											CLog			log;

											ostTemp.str("");
											ost << string(__func__) << "[" << to_string(__LINE__) << "]:ERROR: inserting into `feed` table (" << ost.str() << ")";
											log.Write(ERROR, ostTemp.str());
										}
									}
								}

							}
							else
							{
								{
									ostringstream   ost;
									CLog			log;

									ost.clear();
									ost << __func__ << "[" << __LINE__ << "]: avatar [" << indexPage.GetFilesHandler()->GetName(filesCounter) << "] is not valid image";
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

					} // --- if number uploaded files = 1
					else
					{
						{
							CLog	log;
							log.Write(ERROR, string(__func__) + "[" + to_string(__LINE__) + "]: ERROR: number uploaded images=" + to_string(indexPage.GetFilesHandler()->Count()) + ", but must be 1");
						}

						ostJSONResult.str("");
						ostJSONResult << "{" << std::endl;
						ostJSONResult << "\"result\": \"error\"," << std::endl;
						ostJSONResult << "\"textStatus\": \"number uploaded images must be 1\"," << std::endl;
						ostJSONResult << "\"fileName\": \"\" ," << std::endl;
						ostJSONResult << "\"jqXHR\": \"\"" << std::endl;
						ostJSONResult << "}" << std::endl;
					} // --- if number uploaded files = 1
				} // --- user.GetID() is the company admin
				else
				{
					{
						CLog	log;
						log.Write(ERROR, string(__func__) + "[" + to_string(__LINE__) + "]: ERROR: you are not company admin");
					}

					ostJSONResult.str("");
					ostJSONResult << "{";
					ostJSONResult << "\"result\": \"error\",";
					ostJSONResult << "\"textStatus\": \"you are not company admin\",";
					ostJSONResult << "\"fileName\": \"\" ,";
					ostJSONResult << "\"jqXHR\": \"\"";
					ostJSONResult << "}";
				} // --- user.GetID() is the company admin

				indexPage.RegisterVariableForce("result", ostJSONResult.str());

			} // --- parameter companyID exists and not empty
			else
			{
				{
					CLog	log;
					log.Write(ERROR, string(__func__) + "[" + to_string(__LINE__) + "]: ERROR: companyID parameter missed");
				}

				ostringstream   ost;
				
				ost.str("");
				ost << "{" << std::endl;
				ost << "\"result\": \"error\"," << std::endl;
				ost << "\"textStatus\": \"companyID parameter missed\"," << std::endl;
				ost << "\"fileName\": \"\" ," << std::endl;
				ost << "\"jqXHR\": \"\"" << std::endl;
				ost << "}" << std::endl;

				indexPage.RegisterVariableForce("result", ost.str());
			} // --- parameter companyID exists and not empty
		} // --- user not Guest
		else
		{
			{
				CLog	log;
				log.Write(ERROR, string(__func__) + "[" + to_string(__LINE__) + "]: ERROR: user not found or not logged in");
			}


			ostringstream   ost;
			
			ost.str("");
			ost << "{" << std::endl;
			ost << "\"result\": \"error\"," << std::endl;
			ost << "\"textStatus\": \"" << indexPage.GetVarsHandler()->Get("ErrorDescription") << "\"," << std::endl;
			ost << "\"fileName\": \"\" ," << std::endl;
			ost << "\"jqXHR\": \"\"" << std::endl;
			ost << "}" << std::endl;

			indexPage.RegisterVariableForce("result", ost.str());
		} // --- user not Guest

		if(!indexPage.SetTemplate("json_response.htmlt"))
		{
			CLog	log;

			log.Write(ERROR, string(__func__) + "[" + to_string(__LINE__) + "]:ERROR: template file was missing: ", "json_response_with_braces.htmlt");
			throw CException("Template file was missing");
		}

		indexPage.OutTemplate();

	}
	catch(CExceptionHTML &c)
	{
		CLog	log;

		c.SetLanguage(indexPage.GetLanguage());
		c.SetDB(&db);

		log.Write(ERROR, string(__func__) + "[" + to_string(__LINE__) + "]:ERROR: catch CExceptionHTML: exception reason: [", c.GetReason(), "]");

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

