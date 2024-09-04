#include "config.h"
#include "log.h"

namespace sylar{


//"A.B", 10
// A:
//  B:10
//  C::str
// static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static void ListAllMember(const std::string& prefix, const YAML::Node& node, 
                            std::list<std::pair<std::string, const YAML::Node>>& output)
                            {
                                if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678")!= std::string::npos)
                                {
                                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) <<"Config invalid name= "<< prefix<<":"<<node;
                                    return ;
                                }
                                output.push_back(std::make_pair(prefix, node));
                                if(node.IsMap())
                                {
                                    for(auto it = node.begin();it!=node.end();it++)
                                    {
                                        ListAllMember(prefix.empty()? it->first.Scalar() : prefix+"."+ it->first.Scalar(), it->second, output);

                                    }
                                }

                            }


//使用YAML::Node 初始化配置模块
void Config::LoadFromYaml(const YAML::Node& root)
{
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    ListAllMember("", root, all_nodes);
    
    for(auto& i : all_nodes)
    {
        std::string key = i.first;
        if(key.empty())
        {
            continue;
        }

        std::transform(key.begin(),key.end(),key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);

        if(var)
        {
            if(i.second.IsScalar())
            {
                var->fromString(i.second.Scalar());
            }else{
                std::stringstream ss;
                ss<<i.second;
                var->fromString(ss.str());
            }
        }


    }
    

}

//加载path文件夹里面的配置文件
void Config::LoadFromConfDir(const std::string& path, bool force)
{

}


//查找配置参数,返回配置参数的基类
ConfigVarBase::ptr Config::LookupBase(const std::string& name)
{
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

//遍历配置模版里面所有的配置项
void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb)
{

}



}