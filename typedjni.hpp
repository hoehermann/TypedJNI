#pragma once

#include <jni.h>
#include <string>
#include <stdexcept>
#include <functional>
#include <memory>

/**
 * Exception class for all runtime errors regarding TypedJNI.
 */
class TypedJNIError : public std::runtime_error 
{
    public:
    TypedJNIError(const std::string& what_arg);
};

namespace TypedJNI { // namespace for internal helper functions
/**
 * GetTypeString maps the type arguments to the corresponding JNI type signature.
 * 
 * * GetTypeString<jboolean>() → "Z"
 * * GetTypeString<jbyte>() → "B"
 * * and so on
 * 
 * The full list is at https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/types.html#type_signatures.
 * 
 * Can handle an arbitrary number of arguments: `GetTypeString<jboolean,jbyte>()` → "ZB"
 */
template <typename T>
std::string GetTypeString(){
    // this declares the templated function, but the default template may never actually be used
    // TODO: find out where I got this from
    static_assert(std::is_same<T,void>::value, "Cannot handle this type.");
    return "This actually never gets compiled.";
};
template <> std::string GetTypeString<void>();
template <> std::string GetTypeString<jboolean>();
template <> std::string GetTypeString<jint>();
template <> std::string GetTypeString<jlong>();
template <> std::string GetTypeString<jobject>();
template <> std::string GetTypeString<jstring>();
template <> std::string GetTypeString<jbyteArray>();
template<typename T, typename... Args>
typename std::enable_if<sizeof...(Args) != 0, std::string>::type
GetTypeString() {
    // recursively expand type string
    return GetTypeString<T>() + GetTypeString<Args...>();
};
template<typename... Args>
typename std::enable_if<sizeof...(Args) == 0, std::string>::type
GetTypeString() {
    // end of recursion
    return "";
};

/**
 * Wrapper around GetStaticMethodID(…)
 * 
 * Performs error-checking. Throws an exception on error.
 */
jmethodID GetStaticMethodID(JNIEnv *env, const jclass cls, const std::string name, const std::string & signature);

/**
 * Wrapper around GetMethodID(…)
 * 
 * Performs error-checking. Throws an exception on error.
 */
jmethodID GetMethodID(JNIEnv *env, const jclass cls, const std::string name, const std::string & signature);
} // namespace TypedJNI

/**
 * TypedJNIStaticMethod selects the corresponding JNI function for static methods based on the requested return type.
 * 
 * * TypedJNIStaticMethod<void>::get(…) → CallStaticVoidMethod
 * * TypedJNIStaticMethod<jint>::get(…) → CallStaticIntMethod
 * * and so on
 * 
 * The full list is at https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html#calling_static_methods.
 * 
 * For convenience, jstring is handled explicitly. All other objects are jobject. 
 * 
 * Based on https://stackoverflow.com/questions/9065081/.
 */
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

/**
 * Like TypedJNIStaticMethod, but for object methods.
 */
template<typename T> 
class TypedJNIMethod;

/**
 * Class for a proxy object referencing a Java object.
 * 
 * The local reference to the Java object is deleted when the last copy of the proxy object is destroyed.
 */
class TypedJNIObject {
    public:
    JNIEnv *env = nullptr;
    const jclass cls = nullptr;
    const std::shared_ptr<_jobject> obj = nullptr;
    TypedJNIObject(JNIEnv *env, jclass cls, jobject obj);
    template<typename... Args>
    std::function<Args...> GetMethod(const std::string name) {
        return TypedJNIMethod<Args...>::get(*this, name);
    }
};

template<typename ...Args> 
class TypedJNIMethod<void(Args...)>
{
    public:
    static std::function<void(Args...)> get(const TypedJNIObject & obj, const std::string & name) {
        jmethodID mid = TypedJNI::GetMethodID(obj.env, obj.cls, name, "("+TypedJNI::GetTypeString<Args...>()+")"+TypedJNI::GetTypeString<void>());
        return [obj, mid](Args... args) {
            obj.env->CallVoidMethod(obj.obj.get(), mid, args...);
        };
    }
};
template<typename ...Args> 
class TypedJNIMethod<jint(Args...)>
{
    public:
    static std::function<jint(Args...)> get(const TypedJNIObject & obj, const std::string name) {
        const jmethodID mid = TypedJNI::GetMethodID(obj.env, obj.cls, name, "("+ TypedJNI::GetTypeString<Args...>()+")"+ TypedJNI::GetTypeString<jint>());
        return [obj, mid](Args... args)-> jint {
            return obj.env->CallIntMethod(obj.obj.get(), mid, args...);
        };
    }
};
template<typename ...Args> 
class TypedJNIMethod<jobject(Args...)>
{
    public:
    static std::function<jobject(Args...)> get(const TypedJNIObject & obj, const std::string name) {
        const jmethodID mid = TypedJNI::GetMethodID(obj.env, obj.cls, name, "("+ TypedJNI::GetTypeString<Args...>()+")"+ TypedJNI::GetTypeString<jobject>());
        return [obj, mid](Args... args)-> jobject {
            return obj.env->CallObjectMethod(obj.obj.get(), mid, args...);
        };
    }
};
template<typename ...Args> 
class TypedJNIMethod<jstring(Args...)>
{
    public:
    static std::function<jstring(Args...)> get(const TypedJNIObject & obj, const std::string name) {
        const jmethodID mid = TypedJNI::GetMethodID(obj.env, obj.cls, name, "("+ TypedJNI::GetTypeString<Args...>()+")"+ TypedJNI::GetTypeString<jstring>());
        return [obj, mid](Args... args)-> jstring {
            return static_cast<jstring>(obj.env->CallObjectMethod(obj.obj.get(), mid, args...));
        };
    }
};

/**
 * Special case of TypedJNIMethod for accessing a Java constructor.
 */
template<typename ...Args> 
class TypedJNIConstructor
{
    public:
    static std::function<TypedJNIObject(Args...)> get(JNIEnv *env, const jclass cls) {
        // yes indeed GetMethodID as illustrated at https://stackoverflow.com/questions/7260376/ (not get GetStaticMethodID)
        const jmethodID mid = TypedJNI::GetMethodID(env, cls, "<init>", "("+TypedJNI::GetTypeString<Args...>()+")"+TypedJNI::GetTypeString<void>());
        return [env, cls, mid](Args... args) -> TypedJNIObject {
            return TypedJNIObject(env, cls, env->NewObject(cls, mid, args...));
        };
    }
};

/**
 * Special case of TypedJNIObject for convenient string conversion.
 * 
 * Expects a byte-string of UTF-8 characters.
 * 
 * Copies the string at least once.
 * 
 * Automatically deletes local reference to Java String object when C++ instance goes out of scope. 
 * Use make_persistent() to change this behaviour.
 */
class TypedJNIString {
    private:
    std::shared_ptr<_jstring> jstrptr = nullptr;
    bool persistent = false;
    public:
    TypedJNIString(JNIEnv *env, const std::string & str);
    operator jstring() const;
    jstring make_persistent(bool persistent = true);
};

/**
 * Class for a proxy object referencing a Java class.
 * 
 * Static methods can be accessed as well as the constructors.
 */
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

/**
 * Class for creating a Java VM and Environment.
 * 
 * Wrapper around JNI_CreateJavaVM.
 * 
 * Performs error-checking. Throws an exception on error.
 * 
 * Provides access to Java classes and utility methods.
 * 
 * The VM is destroyed with the TypedJNIEnv object.
 */
class TypedJNIEnv {
    private:
    JavaVM *vm = nullptr;
    public:
    /**
     * The environment is provided by a naked pointer.
     * 
     * This is public so functionality not wrapped by TypedJNI can use direct access.
     */
    JNIEnv *env = nullptr;
    TypedJNIEnv(const TypedJNIEnv&) = delete;
    TypedJNIEnv& operator=(const TypedJNIEnv&) = delete;
    TypedJNIEnv(JavaVMInitArgs vm_args);
    virtual ~TypedJNIEnv();
    
    /**
     * Find a Java class from the Java runtime.
     */
    TypedJNIClass find_class(std::string name);
    
    /**
     * Create a Java String from a std::string.
     */
    TypedJNIString make_jstring(const std::string & str);
};
