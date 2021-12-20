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
	struct Inherit;
	
	template<typename T>
	inline static Config get_config(const T*object);//得到T的类型信息和名称,判断T类型的属性键值对是否建立,没有建立就建立(只建立一次) 
	inline static std::vector<std::string_view>get_serializable_types();
	template<typename FieldType=void*>
	inline static auto get_field(void*object,std::string class_name,std::string field_name); 

	template<typename FieldType=void*,typename ClassType>
	inline static auto get_field(ClassType&object,std::string field_name); 

	inline static std::string get_field_type(std::string class_name,std::string field_name); 
	inline static std::size_t get_field_offset(std::string class_name,std::string field_name); 
		
	template<typename ClassType>
	inline static std::vector<std::string>get_field_names();
	template<typename ClassType>
	inline static std::vector<std::string>get_method_names();
	template<typename ReturnType,typename ObjectType,typename...Args>
	inline static auto get_method(ObjectType&object,const std::string&field_name,Args&&...args);
	//通过字符串访问成员函数,get_field<返回值类型>(对象,字段名,参数列表...);

	template<typename FieldType,typename ClassType>
	inline static void set_field(ClassType&object,std::string field_name,const FieldType&data);//设置属性值，因为已经有类型信息，所以不需要调用default_constructors[type]里面的函数来构造 
	
	inline static void set_field(void*object,std::string class_name,std::string field_name,const auto&value);

	inline static void delete_instance(std::string class_name,void*object); 
	inline static void*get_instance(std::string class_name);
	
	using ClassName=std::string;
	using MethodName=std::unordered_map<std::string,void(EmptyClass::*)(void*)>;
	using FieldName=std::unordered_map<std::string,std::pair<std::string,std::size_t>>;
private:	
	static std::unordered_map<ClassName,FieldName>field;
	static std::unordered_map<ClassName,std::function<void*(void)>>default_constructors;
	static std::unordered_map<ClassName,std::function<void(void*)>>default_deconstructors;
	static std::unordered_map<std::string,MethodName>&method;
};
std::unordered_map<std::string,std::function<void*(void)>>Reflectable::default_constructors;
std::unordered_map<std::string,std::function<void(void*)>>Reflectable::default_deconstructors;
std::unordered_map<std::string,std::unordered_map<std::string,std::pair<std::string,std::size_t>>>Reflectable::field;//(类名,属性名)->(字符串值,偏移量)
std::unordered_map<std::string,std::unordered_map<std::string,void(EmptyClass::*)(void*)>>&Reflectable::method=ConfigPair::from_classmethod_string;

std::vector<std::string_view>Reflectable::get_serializable_types()
{
	std::vector<std::string_view>types;
	for(auto&it:ConfigPair::from_config_string)
		types.push_back(it.first);
	return types;
}
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
	Config config(&field[class_name],object);
	config.update({{"class_name",class_name}});
	return config;
}
template<typename ReturnType,typename ObjectType,typename...Args>
auto Reflectable::get_method(ObjectType&object,const std::string&field_name,Args&&...args)//通过字符串访问成员函数,get_field<返回值类型>(对象,字段名,参数列表...);
{
	try
	{
		auto func=Reflectable::method[GET_TYPE_NAME(ObjectType)][field_name];
		auto method=reinterpret_cast<ReturnType(ObjectType::*)(Args...)>(func);
		return (object.*method)(std::forward<Args>(args)...);
	}
	catch(std::exception&e)
	{
		throw NoSuchMethodException(GET_TYPE_NAME(ObjectType),field_name);
	}
}
template<typename FieldType=void*,typename ClassType>
auto Reflectable::get_field(ClassType&object,std::string field_name)
{
	try
	{
		std::string class_name=GET_TYPE_NAME(ClassType);
		std::size_t offset=Reflectable::field[class_name][field_name].second;
		if constexpr(std::is_same<FieldType,void*>::value)
			return (void*)((std::size_t)(&object)+offset);
		else
			return (*(FieldType*)((std::size_t)(&object)+offset));
	}
	catch(std::exception&e)
	{
		throw NoSuchFieldException(GET_TYPE_NAME(ClassType),field_name);
	}
}
template<typename FieldType=void*>
auto Reflectable::get_field(void*object,std::string class_name,std::string field_name)
{
	try
	{
		std::size_t offset=Reflectable::field[class_name][field_name].second;
		if constexpr(std::is_same<FieldType,void*>::value)
			return (void*)((std::size_t)(object)+offset);
		else
			return (*(FieldType*)((std::size_t)(object)+offset));
	}
	catch(std::exception&e)
	{
		throw NoSuchFieldException(class_name,field_name);
	}
}
std::string Reflectable::get_field_type(std::string class_name,std::string field_name)
{
	return Reflectable::field[class_name][field_name].first;
}
std::size_t Reflectable::get_field_offset(std::string class_name,std::string field_name)
{
	return Reflectable::field[class_name][field_name].second;
}
void*Reflectable::get_instance(std::string class_name)
{
	try
	{
		return Reflectable::default_constructors[class_name]();
	}
	catch(std::exception&e)
	{
		throw NoSuchClassException(class_name);
	}
}
void Reflectable::delete_instance(std::string class_name,void*object)
{
	default_deconstructors[class_name](object);
}
template<typename FieldType,typename ClassType>
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
		static Config config=object.get_config();                           //必须调用get_config,才能建立类型信息,所以这里必须先调用一次Reflectable的get_config
		Reflectable::default_constructors[GET_TYPE_NAME(T)]=[](void)->void* //默认构造函数 
		{
			return (void*)(new T());
		};
		Reflectable::default_deconstructors[GET_TYPE_NAME(T)]=[](void*object)->void  //析构函数 
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
		static_assert(IsSerializableType<T>::value,"There are some objects that use reflection but haven't implement public method Config get_config()const");
		T object;
		static Config config=object.get_config();
		Reflectable::default_constructors[GET_TYPE_NAME(T)]=[](void)->void*          //默认构造函数 
		{
			return (void*)(new T());
		};
		Reflectable::default_deconstructors[GET_TYPE_NAME(T)]=[](void*object)->void  //析构函数 
		{
			delete ((T*)object);
		};
	}
};
template<typename Parent>
struct Reflectable::Inherit
{
	template<typename Object>
	static Config get_config(const Object*object)
	{
		Config parent_config=object->Parent::get_config();                              //得到父类的Config
		auto sub_config=Reflectable::field.find(GET_TYPE_NAME(Object));                 //得到子类的Config
		if(sub_config==Reflectable::field.end())                                        
			field[GET_TYPE_NAME(Object)]=Reflectable::field[GET_TYPE_NAME(Parent)];     //子类的属性继承自父类,注意不要出现同名属性.
		return parent_config;	
	}
};
#endif
