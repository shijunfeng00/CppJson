#ifndef __CONFIG_H__
#define __CONFIG_H__
#include"configpair.h"
#include<unordered_map>
class Config//本身是一个键值对，但在每一个类型第一次调用的时候生成运行时的一些反射相关的信息 
{            //***field_info应该由Serializable对象负责清楚，这里不应该删除指针*** 
public:
	using Field=std::unordered_map<std::string,std::pair<std::string,std::size_t>>;
	Config():field_info(nullptr){}
	template<typename T>
	Config(Field*field_info,T*object);
	//在第一次被调用的时候，需要去创建类型信息,field_info记录属性名称->(类型名称,地址偏移量),记录类型在后续反序列化中需要用到 
	std::string serialized_to_string(bool first_nested_layer=false)const;      //序列化为字符串  
	std::string&operator[](const std::string&key)const; //键值对 
	std::string&operator[](std::string&key);  
	void update(const std::initializer_list<ConfigPair>&pairs);
	//添加变量对,这时候ConfigPair中会记录下类型信息，可以在运行时创建好反序列化时候需要对应的还原函数;
	//config.update({{"namea",this->name1},{"name2",this->name2}});
	auto begin();
	auto end();
private:
	mutable std::unordered_map<std::string,std::string>config;
	Field*field_info;
	std::size_t class_header_address;
	std::size_t class_size;
};
std::ostream operator<<(std::ostream&os,Config&config);
template<typename T>
Config::Config(Config::Field*field_info,T*object): //在第一次被调用的时候，需要去创建类型信息 
	field_info(field_info),                                                                           //为后面的反序列化做准备 
	class_header_address((std::size_t)(object)),                                                        
	class_size(sizeof(T)){}
void Config::update(const std::initializer_list<ConfigPair>&pairs)//添加变量对,config.update({{"namea",this->name1},{"name2",this->name2}});
{
	for(auto&it:pairs)
	{
		if(it.is_field)//如果不是属性，而是成员函数，不用参与序列化过程
		{
			config[it.key]=it.value;
			if(field_info!=nullptr)                                                 //只被创建一次
			{
				(*field_info)[it.key].first=it.type;                                //类型
				(*field_info)[it.key].second=it.address-this->class_header_address; //地址偏移量，用来访问成员变量 
			}
		}
	}
}
std::string&Config::operator[](const std::string&key)const //键值对 
{
	return config[key];
}
std::string&Config::operator[](std::string&key) //键值对 
{
	return config[key];
}
std::string Config::serialized_to_string(bool first_nested_layer)const //序列化的
{
	std::ostringstream oss;
	char end=first_nested_layer?'\n':' ';
	oss<<"{"<<end;
	for(auto it=config.begin(),next=++config.begin();it!=config.end();++it,next!=config.end()?++next:next)
	{
		if(it!=config.end()&&next==config.end()) //最后一个元素，没有逗号
			oss<<"\""<<(*it).first<<"\":"<<(*it).second<<end;
		else
			oss<<"\""<<(*it).first<<"\":"<<(*it).second<<","<<end;
	}
	oss<<"}"<<end;
	return oss.str();
}
auto Config::begin()
{
	return config.begin();
}
auto Config::end()
{
	return config.end();
}
#endif
