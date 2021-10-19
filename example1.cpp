#include"lang/serializable.h"
#include<iostream>
#include<fstream>
#include<cstring>
using namespace std;
struct Node:public Serializable 
{
	Node()
	{
		std::memset(z,0,sizeof(z));
		t={1,2,3,4};
	}
	Config get_config()const 
	{
		Config config=Serializable::get_config(this);
		config.update({
			{"x",x},
			{"y",y},
			{"z",z},
			{"t",t}
		});
		return config;
	}
	tuple<float,std::pair<int,std::string>>x;
	std::vector<Node*>y;
	int z[3];
	std::array<int,4>t;
};
int main()
{
	Serializable::Regist<Node>();                                                   //注册
	Node a=*(Node*)Reflectable::get_instance("Node");                               //创建实例
	Reflectable::set_field(a,"x",make_tuple(3.2f,make_pair(5,string{"test"})));     //通过名称修改属性
	Reflectable::set_field(a,"y",std::vector<Node*>{new Node,nullptr}); 
	int*z=(int*)Reflectable::get_field(a,"z");                                      //获得属性，并进行修改
	z[0]=2021,z[1]=10,z[2]=18;
	a.t[0]=2021,a.t[1]=10,a.t[2]=19,a.t[3]=10;
	std::string json=Serializable::dumps(a);                                        //序列化为json格式的字符串
	cout<<"json\n"<<json<<endl;         
	Node&b=Serializable::loads<Node>(json);                                         //通过json格式的字符串进行反序列化
	cout<<endl<<a.get_config().serialized_to_string(true);
	cout<<endl<<b.get_config().serialized_to_string(true);                          //打印结果
} 
