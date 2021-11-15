#include<iostream>
#include"lang/reflectable.h"
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
struct Point
{
	float x=0,y=0;
	Point add(float delta)
	{
		Point p(*this);
		p.x+=delta;
		p.y+=delta;
		return p;
	}
	Config get_config()const
	{
		Config config=Reflectable::get_config(this);
		config.update({
			{"x",x},
			{"y",y},
			{"add",add},
		});
		return config;
	}
};
int main()
{
	/*构建实例*/
	Reflectable::Regist<Node,Point>();
	void*a=Reflectable::get_instance("Node");                        //创建实例a
	Node b;                                                          //创建实例b
	Point c;
	/*属性值反射*/                    
	Reflectable::set_field(a,"Node","x",4);                          //不显式给出类型,由第三个参数来推断
	Reflectable::set_field<float>(a,"Node","y",5);                   
	Reflectable::set_field<string>(a,"Node","z","test");
	
	int field_x=Reflectable::get_field<int>(a,"Node","x");           //通过字符串名称得到值
	float field_y=Reflectable::get_field<float>(a,"Node","y");
	string field_z=Reflectable::get_field<string>(a,"Node","z");

	cout<<field_x<<" "<<(*(Node*)a).x<<endl;                                       //正常访问成员变量
	cout<<field_y<<" "<<(*(Node*)a).y<<endl;
	cout<<field_z<<" "<<(*(Node*)a).z<<endl;
	
	Reflectable::set_field<int>(b,string("x"),10);
	Reflectable::set_field<float>(b,"y",15);
	Reflectable::set_field<string>(b,"z","cpp");

	field_x=Reflectable::get_field<int>(b,"x");           
	field_y=Reflectable::get_field<float>(b,"y");
	field_z=Reflectable::get_field<std::string>(b,"z");
	
	cout<<field_x<<" "<<b.x<<endl;                                       //正常访问成员变量
	cout<<field_y<<" "<<b.y<<endl;
	cout<<field_z<<" "<<b.z<<endl;
	/*成员函数反射*/
	cout<<Reflectable::get_method<int>(b,"add",6,7)<<endl;
	cout<<Reflectable::get_method<string>(b,"getName")<<endl;
	Point d=Reflectable::get_method<Point>(c,"add",5.f);
	cout<<d.x<<" "<<d.y<<endl;
}
