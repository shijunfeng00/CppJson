//对于派生类的序列化与反序列化
#include"lang/serializable.h"
#include"iostream"
using namespace std;
struct Father
{
	int x=1;
	std::vector<int>y{1,2,3,4};
	virtual Config get_config()const
	{
		Config config=Serializable::get_config(this);
		config.update({
			{"x",x},
			{"y",y}
		});
		return config;
	}
	virtual ~Father(){}
};
struct Son:public Father
{
	float z[4]={5,6,7,8};
	Config get_config()const
	{
		Config config=Serializable::Inherit<Father>::get_config(this);
		config.update({
			{"z",z}
		});
		return config;
	}
};
struct GrandSon:public Son
{
	std::tuple<int,std::string>t={2021,"shijunfeng00"};
	Config get_config()const
	{
		Config config=Serializable::Inherit<Son>::get_config(this);
		config.update({
			{"t",t}
		});
		return config;
	}
};
int main()
{
	Serializable::Regist<Father,Son,GrandSon>();
	Father*son=new GrandSon(); 
	son->x=233;
	std::string json=Serializable::dumps(*son);
	cout<<"json string:\n"<<json<<endl; 
	GrandSon*gs=(GrandSon*)Serializable::loads(json);
	cout<<gs->get_config().serialized_to_string(true)<<endl;
	cout<<gs->x<<endl;
	for(auto&it:gs->y)
		cout<<it<<",";
	cout<<endl;
	for(auto&it:gs->z)
		cout<<it<<",";
	cout<<endl;
	cout<<get<0>(gs->t)<<" "<<get<1>(gs->t);
}
