#include <jni.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <functional>

template <typename T>
std::string GetTypeString(){
    static_assert(std::is_same<T,void>::value, "Cannot handle this type.");
};
template <>
std::string GetTypeString<jstring>(){return "Ljava/lang/String;";};
template <>
std::string GetTypeString<jlong>(){return "J";};
template <>
std::string GetTypeString<void>(){return "V";};
template<typename T, typename... Args>
typename std::enable_if<sizeof...(Args) != 0, std::string>::type
GetTypeString() {
    std::cerr << "There are 1+" << sizeof...(Args) << " Args here. First one resolves to " << GetTypeString<T>() << "." << std::endl;
    return GetTypeString<T>() + GetTypeString<Args...>();
};
template<typename... Args>
typename std::enable_if<sizeof...(Args) == 0, std::string>::type
GetTypeString() {
    std::cerr << "There are no Args here. Returning empty string." << std::endl;
    return "";
};

// interesting for later: https://stackoverflow.com/questions/9065081/how-do-i-get-the-argument-types-of-a-function-pointer-in-a-variadic-template-cla

class TypedJNIClass {
    private:
    JNIEnv *env;
    const jclass cls;
    public:
    TypedJNIClass(JNIEnv *env, jclass cls) : env(env), cls(cls) {
        if (cls == NULL) {
            std::runtime_error("Failed to find class.");
        }
    }
    template<typename... Args>
    std::function<void(Args...)> GetStaticMethod(const std::string name) {
        const std::string signature = "("+GetTypeString<Args...>()+")V";
        //std::cerr << "Looking up static method '" << name << "' with signature '" << signature << "'." << std::endl;
        jmethodID mid = env->GetStaticMethodID(cls, name.c_str(), signature.c_str());
        if (mid == NULL) {
            throw std::runtime_error("Failed to find function '"+name+"'.");
        }
        return [this, mid](Args... args) {
            env->CallStaticVoidMethod(cls, mid, args...);
        };
    }
};
class TypedJNIEnv : public JNIEnv {
    public:
    JavaVM *vm;
    JNIEnv *env;
    TypedJNIEnv(JavaVM *vm, JNIEnv *env) : vm(vm), env(env) {}
    virtual ~TypedJNIEnv() {
        vm->DestroyJavaVM();
    }
    TypedJNIClass FindClass(std::string name) {
        return TypedJNIClass(env, env->FindClass(name.c_str()));
    }
};

int main(int argc, char **argv)
{
    std::cerr << GetTypeString<void>() << std::endl;
    /*
    std::cerr << GetTypeString<jlong>() << std::endl;
    std::cerr << GetTypeString<jstring>() << std::endl;
    std::cerr << GetTypeString<jlong, jstring>() << std::endl;
    */
    
    JavaVM         *vm;
    JNIEnv         *env;
    JavaVMInitArgs  vm_args;
    jint            res;
    jmethodID       mid;
    jstring         jstr;
    jobject         jobj;

    vm_args.version  = JNI_VERSION_1_8;
    vm_args.nOptions = 0;
    res = JNI_CreateJavaVM(&vm, (void **)&env, &vm_args);
    if (res != JNI_OK) {
        printf("Failed to create Java VM.\n");
        return 1;
    }
    TypedJNIEnv tenv(vm, env);
    TypedJNIClass jJava = tenv.FindClass("Java");
    //jJava.GetStaticMethod<void>("printHelloWorld");
    jJava.GetStaticMethod<jlong>("printLong")(1);
    jJava.GetStaticMethod<jlong,jlong>("print2Long")(1,2);

    return 0;
}