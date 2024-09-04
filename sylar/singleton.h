#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__



namespace sylar
{


//单例模式封装类
template <class T, class X=void, int N=0>
class Singleton{
    public:
        //返回单例的裸指针
        static T* GetInstance()
        {
            static T v;
            return &v;
        }
};

}
#endif