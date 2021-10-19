#ifndef __UNPACKING_H__
#define __UNPACKINT_H__
#include<vector>
#include<string>
#include<tuple>
#include<utility>
template<typename T>
struct IsSerializableType; //编译期检测是否有成员函数get_config,如果value=true，说明该类型是支持序列化的非基本类型(实现了get_config方法) 
template<typename T>
struct IsIterableType;     //是否是可迭代的，比如std容器 
template<typename T>
struct IsTupleOrPair;
template<typename Object>
struct IsArrayType;	
template <typename T, size_t N>
inline constexpr size_t GetArrayLength(const T(&)[N]);
inline std::vector<std::string>unpacking_list(const std::string&serialized); //列表解包,"[1,2,3,4]"->["1","2","3","4"]
template<typename Object,int index=0>
inline auto for_each_element(Object&object,auto&&callback);                 //遍历std::tuple,std::pair每个元素,序列化/反序列化 传入函数对象即可

template<typename T>
struct IsSerializableType                                  //编译期检测是否有成员函数get_config,如果value=true
{                                                                      //说明该类型是支持序列化的非基本类型(实现了get_config方法) 
    template<typename U>                                               //可以通过get_config来进行序列化 
        static auto check(int)->decltype(std::declval<U>().get_config(),std::true_type());
    template<typename U>
        static std::false_type check(...);
    static constexpr int value = std::is_same<decltype(check<T>(0)),std::true_type>::value;
};
template<typename T>
struct IsIterableType                                     //是否具有迭代器,如std::vector等容器的序列化 
{
    template<typename U>
        static auto check(int)->decltype(std::declval<typename U::iterator>(),std::true_type());
    template<typename U>
        static std::false_type check(...);
    static constexpr int value = std::is_same<decltype(check<T>(0)),std::true_type>::value;
};
template<typename Object>
struct IsTupleOrPair
{
	template<typename T>
	static constexpr auto check(int)->decltype(std::get<0>(std::declval<T>()),std::true_type());
	template<typename T>
	static constexpr auto check(...)->std::false_type;
	static constexpr int value=std::is_same<decltype(check<Object>(0)),std::true_type>::value;
};
template<typename Object>
struct IsArrayType
{
	template<typename T>
	static constexpr auto check(int)->decltype(GetArrayLength(std::declval<T>()),std::true_type());
	template<typename T>
	static constexpr auto check(...)->std::false_type;
	static constexpr int value=std::is_same<decltype(check<Object>(0)),std::true_type>::value;
};

template <typename T, size_t N>
inline constexpr size_t GetArrayLength(const T(&)[N])
{
    return N;
}
template<typename Object,int index=0>
inline auto for_each_element(Object&object,auto&&callback)                              //对于每一个元素进行遍历
{
	callback(std::get<index>(object),index);
	if constexpr(index+1<std::tuple_size<Object>::value)
		for_each_element<Object,index+1>(object,callback);
}
inline std::vector<std::string>unpacking_list(const std::string&serialized)              //列表解包,"[1,2,3,4]"->["1","2","3","4"]
{                                                                                        //思路参考Serializable::decode
	constexpr int init=0;
	constexpr int parse_fundamental=1;
	constexpr int parse_struct=2;
	constexpr int parse_iterable=3;
	constexpr int parse_string=4;
	constexpr int end_parse=5;
	int state=init;
	std::vector<std::string>vec;
	std::string temp;
	int length=serialized.size();
	int nested_struct=0;             //嵌套的情况：{{},{}}
	int nested_iterable=0;           //嵌套的情况：[[],[],{}]
	for(int i=0;i<length;++i)
	{
		auto&it=serialized[i];
		if(i==0)
			continue;
		if(state==init)
		{
			if(it=='{')
			{
				state=parse_struct;
				nested_struct++;
				temp.push_back(it);
			}
			else if(it=='[')
			{
				state=parse_iterable;
				nested_iterable++;
				temp.push_back(it);
			}	
			else if(it!=','&&it!=' ')
			{
				state=parse_fundamental;
				temp.push_back(it);
			}
			else if(it=='\"')
			{
				state=parse_string;
				temp.push_back(it);
			}
		}
		else if(state==parse_string)
		{
			temp.push_back(it);
			if(it=='\"'&&serialized[i-1]!='\\') //转义字符不是结束
			{
				state=end_parse;
				--i;
			}
		}
		else if(state==parse_struct)
		{
			if(it=='}'||it=='{')
				nested_struct+=(it=='}'?-1:1);
			if(nested_struct==0) //解析完毕
			{
				state=end_parse;
				--i;
				temp.push_back(it);
				continue;
			}
			temp.push_back(it);
		}
		else if(state==parse_iterable)
		{
			if(it==']'||it=='[')
				nested_iterable+=(it==']'?-1:1);
			if(nested_iterable==0)
			{
				state=end_parse;
				--i;
				temp.push_back(it);
				continue;
			}
			temp.push_back(it);
		}
		else if(state==parse_fundamental)
		{
			if(it==','||it==']')
			{
				state=end_parse;
				--i;
				continue;
			}
			temp.push_back(it);
		}
		else if(state==end_parse)
		{
//			std::cout<<"<"<<temp<<">\n";
			vec.push_back(temp);
			temp.clear();
			state=init;
		}
	}
	return vec;
}
#endif
