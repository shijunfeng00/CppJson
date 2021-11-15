#include<iostream>
#include"lang/serializable.h"
#include<vector>
using namespace std;
struct Node
{
	int x=1;
	float y=5;
	std::string z="sjf";
	int add(int x,int y)
	{
		return x+y;
	}
	std::string getName()
	{
		return z;
	}
	Config get_config()const
	{
		Config config=Reflectable::get_config(this);
		config.update({
			{"x",x},
			{"y",y},
			{"z",z},
			{"add",add},
			{"getName",getName}
		});
		return config;
	}
};
int main()
{
	Serializable::Regist<Node>();
	void*object=Reflectable::get_instance("Node");                        //创建实例

	Reflectable::set_field<int>(object,"Node","x",4);                     //通过字符串名称修改成员变量
	Reflectable::set_field<float>(object,"Node","y",5);
	Reflectable::set_field<std::string>(object,"Node","z","test");
	int field_x=Reflectable::get_field<int>(object,"Node","x");           //通过字符串名称得到值
	float field_y=Reflectable::get_field<float>(object,"Node","y");
	string field_z=Reflectable::get_field<string>(object,"Node","z");
	
	cout<<field_x<<" "<<(*(Node*)object).x<<endl;                         //正常访问成员变量,对比结果
	cout<<field_y<<" "<<(*(Node*)object).y<<endl;
	cout<<field_z<<" "<<(*(Node*)object).z<<endl;
	/*序列化与反序列化*/
	
	std::string json=Serializable::dumps(*(Node*)object);                 //序列化
	cout<<json<<endl;
	Node b=Serializable::loads<Node>(json);                               //反序列化
	/*正常访问*/
	cout<<b.x<<endl;                                                      //正常访问成员变量
	cout<<b.y<<endl;
	cout<<b.z<<endl;
	/*成员函数反射*/
	cout<<Reflectable::get_method<int>(b,"add",5,6)<<endl;                //通过字符串名称访问成员函数
	cout<<Reflectable::get_method<string>(b,"getName")<<endl;
}
/*
4 4
5 5
test test
{ "z":"test", "y":5, "x":4, "class_name":"Node" }
4
5
test
11
*/
