#ifndef __CHAT__SERVER__SINGLE__MESSAGE__H__
#define __CHAT__SERVER__SINGLE__MESSAGE__H__

#include <string>
#include "clog.h"

using namespace std;

class CSingleMessage {

private:
	string		requestType;
	string		id, requestID, toID, fromID, toType, fromType;
	string		message, messageStatus;
	string		eventTimestamp;
	double		secondsSince2000;

public:
					CSingleMessage() = default;
	// --- By default assign operator copy all member. 
	// --- It must be override in case of pointers or "deep cpying"
	// CSingleMessage&	operator=(const CSingleMessage& src);

	string			GetID() const;
	void			SetID(string param);
	string			GetRequestType() const;
	void			SetRequestType(string param);
	string			GetMessage() const;
	void			SetMessage(string param);
	string			GetMessageStatus() const;
	void			SetMessageStatus(string param);
	string			GetRequestID() const;
	void			SetRequestID(string param);
	string			GetToID() const;
	void			SetToID(string param);
	string			GetFromID() const;
	void			SetFromID(string param);
	string			GetToType() const;
	void			SetToType(string param);
	string			GetFromType() const;
	void			SetFromType(string param);
	string			GetEventTimestamp() const;
	void			SetEventTimestamp(string param);
	double			GetSecondsSince2000() const;
	void			SetSecondsSince2000(double param);
		
		
					~CSingleMessage();
};

#endif
