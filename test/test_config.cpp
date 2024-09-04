#include "sylar/config.h"
#include <string>
#include <iostream>
#include <yaml-cpp/yaml.h>

void print_yaml(const YAML::Node& node, int level)
{
    if(node.IsScalar())
    {
        std::cout<< std::string(level*4,' ')<< node.Scalar()<<" - "<< node.Type() << " - " << level<<std::endl;
    }else if(node.IsNull())
    {
        std::cout<< std::string(level*4,' ')<< "NULL - " << node.Type()<<" - " << level<<std::endl;
    }else if(node.IsMap())
    {
        for(auto it = node.begin();it!=node.end(); it++)
        {
            std::cout<<std::string(level*4,' ')<<it->first << " - "<<it->second.Type()<< " - "<< level<<std::endl;
            print_yaml(it->second, level+1);
        }
        
    }else if(node.IsSequence())
    {
        for(size_t i=0;i<node.size();i++)
        {
            std::cout<<std::string(level*4,' ')<< i << " - "<< node[i].Type()<<" - "<< level << std::endl;
            print_yaml(node[i], level+1);
        }
    }
}


class Person{
    public:
        Person(){}; 
        std::string m_name;
        int m_age = 0;
        bool m_sex = 0;
        std::string toString(){
            std::stringstream ss;
            ss<<"[Person name="<<m_name<<" age"<<m_age
                <<" sex"<<m_sex<<"]";
            return ss.str();
        }

        bool operator==(const Person& oth) const{
            return m_name==oth.m_name && m_age==oth.m_age && m_sex==oth.m_age;
        }
};

//实现自定义类型的YAML序列化和反序列化,这部分要放在sylar命名空间
namespace sylar{
template<>
class LexicalCast<std::string, Person>{
        public:
            Person operator()(const std::string& v){
                YAML::Node node = YAML::Load(v);
                Person p;
                p.m_name = node["name"].as<std::string>();
                p.m_age = node["age"].as<int>();
                p.m_sex = node["sex"].as<bool>();

                return p;
            }

};


template<>
class LexicalCast<Person, std::string>{
    public:
        std::string operator()(const Person& p)
        {
            YAML::Node node;
            node["name"] = p.m_name;
            node["age"] = p.m_age;
            node["sex"] = p.m_sex;

            std::stringstream ss;
            ss<< node;
            return ss.str();
        }
};

}







int main()
{
    YAML::Node node = YAML::LoadFile("/home/wangjiandong/MySylar/bin/log.yaml");
    print_yaml(node,1);

    std::cout<<YAML::NodeType::Null<<std::endl;
    std::cout<<YAML::NodeType::Scalar<<std::endl;
    std::cout<<YAML::NodeType::Sequence<<std::endl;
    std::cout<<YAML::NodeType::Map<<std::endl;

    return 0;
}