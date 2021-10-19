#ifndef __SERIALIZABLE_H__
#define __SERIALIZABLE_H__
#include<iostream>
#include<functional>
#include<algorithm>
#include"reflectable.h"
class Serializable:public Reflectable
{
public: 
	virtual ~Serializable(){};
	template<typename T,typename ...Args>
	struct Regist;
	template<typename T>
	struct Regist<T>;
	template<typename Object>
	Config get_config(const Object*object)const; //Reflectable::get_config
	template<typename Object>
	static std::string dumps(const Object&object); //序列化对象
	template<typename Object=void*>
	static auto loads(const std::string&json);     //反序列化还原对象
	static std::unordered_map<std::string,std::function<void(void*,const std::string&)>>from_config_string;//不同类型反序列化函数表
	static Config decode(const std::string&serialized);                              
	void from_config(Config&config); //从Config中还原原始对象
};
std::unordered_map<std::string,std::function<void(void*,const std::string&)>>Serializable::from_config_string;
template<typename Object>
Config Serializable::get_config(const Object*object)const
{
	return Reflectable::get_config(object);
}
void Serializable::from_config(Config&config)
{
	std::string&class_name=config["class_name"];
	class_name.erase(                                        
		std::remove_if(class_name.begin(),class_name.end(),[](auto ch){return ch=='\"';}),//去掉两边的引号
		class_name.end());
	for(auto&it:config)
	{
		if(it.first!="class_name")
		{
			auto&field_name=it.first;
			auto&value=it.second;
			std::string&type=Reflectable::get_field_type(class_name,field_name);     //得到类型名称
			void*field=Reflectable::get_field(this,class_name,field_name);           //得到属性地址
			if(type[type.size()-1]=='*'&&value=="null")                              //空指针
				*(void**)field=nullptr;
			else if(value[0]=='{') //struct
				Serializable::from_config_string[type](field,value); //命名上一致
			else if(value[0]=='[') //list
				ConfigPair::from_config_string[type](field,value);
			else 
				ConfigPair::from_config_string[type](field,value);                    //修改值
		}
	}
}
template<typename Object>
std::string Serializable::dumps(const Object&object)
{
	return object.get_config().serialized_to_string();
}
Config Serializable::decode(const std::string&serialized) //简陋的有限状态自动机？
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
		}
		else if(state==parse_value) //开始解析结果
		{
			if(it=='{')                                        //如果是大括号包起来的，那就是struct对象
			{
				value.push_back(it);
				nested_struct_layer++;  //{{{}}}嵌套,遇左大括号加1，又大括号-1,到0则结束
				state=parse_struct;
			}
			else if(it=='[')
			{
				value.push_back(it);
				nested_iterable_layer++; 
				state=parse_iterable;
			}
			else if(it=='\"')
			{
				value.push_back(it);
				state=parse_string;
			}
			else if(it!=' ')                                        //否则就是基本类型
			{
				value.push_back(it);
				state=parse_fundamental;
			}
		}
		else if(state==parse_string)
		{
			value.push_back(it);
			if(it=='\"'&&serialized[i-1]!='\\') // \" 转义字符不是结束.
			{
				state=end_parse;
				--i;
			}
		}
		else if(state==parse_fundamental) 
		{
			if(it==','||it=='}'||it=='\"') //遇到逗号结束,对于最后一个属性，遇到的会是大括号，对于字符串，遇到双引号
			{
				if(it=='\"')
					value.push_back(it);  //双引号需要算进值本身
				state=end_parse;
				--i;
				continue;
			}
			value.push_back(it);
		}
		else if(state==parse_iterable) //可能有嵌套的情况
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
		else if(state==parse_struct)      //遇到大括号结束
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
		else if(state==end_parse)        //解析完一个属性对应的键值对，记录到config中
		{
			state=init;
			config[key]=value;
//			std::cout<<"<key>:"<<key<<"   <value>:"<<value<<std::endl;
			key.clear();
			value.clear();
		}
	}
	return config;
}
template<typename Object=void*>
auto Serializable::loads(const std::string&json)
{
	Config config=Serializable::decode(json);                //从json字符串还原Config
	std::string&class_name=config["class_name"];
	class_name.erase(                                        
		std::remove_if(class_name.begin(),class_name.end(),[](auto ch){return ch=='\"';}),//去掉两边的引号
		class_name.end());																																																																																																																																																
	void*object=Reflectable::get_instance(class_name);        //创建实例
	Serializable::from_config_string[class_name](object,json);//反序列化还原
	if constexpr(std::is_same<Object,void*>::value)
		return object;
	else
		return std::ref(*(Object*)object);
}
template<typename T,typename ...Args>
struct Serializable::Regist
{
	Regist()
	{
		using Tptr=T*;
		from_config_string[GET_TYPE_NAME(T)]=[](void*object,const std::string&value)->void    //实质还是调用from_config进行递归.
		{
			Config config=Serializable::decode(value);
			(*(T*)object).from_config(config);
		};
		from_config_string[GET_TYPE_NAME(Tptr)]=[](void*object,const std::string&value)->void //对应的指针
		{
			Config config=Serializable::decode(value);
			if(value=="null")
			{
				(*(Tptr*)object)=nullptr;
			}
			else
			{
		   		(*(Tptr*)object)=(T*)Reflectable::get_instance(GET_TYPE_NAME(T));
				(*(Tptr*)object)->from_config(config);
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
		from_config_string[GET_TYPE_NAME(T)]=[](void*object,const std::string&value)->void   //实质还是调用from_config进行递归.
		{
			Config config=Serializable::decode(value);
			(*(T*)object).from_config(config);
		};
		from_config_string[GET_TYPE_NAME(Tptr)]=[](void*object,const std::string&value)->void //对应的指针
		{
			Config config=Serializable::decode(value);
			if(value=="null")
			{
				(*(Tptr*)object)=nullptr;
			}
			else
			{
		   		(*(Tptr*)object)=(T*)Reflectable::get_instance(GET_TYPE_NAME(T));
				(*(Tptr*)object)->from_config(config);
			}
		};
		Reflectable::Regist<T>();
	}
};
#endif
