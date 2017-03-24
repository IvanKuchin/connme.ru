#include "chat-server-singlemessage.h"

/*
CSingleMessage&	CSingleMessage::operator=(const CSingleMessage& src)
{
	{
		CLog			log(CHAT_LOG_FILE_NAME);
		ostringstream	ost;

		ost.str("");
		ost << "CSingleMessage::operator=: start";
		log.Write(DEBUG, ost.str());
	}

	if(this != &src)
	{
		SetID(src.GetID());
		SetRequestType(src.GetRequestType());
		SetMessage(src.GetMessage());
		SetMessageStatus(src.GetMessageStatus());
		SetRequestID(src.GetRequestID());
		SetToID(src.GetToID());
		SetFromID(src.GetFromID());
		SetToType(src.GetToType());
		SetFromType(src.GetFromType());
		SetEventTimestamp(src.GetEventTimestamp());
		SetSecondsSince2000(src.GetSecondsSince2000());
	}
	return *this;
}
*/
string CSingleMessage::GetID() const
{
	return id;
}

void CSingleMessage::SetID(string param)
{
	id = param;
}

string CSingleMessage::GetRequestType() const
{
	return requestType;
}

void CSingleMessage::SetRequestType(string param)
{
	requestType = param;
}

string CSingleMessage::GetMessage() const
{
	return message;
}

void CSingleMessage::SetMessage(string param)
{
	message = param;
}

string CSingleMessage::GetMessageStatus() const
{
	return messageStatus;
}

void CSingleMessage::SetMessageStatus(string param)
{
	messageStatus = param;
}

string CSingleMessage::GetRequestID() const
{
	return requestID;
}

void CSingleMessage::SetRequestID(string param)
{
	requestID = param;
}

string CSingleMessage::GetToID() const
{
	return toID;
}

void CSingleMessage::SetToID(string param)
{
	toID = param;
}

string CSingleMessage::GetFromID() const
{
	return fromID;
}

void CSingleMessage::SetFromID(string param)
{
	fromID = param;
}

string CSingleMessage::GetToType() const
{
	return toType;
}

void CSingleMessage::SetToType(string param)
{
	toType = param;
}

string CSingleMessage::GetFromType() const
{
	return fromType;
}

void CSingleMessage::SetFromType(string param)
{
	fromType = param;
}

string CSingleMessage::GetEventTimestamp() const
{
	return eventTimestamp;
}

void CSingleMessage::SetEventTimestamp(string param)
{
	eventTimestamp = param;
}

double CSingleMessage::GetSecondsSince2000() const
{
	return secondsSince2000;
}

void CSingleMessage::SetSecondsSince2000(double param)
{
	secondsSince2000 = param;
}

CSingleMessage::~CSingleMessage()
{

}

