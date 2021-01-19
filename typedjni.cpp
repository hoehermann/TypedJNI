#include <locale> // for std::wstring_convert needed in TypedJNIString
#include <codecvt> // for std::codecvt needed in TypedJNIString
#include <limits> // for range checking needed in TypedJNIString

#include "typedjni.hpp"

TypedJNIError::TypedJNIError(const std::string& what_arg) : std::runtime_error(what_arg) {};

template <> std::string TypedJNI::GetTypeString<void>(){return "V";};
template <> std::string TypedJNI::GetTypeString<jboolean>(){return "Z";};
template <> std::string TypedJNI::GetTypeString<jint>(){return "I";};
template <> std::string TypedJNI::GetTypeString<jlong>(){return "J";};
template <> std::string TypedJNI::GetTypeString<jstring>(){return "Ljava/lang/String;";};
template <> std::string TypedJNI::GetTypeString<jbyteArray>(){return "[B";};

jmethodID TypedJNI::GetStaticMethodID(JNIEnv *env, const jclass cls, const std::string name, const std::string & signature) {
    jmethodID mid = env->GetStaticMethodID(cls, name.c_str(), signature.c_str());
    if (mid == NULL) {
        throw TypedJNIError("Failed to find static method '"+name+"' "+signature+".");
    }
    return mid;
}

jmethodID TypedJNI::GetMethodID(JNIEnv *env, const jclass cls, const std::string name, const std::string & signature) {
    jmethodID mid = env->GetMethodID(cls, name.c_str(), signature.c_str());
    if (mid == NULL) {
        throw TypedJNIError("Failed to find method '"+name+"' "+signature+".");
    }
    return mid;
}

TypedJNIObject::TypedJNIObject(JNIEnv *env, jclass cls, jobject obj) : 
    env(env), 
    cls(cls),
    obj(std::shared_ptr<_jobject>(obj, [env](jobject o){env->DeleteLocalRef(o);})) {};

TypedJNIString::TypedJNIString(JNIEnv *env, const std::string & str) {
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert; 
    std::u16string u16string = convert.from_bytes(str);
    if (u16string.size() > static_cast<size_t>(std::numeric_limits<jint>::max())) {
        throw TypedJNIError("Converted string length "+std::to_string(u16string.size())+" exceeds maximum "+std::to_string(std::numeric_limits<jint>::max())+".");
    }
    static_assert(sizeof(jchar) == sizeof(char16_t), "jchar is not as long as char16_t");
    jstring jstr = env->NewString(reinterpret_cast<const jchar*>(u16string.c_str()), static_cast<jint>(u16string.size()));
    if (jstr == NULL) {
        throw TypedJNIError("NewString failed for UTF-8 string '"+str+"'.");
    }
    jstrptr = std::shared_ptr<_jstring>(jstr, [this, env](jstring s){ 
        if (!persistent) { 
            env->DeleteLocalRef(s);
        }
    });
}

TypedJNIString::operator jstring() const {
    return jstrptr.get();
}

jstring TypedJNIString::make_persistent(bool persistent) {
    this->persistent = persistent;
    return jstrptr.get();
}

TypedJNIClass::TypedJNIClass(JNIEnv *env, jclass cls) : env(env), cls(cls) {
    if (!cls) {
        throw TypedJNIError("Tried to create TypedJNIClass from nullptr.");
    }
};

TypedJNIEnv::TypedJNIEnv(JavaVMInitArgs vm_args) {
    jint res = JNI_CreateJavaVM(&vm, (void **)&env, &vm_args);
    if (res != JNI_OK) {
        throw TypedJNIError(std::string("Failed to create Java VM (error ")+std::to_string(res)+").");
    }
}

TypedJNIEnv::~TypedJNIEnv() {
    env = nullptr;
    vm->DestroyJavaVM();
    vm = nullptr;
}

TypedJNIClass TypedJNIEnv::find_class(std::string name) {
    jclass cls = env->FindClass(name.c_str());
    if (cls == NULL) {
        throw TypedJNIError("Failed to find class '"+name+"'.");
    }
    return TypedJNIClass(env, cls);
}

TypedJNIString TypedJNIEnv::make_jstring(const std::string & str) {
    return TypedJNIString(env, str);
}
