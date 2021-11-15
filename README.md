# CppJson
轻量级C++对象序列化框架，同时支持部分运行时反射
# 项目背景
C++没有原生的反射与序列化，至少从语言层面来说是暂时不支持的
很多时候是需要自己手动去写反射相关的代码
自己实现方案很多，也有很多现成的库可以使用
但是很多代码用起来都异常繁琐，包含各种模板以及宏，比如类似这样的

[Ubp.a：99 行实现 C++ 简单反射](https://zhuanlan.zhihu.com/p/112914534)

["全球最强" | C++ 动态反射库](https://zhuanlan.zhihu.com/p/337200770)

```cpp
int main(){
    Reflection<Point>::Instance()
        .Regist(&Point::x,"x")
        .Regist(&Point::y,"y")
        .Regist(&Point::Add,"Add");
    Point p;
    Reflection<Point>::Instance().Var<float>("x").Of(p)=2.f;
    Reflection<Point>::Instance().Var<float>("y").Of(p)=3.f;
    Point q=Reflection<Point>::Instance().call<Point>("Add",p,1.f);
    for(auto&nv:Reflection<Point>::Instance().Vars())
        cout<<nv.first<<":"<<nv.second->As<float>().Of(p)
}
```

以及像这样用宏来定义的

```cpp
struct test_type0{
DEF_FIELD_BEGIN(test_type0)
private:
    DEF_FIELD(int, x)
public:
    DEF_FIELD(std::string, y)
DEF_FIELD_END
};

struct test_type1{
DEF_FIELD_BEGIN(test_type1)
    DEF_FIELD(test_type0, z)
    DEF_FIELD(std::string, w)
DEF_FIELD_END
};
```

总之我觉得无论是使用也好，还是注册也好，用起来都是很繁琐的。
所以我自然希望实现一个类似于Tensorflow的自动微分那样，只要写好前向传播代码，自动微分就已经自动生成好了。
我希望当我写完CPP的代码，对应的struct/class的序列化、反序列化、反射就已经自动完成了所有注册，
也不用单独为每个class写一个序列化与反序列化，一切东西都用起来足够"原生"。
所以我的代码尽可能简化了代码使用的这一过程，具体实现其实也不复杂，就是各种类型匹配和递归调用的问题。
我将会在后文中给出充分的示例。
# 安装

采用Header Only设计，使用起来非常方便，只需要包含相关头文件即可

```#include"lang/serializble.h"```。

我在自己电脑上使用`C++17/GCC10.3.0`进行编译，除此以外没有什么其他额外的依赖。

# 使用

## 相关限制

目前我的代码支持属性满足以下类型条件的class的序列化与反序列化
原本反射只是为了序列化和反序列化，只能支持属性值，现在已经支持成员函数了，不过并未处理继承与派生，以及函数重载的问题
* C++基本类型(`int`,`float`,`char`,...)
* 含有迭代器的容器(`std::vector`,`std::list`,`std::deque`,...)
* 容器适配器(`std::map`,`std::set`,`std::unordered_map`,`std::unordered_set`,...)
* `std::tuple`和`std::pair`
* 其他正确实现了`get_config`以及调用`Serializable::Regist<T>`注册的类
* 数组`int a[15]`,`std::array<int,15>`
* 上述类型的组合及其指针,`std::vector<std::pair<int*,float*>>`
* 支持派生类的序列化与反序列化,目前不支持多重继承
## 注册
首先看一个简单的例子
```cpp
struct Node
{
	int x;
	float y;
	std::string z;
	int add(int x,int y)
	{
		return x+y;
	}
	std::string getName()
	{
		return z;
	}
};
```
现在我想要这个类支持反射、序列化和反序列化
只需要做出如下修改：
* 实现```Config::get_config```方法
```cpp
	Config get_config()const
	{
		Config config=Serializable::get_config(this);
		config.update({
			{"x",x},
			{"y",y},
			{"z",z},
			{"add",add},
			{"getName",getName}
		});
		return config;
	}
```
* 在main函数中使用```Serializable::Regist<Node>()```完成注册
## 反射、序列化与反序列化

完成简单注册后，然后就可以使用如下方法支持属性的反射，序列化，反序列化
```cpp
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
output:
4
5
test
4
5
test
{ "z":"test", "y":5, "x":4, "class_name":"Node" } 4
5
test
11
test
*/
```
## 序列化、反序列化代码示例
序列化，反序列化关键方法只有两个
```Serializable::dumps```和```Serializable::loads<T>```
分别对应序列化与反序列化，其中反序列化需要显式的给出被反序列化的对象类型
如果不给的话将会返回一个```void*```指针，如果给了参数，会返回一个引用
如果不想调用额外的复制构造函数，可以使用```auto&object=Serializable::loads<T>(json_string)```
### 示例代码1：
```cpp
#include"lang/serializable.h"
#include<iostream>
#include<fstream>
#include<cstring>
using namespace std;
struct Node
{
	Node()
	{
		std::memset(z,0,sizeof(z));
		t={1,2,3,4};
	}
	Config get_config()const                                                    //1.实现Config get_config()const 方法
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
	Serializable::Regist<Node>();                                                   //2.完成简单注册
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
/*
输出结果：
json
{ "t":[2021,10,19,10], "z":[2021,10,18], "y":[{ "t":[1,2,3,4], "z":[0,0,0], "y":[], "x":[0,[0,""]], "class_name":"Node" } ,null], "x":[3.2,[5,"test"]], "class_name":"Node" }

{
"t":[2021,10,19,10],
"z":[2021,10,18],
"y":[{ "t":[1,2,3,4], "z":[0,0,0], "y":[], "x":[0,[0,""]], "class_name":"Node" } ,null],
"x":[3.2,[5,"test"]],
"class_name":"Node"
}

{
"t":[2021,10,19,10],
"z":[2021,10,18],
"y":[{ "t":[1,2,3,4], "z":[0,0,0], "y":[], "x":[0,[0,""]], "class_name":"Node" } ,null],
"x":[3.2,[5,"test"]],
"class_name":"Node"
}
*/
```

### 示例代码2：二叉树的序列化

```cpp
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
```
将该json使用python来进行反序列化：
```python
import json
with open("test.json") as f:
    res=json.loads(f.readline())
res
```

输出结果

```python
{'rson': {'rson': None, 'lson': None, 'value': 6, 'class_name': 'Node'},
 'lson': {'rson': {'rson': {'rson': None,
    'lson': None,
    'value': 6,
    'class_name': 'Node'},
   'lson': {'rson': None, 'lson': None, 'value': 5, 'class_name': 'Node'},
   'value': 11,
   'class_name': 'Node'},
  'lson': {'rson': None, 'lson': None, 'value': 5, 'class_name': 'Node'},
  'value': 55,
  'class_name': 'Node'},
 'value': 49,
 'class_name': 'Node'}
```
### 示例代码3：派生类的序列化与反序列化
```cpp
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
/*
json string:
{ "t":[2021,"shijunfeng00"], "y":[1,2,3,4], "z":[5,6,7,8], "x":233, "class_name":"GrandSon" }
I AM OK
{
"t":[2021,"shijunfeng00"],
"y":[1,2,3,4],
"z":[5,6,7,8],
"x":233,
"class_name":"GrandSon"
}

233
1,2,3,4,
5,6,7,8,
2021 shijunfeng00
*/
```
##反射代码示例
反射相对于序列化和反序列化会多一些细节出来
如果只需要反射,可以只用```#include"lang/reflectable.h"```
以及对应的```Reflectable::Regist<T>()```和```Config config=Reflectable::get_config(this)```
这样的话就只有反射没有序列化/反序列化的功能

###代码示例4：打印属性名称和方法名称
```cpp
#include"lang/reflectable.h"
#include<iostream>
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
	Reflectable::Regist<Node>();
	Node a;
	cout<<"fields:\n";
	for(auto&it:Reflectable::get_field_names<Node>())
		cout<<"name:"<<it<<"	type:"<<Reflectable::get_field_type("Node",it)<<endl;  //打印属性名称
	cout<<"methods:\n";
	for(auto&it:Reflectable::get_method_names<Node>())                                     //打印方法名称
		cout<<"name:"<<it<<endl;
}
```
代码中使用```Node*object=*(Node*)Reflectable::get_instance("Node")```来得到一个Node对象
使用```Reflectable::get_field```来获得属性，对于private属性也能正常访问

* 对于有类型的对象，比如上文的```Node a```，可以采用```int&field=Reflectable::get_field<int>(a,"x");```来得到属性的引用，
  也可以使用```void*field=Reflectable::get_field(a,"x");```来得到一个```void*```指针。
* 对于```void*```指针表示的对象，比如```void*a=Reflectable::get_instance("Node")```得到的对象，需要给出具体类型的字符串名称，
  即```int&field2=Reflectable::get_field<int>(a,"Node","x");```，
  或者```void*field1=Reflectable::get_field(a,"Node","x");```
同样代码里面也可以使用```Reflectable::set_field```来直接设置属性类型
* 使用```Reflectable::set_field(a,"x",5);```或者```Reflectable::set_field<int>(a,"x",5);```
可以显式给出类型，也可以由输入参数自动推断。
* 对于```void*```类型，依然需要给出字符串名称```Reflectable::set_field<int>(a,"Node","x",5);```
对于成员函数的反射，需要显式给出函数的返回值类型```Reflectable::get_method<string>(b,"add",5,6);```
第一个参数是对象实例，然后是函数名称，然后是函数的参数列表
### 示例代码4.2
```cpp
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
/*
output:
4 4
5 5
test test
10 10
15 15
cpp cpp
13
cpp
5 5
*/
```
# 写在最后：
至于为什么我采用了get_config的方法，是受到了keras的启发
```python
class Quantizer2D(tf.keras.layers.Layer):
    def __init__(self,soft_sigma=1.0,hard_sigma=1e7,**kwargs):
        super(Quantizer2D,self).__init__(**kwargs)
        self.soft_sigma=soft_sigma
        self.hard_sigma=hard_sigma
    def quantizer2d(self,centers,tensor,sigma):
        num_centers=centers.shape.as_list()[1]
        shape=tf.shape(tensor)
        batch,width,height,dims=shape[0],shape[1],shape[2],shape[3]
        x=tf.reshape(tensor,[batch,width*height,1,dims])
        c=tf.expand_dims(centers,axis=1)
        dist=tf.reduce_mean(tf.square(x-c),axis=-1)
        one_hot=tf.nn.softmax(-sigma*dist)
        quant=tf.reduce_sum(self.index*one_hot,axis=-1)
        result=tf.reshape(quant,[-1,width,height])
        return result
    def build(self,input_shape):
        center_shape,tensor_shape=input_shape
        num_centers=center_shape.as_list()[1]
        self.index=self.add_weight(name='index',
                                   shape=(num_centers,),
                                   initializer=tf.keras.initializers.Constant([i for i in range(num_centers)]),
                                   dtype=tf.float32,
                                   trainable=False)
        super(Quantizer2D,self).build(input_shape)
        self.built=True
    def call(self,inputs):
        centers,tensor=inputs
        soft_quantizer=self.quantizer2d(centers,tensor,self.soft_sigma)
        hard_quantizer=self.quantizer2d(centers,tensor,self.hard_sigma)
        quantizer=tf.stop_gradient(hard_quantizer-soft_quantizer)+soft_quantizer
        return quantizer
    def get_config(self):
        config=super(Quantizer2D,self).get_config()
        config.update({
            "soft_sigma":self.soft_sigma,
            "hard_sigma":self.hard_sigma
        })
        return config
```
