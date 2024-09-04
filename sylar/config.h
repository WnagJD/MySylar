#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "log.h"
#include "util.h"
#include <yaml-cpp/yaml.h>
#include <boost/lexical_cast.hpp>


namespace sylar{

//配置变量的基类

class ConfigVarBase{
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;
        ConfigVarBase(const std::string& name, const std::string& description= "")
                :m_name(name)
                ,m_description(description)
                {
                    //name 转换为小写
                    std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
                }

        virtual ~ConfigVarBase(){}

        const std::string& getName() const{return m_name;}

        const std::string& getDecription() const{ return m_description;}

        /**
         * @brief 转为字符串
         */
        virtual std::string toString() =0;


        /**
         * @brief 字符串初始化配置变量
         */
        virtual bool fromString(const std::string& val) =0;


        /**
         * @brief 返回配置变量值的类型名称
         */
        virtual std::string getTypeName()const =0;



    protected:
        std::string m_name;
        std::string m_description;

};

/**
 * @brief 简单类型转换模板类(F 原类型 T 目标类型)
 * 简单类型
 */

template<class F, class T>
class LexicalCast{
    public:
        /**
         * @brief 类型转换
         * @param[in] v 源类型值
         * @return 返回v转换后的目标类型
         * @exception 当类型不可转换时,抛出异常
         */
        T operator()(const F& v){
            //boost::lexical_cast 如果出错,会抛出异常
            return boost::lexical_cast<T>(v);
        }
};


/**
 * @brief 类型转化模板类偏特化(YAML string 转换成 std::vector<T>)
 * 偏---部分
 */
template<class T>
class LexicalCast<std::string, std::vector<T>>{
    public:
        std::vector<T> operator()(const std::string& v)
        {
            //v 表示有效的yaml格式的字符串
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for(size_t i=0;i<node.size();i++)
            {
                //清空ss
                ss.str("");
                ss<<node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }

            return vec;

        }
};

/**
 * @brief 类型转化模板类偏特化(std::vector<T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::vector<T>,std::string>{
    public:
        std::string operator()(const std::vector<T>& v){
            YAML::Node node(YAML::NodeType::Sequence);
            for(auto&i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }

            std::stringstream ss;
            ss<<node;
            return ss.str();
        }      
};


// YMAL string 转化为 std::List<T>
template<class T>
class LexicalCast<std::string, std::list<T>>{
    public:
        std::list<T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);
            typename std::list<T> vec;
            std::stringstream ss;
            for(size_t i=0;i<node.size();i++)
            {
                ss.str("");
                ss<<node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }

};

//std::list<T> 转换为 YAML::string
template <class T>
class LexicalCast<std::list<T>, std::string>{
    public:
        std::string operator()(const std::list<T>& v){
            YAML::Node node(YAML::NodeType::Sequence);
            for(auto& i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss<<node;
            return ss.str();
        }

};


//YAML std::string 转换为 std::set<T>
template <class T>
class LexicalCast<std::string, std::set<T>>{
    public:
        std::set<T> operator()(const std::string& v)
        {
            typename std::set<T> vec;
            YAML::Node node = YAML::Load(v);
            std::stringstream ss; 
            for(size_t i=0;i<node.size();i++)
            {
                ss<<"";
                ss<<node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
};

//std::set<T> 转换为YAML::string
template<class T>
class LexicalCast<std::set<T>, std::string>{
    public:
        std::string operator()(const std::set<T>& v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for(auto& i: v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }

            std::stringstream ss;
            ss<<node;
            return ss.str();
        }

};


//YMAL string 转换为 std::unordered_set<T>
template<class T>
class LexicalCast<std::string, std::unordered_set<T>>{
        public:
            std::unordered_set<T> operator()(const std::string& v){
                YAML::Node node = YAML::Load(v);
                std::stringstream ss;
                typename std::unordered_set<T> vec;
                for(size_t i=0;i<node.size();i++)
                {
                    ss.str("");
                    ss<<node[i];
                    vec.insert(LexicalCast<std::string,T>()(ss.str()));
                }

                return vec;
            }
};


//std::set<T> 转换为 YAML string
template<class T>
class LexicalCast<std::unordered_set<T>, std::string>{
    public:
            std::string operator()(const std::unordered_set<T>& v )
            {
                YAML::Node node(YAML::NodeType::Sequence);
                for(auto&i:v)
                {
                    node.push_back(YAML::Load(LexicalCast<T, std::string>()(v)));
                }
                std::stringstream ss;
                ss << node;
                return ss.str();
            }

};


//YAML string 转换为 std::map<std::string, T>
template<class T>
class LexicalCast<std::string, std::map<std::string,T>>{
    public:
        std::map<std::string,T> operator()(const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            
            typename std::map<std::string, T> vec;
            std::stringstream ss;
            for(auto it = node.begin();it!=node.end();it++)
            {
                ss<<"";
                ss<<it->second;
                // vec[it->first] = LexicalCast<std::string, T>()(ss.str());
                vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
};

//std::map<std::string,T> 转换为 YAML string
//将std::map<std::string,T>类型的值先转换为YAML::Node node 结点上的值
//在通过std::stringstream (ss << node)  可以直接将符合YAML格式的node结点上的值转换为std::string 
template<class T>
class LexicalCast<std::map<std::string,T>, std::string>{
    public:
        std::string operator()(const std::map<std::string, T>&v){
            //构造一个node结点
            YAML::Node node(YAML::NodeType::Map);
            for(auto &i : v)
            {
                //构造新的结点
                node[i.first]=YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss<<node;
            return ss.str();
        }
};


//YAML string 转换为 std::unordered_map<std::string, T>
template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>>{
    public:
        std::unordered_map<std::string,T> operator()(const std::string& v){
            //将YAML string 转换为 YAML::Node 结点
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> vec;
            std::stringstream ss;
            for(auto it = node.begin();it!= node.end();it++)
            {
                ss<<"";
                ss<<it->second;
                vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }

            return vec;

        }
};


//std::unordered_map<std::string, T> 转换为 YAML string
//获取std::unordered_map<std::string,T>中的元素,将这个元素去构建node结点
template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string>{
        public:
            std::string operator()(const std::map<std::string, T>&v){
                YAML::Node node(YAML::NodeType::Map);
                for(auto & it : v)
                {
                    node[it.first] = YAML::Load(LexicalCast<T,std::string>()(v));
                }

                std::stringstream ss;
                ss <<node;
                return ss.str();
            }
};

/**
 * 容器类型(复杂类型) 转为 YAML string 
 * 1.保持转换后的string符合YAML格式：通过node节点的自动转换    ss<<node 
 * 2.构建node节点：通过YAML::load(std::string)来构建
 * 3.需要转换容器中元素的值保持string
 */


/**
 * YAMl string 转为 容器类型(复杂类型)
 * 1.YAML string  转换为node节点
 * 2.获取每一个节点,再将节点转换为YAML string
 * 3.不断地将string 转换为元素类型,直到到达简单类型的转换
 */

//配置参数模版子类
//FromStr 从string 转换为T类型的仿函数
//ToStr T类型转换为string的仿函数  string --- YAML格式的字符串
template<class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase{
    public:
        typedef std::shared_ptr<ConfigVar> ptr;
        typedef std::function<void(const T& old_value, const T& new_value)> on_change_cb;

        ConfigVar(const std::string& name, const T&default_value, const std::string& description="")
                :ConfigVarBase(name, description)
                ,m_val(default_value)
                {

                }

        //将配置参数值转换为Yaml字符串
        //转换失败会抛出异常
        std::string toString() override{
            try{

                return ToStr()(m_val); 
            }catch(std::exception& e) //捕获异常
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception "
                                                  << e.what() <<" convert: "<< TypeToName<T>() <<" to string"
                                                  <<" name="<<m_name;
            }
            return "";
        }

        //YAML string 转换为 配置参数值
        //转换失败会抛出异常
        bool fromString(const std::string& val) override{
            try{
                setValue(FromStr()(val));
            }catch(std::exception& e)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::FromStr exception "
                                                  << e.what() <<" convert: string to"<< TypeToName<T>()
                                                  <<" name="<<m_name
                                                  <<" - "<<val;
                                            
            }
            return false;
        }

        //获取当前参数值
        const T getValue()const{return m_val;}

        //设置当前参数值
        //如果参数值发生变化,则通知对应的注册回调函数
        void setValue(const T& val){
            if(val == m_val){
                return ;
            }
            for(auto& i : m_cbs)
            {
                i.second(m_val, val);
            }
            m_val = val;
        }

        //返回参数值的类型名称
        std::string getTypeName() const override{return TypeToName<T>();}

        //添加变化回调函数
        //返回该回调函数对应的唯一id,用于删除回调
        uint64_t addListener(on_change_cb cb)
        {
            static uint64_t s_fun_id =0;
            s_fun_id++;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }

        //删除变化回调函数
        //key对应唯一的id
        void delListener(uint64_t key){
            m_cbs.erase(key);
        }

        //获取回调函数
        //使用唯一的key
        on_change_cb getListener(uint64_t key)
        {
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

        //清理所有的回调函数
        void clearListener(){
            m_cbs.clear();
        }


    private:
        T m_val;

        //变更回调函数, key保持唯一
        std::map<uint64_t, on_change_cb> m_cbs;



} ;



//ConfigVar 管理类
//提供便捷的方法创建/访问ConfigVar类

class Config{
    public:
        typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;

        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& val, const std::string& description="")
        {
            auto it = GetDatas().find(name);
            if(it != GetDatas().end())
            {
                auto temp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if(temp)
                {
                    //name 已存在,并且T的类型和name对应的配置参数值类型是相同的
                    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<" Lookup: name: "<<name<< " exists";
                    return temp;
                }else{
                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << " Lookup name=" << name<<" exists but type not"
                                                      <<TypeToName<T>()<<" real_type="<<it->second->getTypeName()
                                                      <<" "<<it->second->toString();
                    
                    return nullptr;

                }
            }
            //没找到----按照参数值,创建新的ConfigVar配置参数
            if(name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos)
            {
                //说明name参数包括了除"abcdefghikjlmnopqrstuvwxyz._012345678"以外的其他字符
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"Lookup name invalid "<<name;
                //抛出异常
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, val, description));
            GetDatas()[name]=v;
            return v;

        }

        //查找配置参数

        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string& name)
        {
            auto it = GetDatas().find(name);
            if(it == GetDatas().end())
            {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }



        //使用YAML::Node 初始化配置模块
        static void LoadFromYaml(const YAML::Node& root);

        //加载path文件夹里面的配置文件
        static void LoadFromConfDir(const std::string& path, bool force = false);


        //查找配置参数,返回配置参数的基类
        static ConfigVarBase::ptr LookupBase(const std::string& name);

        //遍历配置模版里面所有的配置项
        static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

    private:
        static ConfigVarMap &GetDatas(){
            static ConfigVarMap s_datas;
            return s_datas;
        }

};













}













#endif