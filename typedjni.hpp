#pragma once

#include <jni.h>
#include <string>
#include <stdexcept>
#include <functional>
#include <vector>
#include <memory>

namespace TypedJNI {
template <typename T>
std::string GetTypeString(){
    static_assert(std::is_same<T,void>::value, "Cannot handle this type.");
    return "This actually never gets compiled.";
};
template <>
std::string GetTypeString<jstring>();
template <>
std::string GetTypeString<jboolean>();
template <>
std::string GetTypeString<jint>();
template <>
std::string GetTypeString<jlong>();
template <>
std::string GetTypeString<void>();
template<typename T, typename... Args>
typename std::enable_if<sizeof...(Args) != 0, std::string>::type
GetTypeString() {
    return GetTypeString<T>() + GetTypeString<Args...>();
};
template<typename... Args>
typename std::enable_if<sizeof...(Args) == 0, std::string>::type
GetTypeString() {
    return "";
};

jmethodID GetStaticMethodID(JNIEnv *env, const jclass cls, const std::string name, const std::string & signature);
jmethodID GetMethodID(JNIEnv *env, const jclass cls, const std::string name, const std::string & signature);
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
template<typename ...Args> 
class TypedJNIMethod<jobject(Args...)>
{
    public:
    static std::function<jobject(Args...)> get(JNIEnv *env, const jclass cls, const jobject obj, const std::string name) {
        const jmethodID mid =  TypedJNI::GetMethodID(env, cls, name, "("+ TypedJNI::GetTypeString<Args...>()+")"+ TypedJNI::GetTypeString<jobject>());
        return [env, obj, mid](Args... args)-> jobject {
            return env->CallObjectMethod(obj, mid, args...);
        };
    }
};
/*
 * Handle jstring for convenience. Everything else is just jobject. 
 * Might be extended some day. But only types officially mentioned at 
 * https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/types.html#type_signatures
 * can be considered here.
 */
template<typename ...Args> 
class TypedJNIMethod<jstring(Args...)>
{
    public:
    static std::function<jstring(Args...)> get(JNIEnv *env, const jclass cls, const jobject obj, const std::string name) {
        const jmethodID mid =  TypedJNI::GetMethodID(env, cls, name, "("+ TypedJNI::GetTypeString<Args...>()+")"+ TypedJNI::GetTypeString<jstring>());
        return [env, obj, mid](Args... args)-> jstring {
            return static_cast<jstring>(env->CallObjectMethod(obj, mid, args...));
        };
    }
};

class TypedJNIObject {
    private:
    JNIEnv *env = nullptr;
    jclass cls = nullptr;
    std::shared_ptr<_jobject> obj = nullptr;
    public:
    TypedJNIObject(JNIEnv *env, jclass cls, jobject obj);
    template<typename... Args>
    std::function<Args...> GetMethod(const std::string name) {
        return TypedJNIMethod<Args...>::get(env, cls, obj.get(), name);
    }
};

template<typename ...Args> 
class TypedJNIConstructor
{
    public:
    static std::function<TypedJNIObject(Args...)> get(JNIEnv *env, const jclass cls) {
        // yes indeed GetMethodID as illustrated at https://stackoverflow.com/questions/7260376/
        const jmethodID mid = TypedJNI::GetMethodID(env, cls, "<init>", "("+TypedJNI::GetTypeString<Args...>()+")"+TypedJNI::GetTypeString<void>());
        return [env, cls, mid](Args... args) -> TypedJNIObject {
            return TypedJNIObject(env, cls, env->NewObject(cls, mid, args...));
        };
    }
};

class TypedJNIClass {
    private:
    JNIEnv *env = nullptr;
    public:
    const jclass cls = nullptr;
    TypedJNIClass(JNIEnv *env, jclass cls);
    template<typename... Args>
    std::function<Args...> GetStaticMethod(const std::string name) {
        return TypedJNIStaticMethod<Args...>::get(env, cls, name);
    }
    template<typename... Args>
    std::function<TypedJNIObject(Args...)> GetConstructor() {
        return TypedJNIConstructor<Args...>::get(env, cls);
    }
};

class TypedJNIString {
    private:
    std::shared_ptr<_jstring> jstrptr = nullptr;
    public:
    TypedJNIString(JNIEnv *env, const std::string & str);
    operator jstring() const;
};

class TypedJNIEnv {
    public:
    JavaVM *vm = nullptr;
    JNIEnv *env = nullptr;
    TypedJNIEnv(const TypedJNIEnv&) = delete;
    TypedJNIEnv& operator=(const TypedJNIEnv&) = delete;
    TypedJNIEnv(JavaVMInitArgs vm_args);
    virtual ~TypedJNIEnv();
    TypedJNIClass find_class(std::string name);
    TypedJNIString make_jstring(const std::string & str);
};
