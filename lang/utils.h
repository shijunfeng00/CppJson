#ifndef __UNPACKING_H__
#define __UNPACKING_H__
#include<vector>
#include<string>
#include<tuple>
#include<utility>
struct EmptyClass{}; //在Reflectanle::get_method与Reflectable::classmethod_wrapper中有使用，用于类型转换的"中介"
template<typename T>
struct HasCustomSerializeMethod;
//是否有自定义的序列化与反序列化成员函数
template<typename T>
struct IsSetOrMap;       
//std::unordered_map,std::map,std::set,std::unordered_set
template<typename T>
struct IsSerializableType; 
//编译期检测是否有成员函数get_config,如果value=true，说明该类型是支持序列化的非基本类型(实现了get_config方法) 
template<typename T>
struct IsIterableType;     
//是否是可迭代的，比如std容器 
template<typename T>
struct IsTupleOrPair;
//是否是std::tuple,std::pair
template<typename Object>
struct IsArrayType;	
//是否是C++的原生数组,int a[15]，不过IsArrayType<std::array<int,15>>::value=0,这个会被检测为tuple。但是似乎没什么影响，凑合着用了
template<typename Method>
struct IsClassMethodType;
//是否是成员函数
struct HashFunc;
//对于std::pair<A,B>计算哈希值
struct EqualKey;
//对于std::pair<A,B>对象的键值比较,哈希碰撞的比较定义,需要知道两个自定义对象是否相等
template <typename T, size_t N>
inline constexpr size_t GetArrayLength(const T(&)[N]);
//编译期获得数组长度
inline std::vector<std::string>unpacking_list(const std::string&serialized); 
//列表解包,"[1,2,3,4]"->["1","2","3","4"]
template<typename Object,int index=0>
inline auto for_each_element(Object&object,auto&&callback);                 
//遍历std::tuple,std::pair每个元素,序列化/反序列化 传入函数对象即可

template<typename T>
struct HasCustomSerializeMethod
{
    template<typename U>                                               
    static auto check_dumps(int)->decltype(std::declval<U>().custom_dumps(),std::true_type());
    template<typename U>
	static std::false_type check_dumps(...);
    template<typename U>                                               
    static auto check_loads(int)->decltype(std::declval<U>().custom_loads(),std::true_type());
    template<typename U>
	static std::false_type check_loads(...);
    static constexpr int value1=std::is_same<decltype(check_dumps<T>(0)),std::true_type>::value;
    static constexpr int value2=std::is_same<decltype(check_loads<T>(0)),std::true_type>::value;
    static constexpr int value=value1&&value2;
};
template<typename T>
struct IsSetOrMap                                               
{                                                                 
    template<typename U>                                               
        static auto check(int)->decltype(std::declval<U>().insert(std::declval<decltype(*std::declval<U>().begin())>()),std::true_type());
    template<typename U>
        static std::false_type check(...);
    static constexpr int value = std::is_same<decltype(check<T>(0)),std::true_type>::value;
};
template<typename T>
struct IsSerializableType                                              //编译期检测是否有成员函数get_config,如果value=true
{                                                                      //说明该类型是支持序列化的非基本类型(实现了get_config方法) 
    template<typename U>                                               //可以通过get_config来进行序列化 
        static auto check(int)->decltype(std::declval<U>().get_config(),std::true_type());
    template<typename U>
        static std::false_type check(...);
    static constexpr int value = std::is_same<decltype(check<T>(0)),std::true_type>::value;
};
template<typename T>
struct IsIterableType                                                  //是否具有迭代器,如std::vector等容器的序列化 
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

template<typename Method>
struct IsClassMethodType
{
	template<typename ClassType,typename ReturnType,typename...Args>
	static constexpr ReturnType match(ReturnType(ClassType::*method)(Args...args));
	template<typename T>
	static constexpr auto check(int)->decltype(match(std::declval<T>()),std::true_type());
	template<typename T>
	static constexpr std::false_type check(...);
	static constexpr int value=std::is_same<decltype(check<Method>(0)),std::true_type>::value;
};
struct HashFunc
{
	template<typename T, typename U>
	size_t operator()(const std::pair<T, U>& p) const {
		return std::hash<T>()(p.first) ^ std::hash<U>()(p.second);
	}
};

// 键值比较，哈希碰撞的比较定义，需要直到两个自定义对象是否相等
struct EqualKey {
	template<typename T, typename U>
	bool operator ()(const std::pair<T, U>& p1, const std::pair<T, U>& p2) const {
		return p1.first == p2.first && p1.second == p2.second;
	}
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
	enum State{init,parse_fundamental,parse_string,parse_struct,parse_iterable,end_parse}state=init;
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
