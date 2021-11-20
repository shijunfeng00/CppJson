#ifndef __SERIALIZABLE_EXCEPTION_H__
#define __SERIALIZABLE_EXCEPTION_H__
#include<string>
#include<sstream>
class NotSerializableException:public std::exception 
{ 
public:
    explicit NotSerializableException(const std::string&type_name);
    virtual ~NotSerializableException()throw();
    virtual const char*what()const throw();
protected: 
    std::string message;
}; 
class JsonDecodeException:public std::exception
{
public:
	explicit JsonDecodeException(const int line,const int column);
	virtual ~JsonDecodeException() throw();
	virtual const char*what()const throw()=0;
protected:
	std::string message;
};
class JsonDecodeNameException:public JsonDecodeException
{
public:
	explicit JsonDecodeNameException(const int line,const int column);
	virtual ~JsonDecodeNameException();
	virtual const char*what()const throw();
};
class JsonDecodeDelimiterException:public JsonDecodeException
{
public:
	explicit JsonDecodeDelimiterException(const int line,const int column);
	virtual ~JsonDecodeDelimiterException();
	virtual const char*what()const throw();
};
class JsonDecodeUnknowException:public JsonDecodeException
{
public:
	explicit JsonDecodeUnknowException();
	virtual ~JsonDecodeUnknowException();
	virtual const char*what()const throw();
};
/********************************************************************************************/
NotSerializableException::NotSerializableException(const std::string&type_name)
{
    std::ostringstream oss;
    oss<<"NotSerializableException:";
    oss<<"Object of type < "<<type_name<<" > is not JSON serializable.";
    message=oss.str();
}
NotSerializableException::~NotSerializableException()throw(){}
const char*NotSerializableException::what()const throw()
{ 
	return message.c_str();
}
/********************************************************************************************/
JsonDecodeException::JsonDecodeException(const int line,const int column):
	message("JsonDecodeException:")
	{}
JsonDecodeException::~JsonDecodeException()throw(){}
/********************************************************************************************/
JsonDecodeDelimiterException::~JsonDecodeDelimiterException(){}
const char*JsonDecodeDelimiterException::what()const throw()
{
	return this->message.c_str();
}

JsonDecodeDelimiterException::JsonDecodeDelimiterException(const int line,const int column):
	JsonDecodeException(line,column)
	{
		std::ostringstream oss;
		oss<<"Expecting ',' delimiter: line "<<line<<" column "<<column<<".";
		message+=oss.str();
	}
/********************************************************************************************/	
JsonDecodeNameException::~JsonDecodeNameException(){}
const char*JsonDecodeNameException::what()const throw()
{
	return this->message.c_str();
}

JsonDecodeNameException::JsonDecodeNameException(const int line,const int column):
	JsonDecodeException(line,column)
	{
		std::ostringstream oss;
		oss<<"Expecting property name enclosed in double quotes: line "<<line<<" column "<<column<<".";
		message+=oss.str();
	}
/********************************************************************************************/	
JsonDecodeUnknowException::~JsonDecodeUnknowException(){}
const char*JsonDecodeUnknowException::what()const throw()
{
	return this->message.c_str();
}

JsonDecodeUnknowException::JsonDecodeUnknowException():
	JsonDecodeException(0,0)
	{
		std::ostringstream oss;
		oss<<"An unknow error occurred in decoding Json data.";
		message+=oss.str();
	}
#endif
	
