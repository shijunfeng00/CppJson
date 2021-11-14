#ifndef __REFLECTABLE_H__
#define __REFLECTABLE_H__
#include"config.h"
#include<vector>
#include<functional>
struct Reflectable
{
public:
	virtual ~Reflectable(){};
	template<typename T,typename ...Args>
	struct Regist;
	
	template<typename T>
	struct Regist<T>;
	
	template<typename T>
	static Config get_config(const T*object);//得到T的类型信息和名称,判断T类型的属性键值对是否建立,没有建立就建立(只建立一次) 
	
	template<typename FieldType=void*>
	static FieldType get_field(void*object,std::string class_name,std::string field_name); 

	template<typename FieldType=void*,typename ClassType>
	static FieldType get_field(ClassType&object,std::string field_name); 

	static std::string get_field_type(std::string class_name,std::string field_name); 
	template<typename ClassType>
	static std::vector<std::string>get_field_names();
	template<typename ClassType>
	static std::vector<std::string>get_method_names();
	
	template<typename ReturnType,typename ObjectType,typename...Args>
	static auto get_method(ObjectType&object,const std::string&field_name,Args&&...args);
	//通过字符串访问成员函数,get_field<返回值类型>(对象,字段名,参数列表...);

	template<typename ClassType,typename FieldType>
	static void set_field(ClassType&object,std::string field_name,const FieldType&data);//设置属性值，因为已经有类型信息，所以不需要调用default_constructors[type]里面的函数来构造 
	
	static void set_field(void*object,std::string class_name,std::string field_name,const auto&value);

	static void delete_instance(std::string class_name,void*object); 
	static void*get_instance(std::string class_name);
	
	using ClassName=std::string;
	using MethodName=std::unordered_map<std::string,void(EmptyClass::*)(void*)>;
	using FieldName=std::unordered_map<std::string,std::pair<std::string,std::size_t>>;
//private:	
	static std::unordered_map<ClassName,FieldName>field;
	static std::unordered_map<ClassName,std::function<void*(void)>>default_constructors;
	static std::unordered_map<ClassName,std::function<void(void*)>>default_deconstructors;
	static std::unordered_map<std::string,MethodName>&method;
};
std::unordered_map<std::string,std::function<void*(void)>>Reflectable::default_constructors;
std::unordered_map<std::string,std::function<void(void*)>>Reflectable::default_deconstructors;
std::unordered_map<std::string,std::unordered_map<std::string,std::pair<std::string,std::size_t>>>Reflectable::field;
std::unordered_map<std::string,std::unordered_map<std::string,void(EmptyClass::*)(void*)>>&Reflectable::method=ConfigPair::from_classmethod_string;
template<typename ClassType>
std::vector<std::string>Reflectable::get_field_names()
{
	static std::vector<std::string>names=[&]()->std::vector<std::string> //这样的话就只会被求值一次.
	{                                                                    //考虑到C++的类的属性和方法并不能运行时动态改变
		std::vector<std::string>names;
		for(auto&it:field[GET_TYPE_NAME(ClassType)])
		{
			names.push_back(it.first);
		}
		return names;	
	}();		
	return names;
}
template<typename ClassType>
std::vector<std::string>Reflectable::get_method_names()
{
	static std::vector<std::string>names=[&]()->std::vector<std::string>
	{
		std::vector<std::string>names;
		for(auto&it:method[GET_TYPE_NAME(ClassType)])
		{
			names.push_back(it.first);
		}
		return names;	
	}();		
	return names;
}
template<typename T>
Config Reflectable::get_config(const T*object)
{
	std::string class_name=GET_TYPE_NAME(T);
	Config config(&field[class_name],object);//如果不存在就创建，否则不做操作 
	config.update({{"class_name",class_name}});
	return config;
}
template<typename ReturnType,typename ObjectType,typename...Args>
auto Reflectable::get_method(ObjectType&object,const std::string&field_name,Args&&...args)//通过字符串访问成员函数,get_field<返回值类型>(对象,字段名,参数列表...);
{
	auto func=Reflectable::method[GET_TYPE_NAME(ObjectType)][field_name];
	auto method=reinterpret_cast<ReturnType(ObjectType::*)(Args...)>(func);
	return (object.*method)(std::forward<Args>(args)...);
}
template<typename FieldType=void*,typename ClassType>
FieldType Reflectable::get_field(ClassType&object,std::string field_name)
{
	std::string class_name=GET_TYPE_NAME(ClassType);
	std::size_t offset=Reflectable::field[class_name][field_name].second;
	if constexpr(std::is_same<FieldType,void*>::value)
		return (void*)((std::size_t)(&object)+offset);
	else
		return std::ref(*(FieldType*)((std::size_t)(&object)+offset));
}
template<typename FieldType=void*>
FieldType Reflectable::get_field(void*object,std::string class_name,std::string field_name)
{
	std::size_t offset=Reflectable::field[class_name][field_name].second;
	if constexpr(std::is_same<FieldType,void*>::value)
		return (void*)((std::size_t)(object)+offset);
	else
		return std::ref(*(FieldType*)((std::size_t)(object)+offset));
}
std::string Reflectable::get_field_type(std::string class_name,std::string field_name)
{
	return Reflectable::field[class_name][field_name].first;
}
void*Reflectable::get_instance(std::string class_name)
{
	return Reflectable::default_constructors[class_name]();
}
void Reflectable::delete_instance(std::string class_name,void*object)
{
	default_deconstructors[class_name](object);
}
template<typename ClassType,typename FieldType>
void Reflectable::set_field(ClassType&object,std::string field_name,const FieldType&data)
{
	std::string class_name=GET_TYPE_NAME(ClassType);
	std::size_t offset=Reflectable::field[class_name][field_name].second;
	*(FieldType*)((std::size_t)(&object)+offset)=data;
}
void Reflectable::set_field(void*object,std::string class_name,std::string field_name,const auto&value)
{
	std::size_t offset=Reflectable::field[class_name][field_name].second;
	using field_type=typename std::remove_const<typename std::remove_reference<decltype(value)>::type>::type;
	*(field_type*)((std::size_t)(object)+offset)=value;
}
template<typename T,typename ...Args>
struct Reflectable::Regist
{
	Regist()
	{
		T object;
		object.get_config(&object);//必须调用get_config,才能建立类型信息,所以这里必须先调用一次Reflectable的get_config 
		Reflectable::default_constructors[GET_TYPE_NAME(T)]=[](void)->void* //默认构造函数 
		{
			return (void*)(new T());
		};
		Reflectable::default_deconstructors[GET_TYPE_NAME(T)]=[](void*object)->void //析构函数 
		{
			delete ((T*)object);
		};
		Regist<Args...>();
	}
};
template<typename T>
struct Reflectable::Regist<T>
{
	Regist()
	{
		T object;
		object.get_config();
		Reflectable::default_constructors[GET_TYPE_NAME(T)]=[](void)->void* //默认构造函数 
		{
			return (void*)(new T());
		};
		Reflectable::default_deconstructors[GET_TYPE_NAME(T)]=[](void*object)->void //析构函数 
		{
			delete ((T*)object);
		};
	}
};
#endif
