#ifndef __SERIALIZABLE_EXCEPTION_H__
#define __SERIALIZABLE_EXCEPTION_H__
#include<string>
#include<sstream>
struct NoSuchFieldException:public std::exception
{ 
public:
    explicit NoSuchFieldException(const std::string&type_name,const std::string&field_name);
    virtual ~NoSuchFieldException()throw();
    virtual const char*what()const throw();
protected: 
    std::string message;
};
struct NoSuchMethodException:public std::exception
{ 
public:
    explicit NoSuchMethodException(const std::string&type_name,const std::string&field_name);
    virtual ~NoSuchMethodException()throw();
    virtual const char*what()const throw();
protected: 
    std::string message;
};

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
class JsonDecodeDelimiterException:public JsonDecodeException
{
public:
	explicit JsonDecodeDelimiterException(const char&ch);
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
NoSuchFieldException::NoSuchFieldException(const std::string&type_name,const std::string&field_name)
{
	std::ostringstream oss;
	oss<<"Object of type <"<<type_name<<"> dose not has field named '"<<field_name<<"' .";
	this->message=oss.str();
}
NoSuchFieldException::~NoSuchFieldException()
{
	
}
const char*NoSuchFieldException::what()const throw()
{
	return this->message.c_str();
}
/********************************************************************************************/
NoSuchMethodException::NoSuchMethodException(const std::string&type_name,const std::string&field_name)
{
	std::ostringstream oss;
	oss<<"Object of type <"<<type_name<<"> dose not has method named '"<<field_name<<"' .";
	this->message=oss.str();
}
NoSuchMethodException::~NoSuchMethodException()
{
	
}
const char*NoSuchMethodException::what()const throw()
{
	return this->message.c_str();
}
/********************************************************************************************/
NotSerializableException::NotSerializableException(const std::string&type_name)
{
    std::ostringstream oss;
    oss<<"NotSerializableException:";
    oss<<"Object of type < "<<type_name<<" > is not JSON serializable.";
    message=oss.str();
}
NotSerializableException::~NotSerializableException()throw()
{
	
}
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
JsonDecodeDelimiterException::~JsonDecodeDelimiterException()
{
	
}
const char*JsonDecodeDelimiterException::what()const throw()
{
	return this->message.c_str();
}

JsonDecodeDelimiterException::JsonDecodeDelimiterException(const char&ch):
	JsonDecodeException(0,0)
	{
		std::ostringstream oss;
		oss<<"Expecting '"<<ch<<"' delimiter in decoding json data.";
		message+=oss.str();
	}
/********************************************************************************************/	
JsonDecodeUnknowException::~JsonDecodeUnknowException()
{
	
}
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
	
