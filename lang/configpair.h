#ifndef __CONFIGPAIR_H__
#define __CONFIGPAIR_H__
#include<string>
#include<typeinfo>
#include<sstream>
#include<vector>
#include<cxxabi.h>
#include<unordered_map>
#include<functional>
#include"utils.h"
#define GET_TYPE_NAME(type)\
abi::__cxa_demangle(typeid(type).name(),0,0,0)
struct ConfigPair
{
	template<typename T>
	ConfigPair(const std::string&name,const T&object);
	std::string key;    //成员变量名称，与声明的名称对应 
	std::string value;  //成员函数的值转化为字符串的结果(如果是基本类型，则直接转化为字符串，否则为嵌套字典结构) 
	std::string type;   //类型,GET_TYPE_NAME(T),相当于记录下类型了，反序列化会用到 
	std::size_t address;//地址，后面将用来计算成员变量地址偏移量，反序列化的时候通过(void*)(对象地址+偏移量)来访问成员变量 
	template<typename Object>
	static std::string get_config_string(const Object&object);
	static std::unordered_map<std::string,std::function<void(void*,const std::string&)>>from_config_string; //从字符串中还原数据
};
std::unordered_map<std::string,std::function<void(void*,const std::string&)>>ConfigPair::from_config_string; //从字符串中还原数据
template<typename T>
ConfigPair::ConfigPair(const std::string&name,const T&object):
	key(name),
	value(ConfigPair::get_config_string<T>(object)),
	type(GET_TYPE_NAME(T)),
	address((std::size_t)&object){}
template<typename Object>
std::string ConfigPair::get_config_string(const Object&field)
{
	std::ostringstream oss;
	if constexpr(std::is_fundamental<Object>::value&(!std::is_same<Object,char*>::value)) //基本类型,int,float,..,但是排除char*
	{
		#ifdef __SERIALIZABLE_H__ //只有序列化的时候才需要这个
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void //从字符串中还原数据
		{
			std::istringstream iss(str);
			Object value;
			iss>>value;
			*(Object*)field=value;
		};
		#endif
		oss<<field;
		return oss.str();
	}
	else if constexpr(std::is_same<Object,std::string>::value)  //字符串,std::string
	{
		#ifdef __SERIALIZABLE_H__
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void //只测试过std::string
		{
			Object value;
			char ch;//吃掉双引号
			std::istringstream iss(str);
			iss>>ch>>value;
			value.pop_back();
			*(Object*)field=value;
		};
		#endif
		oss<<"\""<<field<<"\"";
		return oss.str();
	}
	else if constexpr(std::is_same<Object,char*>::value)  //字符串,char*，这个没DEBGU，可能有坑
	{
		#ifdef __SERIALIZABLE_H__
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void
		{
			std::string value;
			char ch;//吃掉双引号
			std::istringstream iss(str);
			iss>>ch>>value;
			value.pop_back();
			field=(void*)malloc(sizeof(char)*(value.size()+1)); //传入的field最好是nullptr
			memcpy(field,value.c_str(),value.size()+1);//加1是因为'\0'
		};
		#endif
		oss<<"\""<<field<<"\"";
		return oss.str();
	}
	else if constexpr(std::is_pointer<Object>::value) //指针
	{
		#ifdef __SERIALIZABLE_H__
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void
		{
			using type=typename std::remove_pointer<Object>::type;
			Object value=nullptr;
			if(str!="null")
			{
				value=new typename std::remove_pointer<Object>::type();
				if constexpr(!IsSerializableType<Object>::value)
					from_config_string[GET_TYPE_NAME(type)](&(*value),str);
				else
					Object::from_config_string[GET_TYPE_NAME(type)](&(*value),str);
			}
			*(Object*)field=value;
		};
		#endif
		if(field==nullptr)
			return"null";
		return get_config_string<typename std::remove_pointer<Object>::type>(*field);
	}
	else if constexpr(IsSerializableType<Object>::value) //可序列化的非基本类型
	{
		#ifdef __SERIALIZABLE_H__
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void
		{
			Object::from_config_string[GET_TYPE_NAME(Object)](field,str);
		};
		#endif
		return field.get_config().serialized_to_string(false);
	}
	else if constexpr(IsTupleOrPair<Object>::value)
	{
		oss<<"[";
		for_each_element(field,[&](auto&it,int index){
			if(index+1<(int)std::tuple_size<Object>::value)
				oss<<get_config_string(it)<<",";
			else
				oss<<get_config_string(it);
		});
		oss<<"]";
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void 
		{
			auto values=unpacking_list(str);                             //字符串形式的值
			for_each_element(*(Object*)field,[&](auto&it,int index){
				std::string type_name=GET_TYPE_NAME(decltype(it));       //得到每个元素的类型
				from_config_string[type_name](&it,values[index]);	     //然后找到对应的函数,依次还原
			}); //tuple每个位的元素类型是不会改变的
		};
		return oss.str();
	}
	else if constexpr(IsIterableType<Object>::value&(!std::is_same<Object,std::string>::value))//迭代器
	{
		using element_type=typename std::remove_const<typename std::remove_reference<decltype(*field.begin())>::type>::type;
		std::ostringstream oss;
		oss<<"[";
		std::size_t index=0;
		for(auto&it:field)
		{
			index++;
			if(index==field.size()) //最后一个元素
				oss<<get_config_string<element_type>(it);
			else
				oss<<get_config_string<element_type>(it)<<",";
		}
		oss<<"]";
		#ifdef __SERIALIZABLE_H__
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void
		{
			std::vector<std::string>values=unpacking_list(str);
			Object object(values.size());//元素个数是可以确定的，一次性分配好空间
			int index=0;
			for(auto&it:object)                                     //Object可能是std::list,std::deque,所以不能直接下标访问
			{
				if constexpr(std::is_pointer<element_type>::value&&IsSerializableType<typename std::remove_pointer<element_type>::type>::value)
				{ //Serialiable子类指针
					using remove_ptr_type=typename std::remove_pointer<element_type>::type;
					remove_ptr_type::from_config_string[GET_TYPE_NAME(element_type)](&it,values[index]);
				}
				else if constexpr(IsSerializableType<element_type>::value)
					element_type::from_config_string[GET_TYPE_NAME(element_type)](&it,values[index]); 
				else
					from_config_string[GET_TYPE_NAME(element_type)](&it,values[index]);//逐个元素还原
				++index;
			}	
			*(Object*)field=std::move(object);
		};
		#endif	
		return oss.str();
	}
	else if constexpr(IsArrayType<Object>::value) //C++原生数组,int a[15];
	{
		using element_type=typename std::remove_const<typename std::remove_reference<decltype(field[0])>::type>::type;
		constexpr std::size_t length=sizeof(Object)/sizeof(element_type);
		oss<<"[";
		for(unsigned int i=0;i<length;i++)
		{
			oss<<get_config_string<element_type>(field[i]);
			if(i+1<length)
				oss<<",";
		}
		oss<<"]";
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void
		{
			std::string type_name=GET_TYPE_NAME(element_type);
			auto values=unpacking_list(str);
			const std::size_t length=values.size();
			for(unsigned i=0;i<length;i++)
				from_config_string[type_name]((void*)((std::size_t)field+sizeof(element_type)*i),values[i]);
		};
		return oss.str();
	}
	return "<not serializable object>";
}
#endif
