#include"lang/serializable.h"
#include<iostream>
#include<vector>
using namespace std;
struct Node
{
	int x=1;
	float y=2;
	std::string z="3";
	Config get_config()const
	{
		Config config=Serializable::get_config(this);
		config.update({
			{"x",x},
			{"y",y},
			{"z",z}
		});
		return config;
	}
};
int main()
{
	Serializable::Regist<Node>();                                         //注册
	void*object=Reflectable::get_instance("Node");                        //创建实例
	Reflectable::set_field<int>(object,"Node","x",4);                     //通过反射修改值
	Reflectable::set_field<float>(object,"Node","y",5);
	Reflectable::set_field<std::string>(object,"Node","z","test");
	
	cout<<Reflectable::get_field<int>(object,"Node","x")<<endl;           //通过反射得到值
	cout<<Reflectable::get_field<float>(object,"Node","y")<<endl;
	cout<<Reflectable::get_field<std::string>(object,"Node","z")<<endl;
	
	cout<<(*(Node*)object).x<<endl;                                       //正常访问成员变量
	cout<<(*(Node*)object).y<<endl;
	cout<<(*(Node*)object).z<<endl;
	
	std::string json=Serializable::dumps(*(Node*)object);                 //序列化
	
	cout<<json;
	
	Node b=Serializable::loads<Node>(json);                               //反序列化
	
	cout<<b.x<<endl;                                                      //正常访问成员变量
	cout<<b.y<<endl;
	cout<<b.z<<endl;
}
