#include<iostream>
#include<functional>
#include<fstream>
#include"lang/serializable.h"
using namespace std;
struct Node
{
	Node(Node*lson,Node*rson,std::function<int(Node*,Node*)>eval):value(eval(lson,rson)),lson(lson),rson(rson){}
	Node(int value=0):value(value),lson(nullptr),rson(nullptr){}
	~Node()
	{
		if(lson!=nullptr)
			delete lson;
		if(rson!=nullptr)
			delete rson;
	}
	Config get_config()const
	{
		Config config=Serializable::get_config(this);
		config.update({
			{"value",value},
			{"lson",lson},
			{"rson",rson}
		});
		return config;
	}
	int value;
	Node*lson;
	Node*rson;
};
int main()
{
	Serializable::Regist<Node>();
	ifstream fin("test.json",ios::in);
	ofstream fout("test.json",ios::out);
	auto add=[](Node*x,Node*y){return x->value+y->value;};
	auto sub=[](Node*x,Node*y){return x->value-y->value;};
	auto mul=[](Node*x,Node*y){return x->value*y->value;};
	Node*x=new Node(5);
	Node*y=new Node(6);
	Node*z=new Node(x,y,add); //z=x+y=11
	z=new Node(x,z,mul);      //z=x*z=55
	z=new Node(z,y,sub);      //z=z-y=49
	std::string json=Serializable::dumps(*z);  //序列化
	fout<<json;
	fout.close();
	
	getline(fin,json);
	fin.close();
	cout<<"json string:\n"<<json<<endl<<endl;  
	Node*root=(Node*)Serializable::loads(json); //反序列化
	cout<<root->value;	 
}
/*
输出结果：
json string:
{ "rson":{ "rson":null, "lson":null, "value":6, "class_name":"Node" } , "lson":{ "rson":{ "rson":{ "rson":null, "lson":null, "value":6, "class_name":"Node" } , "lson":{ "rson":null, "lson":null, "value":5, "class_name":"Node" } , "value":11, "class_name":"Node" } , "lson":{ "rson":null, "lson":null, "value":5, "class_name":"Node" } , "value":55, "class_name":"Node" } , "value":49, "class_name":"Node" }

49
*/
