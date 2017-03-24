#include "chat-server-protocol-message.h"

/* message protocol */
extern	CMysql						db;
extern	double						dbConnectionInitTimestamp;
struct per_session_data__message 	*connectionsList = NULL;
CPresenceCache						presenceCache;
CChatRequestRatelimiter				requestRatelimiter; 

inline bool isFileExists(const std::string& name) {
	struct stat buffer;
	return (stat (name.c_str(), &buffer) == 0);
}

// --- saving blob to /tmp/tmp_(prefix).jpg file
//     Input: void *content, uint size
//     Output: (prefix)
string SavePreImageToTmpLocation(const string &fileContent)
{
	// FILE	        *f;
	int		        folderID = (int)(rand()/(RAND_MAX + 1.0) * FEEDIMAGE_NUMBER_OF_FOLDERS) + 1;
	string	        filePrefix = GetRandom(20);
	string	        file2Check, tmpFile2Check, tmpImageJPG, fileName, fileExtention;
	size_t			base64Prefix = 0;

	{
		CLog	log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost << "SavePreImageToTmpLocation: start (size of file to be saved is " << fileContent.length() << ")";
		log.Write(DEBUG, ost.str());
	}


	//--- check image file existing
	do
	{
		ostringstream	ost;
        // string          tmp;
        // std::size_t  	foundPos;

		folderID = (int)(rand()/(RAND_MAX + 1.0) * CHAT_IMAGE_NUMBER_OF_FOLDERS) + 1;
		filePrefix = GetRandom(20);
        fileExtention = ".jpg";

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
		CLog	log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost << "int main(void): Save file to /tmp for checking of image validity [" << tmpFile2Check << "]";
		log.Write(DEBUG, ost.str());
	}

	// --- strip the header "data:image/XXXX;base64,"
	base64Prefix = fileContent.find(";base64,");
	if((base64Prefix != string::npos) && ((base64Prefix + strlen(";base64,")) < fileContent.length()))
	{
	    // --- Save file to "/tmp/" for checking of image validity
		// ofstream	ofs;
		// ofs.open(tmpFile2Check, ofstream::out);
		// ofs << (fileContent.substr(base64Prefix + strlen(";base64,")));
		// // ofs << base64_decode(fileContent.substr(base64Prefix + strlen(";base64,")));
		// ofs.close();
	}
	else
	{
		CLog	log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost << "int main(void): received file having problem with base64 encoding (file length " << fileContent.length() << " bytes)";
		log.Write(DEBUG, ost.str());
	}

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost << "SavePreImageToTmpLocation: end (file saved to " << tmpFile2Check << ")";
		log.Write(DEBUG, ost.str());
	}
    return filePrefix;
}


int	GetConnectionsNumber()
{
	struct per_session_data__message *connectionIterator = connectionsList;
	int			i = 0;
 
	i = 0; 
	while(connectionIterator)
	{
		connectionIterator = connectionIterator->nextConnection;
		i++;
	}
	return i;
}

bool	CloseSingleTextMessageConnection(struct per_session_data__message *connection)
{
	{
		CLog	log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "CloseSingleTextMessageConnection: begin (user = " << (connection->user ? connection->user->GetID() : "no-user") << ")";
		log.Write(DEBUG, ost.str());
	}

	for(int i = 0; i < CHAT_MAX_MESSAGE_QUEUE; i++)
		if(connection->messageList[i])
		{
			delete connection->messageList[i];
		}

	if(connection->user) delete connection->user;
	if(connection->packetReassemble) delete connection->packetReassemble;


	{
		CLog	log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "CloseSingleTextMessageConnection: end";
		log.Write(DEBUG, ost.str());
	}
	return	true;
}

bool	CloseAllTextMessageConnections()
{
	struct per_session_data__message *connectionIterator = connectionsList;
	int			i = 0;

	{
		CLog	log(CHAT_LOG_FILE_NAME);
		log.Write(DEBUG, "CloseAllTextMessageConnections: begin");
	}

	i = 0; 
	while(connectionIterator)
	{
 
		lws_close_reason(connectionIterator->wsi, LWS_CLOSE_STATUS_GOINGAWAY,
				 (unsigned char *)"server shutdown", 15);

		connectionIterator = connectionIterator->nextConnection;
		i++;
	}

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "CloseAllTextMessageConnections: end (closed " << i << " connections)";
		log.Write(DEBUG, ost.str());
	}

	return true;
}

int GetLengthWriteQueue(struct per_session_data__message *pss)
{
	int		messageQueueLength = 0;
	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "GetLengthWriteQueue: start (to user [" << (pss->user ? pss->user->GetID() : "no-user") << "])";
		log.Write(DEBUG, ost.str());
	}
	
	messageQueueLength = (pss->ringBufferHead - pss->ringBufferTail) & (CHAT_MAX_MESSAGE_QUEUE);

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "GetLengthWriteQueue: start (to user [" << (pss->user ? pss->user->GetID() : "no-user") << "], queue message len [" << messageQueueLength << "])";
		log.Write(DEBUG, ost.str());
	}

	return messageQueueLength;
}

string GetMessageInJSONFormat(CSingleMessage *messageObj)
{
	ostringstream	ost;
	char			temp[128];

	memset(temp, 0, 128);
	convert_cp1251_to_utf8(GetHumanReadableTimeDifferenceFromNow(messageObj->GetEventTimestamp()).c_str(), temp, 128);


	ost.str("");
	ost << "{\"RequestType\":\"" << messageObj->GetRequestType() << "\",\
\"RequestID\":\"" << messageObj->GetRequestID() << "\",\
\"id\":\"" << messageObj->GetID() << "\",\
\"toType\":\"" << messageObj->GetToType() << "\",\
\"toID\":\"" << messageObj->GetToID() << "\",\
\"fromType\":\"" << messageObj->GetFromType() << "\",\
\"fromID\":\"" << messageObj->GetFromID() << "\",\
\"messageStatus\":\"" << messageObj->GetMessageStatus() << "\",\
\"eventTimestamp\":\"" << messageObj->GetEventTimestamp() << "\",\
\"eventTimestampDelta\":\"" << temp << "\",\
\"secondsSinceY2k\":\"" << to_string(messageObj->GetSecondsSince2000()) << "\",\
\"message\":\"" << messageObj->GetMessage() << "\"}";

	return ost.str();
}

string GetSimpleTextInJSONFormat(CSingleMessage *messageObj)
{
	ostringstream		ost;
	char				tempHumanReadableTimestamp[128];
	char				*tempMessage;
	int					tempMessageLen = messageObj->GetMessage().length() * 4;
	unique_ptr<char[]> 	tempSmartPointer(new char[tempMessageLen]);
	// auto				tempSmartPointer = make_unique<char[]>(tempMessageLen);

	memset(tempHumanReadableTimestamp, 0, 128);
	convert_cp1251_to_utf8(GetHumanReadableTimeDifferenceFromNow(messageObj->GetEventTimestamp()).c_str(), tempHumanReadableTimestamp, 128);

	tempMessage = tempSmartPointer.get();
	// tempMessage = (char *)malloc(tempMessageLen);
	memset(tempMessage, 0, tempMessageLen);
	convert_cp1251_to_utf8(messageObj->GetMessage().c_str(), tempMessage, tempMessageLen - 1);


	ost.str("");
	ost << "{\"RequestType\":\"" << messageObj->GetRequestType() << "\",\
\"RequestID\":\"" << messageObj->GetRequestID() << "\",\
\"id\":\"" << messageObj->GetID() << "\",\
\"toType\":\"" << messageObj->GetToType() << "\",\
\"toID\":\"" << messageObj->GetToID() << "\",\
\"fromType\":\"" << messageObj->GetFromType() << "\",\
\"fromID\":\"" << messageObj->GetFromID() << "\",\
\"messageStatus\":\"" << messageObj->GetMessageStatus() << "\",\
\"eventTimestamp\":\"" << messageObj->GetEventTimestamp() << "\",\
\"eventTimestampDelta\":\"" << tempHumanReadableTimestamp << "\",\
\"secondsSinceY2k\":\"" << to_string(messageObj->GetSecondsSince2000()) << "\",\
" << tempMessage << "}";

	// free(tempMessage);
	tempSmartPointer.reset(nullptr);

	return ost.str();
}


string GetStringToWrite(struct per_session_data__message *pss)
{
	string	result = "";

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "GetStringToWrite: start (to user [" << (pss->user ? pss->user->GetID() : "no-user") << "])";
		log.Write(DEBUG, ost.str());
	}
	
	if(pss->messageList[pss->ringBufferTail])
	{
		if(pss->messageList[pss->ringBufferTail]->GetRequestType() == "SendMessage")
		{
			result = GetMessageInJSONFormat(pss->messageList[pss->ringBufferTail]);
			{
				CLog			log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "GetStringToWrite: SendMessage: string (" << result << ")";
				log.Write(DEBUG, ost.str());
			}
		}
		else if(pss->messageList[pss->ringBufferTail]->GetRequestType() == "SendImage")
		{
			result = GetMessageInJSONFormat(pss->messageList[pss->ringBufferTail]);
			{
				CLog			log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "GetStringToWrite: SendImage: string (" << result << ")";
				log.Write(DEBUG, ost.str());
			}
		}
		else if(pss->messageList[pss->ringBufferTail]->GetRequestType() == "OpenSession")
		{
			result = GetSimpleTextInJSONFormat(pss->messageList[pss->ringBufferTail]);
			{
				CLog			log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "GetStringToWrite: OpenSession: string (" << result << ")";
				log.Write(DEBUG, ost.str());
			}
		}
		else if(pss->messageList[pss->ringBufferTail]->GetRequestType() == "GetInitialData")
		{
			result = GetSimpleTextInJSONFormat(pss->messageList[pss->ringBufferTail]);
			{
				CLog			log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "GetStringToWrite: GetInitialData: string (" << (result.length() > LOG_FILE_MAX_LENGTH ? (result.substr(0, LOG_FILE_MAX_LENGTH) + " ... (stripped due to long output)") : result) << ")";
				log.Write(DEBUG, ost.str());
			}
		}
		else if(pss->messageList[pss->ringBufferTail]->GetRequestType() == "GetMessageBlock")
		{
			result = GetSimpleTextInJSONFormat(pss->messageList[pss->ringBufferTail]);
			{
				CLog			log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "GetStringToWrite: GetMessageBlock: string (" << result << ")";
				log.Write(DEBUG, ost.str());
			}
		}
		else if(pss->messageList[pss->ringBufferTail]->GetRequestType() == "ChangeMessageStatus")
		{
			result = GetSimpleTextInJSONFormat(pss->messageList[pss->ringBufferTail]);
			{
				CLog			log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "GetStringToWrite: ChangeMessageStatus: string (" << result << ")";
				log.Write(DEBUG, ost.str());
			}
		}
		else if(pss->messageList[pss->ringBufferTail]->GetRequestType() == "PresenceUpdate")
		{
			result = GetSimpleTextInJSONFormat(pss->messageList[pss->ringBufferTail]);
			{
				CLog			log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "GetStringToWrite: PresenceUpdate: string (" << result << ")";
				log.Write(DEBUG, ost.str());
			}
		}
		else if(pss->messageList[pss->ringBufferTail]->GetRequestType() == "MessageTypingNotification")
		{
			result = GetSimpleTextInJSONFormat(pss->messageList[pss->ringBufferTail]);
			{
				CLog			log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "GetStringToWrite: MessageTypingNotification: string (" << result << ")";
				log.Write(DEBUG, ost.str());
			}
		}
		else
		{
			{
				CLog			log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "GetStringToWrite: ERROR: unsupported RequestType [" << pss->messageList[pss->ringBufferTail]->GetRequestType() << "]";
				log.Write(ERROR, ost.str());
			}
		}
	}
	else
	{
		result = "";
		// pss->ringBufferTail = (pss->ringBufferTail + 1) & (CHAT_MAX_MESSAGE_QUEUE);
		{
			CLog			log(CHAT_LOG_FILE_NAME);
			ostringstream	ost;

			ost.str("");
			ost << "GetStringToWrite: ERROR: message in messageList = NULL";
			log.Write(ERROR, ost.str());
		}
	}

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "GetStringToWrite: end (to user [" << (pss->user ? pss->user->GetID() : "no-user") << "])";
		log.Write(DEBUG, ost.str());
	}

	return result;
}

bool PopSingleMessageFromUserQueue(struct per_session_data__message *pss)
{
	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "PopSingleMessageFromUserQueue: start";
		log.Write(DEBUG, ost.str());
	}
	if(pss->messageList[pss->ringBufferTail])
	{
		delete pss->messageList[pss->ringBufferTail];
		pss->messageList[pss->ringBufferTail] = NULL;
		pss->ringBufferTail = (pss->ringBufferTail + 1) & (CHAT_MAX_MESSAGE_QUEUE);
	}
	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "PopSingleMessageFromUserQueue: finish";
		log.Write(DEBUG, ost.str());
	}

	return true;	
}

bool PushSingleMessageToUserQueue(CSingleMessage *singleMessage, struct per_session_data__message *pss)
{
	bool			result = false;
	unsigned int	queueSize;

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "AddSingleMessageToUserQueue: start (RequestID [" << singleMessage->GetRequestID() << "], message queue length [" << ((pss->ringBufferHead - pss->ringBufferTail) & (CHAT_MAX_MESSAGE_QUEUE)) << "], user [" << (pss->user ? pss->user->GetID() : "no-user") << "])";
		log.Write(DEBUG, ost.str());
	}

	queueSize = ((pss->ringBufferHead - pss->ringBufferTail) & (CHAT_MAX_MESSAGE_QUEUE));
	if(queueSize < (CHAT_MAX_MESSAGE_QUEUE - 1))
	{
		CSingleMessage	*tempSingleMessage = new CSingleMessage;

		*tempSingleMessage = *singleMessage;
		pss->messageList[pss->ringBufferHead] = tempSingleMessage;
		pss->ringBufferHead = (pss->ringBufferHead + 1) & (CHAT_MAX_MESSAGE_QUEUE);
		lws_callback_on_writable(pss->wsi);
		
		result = true;
	}
	else
	{
		lws_rx_flow_control(pss->wsi, 0); // --- libwebsocket throttling enabled, receiving from socket prohibited
		{
			CLog			log(CHAT_LOG_FILE_NAME);
			ostringstream	ost;

			ost.str("");
			ost << "AddSingleMessageToUserQueue:ERROR: drop due to queue overflow (queue size = " << queueSize << "), throttling enabled";
			log.Write(ERROR, ost.str());
		}
	}
	
	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "AddSingleMessageToUserQueue: end";
		log.Write(DEBUG, ost.str());
	}

	return result;
}

bool ReplicateMessageToAllConnectionsSrcUser(CSingleMessage *singleMessage)
{
	bool								result = true;
	struct per_session_data__message 	*connectionIterator = connectionsList;
	int									i;

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "ReplicateMessageToAllConnectionsSrcUser(userID[" << singleMessage->GetFromID() << "]): start";
		log.Write(DEBUG, ost.str());
	}

	i = 0; 
	while(connectionIterator)
	{
		if(connectionIterator->user)
		{

	 		if(singleMessage->GetFromID() == connectionIterator->user->GetID()) 
			{
				if(PushSingleMessageToUserQueue(singleMessage, connectionIterator))
				{

				}
				else
				{
					{
						CLog			log(CHAT_LOG_FILE_NAME);
						ostringstream	ost;

						ost.str("");
						ost << "ReplicateMessageToAllConnectionsSrcUser: ERROR can't replicate message to user [" << singleMessage->GetFromID() << "] connection";
						log.Write(ERROR, ost.str());
					} // --- CLog scoping

				} // --- if(PushSingleMessageToUserQueue)
			} // --- if(singleMessage->GetFromID() == connectionIterator->user->GetID())
		}
		else
		{
			{
				CLog			log(CHAT_LOG_FILE_NAME);
				struct timeval	stm;
				ostringstream	ost;

				gettimeofday(&stm, NULL);
				ost.str("");
				ost << "ReplicateMessageToAllConnectionsSrcUser: ERROR: cnx (cnx lifetime = " << to_string(stm.tv_sec - connectionIterator->tv_established.tv_sec) << " sec.) without user assigned, might be user do not OpenSession yet, if cnx will be alive long it must be dropped ...";

				log.Write(ERROR, ost.str());
			}
		} // --- if(connectionIterator->user)

		connectionIterator = connectionIterator->nextConnection;
		i++;
	}


	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "ReplicateMessageToAllConnectionsSrcUser: end (" << (result ? "true" : "false") << ")";
		log.Write(DEBUG, ost.str());
	}

	return result;
}

bool ReplicateMessageToAllConnectionsDstUser(CSingleMessage *singleMessage)
{
	bool								result = true;
	struct per_session_data__message 	*connectionIterator = connectionsList;
	int									i;

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "ReplicateMessageToAllConnectionsDstUser(userID[" << singleMessage->GetFromID() << "]): start";
		log.Write(DEBUG, ost.str());
	}

	i = 0; 
	while(connectionIterator)
	{
		if(connectionIterator->user)
		{

	 		if(singleMessage->GetToID() == connectionIterator->user->GetID()) 
			{
				if(PushSingleMessageToUserQueue(singleMessage, connectionIterator))
				{

				}
				else
				{
					{
						CLog			log(CHAT_LOG_FILE_NAME);
						ostringstream	ost;

						ost.str("");
						ost << "ReplicateMessageToAllConnectionsDstUser: ERROR can't replicate message to user [" << singleMessage->GetFromID() << "] connection";
						log.Write(ERROR, ost.str());
					} // --- CLog scoping

				} // --- if(PushSingleMessageToUserQueue(singleMessage, connectionIterator))
			} // --- if message_recipient == destination recipient
		}
		else
		{
			{
				CLog			log(CHAT_LOG_FILE_NAME);
				struct timeval	stm;
				ostringstream	ost;

				gettimeofday(&stm, NULL);
				ost.str("");
				ost << "ReplicateMessageToAllConnectionsSrcUser: ERROR: cnx (cnx lifetime = " << to_string(stm.tv_sec - connectionIterator->tv_established.tv_sec) << " sec.) without user assigned, might be user do not OpenSession yet, if cnx will be alive long it must be dropped ...";

				log.Write(ERROR, ost.str());
			}
		} // --- if(connectionIterator->user)

		connectionIterator = connectionIterator->nextConnection;
		i++;
	}


	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "ReplicateMessageToAllConnectionsDstUser: end (" << (result ? "true" : "false") << ")";
		log.Write(DEBUG, ost.str());
	}

	return result;
}

// "toUser", inputParams.isNameExists("toID"), "fromUser", pss->user->GetID(), inputParams.isNameExists("msg")
string SubmitMessageToDB(string toType, string toID, string fromType, string fromID, string messageUTF8, string messageStatus)
{
	string			result = "";
	ostringstream	ost;
	unsigned long	messageID;
	string			messageCP1251;

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "SubmitMessageToDB: start";
		log.Write(DEBUG, ost.str());
	}
	
	{
		unique_ptr<char[]>	tempSmartPointer(new char[messageUTF8.length() * 4]);
		// char	*tmpMessage = (char *)malloc(messageUTF8.length() * 2);
		char	*tmpMessage = tempSmartPointer.get();


		memset(tmpMessage, 0, messageUTF8.length() * 4);
		if(!convert_utf8_to_windows1251(messageUTF8.c_str(), tmpMessage, messageUTF8.length() * 4 - 1))
		{
			{
				CLog			log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "SubmitMessageToDB: ERROR: converting utf8->cp1251";
				log.Write(ERROR, ost.str());
			}
		}

		messageCP1251 = tmpMessage;
		// free(tmpMessage);
		messageCP1251 = CleanUPText(messageCP1251);
	}

	ost.str("");
	ost << "INSERT INTO `chat_messages` (`message`, `fromType`, `fromID`, `toType`, `toID`, `messageStatus`, `eventTimestamp`, `secondsSinceY2k`) \
			VALUES (\"" << messageCP1251 << "\",\"" << fromType << "\", \"" << fromID << "\", \"" << toType << "\",\"" << toID << "\", \"" << messageStatus << "\", NOW(), \"" << to_string((double)GetSecondsSinceY2k()) << "\")";
	messageID = db.InsertQueryDB(ost.str());
	if(messageID == 0)
	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "SubmitMessageToDB: ERROR: message has not been saved in DB";
		log.Write(DEBUG, ost.str());
	}
	else
	{
		ost.str("");
		ost << messageID;
		result = ost.str();
	}

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "SubmitMessageToDB: end (" << result << ")";
		log.Write(DEBUG, ost.str());
	}

	return result;
}

string GetChatInitialData(struct per_session_data__message *pss, const string activeUserID)
{
	ostringstream	ost, ostFinal, friendsSqlQuery, chatMessageQuery;
	string			sessid, lookForKey, userArray = "", messageArray = "";
	int				affected;

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "GetChatInitialData: start (" << (pss->user ? pss->user->GetID() : "no-user") << ")";
		log.Write(DEBUG, ost.str());
	}

	if(pss->user)
	{
		// --- build friend list from three sources
		// --- 1) all direct friends (table `users_friends`)
		// --- 2) friends from `contact_list` (table `contact_list`)
		// --- 3) user from WebSocket parameter ActiveUserID
		unordered_map<long int, string>		userList;


		friendsSqlQuery.str("");
		// --- 1) all direct friends (table `users_friends`)
		ost.str("");
		ost << "select `friendID` from `users_friends` where `userID`='" << pss->user->GetID() << "';";
		affected = db.Query(ost.str());
		if(affected)
		{
			for(int i = 0; i < affected; i++)
			{
				userList.emplace( atol(db.Get(i, "friendID")), "");
			}
		}
		// --- 2) friends from `contact_list` (table `contact_list`)
		ost.str("");
		ost << "select `friendID` from `chat_contact_list` where `userID`='" << pss->user->GetID() << "';";
		affected = db.Query(ost.str());
		if(affected)
		{
			for(int i = 0; i < affected; i++)
			{
				userList.emplace( atol(db.Get(i, "friendID")), "");
			}
		}

		// --- 3) user from WebSocket parameter ActiveUserID
		if(activeUserID.length() && stoul(activeUserID))
			if(userList.find(stoul(activeUserID)) == userList.end())
			{
				// --- add user from WebSocket param to contact_list
				userList.emplace( atol(activeUserID.c_str()), "");

				ost.str("");
				ost << "INSERT INTO `chat_contact_list`(`userID`, `friendID`) VALUES (\"" << pss->user->GetID() << "\",\"" << activeUserID << "\"),(\"" << activeUserID << "\",\"" << pss->user->GetID() << "\");";
				db.Query(ost.str());
			}

		if(userList.size())
		{
			friendsSqlQuery << "select * from `users` where `isActivated`='Y' and `isblocked`='N' and `id` IN (";
/*
			for(int i = 0; i < affected; i++)
			{
				friendsSqlQuery << (i > 0 ? ", " : "") << db.Get(i, "friendID");
			}
*/
			string	tempString = "";
			for(auto &it : userList)
			{
				tempString += (tempString.length() ? ", " : "");
				tempString += to_string(it.first);
			}
			friendsSqlQuery << tempString << ");";

			{
				CLog	log(CHAT_LOG_FILE_NAME);
				log.Write(DEBUG, "GetChatInitialData: query for JSON prepared [", friendsSqlQuery.str(), "]");
			}
			userArray = GetUserListInJSONFormat(friendsSqlQuery.str(), &db, pss->user);

			// --- get messages for friends users
			affected = db.Query(friendsSqlQuery.str());
			if(affected)
			{
				list<string>	friendList;
				for(int i = 0; i < affected; i++)
				{
					friendList.push_back(db.Get(i, "id"));
				}

				messageArray = "";
				for(auto it = friendList.begin(); it != friendList.end(); it++)
				{
					string 		tempString;
					long int	messageID30th = 0, messageIDLastUnread = 0;

					chatMessageQuery.str("");
					chatMessageQuery << "select `id` from `chat_messages` where \
					((`toID`  ='" << pss->user->GetID() << "') and (`fromID`='" << *it << "')) \
					or \
					((`toID`  ='" << *it << "') and (`fromID`='" << pss->user->GetID() << "')) ORDER BY  `chat_messages`.`id` DESC LIMIT " << (CHAT_MESSAGES_PER_PAGE - 1) << ", 1;";
					if(db.Query(chatMessageQuery.str()))
					{
						messageID30th = atol(db.Get(0, "id"));

						chatMessageQuery.str("");
						chatMessageQuery << "select `id` from `chat_messages` where \
						(((`toID`  ='" << pss->user->GetID() << "') and (`fromID`='" << *it << "')) \
						or \
						((`toID`  ='" << *it << "') and (`fromID`='" << pss->user->GetID() << "'))) \
						and \
						(`messageStatus`='unread' or `messageStatus`='sent' or `messageStatus`='delivered') \
						 ORDER BY  `chat_messages`.`id` ASC LIMIT 0, 1;";
						if(db.Query(chatMessageQuery.str()))
						{
							messageIDLastUnread = atol(db.Get(0, "id"));
						}
						else
						{
							// --- absolute value of messageIDLastUnread is _not_ important
							// --- it must be bigger, to avoid influencing
							messageIDLastUnread = messageID30th + 1;
						}

						if(messageID30th <= messageIDLastUnread)
						{
							chatMessageQuery.str("");
							chatMessageQuery << "select * from `chat_messages` where \
							((`toID`  ='" << pss->user->GetID() << "') and (`fromID`='" << *it << "')) \
							or \
							((`toID`  ='" << *it << "') and (`fromID`='" << pss->user->GetID() << "')) ORDER BY  `chat_messages`.`id` DESC LIMIT 0," << CHAT_MESSAGES_PER_PAGE << ";";
						}
						else
						{
							chatMessageQuery.str("");
							chatMessageQuery << "select * from `chat_messages` where \
							(((`toID`  ='" << pss->user->GetID() << "') and (`fromID`='" << *it << "')) \
							or \
							((`toID`  ='" << *it << "') and (`fromID`='" << pss->user->GetID() << "'))) \
							and \
							(`id`>='" << messageIDLastUnread << "') \
							 ORDER BY  `chat_messages`.`id` DESC;";
						}

					}
					else
					{
						chatMessageQuery.str("");
						chatMessageQuery << "select * from `chat_messages` where \
						((`toID`  ='" << pss->user->GetID() << "') and (`fromID`='" << *it << "')) \
						or \
						((`toID`  ='" << *it << "') and (`fromID`='" << pss->user->GetID() << "')) ORDER BY  `chat_messages`.`id` DESC LIMIT 0, " << CHAT_MESSAGES_PER_PAGE << ";";

					} // --- get 30-th messageID

					tempString = GetChatMessagesInJSONFormat(chatMessageQuery.str(), &db);
					messageArray += ((messageArray.length() && tempString.length()) ? "," : "") + tempString;
				}
			}


			ostFinal.str("");
			ostFinal << "\"result\": \"success\"," << std::endl;
			ostFinal << "\"userArray\": [" << userArray << "]," << std::endl;
			ostFinal << "\"messageArray\": [" << messageArray << "]";

	/*		friendsSqlQuery << "select * from `users` where `isActivated`='Y' and `isblocked`='N' and `id` IN (";
			for(int i = 0; i < affected; i++)
			{
				friendsSqlQuery << (i > 0 ? ", " : "") << db.Get(i, "friendID");
			}
			friendsSqlQuery << ");";

			{
				CLog	log;
				log.Write(DEBUG, "int main(void):action == JSON_chatGetInitialData: query for JSON prepared [", friendsSqlQuery.str(), "]");
			}
			userArray = GetUserListInJSONFormat(friendsSqlQuery.str(), &db, pss->user);

			chatMessageQuery.str("");
			chatMessageQuery << "select * from `chat_messages` where `toID`='" << pss->user->GetID() << "' or `fromID`='" << pss->user->GetID() << "';";
			messageArray = GetChatMessagesInJSONFormat(chatMessageQuery.str(), &db);

			ostFinal.str("");
			ostFinal << "\"result\": \"success\"," << std::endl;
			ostFinal << "\"userArray\": [" << userArray << "]," << std::endl;
			ostFinal << "\"messageArray\": [" << messageArray << "]";
	*/	}
		else
		{
			
			{
				CLog			log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "GetChatInitialData(): DEBUG: there is no friends on this account";
				log.Write(DEBUG, ost.str());
			}

			ostFinal.str("");
			ostFinal << "\"result\": \"fail\",";
			ostFinal << "\"description\": \"there is no friends on this account\",";
			ostFinal << "\"userArray\": [],";
			ostFinal << "\"messageArray\": []";
		} // --- if(user (pss->user->GetID()) found)
	}
	else
	{
		{
				CLog			log(CHAT_LOG_FILE_NAME);
				struct timeval	stm;
				ostringstream	ost;

				gettimeofday(&stm, NULL);
				ost.str("");
				ost << "ReplicateMessageToAllConnectionsSrcUser: ERROR: cnx (cnx lifetime = " << to_string(stm.tv_sec - pss->tv_established.tv_sec) << " sec.) without user assigned, might be user do not OpenSession yet, if cnx will be alive long it must be dropped ...";

				log.Write(ERROR, ost.str());
		}
	} // --- if(connectionIterator->user)

	{
		CLog	log(CHAT_LOG_FILE_NAME);
		log.Write(DEBUG, "GetChatInitialData: end");
	}

	return ostFinal.str();
}

string GetMessageBlock(string friendID, string minMessageID, struct per_session_data__message *pss)
{
	ostringstream	ostFinal, chatMessageQuery;
	string			messageArray = "";

	{
		CLog	log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "GetMessageBlock ( friendID" << friendID << ", minMessageID = " << minMessageID << ", pss ): start";
		log.Write(DEBUG, ost.str());
	}

	if(pss->user)
	{
		string tempString;
		messageArray = "";

		chatMessageQuery.str("");
		chatMessageQuery << "select * from `chat_messages` where \
		(((`toID`  ='" << pss->user->GetID() << "') and (`fromID`='" << friendID << "')) \
			or \
		((`toID`  ='" << friendID << "') and (`fromID`='" << pss->user->GetID() << "'))) \
		and \
		(`id` < '" << minMessageID << "') \
		ORDER BY  `chat_messages`.`id` DESC LIMIT 0 , 30;";
		messageArray = GetChatMessagesInJSONFormat(chatMessageQuery.str(), &db);

		ostFinal.str("");
		ostFinal << "\"result\": \"success\",";
		ostFinal << "\"friendID\": \"" << friendID << "\",";
		ostFinal << "\"messageArray\": [" << messageArray << "]";
	}
	else
	{
		{
				CLog			log(CHAT_LOG_FILE_NAME);
				struct timeval	stm;
				ostringstream	ost;

				gettimeofday(&stm, NULL);
				ost.str("");
				ost << "ReplicateMessageToAllConnectionsSrcUser: ERROR: cnx (cnx lifetime = " << to_string(stm.tv_sec - pss->tv_established.tv_sec) << " sec.) without user assigned, might be user do not OpenSession yet, if cnx will be alive long it must be dropped ...";

				log.Write(ERROR, ost.str());
		}
	} // --- if(connectionIterator->user)


	{
		CLog	log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "GetMessageBlock: end (response.length() = " << ostFinal.str().length() << ")";
		log.Write(DEBUG, ost.str());
	}

	return ostFinal.str();
}

bool ChangeMessageStatusInDB(string messageID, string messageStatus, struct per_session_data__message *pss)
{
	bool		result = false;

	{
		CLog	log(CHAT_LOG_FILE_NAME);
		log.Write(DEBUG, "ChangeMessageStatusInDB: start");
	}

	if(pss->user)
	{
		if(messageID.length() && messageStatus.length())
				{
					if((messageStatus == "unread") || (messageStatus == "read") || (messageStatus == "delivered") || (messageStatus == "sent") || (messageStatus == "unread_const"))
					{
						ostringstream	ost;
		
						ost.str("");
						ost << "select * from `chat_messages` where `id`='" << messageID << "';";
						if(db.Query(ost.str()))
						{
							if(db.Get(0, "toID") == pss->user->GetID())
							{
								result = true;
		
								ost.str("");
								ost << "UPDATE `chat_messages` SET `messageStatus`='" << messageStatus << "' WHERE `id`='" << messageID << "';";
								db.Query(ost.str());
							}
							else
							{
								{
									CLog	log(CHAT_LOG_FILE_NAME);
									ostringstream	ost;
		
									ost.str("");
									ost << "ChangeMessageStatusInDB: ERROR: user [" << pss->user->GetID() << "] can't update message status [" << messageID << "]. Only ["  << db.Get(0, "toID") << "] permitted to change status";
									log.Write(ERROR, ost.str());
								}
							}
						}
						else
						{
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								log.Write(ERROR, "ChangeMessageStatusInDB: ERROR: messageID [", messageID, "] not found in DB");
							}
						}
		
					}
					else
					{
						{
							CLog	log(CHAT_LOG_FILE_NAME);
							log.Write(ERROR, "ChangeMessageStatusInDB: ERROR: messageStatus is wrong");
						}
					}
				}
				else
				{
					{
						CLog	log(CHAT_LOG_FILE_NAME);
						log.Write(ERROR, "ChangeMessageStatusInDB: ERROR: one of mandatory parameters empty");
					}
				}
	}
	else
	{
		{
				CLog			log(CHAT_LOG_FILE_NAME);
				struct timeval	stm;
				ostringstream	ost;

				gettimeofday(&stm, NULL);
				ost.str("");
				ost << "ReplicateMessageToAllConnectionsSrcUser: ERROR: cnx (cnx lifetime = " << to_string(stm.tv_sec - pss->tv_established.tv_sec) << " sec.) without user assigned, might be user do not OpenSession yet, if cnx will be alive long it must be dropped ...";

				log.Write(ERROR, ost.str());
		}
	} // --- if(connectionIterator->user)

	{
		CLog	log(CHAT_LOG_FILE_NAME);
		log.Write(DEBUG, "ChangeMessageStatusInDB: end");
	}
	return result;
}

string GetFromDBByMessageID(string messageID)
{
	string		result = "";

	{
		CLog	log(CHAT_LOG_FILE_NAME);
		log.Write(DEBUG, "GetFromDBByMessageID: start");
	}

	if(messageID.length())
	{
		ostringstream	ost;

		ost.str("");
		ost << "select * from `chat_messages` where `id`='" << messageID << "';";
		if(db.Query(ost.str()))
		{
			result = db.Get(0, "fromID");
		}
		else
		{
			{
				CLog	log(CHAT_LOG_FILE_NAME);
				log.Write(ERROR, "GetFromDBByMessageID: ERROR: messageID [", messageID, "] not found in DB");
			}
		}
	}
	else
	{
		{
			CLog	log(CHAT_LOG_FILE_NAME);
			log.Write(ERROR, "GetFromDBByMessageID: ERROR: one of mandatory parameters empty");
		}
	}
	
	{
		CLog	log(CHAT_LOG_FILE_NAME);
		log.Write(DEBUG, "GetFromDBByMessageID: end");
	}
	return result;
}

bool FillinUserIDBySessID(string sessIDFromChat, struct per_session_data__message *pss)
{
	bool			result = true;
	ostringstream	ost;
	CCgi			indexPage;
	CUser			*userTemp;

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "FillinUserIDBySessID(" << sessIDFromChat << ", *pss): begin";
		log.Write(DEBUG, ost.str());
	}

		indexPage.SetDB(&db);
		if(indexPage.SessID_Load_FromDB(sessIDFromChat)) 
		{
			if(indexPage.SessID_CheckConsistency()) 
			{
				{
					if(indexPage.SessID_Get_UserFromDB() != "Guest") {
						userTemp = new CUser();
						userTemp->SetDB(&db);
						// --- place 2change user from user.email to user.id 
						if(userTemp->GetFromDBbyEmailNoPassword(indexPage.SessID_Get_UserFromDB()))
						{
							ostringstream	ost1;
							string			avatarPath;

							pss->user = userTemp;

							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost << "FillinUserIDBySessID: user [" << userTemp->GetLogin() << "] logged in";
								log.Write(DEBUG, ost.str());
							}
						}
						else
						{
							// --- enforce to close session
							result = false;

							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost << "FillinUserIDBySessID: ERROR user [" << indexPage.SessID_Get_UserFromDB() << "] session exists on client device, but not in the DB. Change the [action = logout].";
								log.Write(ERROR, ost.str());
							}

						}
					}
				

				}
			}
			else 
			{
				{
					CLog			log(CHAT_LOG_FILE_NAME);
					ostringstream	ost;

					ost.str("");
					ost << "FillinUserIDBySessID: ERROR session consistency check failed";
					log.Write(ERROR, ost.str());
				}

				result = false;
			}
		}
		else 
		{
			ostringstream	ost;

			{
				CLog			log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "FillinUserIDBySessID: cookie session and DB session is not equal. Need to recreate session";
				log.Write(DEBUG, ost.str());
			}

			result = false;
		} // --- if(indexPage.SessID_Load_FromDB(sessidHTTP)) 
	

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "FillinUserIDBySessID: end";
		log.Write(DEBUG, ost.str());
	}

	return result;
} 

// --- Quote Words: split string into vector<string>
// --- input: string, reference to vector
// --- output: number converted words
int BuildUsersMapFromString(const string src, unordered_map<long int, string> &dst)
{
	string		trimmedStr = src;
	int			wordCounter = 0;

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "BuildUsersMapFromString(" << src << "): start";
		log.Write(DEBUG, ost.str());
	}

	trim(trimmedStr);

	if(trimmedStr.length())
	{
		std::size_t	prevPointer = 0, nextPointer;

		prevPointer = 0, wordCounter = 0;
		do
		{
			nextPointer = trimmedStr.find(",", prevPointer);
			if(nextPointer == string::npos)
			{
				dst.emplace( atol(trimmedStr.substr(prevPointer).c_str()), "");
			}
			else
			{
				dst.emplace( atol(trimmedStr.substr(prevPointer, nextPointer - prevPointer).c_str()), "");
			}
			prevPointer = nextPointer + 1;
			wordCounter++;
		} while( (nextPointer != string::npos) );
	}

	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "BuildUsersMapFromString(" << src << "): end (number converted words = " << wordCounter << ")";
		log.Write(DEBUG, ost.str());
	}

	return wordCounter;
}

int	PacketReassembly(struct per_session_data__message *pss, void *in, size_t len)
{
	int result = 0;

	{
		CLog	log(CHAT_LOG_FILE_NAME);
		log.Write(DEBUG, "PacketReassembly: start (len = ", to_string(len), ")");
	} // --- CLog

	// --- packet has been fragmented
	if(pss->packetReassemble == NULL)
	{
		{
			CLog	log(CHAT_LOG_FILE_NAME);
			ostringstream	ost;

			ost.str("");
			ost << "PacketReassembly: allocate " << CHAT_MAX_PACKET_SIZE << " bytes memory for fragmented packet";
			log.Write(DEBUG, ost.str());
		} // --- CLog

		pss->packetSize = 0;
		pss->packetAllocatedSize = CHAT_MAX_PACKET_SIZE;
		pss->packetReassemble = (char *)malloc(CHAT_MAX_PACKET_SIZE);
		if(!pss->packetReassemble)
		{
			{
				CLog	log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "PacketReassembly: ERROR: fail to allocate memory for packet reassembly";
				log.Write(ERROR, ost.str());
			} // --- CLog

			result = -1;
		}
		else
		{
			memset(pss->packetReassemble, 0, CHAT_MAX_PACKET_SIZE);
		}
	}
	if((len + pss->packetSize) >= (pss->packetAllocatedSize - 1))
	{
		// reallocate  memory
		{
			CLog	log(CHAT_LOG_FILE_NAME);
			ostringstream	ost;

			ost.str("");
			ost << "PacketReassembly: reallocate additional chunk (" << CHAT_MAX_PACKET_SIZE << " bytes, final packet size = " << (pss->packetAllocatedSize + CHAT_MAX_PACKET_SIZE) << ")";
			log.Write(DEBUG, ost.str());
		} // --- CLog
		
		pss->packetReassemble = (char *)realloc(pss->packetReassemble, pss->packetAllocatedSize + CHAT_MAX_PACKET_SIZE);
		if(!pss->packetReassemble)
		{
			{
				CLog	log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "PacketReassembly: ERROR: fail to reallocate memory for packet reassembly";
				log.Write(ERROR, ost.str());
			} // --- CLog

			result = -1;
		}
		else
		{
			pss->packetAllocatedSize += CHAT_MAX_PACKET_SIZE;
			memset(pss->packetReassemble + pss->packetSize, 0, pss->packetAllocatedSize - pss->packetSize);
		}
	}
	if((len + pss->packetSize) < (pss->packetAllocatedSize - 1))
	{
		{
			CLog	log(CHAT_LOG_FILE_NAME);
			ostringstream	ost;

			ost.str("");
			ost << "PacketReassembly: copy fragment at offset " << pss->packetSize;
			log.Write(DEBUG, ost.str());
		} // --- CLog
		memcpy(pss->packetReassemble + pss->packetSize, in, len);
		pss->packetSize += len;
		pss->packetReassemble[pss->packetSize] = 0;

		result = pss->packetSize;
/*
		{
			CLog	log(CHAT_LOG_FILE_NAME);
			ostringstream	ost;

			ost.str("");
			log.Write(DEBUG, "PacketReassembly: ChunkDump:" (char *)in);
		} // --- CLog
*/
	}
	else
	{
		{
			CLog	log(CHAT_LOG_FILE_NAME);
			ostringstream	ost;

			ost.str("");
			ost << "PacketReassembly: ERROR: reassembled packet too big (" << len + pss->packetSize << " > " << CHAT_MAX_PACKET_SIZE << ")";
			log.Write(ERROR, ost.str());
		} // --- CLog

		result = -1;
	}

	{
		CLog	log(CHAT_LOG_FILE_NAME);
		log.Write(DEBUG, "PacketReassembly: end");
	} // --- CLog

	return result;
}

bool CheckDBConnectionReset()
{
	double 		currentTimestamp;
	bool		result = true;

	{
		CLog	log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "ChekDBConnectionReset: start";
		log.Write(DEBUG, ost.str());
	} // --- CLog

	currentTimestamp = GetSecondsSinceY2k();
	if((currentTimestamp - dbConnectionInitTimestamp) > CHAT_DB_CONNECTION_RESET_TIMEOUT)
	{
		// --- reset DB timeout connection
		dbConnectionInitTimestamp = currentTimestamp;
			
		db.CloseDB();
		if(db.Connect(DB_NAME, DB_LOGIN, DB_PASSWORD) < 0)
		{
			result = false;
			{
				CLog	log(CHAT_LOG_FILE_NAME);
				log.Write(ERROR, "CheckDBConnectionReset: ERROR: Can not connect to mysql database");
			}
		}

#ifdef MYSQL_4
		db.Query("set names cp1251");
#endif

	}

	{
		CLog	log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "ChekDBConnectionReset: end (result = " << (result ? "true" : "false") << ")";
		log.Write(DEBUG, ost.str());
	} // --- CLog

	return result;
}


int
callback_lws_message(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	// unsigned char buf[LWS_PRE + 512];
	struct per_session_data__message *pss =
			(struct per_session_data__message *)user;
	// unsigned char *p = &buf[LWS_PRE];
	char name[128], rip[128];

	if(!CheckDBConnectionReset())
	{
		{
			CLog	log(CHAT_LOG_FILE_NAME);
			log.Write(DEBUG, "callback_lws_message: ERROR: error reconnecting to DB, exit from thread.");
		}
		
		return -1;
	}

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:

		{
			CLog	log(CHAT_LOG_FILE_NAME);
			log.Write(DEBUG, "callback_lws_message: LWS_CALLBACK_ESTABLISHED: begin");
		}
 
		lws_set_extension_option(wsi, "permessage-deflate", "rx_buf_size", "10");

		pss->nextConnection = connectionsList;
		connectionsList = pss;

		pss->wsi = wsi;
		lws_get_peer_addresses(wsi, lws_get_socket_fd(wsi), name,
		       sizeof(name), rip, sizeof(rip));
		sprintf(pss->ip, "%s (%s)", name, rip);
		gettimeofday(&pss->tv_established, NULL);
		strcpy(pss->user_agent, "unknown");
		lws_hdr_copy(wsi, pss->user_agent, sizeof(pss->user_agent),
			     WSI_TOKEN_HTTP_USER_AGENT);

		memset(pss->sessID, 0, sizeof(pss->sessID));
		pss->user = NULL;
		{
			CLog			log(CHAT_LOG_FILE_NAME);
			ostringstream	ost;

			ost.str("");
			ost << "callback_lws_message: LWS_CALLBACK_ESTABLISHED: end (# of conn's: " << GetConnectionsNumber() << ")";
			log.Write(DEBUG, ost.str());
		}
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		{
			int		bytesWrittenToSocket = 0;
			{
				CLog	log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "callback_lws_message: LWS_CALLBACK_SERVER_WRITEABLE: start";
				log.Write(DEBUG, ost.str());
			}

			if(!pss->closeFlag)
			{
				bytesWrittenToSocket = 0;
				if(GetLengthWriteQueue(pss))
				{
					string	writeString = GetStringToWrite(pss);
					PopSingleMessageFromUserQueue(pss);

					if(writeString.length())
					{

						int					bufferLength = LWS_SEND_BUFFER_PRE_PADDING + writeString.length() + LWS_SEND_BUFFER_POST_PADDING;
						unique_ptr<char[]>	tempSmartPointer(new char[bufferLength]);	
						char				*bufferToWrite;

						// bufferToWrite = (char *)malloc(bufferLength);
						bufferToWrite = tempSmartPointer.get();

						memset(bufferToWrite, 0, bufferLength);
						memcpy(bufferToWrite + LWS_SEND_BUFFER_PRE_PADDING, writeString.c_str(), bufferLength - (LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING));
						bytesWrittenToSocket = lws_write(wsi, (unsigned char *)(bufferToWrite + LWS_SEND_BUFFER_PRE_PADDING), bufferLength - (LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING), LWS_WRITE_TEXT);
						if (bytesWrittenToSocket < (bufferLength - (LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING)))
						{
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_SERVER_WRITEABLE: ERROR written " << bytesWrittenToSocket << " bytes, but requires " << (bufferLength - (LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING)) << ".";
								log.Write(ERROR, ost.str());
							}
							lwsl_err("ERROR %d of %d writing to di socket\n", bytesWrittenToSocket, bufferLength);
							return -1;
						}

						// free(bufferToWrite);
						tempSmartPointer.reset(nullptr);

						if(GetLengthWriteQueue(pss) == (CHAT_MAX_MESSAGE_QUEUE - 15))
						{
							lws_rx_flow_control(wsi, 1);
							{
								CLog			log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_SERVER_WRITEABLE: socket throttling disabled (queue length = " << GetLengthWriteQueue(pss) << ")";
								log.Write(ERROR, ost.str());
							}		
						}

						if(GetLengthWriteQueue(pss))
							lws_callback_on_writable(pss->wsi);
					}
					else
					{
						{
							CLog	log(CHAT_LOG_FILE_NAME);
							ostringstream	ost;

							ost.str("");
							ost << "callback_lws_message: LWS_CALLBACK_SERVER_WRITEABLE: ERROR: message to write is empty";
							log.Write(ERROR, ost.str());
						} // --- CLog scope
					} // --- if(writeString.length())
				} // --- if(GetLengthWriteQueue(pss))
			} // --- if(!pss->closeFlag)
			else
			{
				{
					CLog	log(CHAT_LOG_FILE_NAME);
					ostringstream	ost;

					ost.str("");
					ost << "callback_lws_message: LWS_CALLBACK_SERVER_WRITEABLE: signal connection to close (cnxID = " << hex << pss->wsi << " )";
					log.Write(DEBUG, ost.str());
				}

				// --- returning negative gracefully closing connection
				return -1;
			}

			{
				CLog	log(CHAT_LOG_FILE_NAME);
				ostringstream	ost;

				ost.str("");
				ost << "callback_lws_message: LWS_CALLBACK_SERVER_WRITEABLE: end (written " << bytesWrittenToSocket << " bytes.)";
				log.Write(DEBUG, ost.str());
			}
			break;
		}

	case LWS_CALLBACK_RECEIVE:
		{
			CLog	log(CHAT_LOG_FILE_NAME);
			ostringstream	ost;

			ost.str("");
			ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: begin (number received bytes " << len << ", remaining packet payload " <<  lws_remaining_packet_payload(wsi) <<" , final frag " << lws_is_final_fragment(wsi) << ")";
			log.Write(DEBUG, ost.str());
		}

		if(!lws_remaining_packet_payload(wsi) && lws_is_final_fragment(wsi))
		{
			// --- final fragment or packet has not been fragmented
			string	socketRawData = (const char*)in;
			JSONParser inputParams;

			{
				int	result = PacketReassembly(pss, in, len);
				if(result == -1) 
				{
					{
						CLog	log(CHAT_LOG_FILE_NAME);
						ostringstream	ost;

						ost.str("");
						ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: ERROR: memory allocation error";
						log.Write(ERROR, ost.str());
					}
					return -1;
				}
				if(result == 0)
				{
					{
						CLog	log(CHAT_LOG_FILE_NAME);
						ostringstream	ost;

						ost.str("");
						ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: ERROR: packet too big";
						log.Write(ERROR, ost.str());
					}
				}

				socketRawData = (const char *)pss->packetReassemble;
				free(pss->packetReassemble);
				pss->packetReassemble = NULL;
			}


			if(inputParams.ParseJSONObject(socketRawData))
			{
				inputParams.DumpParamHash();

				if(inputParams.GetValue("RequestType") == "CloseSession")
				{
					{
						CLog	log(CHAT_LOG_FILE_NAME);
						ostringstream	ost;

						ost.str("");
						ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: CloseSession: start";
						log.Write(DEBUG, ost.str());
					}
					lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY,
							 (unsigned char *)"seeya", 5);
					return -1;
					
				}
				if(inputParams.GetValue("RequestType") == "GetInitialData")
				{
					ostringstream	ostResult;

					{
						CLog	log(CHAT_LOG_FILE_NAME);
						ostringstream	ost;

						ost.str("");
						ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: GetInitialData: start";
						log.Write(DEBUG, ost.str());
					}

					ostResult.str("");

					if(pss->user && (pss->user->GetLogin() != "Guest"))
					{

						if( inputParams.isNameExists("RequestType") && 
							inputParams.isNameExists("RequestID"))
						{
							ostResult.str("");
							ostResult << "\"status\":\"ok\"," << GetChatInitialData(pss, inputParams.GetValue("ActiveUserID"));
						}
						else
						{
							ostResult.str("");
							ostResult << "\"status\":\"fail\", \"description\": \"one of mandatory parameters missed\"";
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: GetInitialData: one of mandatory parameters missed";
								log.Write(DEBUG, ost.str());
							} // --- clog
						} // --- some mandatory parameters missed
					}
					else
					{
						ostResult.str("");
						ostResult << "\"status\":\"fail\", \"description\": \"user must login before sending messages\"";
						{
							CLog	log(CHAT_LOG_FILE_NAME);
							ostringstream	ost;

							ost.str("");
							ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: GetInitialData: user must login before sending messages";
							log.Write(DEBUG, ost.str());
						} // --- clog
					} // --- user must be logged in

					{
						CSingleMessage	*singleMessage;

						singleMessage = new CSingleMessage;

						singleMessage->SetMessage(ostResult.str());
						singleMessage->SetRequestType(inputParams.GetValue("RequestType"));
						singleMessage->SetRequestID(inputParams.GetValue("RequestID"));
						singleMessage->SetEventTimestamp(GetLocalFormattedTimestamp());
						singleMessage->SetSecondsSince2000(GetSecondsSinceY2k());

						if(PushSingleMessageToUserQueue(singleMessage, pss))
						{

						}
						else
						{
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: GetInitialData: ERROR: PushSingleMessageToUserQueue returns false";
								log.Write(ERROR, ost.str());
							} // --- end CLog
						}

						delete singleMessage;
					} // --- scope definition
				} // --- if(inputParams.GetValue("RequestType") == "GetInitialData")
				if(inputParams.GetValue("RequestType") == "GetMessageBlock")
				{
					ostringstream	ostResult;

					{
						CLog	log(CHAT_LOG_FILE_NAME);
						ostringstream	ost;

						ost.str("");
						ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: GetMessageBlock: start";
						log.Write(DEBUG, ost.str());
					}

					ostResult.str("");

					if(pss->user && (pss->user->GetLogin() != "Guest"))
					{

						if( inputParams.isNameExists("RequestType") && 
							inputParams.isNameExists("RequestID") && 
							inputParams.isNameExists("friendUser") && 
							inputParams.isNameExists("minMessageID"))
						{
							ostResult.str("");
							ostResult << "\"status\":\"ok\"," << GetMessageBlock(inputParams.GetValue("friendUser"), inputParams.GetValue("minMessageID"), pss);
						}
						else
						{
							ostResult.str("");
							ostResult << "\"status\":\"fail\", \"description\": \"one of mandatory parameters missed\"";
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: GetMessageBlock: one of mandatory parameters missed";
								log.Write(DEBUG, ost.str());
							} // --- clog
						} // --- some mandatory parameters missed
					}
					else
					{
						ostResult.str("");
						ostResult << "\"status\":\"fail\", \"description\": \"user must login before sending messages\"";
						{
							CLog	log(CHAT_LOG_FILE_NAME);
							ostringstream	ost;

							ost.str("");
							ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: GetMessageBlock: user must login before sending messages";
							log.Write(DEBUG, ost.str());
						} // --- clog
					} // --- user must be logged in

					{
						CSingleMessage	*singleMessage;

						singleMessage = new CSingleMessage;

						singleMessage->SetMessage(ostResult.str());
						singleMessage->SetRequestType(inputParams.GetValue("RequestType"));
						singleMessage->SetRequestID(inputParams.GetValue("RequestID"));
						singleMessage->SetEventTimestamp(GetLocalFormattedTimestamp());
						singleMessage->SetSecondsSince2000(GetSecondsSinceY2k());

						if(PushSingleMessageToUserQueue(singleMessage, pss))
						{

						}
						else
						{
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: GetMessageBlock: ERROR: PushSingleMessageToUserQueue returns false";
								log.Write(ERROR, ost.str());
							} // --- end CLog
						}

						delete singleMessage;
					} // --- scope definition
				} // --- if(inputParams.GetValue("RequestType") == "GetMessageBlock")
				if(inputParams.GetValue("RequestType") == "ChangeMessageStatus")
				{
					ostringstream	ostResult;
					bool			updateStatus = false;

					{
						CLog	log(CHAT_LOG_FILE_NAME);
						ostringstream	ost;

						ost.str("");
						ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: ChangeMessageStatus: start";
						log.Write(DEBUG, ost.str());
					}

					ostResult.str("");

					if(pss->user && (pss->user->GetLogin() != "Guest"))
					{

						if( inputParams.isNameExists("RequestType") && 
							inputParams.isNameExists("RequestID") && 
							inputParams.isNameExists("id") && 
							inputParams.isNameExists("messageStatus"))
						{
							if(ChangeMessageStatusInDB(inputParams.GetValue("id"), inputParams.GetValue("messageStatus"), pss))
							{
								updateStatus = true;
								ostResult.str("");
								ostResult << "\"status\":\"ok\"";
							}
							else
							{
								ostResult.str("");
								ostResult << "\"status\":\"fail\", \"description\": \"failed to update message status in DB\"";
								{
									CLog	log(CHAT_LOG_FILE_NAME);
									ostringstream	ost;

									ost.str("");
									ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: ChangeMessageStatus: failed to update message status in DB";
									log.Write(DEBUG, ost.str());
								} // --- clog
							} // --- DB update fail
							
						}
						else
						{
							ostResult.str("");
							ostResult << "\"status\":\"fail\", \"description\": \"one of mandatory parameters missed\"";
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: ChangeMessageStatus: one of mandatory parameters missed";
								log.Write(DEBUG, ost.str());
							} // --- clog
						} // --- some mandatory parameters missed
					}
					else
					{
						ostResult.str("");
						ostResult << "\"status\":\"fail\", \"description\": \"user must login before sending messages\"";
						{
							CLog	log(CHAT_LOG_FILE_NAME);
							ostringstream	ost;

							ost.str("");
							ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: ChangeMessageStatus: user must login before sending messages";
							log.Write(DEBUG, ost.str());
						} // --- clog
					} // --- user must be logged in

					if(updateStatus)
					{
						CSingleMessage	*singleMessage;

						singleMessage = new CSingleMessage;

						singleMessage->SetMessage(ostResult.str());
						singleMessage->SetRequestType(inputParams.GetValue("RequestType"));
						singleMessage->SetRequestID(inputParams.GetValue("RequestID"));
						singleMessage->SetID(inputParams.GetValue("id"));
						singleMessage->SetMessageStatus(inputParams.GetValue("messageStatus"));
						singleMessage->SetFromID(GetFromDBByMessageID(inputParams.GetValue("id")));
						singleMessage->SetEventTimestamp(GetLocalFormattedTimestamp());
						singleMessage->SetSecondsSince2000(GetSecondsSinceY2k());

						if(ReplicateMessageToAllConnectionsSrcUser(singleMessage))
						{

						}
						else
						{
							
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: ChangeMessageStatus: ERROR: PushSingleMessageToUserQueue returns false";
								log.Write(ERROR, ost.str());
							} // --- end CLog
						}

						delete singleMessage;
					} // --- scope definition
				} // --- if(inputParams.GetValue("RequestType") == "ChangeMessageStatus")
				if(inputParams.GetValue("RequestType") == "MessageTypingNotification")
				{
					ostringstream	ostResult;
					bool			typingStatus = false;

					{
						CLog	log(CHAT_LOG_FILE_NAME);
						ostringstream	ost;

						ost.str("");
						ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: MessageTypingNotification: start";
						log.Write(DEBUG, ost.str());
					}

					ostResult.str("");

					if(pss->user && (pss->user->GetLogin() != "Guest"))
					{

						if( inputParams.isNameExists("RequestType") && 
							inputParams.isNameExists("RequestID") && 
							inputParams.isNameExists("toID"))
						{
							typingStatus = true;
							ostResult.str("");
							ostResult << "\"status\":\"ok\"";
						}
						else
						{
							ostResult.str("");
							ostResult << "\"status\":\"fail\", \"description\": \"one of mandatory parameters missed\"";
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: MessageTypingNotification: one of mandatory parameters missed";
								log.Write(DEBUG, ost.str());
							} // --- clog
						} // --- some mandatory parameters missed
					}
					else
					{
						ostResult.str("");
						ostResult << "\"status\":\"fail\", \"description\": \"user must login before sending messages\"";
						{
							CLog	log(CHAT_LOG_FILE_NAME);
							ostringstream	ost;

							ost.str("");
							ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: MessageTypingNotification: user must login before sending messages";
							log.Write(DEBUG, ost.str());
						} // --- clog
					} // --- user must be logged in

					if(typingStatus)
					{
						CSingleMessage	*singleMessage;

						singleMessage = new CSingleMessage;

						singleMessage->SetMessage(ostResult.str());
						singleMessage->SetRequestType(inputParams.GetValue("RequestType"));
						singleMessage->SetRequestID(inputParams.GetValue("RequestID"));
						singleMessage->SetToID(inputParams.GetValue("toID"));
						singleMessage->SetFromID(pss->user->GetID());
						singleMessage->SetEventTimestamp(GetLocalFormattedTimestamp());
						singleMessage->SetSecondsSince2000(GetSecondsSinceY2k());

						if(ReplicateMessageToAllConnectionsDstUser(singleMessage))
						{

						}
						else
						{
							
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: MessageTypingNotification: ERROR: PushSingleMessageToUserQueue returns false";
								log.Write(ERROR, ost.str());
							} // --- end CLog
						}

						delete singleMessage;
					} // --- scope definition
				} // --- if(inputParams.GetValue("RequestType") == "MessageTypingNotification")
				if(inputParams.GetValue("RequestType") == "PresenceUpdate")
				{
					ostringstream	ostResult;
					bool			result = false;

					{
						CLog	log(CHAT_LOG_FILE_NAME);
						ostringstream	ost;

						ost.str("");
						ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: PresenceUpdate: start";
						log.Write(DEBUG, ost.str());
					}

					ostResult.str("");

					if(pss->user && (pss->user->GetLogin() != "Guest"))
					{

						if( inputParams.isNameExists("RequestType") && 
							inputParams.isNameExists("RequestID") && 
							inputParams.isNameExists("userList"))
						{
							string							strUserList = CleanUPText(inputParams.GetValue("userList"));
							unordered_map<long int, string>	usersMap;

							BuildUsersMapFromString(strUserList, usersMap);

							if(usersMap.size())
							{
								ostringstream	tempStr;
								tempStr.str("");

								for(auto it = usersMap.begin(); it != usersMap.end(); it++)
								{
									string		strSecondsSinceY2k = presenceCache.GetUserLastOnline(it->first);

									if(strSecondsSinceY2k.length())
									{

										tempStr << (tempStr.str().length() ? "," : "");
										tempStr << "{ \"" << it->first << "\": \"" << strSecondsSinceY2k << "\" }";
									}
									else
									{
										
										{
											CLog	log(CHAT_LOG_FILE_NAME);
											ostringstream	ost;

											ost.str("");
											ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: PresenceUpdate: ERROR: user [" << it->first << "] can't get cached data";
											log.Write(ERROR, ost.str());
										} // --- clog
									}
									
								}

								if(tempStr.str().length())
								{
									result = true;
									ostResult.str("");
									ostResult << "\"status\":\"ok\",\"presenceCache\":[" << tempStr.str() << "]";
								}
								else
								{
									ostResult << "\"status\":\"fail\", \"description\": \"failed to get data from cache\"";
									{
										CLog	log(CHAT_LOG_FILE_NAME);
										ostringstream	ost;

										ost.str("");
										ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: PresenceUpdate: ERROR: failed to get data from cache";
										log.Write(ERROR, ost.str());
									} // --- clog
								} // --- DB update fail
							}
							else
							{
								
									ostResult.str("");
									ostResult << "\"status\":\"fail\", \"description\": \"user list empty after parsing\"";
									{
										CLog	log(CHAT_LOG_FILE_NAME);
										ostringstream	ost;

										ost.str("");
										ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: PresenceUpdate: ERROR: user list empty after parsing";
										log.Write(ERROR, ost.str());
									} // --- clog
							}
							
						}
						else
						{
							ostResult.str("");
							ostResult << "\"status\":\"fail\", \"description\": \"one of mandatory parameters missed\"";
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: PresenceUpdate: ERROR: one of mandatory parameters missed";
								log.Write(ERROR, ost.str());
							} // --- clog
						} // --- some mandatory parameters missed
					}
					else
					{
						ostResult.str("");
						ostResult << "\"status\":\"fail\", \"description\": \"user must login before sending messages\"";
						{
							CLog	log(CHAT_LOG_FILE_NAME);
							ostringstream	ost;

							ost.str("");
							ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: PresenceUpdate: user must login before sending messages";
							log.Write(DEBUG, ost.str());
						} // --- clog
					} // --- user must be logged in

					if(result)
					{
						CSingleMessage	*singleMessage;

						singleMessage = new CSingleMessage;

						singleMessage->SetMessage(ostResult.str());
						singleMessage->SetRequestType(inputParams.GetValue("RequestType"));
						singleMessage->SetRequestID(inputParams.GetValue("RequestID"));
						singleMessage->SetEventTimestamp(GetLocalFormattedTimestamp());
						singleMessage->SetSecondsSince2000(GetSecondsSinceY2k());

						if(PushSingleMessageToUserQueue(singleMessage, pss))
						{

						}
						else
						{
							
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: PresenceUpdate: ERROR: PushSingleMessageToUserQueue returns false";
								log.Write(ERROR, ost.str());
							} // --- end CLog
						}

						delete singleMessage;
					} // --- scope definition
				} // --- if(inputParams.GetValue("RequestType") == "PresenceUpdate")
				if(inputParams.GetValue("RequestType") == "OpenSession")
				{
					string	sessid;
					string	result;
					string	remoteAddr = (pss->ip ? pss->ip : "");

					{
						CLog	log(CHAT_LOG_FILE_NAME);
						ostringstream	ost;

						ost.str("");
						ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: OpenSession: start";
						log.Write(DEBUG, ost.str());
					}


					requestRatelimiter.IncreaseCounter(remoteAddr);
					if(!requestRatelimiter.isRatelimited(remoteAddr))
					{
						sessid = inputParams.GetValue("sessid");
						if(sessid.length())
						{
							if(FillinUserIDBySessID(sessid, pss))
							{
								result = "\"status\":\"ok\"";
								{
									CLog	log(CHAT_LOG_FILE_NAME);
									ostringstream	ost;

									ost.str("");
									ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: OpenSession: session [" << sessid << "] created for user " << pss->user->GetID();
									log.Write(DEBUG, ost.str());
								} // --- end CLog

							}
							else
							{
								result = "\"status\":\"fail\",\"description\":\"there is no user matched by sessid\"";
								{
									CLog	log(CHAT_LOG_FILE_NAME);
									ostringstream	ost;

									ost.str("");
									ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: OpenSession: there is no user matched by sessid";
									log.Write(DEBUG, ost.str());
								} // --- end CLog
							} // --- if(FillinUserIDBySessID)
						}
						else
						{
							result = "\"status\":\"fail\",\"description\":\"sessid empty\"";
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: OpenSession: sessid empty, can't create session";
								log.Write(DEBUG, ost.str());
							} // --- end CLog
							
						}

						if(result.length())
						{
							CSingleMessage	*singleMessage;

							singleMessage = new CSingleMessage;

							singleMessage->SetMessage(result);
							singleMessage->SetRequestType(inputParams.GetValue("RequestType"));
							singleMessage->SetRequestID(inputParams.GetValue("RequestID"));
							singleMessage->SetEventTimestamp(GetLocalFormattedTimestamp());
							singleMessage->SetSecondsSince2000(GetSecondsSinceY2k());

							if(PushSingleMessageToUserQueue(singleMessage, pss))
							{
							}
							else
							{
								
								{
									CLog	log(CHAT_LOG_FILE_NAME);
									ostringstream	ost;

									ost.str("");
									ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: OpenSession: ERROR: PushSingleMessageToUserQueue returns false";
									log.Write(ERROR, ost.str());
								} // --- end CLog
							}

							delete singleMessage;
						} // --- if result is not empty
						else
						{
							
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: OpenSession: ERROR: response to OpenSession is empty, nothing to send back";
								log.Write(ERROR, ost.str());
							} // --- end CLog
						}
					} // --- open request has been rate limited 
					else
					{
						// close connection due to rate-limiting
						pss->closeFlag = true;
						{
							CLog	log(CHAT_LOG_FILE_NAME);
							ostringstream	ost;

							ost.str("");
							ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: OpenSession: ERROR: request OpenSession has been ratelimited";
							log.Write(ERROR, ost.str());
						} // --- end CLog

						lws_callback_on_writable(pss->wsi);
					} // --- open request has been rate limited 
				} // --- if(inputParams.GetValue("RequestType") == "OpenSession")
				if(inputParams.GetValue("RequestType") == "SendMessage")
				{
					ostringstream	ostResult;

					{
						CLog	log(CHAT_LOG_FILE_NAME);
						ostringstream	ost;

						ost.str("");
						ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendMessage: start";
						log.Write(DEBUG, ost.str());
					}

					ostResult.str("");

					if(pss->user && (pss->user->GetLogin() != "Guest"))
					{

						if( inputParams.isNameExists("RequestType") && 
							inputParams.isNameExists("RequestID") && 
							inputParams.isNameExists("toType") && 
							inputParams.isNameExists("toID") && 
							inputParams.isNameExists("message"))
						{
							string	messageBody = CleanUPText(inputParams.GetValue("message"));

							if(messageBody.length())
							{
								if(inputParams.GetValue("toType") == "toUser")
								{
									string	messageID = SubmitMessageToDB("toUser", inputParams.GetValue("toID"), "fromUser", pss->user->GetID(), inputParams.GetValue("message"), "sent");

									if(messageID.length())
									{
										CSingleMessage	*singleMessage;

										singleMessage = new CSingleMessage;

										singleMessage->SetID(messageID);
										singleMessage->SetMessage(messageBody);
										singleMessage->SetRequestType(inputParams.GetValue("RequestType"));
										singleMessage->SetRequestID(inputParams.GetValue("RequestID"));
										singleMessage->SetToID(inputParams.GetValue("toID"));
										singleMessage->SetFromID(pss->user->GetID());
										singleMessage->SetToType("toUser");
										singleMessage->SetFromType("fromUser");
										singleMessage->SetMessageStatus("sent");
										singleMessage->SetEventTimestamp(GetLocalFormattedTimestamp());
										singleMessage->SetSecondsSince2000(GetSecondsSinceY2k());

										if(ReplicateMessageToAllConnectionsSrcUser(singleMessage))
										{

										}
										else
										{
											{
												CLog	log(CHAT_LOG_FILE_NAME);
												ostringstream	ost;

												ost.str("");
												ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendMessage: can't replicate message to all connections source user";
												log.Write(DEBUG, ost.str());
											}
										}

										if(ReplicateMessageToAllConnectionsDstUser(singleMessage))
										{

										}
										else
										{
											{
												CLog	log(CHAT_LOG_FILE_NAME);
												ostringstream	ost;

												ost.str("");
												ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendMessage: can't replicate message to all connections destination user";
												log.Write(DEBUG, ost.str());
											}
										}

										delete singleMessage;
									}
									else
									{
										ostResult.str("");
										ostResult << "{ \"ResponseType\":\"SendMessage\", \"RequestID\":\"" << inputParams.GetValue("RequestID") << "\", \"status\":\"fail\", \"description\": \"error updating DB\" }";
										{
											CLog	log(CHAT_LOG_FILE_NAME);
											ostringstream	ost;

											ost.str("");
											ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendMessage: ERROR: error updating DB";
											log.Write(ERROR, ost.str());
										}
									}
								}
								else
								{
									ostResult.str("");
									ostResult << "{ \"ResponseType\":\"SendMessage\", \"RequestID\":\"" << inputParams.GetValue("RequestID") << "\", \"status\":\"fail\", \"description\": \"the only supported message type is [toUser]\" }";
									{
										CLog	log(CHAT_LOG_FILE_NAME);
										ostringstream	ost;

										ost.str("");
										ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendMessage: ERROR: the only supported message type is [toUser]";
										log.Write(ERROR, ost.str());
									} // --- clog
								} // --- supported message types
							}
							else
							{
								ostResult.str("");
								ostResult << "{ \"ResponseType\":\"SendMessage\", \"RequestID\":\"" << inputParams.GetValue("RequestID") << "\", \"status\":\"fail\", \"description\": \"message is empty\" }";
								{
									CLog	log(CHAT_LOG_FILE_NAME);
									ostringstream	ost;

									ost.str("");
									ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendMessage: empty message";
									log.Write(DEBUG, ost.str());
								} // --- clog
							} // --- message is empty ?
						}
						else
						{
							ostResult.str("");
							ostResult << "{ \"ResponseType\":\"SendMessage\", \"RequestID\":\"" << inputParams.GetValue("RequestID") << "\", \"status\":\"fail\", \"description\": \"one of mandatory parameters missed\" }";
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendMessage: one of mandatory parameters missed";
								log.Write(DEBUG, ost.str());
							} // --- clog
						} // --- some mandatory parameters missed
					}
					else
					{
						ostResult.str("");
						ostResult << "{ \"ResponseType\":\"SendMessage\", \"RequestID\":\"" << inputParams.GetValue("RequestID") << "\", \"status\":\"fail\", \"description\": \"user must login before sending messages\" }";
						{
							CLog	log(CHAT_LOG_FILE_NAME);
							ostringstream	ost;

							ost.str("");
							ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendMessage: user must login before sending messages";
							log.Write(DEBUG, ost.str());
						} // --- clog
					} // --- user must be logged in
				} // --- if(inputParams.GetValue("RequestType") == "SendMessage")
				if(inputParams.GetValue("RequestType") == "SendImage")
				{
					ostringstream	ostResult;

					{
						CLog	log(CHAT_LOG_FILE_NAME);
						ostringstream	ost;

						ost.str("");
						ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendImage: start";
						log.Write(DEBUG, ost.str());
					}

					ostResult.str("");

					if(pss->user && (pss->user->GetLogin() != "Guest"))
					{
						if( inputParams.isNameExists("RequestType") && 
							inputParams.isNameExists("RequestID") && 
							inputParams.isNameExists("toType") && 
							inputParams.isNameExists("toID") && 
							inputParams.isNameExists("fileType") && 
							inputParams.isNameExists("fileName") && 
							inputParams.isNameExists("fileSize") && 
							inputParams.isNameExists("blob"))
						{
							string	fileType = CleanUPText(inputParams.GetValue("fileType"));

							if(fileType.find("image") != string::npos)
							{
								if(inputParams.GetValue("toType") == "toUser")
								{
									string	tmpFileName = SavePreImageToTmpLocation(inputParams.GetValue("blob"));

//									string	messageID = SubmitMessageToDB("toUser", inputParams.GetValue("toID"), "fromUser", pss->user->GetID(), inputParams.GetValue("message"), "sent");
									string	messageID = "0";
									if(messageID.length())
									{
										CSingleMessage	*singleMessage;

										singleMessage = new CSingleMessage;

										singleMessage->SetID(messageID);
										singleMessage->SetMessage("here is image preview");
										singleMessage->SetRequestType(inputParams.GetValue("RequestType"));
										singleMessage->SetRequestID(inputParams.GetValue("RequestID"));
										singleMessage->SetToID(inputParams.GetValue("toID"));
										singleMessage->SetFromID(pss->user->GetID());
										singleMessage->SetToType("toUser");
										singleMessage->SetFromType("fromUser");
										singleMessage->SetMessageStatus("sent");
										singleMessage->SetEventTimestamp(GetLocalFormattedTimestamp());
										singleMessage->SetSecondsSince2000(GetSecondsSinceY2k());

										if(ReplicateMessageToAllConnectionsSrcUser(singleMessage))
										{

										}
										else
										{
											{
												CLog	log(CHAT_LOG_FILE_NAME);
												ostringstream	ost;

												ost.str("");
												ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendImage: can't replicate message to all connections source user";
												log.Write(DEBUG, ost.str());
											}
										}

/*
										if(ReplicateMessageToAllConnectionsDstUser(singleMessage))
										{

										}
										else
										{
											{
												CLog	log(CHAT_LOG_FILE_NAME);
												ostringstream	ost;

												ost.str("");
												ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendImage: can't replicate message to all connections destination user";
												log.Write(DEBUG, ost.str());
											}
										}
*/

										delete singleMessage;
									}
									else
									{
										ostResult.str("");
										ostResult << "{ \"ResponseType\":\"SendImage\", \"RequestID\":\"" << inputParams.GetValue("RequestID") << "\", \"status\":\"fail\", \"description\": \"error updating DB\" }";
										{
											CLog	log(CHAT_LOG_FILE_NAME);
											ostringstream	ost;

											ost.str("");
											ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendImage: ERROR: error updating DB";
											log.Write(ERROR, ost.str());
										}
									}
								}
								else
								{
									ostResult.str("");
									ostResult << "{ \"ResponseType\":\"SendImage\", \"RequestID\":\"" << inputParams.GetValue("RequestID") << "\", \"status\":\"fail\", \"description\": \"the only supported message type is [toUser]\" }";
									{
										CLog	log(CHAT_LOG_FILE_NAME);
										ostringstream	ost;

										ost.str("");
										ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendImage: ERROR: the only supported message type is [toUser]";
										log.Write(ERROR, ost.str());
									} // --- clog
								} // --- supported message types
							} // --- find "image" in filetype
							else
							{
								ostResult.str("");
								ostResult << "{ \"ResponseType\":\"SendImage\", \"RequestID\":\"" << inputParams.GetValue("RequestID") << "\", \"status\":\"fail\", \"description\": \"fileType is not image\" }";
								{
									CLog	log(CHAT_LOG_FILE_NAME);
									ostringstream	ost;

									ost.str("");
									ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendImage: fileType [" << fileType << "] is not image";
									log.Write(DEBUG, ost.str());
								} // --- clog
							} // --- message is empty ?
						}
						else
						{
							ostResult.str("");
							ostResult << "{ \"ResponseType\":\"SendImage\", \"RequestID\":\"" << inputParams.GetValue("RequestID") << "\", \"status\":\"fail\", \"description\": \"one of mandatory parameters missed\" }";
							{
								CLog	log(CHAT_LOG_FILE_NAME);
								ostringstream	ost;

								ost.str("");
								ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendImage: one of mandatory parameters missed";
								log.Write(DEBUG, ost.str());
							} // --- clog
						} // --- some mandatory parameters missed
					}
					else
					{
						ostResult.str("");
						ostResult << "{ \"ResponseType\":\"SendImage\", \"RequestID\":\"" << inputParams.GetValue("RequestID") << "\", \"status\":\"fail\", \"description\": \"user must login before sending messages\" }";
						{
							CLog	log(CHAT_LOG_FILE_NAME);
							ostringstream	ost;

							ost.str("");
							ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: SendImage: user must login before sending messages";
							log.Write(DEBUG, ost.str());
						} // --- clog
					} // --- user must be logged in
				} // --- if("SendImage")
				
			}
			else
			{
				{
					CLog	log(CHAT_LOG_FILE_NAME);
					ostringstream	ost;

					ost.str("");
					ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: fail to parse client request in json-format";
					log.Write(DEBUG, ost.str());
				} // --- CLog
			} // --- JSON parsing
		} // --- packet has been fragmented
		else
		{
			int	result = PacketReassembly(pss, in, len);
			if(result == -1) 
			{
				{
					CLog	log(CHAT_LOG_FILE_NAME);
					ostringstream	ost;

					ost.str("");
					ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: ERROR: memory allocation error";
					log.Write(ERROR, ost.str());
				}
				return -1;
			}
			if(result == 0)
			{
				{
					CLog	log(CHAT_LOG_FILE_NAME);
					ostringstream	ost;

					ost.str("");
					ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: ERROR: packet too big";
					log.Write(ERROR, ost.str());
				}
			}
		}

		{
			CLog	log(CHAT_LOG_FILE_NAME);
			ostringstream	ost;

			ost.str("");
			ost << "callback_lws_message: LWS_CALLBACK_RECEIVE: end";
			log.Write(DEBUG, ost.str());
		}
		break;
	/*
	 * this just demonstrates how to use the protocol filter. If you won't
	 * study and reject connections based on header content, you don't need
	 * to handle this callback
	 */
	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		dump_handshake_info(wsi);
		/* you could return non-zero here and kill the connection */
		break;

	/*
	 * this just demonstrates how to handle
	 * LWS_CALLBACK_WS_PEER_INITIATED_CLOSE and extract the peer's close
	 * code and auxiliary data.  You can just not handle it if you don't
	 * have a use for this.
	 */
	case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
		{
			CLog	log(CHAT_LOG_FILE_NAME);
			ostringstream	ost;

			ost.str("");
			ost << "callback_lws_message: LWS_CALLBACK_WS_PEER_INITIATED_CLOSE: begin (len = " << len << ")";
			log.Write(DEBUG, ost.str());
		}
		lwsl_notice("LWS_CALLBACK_WS_PEER_INITIATED_CLOSE: len %d\n", len);
		{
			CLog	log(CHAT_LOG_FILE_NAME);
			log.Write(DEBUG, "callback_lws_message: LWS_CALLBACK_WS_PEER_INITIATED_CLOSE: begin");
		}
		break;

	case LWS_CALLBACK_CLOSED:
		{
			CLog	log(CHAT_LOG_FILE_NAME);
			ostringstream	ost;

			ost.str("");
			ost << "callback_lws_message: LWS_CALLBACK_CLOSED: begin";
			log.Write(DEBUG, ost.str());
		}

		{
			struct per_session_data__message **pp;

			pp = &connectionsList;
			while (*pp) {
				if (*pp == pss) {
					*pp = pss->nextConnection;
					pss->nextConnection = NULL;
					break;
				}
				pp = &((*pp)->nextConnection);
			}

			CloseSingleTextMessageConnection(pss);
		}


		{
			CLog			log(CHAT_LOG_FILE_NAME);
			ostringstream	ost;

			ost.str("");
			ost << "callback_lws_message: LWS_CALLBACK_CLOSED: end (remaining # of conn's: " << GetConnectionsNumber() << ")";
			log.Write(DEBUG, ost.str());
		}
		break;

	default:
		break;
	}

	return 0;
}

