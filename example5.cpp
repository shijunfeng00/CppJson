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
		cout<<json<<endl;
		decltype(object) &p=Serializable::loads<decltype(object)>(json);
		cout<<get<0>(p)<<endl;
		cout<<get<1>(p)[0]<<" "<<get<1>(p)[1]<<" "<<get<1>(p)[2];
	}
	catch(std::exception&e)
	{
		cout<<e.what();
	}
}
/*
output:
[5,[4,5,6]]
std::vector<int, std::allocator<int> >
5
4 5 6
*/
