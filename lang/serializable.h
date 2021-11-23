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
	template<typename T,typename ...Args>
	struct Regist;
	template<typename T>
	struct Inherit;                                       //如果该类是子类,使用Serializable::Inherit<Father>::get_config(this).暂时没考虑多继承的问题
	template<typename T>                                  //未来考虑采用Serializable::Inherit<FatherA,FatherB,FatherC>::get_config(this)来接受多继承问题
	struct Regist<T>;
	template<typename Object>
	static Config get_config(const Object*object);        //Reflectable::get_config
	template<typename Object>
	static std::string dumps(const Object&object);        //序列化对象
	template<typename Object=void*>
	static auto loads(const std::string&json);            //反序列化还原对象
	static Config decode(const std::string&serialized);   //从字符串中还原Config对象
	template<typename Object>                             
	static void from_config(Object*object,Config&config); //从Config中还原原始对象
};
template<typename Object>
Config Serializable::get_config(const Object*object)
{
	return Reflectable::get_config(object);	
}
template<typename Object>
void Serializable::from_config(Object*object,Config&config)
{
/*
	std::string&class_name=config["class_name"];
	class_name.erase(                                        
		std::remove_if(class_name.begin(),class_name.end(),[](auto ch){return ch=='\"';}),//去掉两边的引号
		class_name.end());
	std::cout<<class_name<<" "<<GET_TYPE_NAME(Object)<<std::endl;
	std::cout<<(class_name==std::string(GET_TYPE_NAME(Object)))<<std::endl<<std::endl;
*/
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
template<typename Object>
std::string Serializable::dumps(const Object&object)
{
	return object.get_config().serialized_to_string();
}
Config Serializable::decode(const std::string&serialized)                    
{
	constexpr int init=0;                                                          //定义各种状态
	constexpr int parse_value=1;
	constexpr int parse_struct=2;
	constexpr int parse_fundamental=3;
	constexpr int parse_iterable=4;
	constexpr int parse_string=5;
	constexpr int end_parse=6;
	std::string key;
	std::string value;
	int nested_struct_layer=0;
	int nested_iterable_layer=0;
	int state=init;
	int length=serialized.size();
	Config config;
	for(int i=0;i<length;++i)
	{
		auto&it=serialized[i];
		if(state==init)                                        //在冒号以前的字符为属性名
		{
			if(it==':')
				state=parse_value;                             //冒号以后就是属性值对应的字符串
			else if(it!='\"'&&it!='{'&&it!=','&&it!=' ')       //但是得排除两边的双引号
				key.push_back(it);
			if(it=='{')
				nested_struct_layer++;
			if(it=='[')
				nested_iterable_layer++;
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
			state=init;
			config[key]=value;
//			std::cout<<"<key>:"<<key<<"   <value>:"<<value<<std::endl;
			key.clear();
			value.clear();
		}
	}
	if(serialized[length-1]=='}')                               //特判最后一个字符
		nested_struct_layer--;                                  //因为他不属于"解析struct对象"，所以}不会被计算进去.
			
	if(!(state==end_parse&&nested_iterable_layer==0&&nested_struct_layer==0))
	{   //不为零说明左右括号数量不匹配，说明字符串并不是合法的Json字串
//		std::cout<<nested_iterable_layer<<" "<<nested_struct_layer<<std::endl;
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
template<typename Object=void*>
auto Serializable::loads(const std::string&json)
{
	static_assert(!std::is_same<Object,void*>::value,"Not implemented yet.");
	/*
	Config config=Serializable::decode(json);                                             //从json字符串还原Config
	std::string&class_name=config["class_name"];
	class_name.erase(                                        
		std::remove_if(class_name.begin(),class_name.end(),[](auto ch){return ch=='\"';}),//去掉两边的引号
		class_name.end());
	std::cout<<class_name<<" "<<GET_TYPE_NAME(Object)<<std::endl;
	std::cout<<(class_name==std::string(GET_TYPE_NAME(Object)))<<std::endl<<std::endl;
	*/
	std::string class_name=GET_TYPE_NAME(Object);
	void*object=nullptr;
	try
	{																																																																																																																																															
		object=Reflectable::get_instance(class_name);                                         //创建实例
		ConfigPair::from_config_string[class_name](object,json);                              //反序列化还原
		if constexpr(std::is_same<Object,void*>::value)
			return object;
		else
			return std::ref(*(Object*)object);	
	}
	catch(std::exception&e)                                                                   //在反序列化中由于错误的字段名或者不合法的Json字串导致解码失败
	{      
		Reflectable::delete_instance(class_name,object);                                      //暂时还没想好怎么去具体的检测是哪里出了什么问题，因此统一抛出一个Unknow异常.
		throw JsonDecodeUnknowException();
	}
	
}
template<typename T,typename ...Args>
struct Serializable::Regist
{
	Regist()
	{
		using Tptr=T*;
		ConfigPair::from_config_string[GET_TYPE_NAME(T)]=[](void*object,const std::string&value)->void    //实质还是调用from_config进行递归.
		{
			Config config=Serializable::decode(value);
			Serializable::from_config((T*)object,config);
		};
		ConfigPair::from_config_string[GET_TYPE_NAME(Tptr)]=[](void*object,const std::string&value)->void //对应的指针
		{
			Config config=Serializable::decode(value);
			if(value=="null")
			{
				(*(Tptr*)object)=nullptr;
			}
			else
			{
		   		(*(Tptr*)object)=(T*)Reflectable::get_instance(GET_TYPE_NAME(T));
				Serializable::from_config((*(Tptr*)object),config);
			}
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
		using Tptr=T*;
		ConfigPair::from_config_string[GET_TYPE_NAME(T)]=[](void*object,const std::string&value)->void   //实质还是调用from_config进行递归.
		{
			Config config=Serializable::decode(value);
			Serializable::from_config((T*)object,config);
		};
		ConfigPair::from_config_string[GET_TYPE_NAME(Tptr)]=[](void*object,const std::string&value)->void //对应的指针
		{
			Config config=Serializable::decode(value);
			if(value=="null")
			{
				(*(Tptr*)object)=nullptr;
			}
			else
			{
		   		(*(Tptr*)object)=(T*)Reflectable::get_instance(GET_TYPE_NAME(T));
				Serializable::from_config((*(Tptr*)object),config);
			}
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
		Config parent_config=object->Parent::get_config();
		auto sub_config=Reflectable::field.find(GET_TYPE_NAME(Object));
		if(sub_config==Reflectable::field.end())
			field[GET_TYPE_NAME(Object)]=Reflectable::field[GET_TYPE_NAME(Parent)];
		Config config=Serializable::get_config(object);
		for(auto&it:parent_config)
			if(it.first!="class_name")
				config[it.first]=it.second;
		return config;
	}
};
#endif
