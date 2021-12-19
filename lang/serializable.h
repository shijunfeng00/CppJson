#ifndef __SERIALIZABLE_H__
#define __SERIALIZABLE_H__
#include<iostream>
#include<functional>
#include<algorithm>
#include"reflectable.h"
#include"exception.h"
class Serializable:public Reflectable
{
public: 
	virtual ~Serializable(){};
	template<typename T> //如果该类是子类,使用Serializable::Inherit<Father>::get_config(this).          
	struct Inherit;      
	template<typename T,typename ...Args>
	struct MultiInherit;
	template<typename T,typename ...Args>
	struct Regist;                      
	template<typename T>                        
	struct Regist<T>;
	template<typename Object>
	inline static Config get_config(const Object*object);        //Reflectable::get_config
	template<typename Object>
	inline static std::string dumps(const Object&object);        //序列化对象
	template<typename Type>
	inline static std::string dumps(const std::initializer_list<Type>&object);
	template<typename Object=void*>
	inline static auto loads(const std::string&json);            //反序列化还原对象
	inline static void*loads(const std::string&json,const std::string&class_name)
	{
		void*object=nullptr;
		try
		{
			object=Reflectable::get_instance(class_name);
			ConfigPair::from_config_string[class_name](object,json);
			return object;
		}
		catch(JsonDecodeDelimiterException&e)
		{
			throw e;
		}
		catch(JsonDecodeUnknowException&e)
		{
			throw e;
		}
		catch(std::exception&e)                                                                   //在反序列化中由于错误的字段名或者不合法的Json字串导致解码失败
		{
			throw JsonDecodeUnknowException(__LINE__,__FILE__);
		}	
	}
	template<typename Object>                             
	inline static void from_config(Object*object,Config&config); //从Config中还原原始对象
	
	inline static Config decode(const std::string&json);   //从字符串中还原Config对象(仅自定义struct/class对象,因为这个只负责解析字典dict)
	inline static Config decode_view(const std::string&json);   //为替换为std::string_view，优化内存做准备，同时得到属性子串在原字串中的下标索引
};
template<typename Object>
Config Serializable::get_config(const Object*object)
{
	return Reflectable::get_config(object);	
}
template<typename Object>
void Serializable::from_config(Object*object,Config&config)
{
	std::string class_name=GET_TYPE_NAME(Object);
	for(auto&it:config)
	{
		if(it.first!="class_name")
		{
			auto&field_name=it.first;
			auto&value=it.second;
			std::string type=Reflectable::get_field_type(class_name,field_name);     //得到类型名称
			void*field=Reflectable::get_field(object,class_name,field_name);         //得到属性地址
			if(type[type.size()-1]=='*'&&value=="null")                              //空指针
				*(void**)field=nullptr;
			else 
				ConfigPair::from_config_string[type](field,value);                   //递归进行反序列化
		}
	}
}
template<typename Type>
std::string Serializable::dumps(const std::initializer_list<Type>&object)
{
	return ConfigPair::get_config_string<std::vector<Type>>(std::vector<Type>(object));
}
template<typename Object>
std::string Serializable::dumps(const Object&object)
{
	if constexpr(IsSerializableType<Object>::value)        //实现了Config get_config()const的结构体,类                                
		return object.get_config().serialized_to_string(); //如果没有实现get_config,会抛出NotSerializableException异常
	else
		return ConfigPair::get_config_string(object); //int,std::vector等可以进行序列化的类型
}                                                     //如果是其他不可序列化的类型，同样会抛出NotSerializableException异常
Config Serializable::decode(const std::string&json)                    
{
	enum State{init,parse_value,parse_struct,parse_fundamental,parse_iterable,parse_string,end_parse}state=init;
	std::string key,value;
	int nested_struct_layer=0,nested_iterable_layer=0;
	std::string serialized=[&]()->std::string //删掉转义字符和空格
	{
		bool is_in_string=false;
		std::string strip;
		for(auto&it:json)
		{
			if(it=='\"')
				is_in_string^=1;
			if(!is_in_string&&(it=='\n'||it=='\t'||it==' '))
				continue;
			strip.push_back(it);
		}
		return strip;
	}();
	int length=serialized.size();
	Config config;
	if(serialized[0]!='{')
		throw JsonDecodeDelimiterException('{');
	if(serialized[length-1]!='}')
		throw JsonDecodeDelimiterException('}');
	for(int i=0;i<length;++i)
	{
		auto&it=serialized[i];
		if(state==init)                                        //在冒号以前的字符为属性名
		{
			if(it==':')
				state=parse_value;                             //冒号以后就是属性值对应的字符串
			else if(it!='\"'&&it!='{'&&it!=','&&it!=' ')       //但是得排除两边的双引号
				key.push_back(it);
		}
		else if(state==parse_value)                            //开始解析结果
		{
			if(it=='{')                                        //如果是大括号包起来的，那就是struct对象
			{
				value.push_back(it);
				nested_struct_layer++;                         //{{{}}}嵌套,遇左大括号加1，又大括号-1,到0则结束
				state=parse_struct;
			}
			else if(it=='[')                                   //列表,"[1,2,3,4,5]"
			{
				value.push_back(it);
				nested_iterable_layer++;                       //可能遇到嵌套列表的情况
				state=parse_iterable;
			}
			else if(it=='\"')
			{
				value.push_back(it);
				state=parse_string;
			}
			else if(it!=' ')                                   //否则就是基本类型
			{
				value.push_back(it);
				state=parse_fundamental;
			}
		}
		else if(state==parse_string)
		{
			value.push_back(it);
			if(it=='\"'&&serialized[i-1]!='\\')                // \" 转义字符不是结束.
			{
				state=end_parse;
				--i;
			}
		}
		else if(state==parse_fundamental) 
		{
			if(it==','||it=='}'||it=='\"')                     //遇到逗号结束,对于最后一个属性，遇到的会是大括号，对于字符串，遇到双引号
			{
				if(it=='\"')
					value.push_back(it);                       //双引号需要算进值本身
				state=end_parse;
				--i;
				continue;
			}
			value.push_back(it);
		}
		else if(state==parse_iterable)                          //可能有嵌套的情况
		{
			if(it==']'||it=='[')
			{
				nested_iterable_layer+=(it==']'?-1:1);
				value.push_back(it);
				if(nested_iterable_layer==0)
				{
					state=end_parse;
					--i;
				}
				continue;
			}
			value.push_back(it);
		}
		else if(state==parse_struct)                             //遇到大括号结束
		{
			if(it=='}'||it=='{')
			{
				nested_struct_layer+=(it=='}'?-1:1);
				value.push_back(it);
				if(nested_struct_layer==0)
				{
					state=end_parse;
					--i;
				}
				continue;
			}
			value.push_back(it);
		}
		else if(state==end_parse)                                //解析完一个属性对应的键值对，记录到config中
		{
//			std::cout<<"key<"<<key<<"> value<"<<value<<">"<<std::endl;
			state=init;
			config[key]=value;
			key.clear();
			value.clear();
		}
	}
	if(!(state==end_parse&&nested_iterable_layer==0&&nested_struct_layer==0)) //不为零说明左右括号数量不匹配，说明字符串并不是合法的Json字串
	{ 
		if(nested_iterable_layer>0)
			throw JsonDecodeDelimiterException(']');
		else if(nested_iterable_layer<0)
			throw JsonDecodeDelimiterException('[');
		if(nested_struct_layer>0)
			throw JsonDecodeDelimiterException('}');
		else if(nested_struct_layer<0)
			throw JsonDecodeDelimiterException('{');
	}
	return config;
}
template<typename Object>
auto Serializable::loads(const std::string&json)
{
	std::string class_name=GET_TYPE_NAME(Object);
	if constexpr(IsSerializableType<Object>::value) //实现了get_config的可序列化对象
	{
		try
		{																																																																																																																																													
			Object object;
			ConfigPair::from_config_string[class_name](&object,json);                              //反序列化还原
			return object;
		}
		catch(JsonDecodeDelimiterException&e)
		{
			throw e;
		}
		catch(JsonDecodeUnknowException&e)
		{
			throw e;
		}
		catch(std::exception&e)                                                                   //在反序列化中由于错误的字段名或者不合法的Json字串导致解码失败
		{
			throw JsonDecodeUnknowException(__LINE__,__FILE__);
		}	
	}
	else //其他的没有实现get_config方法，但是是int,std::vector等可以序列化的class
	{
		try
		{
			Object object;
			ConfigPair::from_config_string[class_name](&object,json);
			return object;
		}
		catch(JsonDecodeDelimiterException&e)
		{
			throw e;
		}
		catch(JsonDecodeUnknowException&e)
		{
			throw e;
		}
		catch(std::exception&e)
		{
			throw JsonDecodeUnknowException(__LINE__,__FILE__);
		}
	}
}
template<typename T,typename ...Args>
struct Serializable::Regist
{
	Regist()
	{
		ConfigPair::from_config_string[GET_TYPE_NAME(T)]=[](void*object,const std::string&value)->void    //实质还是调用from_config进行递归.
		{
			Config config=Serializable::decode(value);
			Serializable::from_config((T*)object,config);
		};
		Reflectable::Regist<T>();
		Serializable::Regist<Args...>();
	}
};
template<typename T>
struct Serializable::Regist<T>
{
	Regist()
	{
		ConfigPair::from_config_string[GET_TYPE_NAME(T)]=[](void*object,const std::string&value)->void   //实质还是调用from_config进行递归.
		{
			Config config=Serializable::decode(value);
			Serializable::from_config((T*)object,config);
		};
		Reflectable::Regist<T>();
	}
};
template<typename Parent>
struct Serializable::Inherit
{
	template<typename Object>
	static Config get_config(const Object*object)
	{
		Config parent_config=Reflectable::Inherit<Parent>::get_config(object);
		Config config=Serializable::get_config(object);                                 //同时Config也要进行合并，得到父类Config，然后添加子类Config内容
		for(auto&it:parent_config)
			if(it.first!="class_name")
				config[it.first]=it.second;
		return config;
	}
};
#endif
