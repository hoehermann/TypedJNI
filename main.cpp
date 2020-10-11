#include <jni.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>

namespace TypedJNI {
template <typename T>
std::string GetTypeString(){
    static_assert(std::is_same<T,void>::value, "Cannot handle this type.");
};
template <>
std::string GetTypeString<jstring>(){return "Ljava/lang/String;";};
template <>
std::string GetTypeString<jint>(){return "I";};
template <>
std::string GetTypeString<jlong>(){return "J";};
template <>
std::string GetTypeString<void>(){return "V";};
template<typename T, typename... Args>
typename std::enable_if<sizeof...(Args) != 0, std::string>::type
GetTypeString() {
    std::cerr << "There are 1 + " << sizeof...(Args) << " Args here. First one resolves to " << GetTypeString<T>() << "." << std::endl;
    return GetTypeString<T>() + GetTypeString<Args...>();
};
template<typename... Args>
typename std::enable_if<sizeof...(Args) == 0, std::string>::type
GetTypeString() {
    std::cerr << "There are no Args here. Returning empty string." << std::endl;
    return "";
};

jmethodID GetStaticMethodID(JNIEnv *env, const jclass cls, const std::string name, const std::string & signature) {
    jmethodID mid = env->GetStaticMethodID(cls, name.c_str(), signature.c_str());
    if (mid == NULL) {
        throw std::runtime_error("Failed to find static method '"+name+"' "+signature+".");
    }
    return mid;
}
jmethodID GetMethodID(JNIEnv *env, const jclass cls, const std::string name, const std::string & signature) {
    jmethodID mid = env->GetMethodID(cls, name.c_str(), signature.c_str());
    if (mid == NULL) {
        throw std::runtime_error("Failed to find method '"+name+"' "+signature+".");
    }
    return mid;
}
} // namespace TypedJNI

// based on https://stackoverflow.com/questions/9065081/
template<typename T> 
class TypedJNIStaticMethod;
template<typename ...Args> 
class TypedJNIStaticMethod<void(Args...)>
{
    public:
    static std::function<void(Args...)> get(JNIEnv *env, const jclass cls, const std::string & name) {
        jmethodID mid = TypedJNI::GetStaticMethodID(env, cls, name, "("+TypedJNI::GetTypeString<Args...>()+")"+TypedJNI::GetTypeString<void>());
        return [env, cls, mid](Args... args) {
            env->CallStaticVoidMethod(cls, mid, args...);
        };
    }
};
template<typename ...Args> 
class TypedJNIStaticMethod<jint(Args...)>
{
    public:
    static std::function<jint(Args...)> get(JNIEnv *env, const jclass cls, const std::string name) {
        const jmethodID mid =  TypedJNI::GetStaticMethodID(env, cls, name, "("+ TypedJNI::GetTypeString<Args...>()+")"+ TypedJNI::GetTypeString<jint>());
        return [env, cls, mid](Args... args)-> jint {
            return env->CallStaticIntMethod(cls, mid, args...);
        };
    }
};


template<typename T> 
class TypedJNIMethod;
template<typename ...Args> 
class TypedJNIMethod<void(Args...)>
{
    public:
    static std::function<void(Args...)> get(JNIEnv *env, const jclass cls, const jobject obj, const std::string & name) {
        jmethodID mid = TypedJNI::GetMethodID(env, cls, name, "("+TypedJNI::GetTypeString<Args...>()+")"+TypedJNI::GetTypeString<void>());
        return [env, obj, mid](Args... args) {
            env->CallVoidMethod(obj, mid, args...);
        };
    }
};
template<typename ...Args> 
class TypedJNIMethod<jint(Args...)>
{
    public:
    static std::function<jint(Args...)> get(JNIEnv *env, const jclass cls, const jobject obj, const std::string name) {
        const jmethodID mid =  TypedJNI::GetMethodID(env, cls, name, "("+ TypedJNI::GetTypeString<Args...>()+")"+ TypedJNI::GetTypeString<jint>());
        return [env, obj, mid](Args... args)-> jint {
            return env->CallIntMethod(obj, mid, args...);
        };
    }
};

class TypedJNIObject {
    private:
    JNIEnv *env;
    jclass cls;
    jobject obj;
    public:
    TypedJNIObject(JNIEnv *env, jclass cls, jobject obj) : env(env), cls(cls), obj(obj) {};
    template<typename... Args>
    std::function<Args...> GetMethod(const std::string name) {
        return TypedJNIMethod<Args...>::get(env, cls, obj, name);
    }
    // TODO: clean up object / reference count on destruction
};

template<typename ...Args> 
class TypedJNIConstructor
{
    public:
    static std::function<TypedJNIObject(Args...)> get(JNIEnv *env, const jclass cls) {
        const jmethodID mid = TypedJNI::GetMethodID(env, cls, "<init>", "("+TypedJNI::GetTypeString<Args...>()+")"+TypedJNI::GetTypeString<void>());
        return [env, cls, mid](Args... args) -> TypedJNIObject {
            return TypedJNIObject(env, cls, env->NewObject(cls, mid, args...));
        };
    }
};

class TypedJNIClass {
    private:
    JNIEnv *env;
    public:
    const jclass cls;
    TypedJNIClass(JNIEnv *env, jclass cls) : env(env), cls(cls) {
        if (cls == NULL) {
            std::runtime_error("Failed to find class.");
        }
    }
    template<typename... Args>
    std::function<Args...> GetStaticMethod(const std::string name) {
        return TypedJNIStaticMethod<Args...>::get(env, cls, name);
    }
    template<typename... Args>
    std::function<TypedJNIObject(Args...)> GetConstructor() {
        return TypedJNIConstructor<Args...>::get(env, cls);
    }
};
class TypedJNIEnv : public JNIEnv {
    public:
    JavaVM *vm;
    JNIEnv *env;
    TypedJNIEnv(JavaVMInitArgs vm_args) {
        jint res = JNI_CreateJavaVM(&vm, (void **)&env, &vm_args);
        if (res != JNI_OK) {
            std::runtime_error("Failed to create Java VM.");
        }
    }
    virtual ~TypedJNIEnv() {
        vm->DestroyJavaVM();
    }
    TypedJNIClass FindClass(std::string name) {
        return TypedJNIClass(env, env->FindClass(name.c_str()));
    }
};

int main(int argc, char **argv)
{
    JavaVMInitArgs  vm_args;
    std::vector<JavaVMOption> options(3);
    options.at(0).optionString = const_cast<char*>("-verbose:gc");
    options.at(1).optionString = const_cast<char*>("-XX:+PrintGCTimeStamps");
    options.at(2).optionString = const_cast<char*>("-XX:+PrintGCDetails");
    vm_args.options = options.data();
    vm_args.nOptions = options.size();
    vm_args.version  = JNI_VERSION_1_8;
    TypedJNIEnv tenv(vm_args);
    TypedJNIClass javacls = tenv.FindClass("Java");
    javacls.GetStaticMethod<void(void)>("printHelloWorld");
    javacls.GetStaticMethod<void(jlong)>("printLong")(1);
    javacls.GetStaticMethod<void(jlong,jlong)>("print2Long")(1,2);
    const long i = 1;
    std::cout << i << " incremented by Java is " << javacls.GetStaticMethod<jint(jint)>("increment")(i) << std::endl;
    
    TypedJNIObject javaobj = javacls.GetConstructor<jint>()(1);
    javaobj.GetMethod<jint(jint)>("incrementCounterBy")(2);
    javaobj.GetMethod<void(void)>("printCounter")();

    return 0;
}