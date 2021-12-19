#include<iostream>
#include"lang/serializable.h"
#include"lang/exception.h"
#include<vector>
#include<memory>
using namespace std;
int main()
{
	try
	{
		auto object=std::tuple<int,std::vector<int>>{5,{4,5,6}};
		string json=Serializable::dumps(object);
		decltype(object) p=Serializable::loads<decltype(object)>(json);
		cout<<json<<endl;
		cout<<get<0>(p)<<endl;
		cout<<get<1>(p)[0]<<" "<<get<1>(p)[1]<<" "<<get<1>(p)[2]<<endl;
		cout<<"----------\n";
		json=Serializable::dumps(2.718f);
		float e=Serializable::loads<float>(json);
		cout<<json<<endl;
		cout<<e<<endl;
		cout<<"----------\n";
		json=Serializable::dumps({1,1,2,3,5,8});
		std::vector<int>f1=Serializable::loads<std::vector<int>>(json);
		cout<<json<<endl;
		for(auto&it:f1)
			cout<<it<<",";
		cout<<endl<<endl;
	}
	catch(std::exception&e)
	{
		cout<<e.what();
	}
}
