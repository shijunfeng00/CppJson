#include"../lang/serializable.h"
#include<vector>
#include<list>
#include<array>
#include<tuple>
#include<fstream>
/*随便写的，性能，内存问题都暂不考虑，只关心结果正确与否*/
struct TestA
{
	int x;
	float y;
	std::string z;
	TestA(){}
	void set_x(auto x)
	{
		this->x=x;
	}
	void set_y(auto y)
	{
		this->y=y;
	}
	void set_z(auto z)
	{
		this->z=z;
	}
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
struct TestB
{
	TestB(){}
	static TestB* push_front(TestB*head,TestB new_data_obj)
	{
		TestB*new_data=new TestB(new_data_obj);
		new_data->next=head;
		head=new_data;
		return head;
	}
	static TestB get_instance(int x,float y,std::string z)
	{
		TestB obj;
		obj.data=std::make_tuple(x,make_pair(y,z));
		return obj;
	}
	TestB*next=nullptr;
	std::tuple<int,std::pair<float,std::string>>data;
	Config get_config()const
	{
		Config config=Serializable::get_config(this);
		config.update({
			{"next",next},
			{"data",data}
		});
		return config;
	}
};
struct TestC
{
	std::tuple<int,TestA,TestB>tp;
	std::vector<int>vec;
	std::list<int>lt;
	std::array<int,5>arr;
	int a[3];
	Config get_config()const
	{
		Config config=Serializable::get_config(this);
		config.update({
			{"tp",tp},
			{"vec",vec},
			{"lt",lt},
			{"arr",arr},
			{"a",a}
		});
		return config;
	}
};
struct TestD:public TestA
{
	double t;
	Config get_config()const
	{
		Config config=Serializable::Inherit<TestA>::get_config(this);
		config.update({
			{"t",t}
		});
		return config;
	}
};
void testA()
{
	std::cout<<"in:"<<__PRETTY_FUNCTION__<<std::endl;
	TestA a;
	a.x=2;
	a.y=2.3;
	a.z="nothing";
	std::string json=Serializable::dumps(a);
	std::cout<<"json:"<<json<<std::endl;
	TestA b=Serializable::loads<TestA>(json);
	std::cout<<b.get_config().serialized_to_string(true)<<std::endl;
}
void testB()
{
	std::cout<<"in:"<<__PRETTY_FUNCTION__<<std::endl;
	TestB a=TestB::get_instance(5,5.5,"test_b_1");
	std::string json=Serializable::dumps(a);
	std::cout<<"json:"<<json<<std::endl;
	TestB b=Serializable::loads<TestB>(json);
	std::cout<<b.get_config().serialized_to_string(true)<<std::endl;
	TestB*head=new TestB(a);
	head=TestB::push_front(head,TestB::get_instance(6,6.6,"test_b_2"));
	head=TestB::push_front(head,TestB::get_instance(7,7.7,"test_b_3"));
	head=TestB::push_front(head,TestB::get_instance(8,8.8,"test_b_4"));
	json=Serializable::dumps(*head);
	std::cout<<"json:"<<json<<std::endl;
	TestB c=Serializable::loads<TestB>(json);
	std::cout<<c.get_config().serialized_to_string(true)<<std::endl;
}
void testC()
{
	std::cout<<"in:"<<__PRETTY_FUNCTION__<<std::endl;
	TestA a;
	a.x=1;
	a.y=1.1;
	a.z="testa";
	TestB b=TestB::get_instance(2,2.2,"testb");
	TestC c;
	c.tp={5,a,b};
	c.vec=std::vector{1,2,3,4,5};
	c.lt=std::list{2,3,4,5,6};	
	c.arr=std::array<int,5>{3,4,5,6,7};
	c.a[0]=c.a[1]=c.a[2]=9;
	std::string json=Serializable::dumps(c);
	std::cout<<"json:"<<json<<std::endl;
	TestC d=Serializable::loads<TestC>(json);
	std::cout<<d.get_config().serialized_to_string(true)<<std::endl;
}
void testD()
{
	std::cout<<"in:"<<__PRETTY_FUNCTION__<<std::endl;
	TestD a;
	a.set_x(3);
	a.set_y(4);
	a.set_z("test");
	a.t=23.45;
	std::string json=Serializable::dumps(a);
	std::cout<<"json:"<<json<<std::endl;
	TestD b=Serializable::loads<TestD>(json);
	std::cout<<b.get_config().serialized_to_string(true)<<std::endl;
}
void testE()
{
	std::cout<<"in:"<<__PRETTY_FUNCTION__<<std::endl;
	std::string json=Serializable::dumps({3,1,4,1,5,9,2,6});
	auto arr1=Serializable::loads<std::array<int,8>>(json);
	auto arr2=Serializable::loads<std::vector<int>>(json);
	std::cout<<"\njson:\n";
	std::cout<<Serializable::dumps(arr1);
	std::cout<<"\njson\n";
	std::cout<<Serializable::dumps(arr2);
	json=Serializable::dumps(std::make_tuple(1,TestB(),std::string{"i wanna be the guy."}));
	auto tp=Serializable::loads<std::tuple<int,TestB,std::string>>(json);
	std::cout<<"\njson\n";
	std::cout<<Serializable::dumps(tp);
	std::cout<<std::endl;
}
int verify_output(std::string std_output_file,std::string output_file)
{
	std::ifstream fin1(std_output_file,std::ios::in);
	std::ifstream fin2(output_file,std::ios::in);
	std::string content1((std::istreambuf_iterator<char>(fin1)),std::istreambuf_iterator<char>());	
	std::string content2((std::istreambuf_iterator<char>(fin2)),std::istreambuf_iterator<char>());	
	if(content1==content2)
		return -1;
	if(content1.size()>content2.size())
		std::swap(content1,content2);
	for(unsigned i=0;i<content1.size();i++)
		if(content1[i]!=content2[i])
			return i;
	return content1.size();
}
auto get_content(std::string std_output_file,std::string output_file,const int index,const int content_length=10)
{
	std::ifstream fin1(std_output_file,std::ios::in);
	std::ifstream fin2(output_file,std::ios::in);
	std::string content1((std::istreambuf_iterator<char>(fin1)),std::istreambuf_iterator<char>());	
	std::string content2((std::istreambuf_iterator<char>(fin2)),std::istreambuf_iterator<char>());	
	int begin=std::max(0,index-content_length);
	int end=std::min((int)content2.size(),index+content_length);
	return std::make_pair(
		std::string_view{content1}.substr(begin,end-begin),
		std::string_view{content2}.substr(begin,end-begin)
	);
}
int main()
{
	const char*std_output_file="test.serializable.log";
	const char*output_file="test.serializable.tmp";
	freopen("test.serializable.tmp","w",stdout);
	Serializable::Regist<TestA,TestB,TestC,TestD>();
	testA();
	testB();
	testC();
	testD();
	testE();
	freopen("con","w",stdout);
	auto ret=verify_output(std_output_file,output_file);
	std::cout<<"test serialization and deserialization:";
	if(ret==-1)
	{
		std::cout<<"pass"<<std::endl;
	}
	else
	{
		std::cout<<"not path,different in char:"<<ret<<std::endl;
		std::cout<<"details:\n";
		auto [std_out,out]=get_content(std_output_file,output_file,ret,10);
		std::cout<<"stdandard output:\n..."<<std_out<<"...\nyour code's output:\n..."<<out<<"...";
	}
	std::cout<<std::endl;
	system("pause.");
}
