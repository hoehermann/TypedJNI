#include "typedjni.hpp"

template <>
std::string TypedJNI::GetTypeString<jstring>(){return "Ljava/lang/String;";};
template <>
std::string TypedJNI::GetTypeString<jint>(){return "I";};
template <>
std::string TypedJNI::GetTypeString<jlong>(){return "J";};
template <>
std::string TypedJNI::GetTypeString<void>(){return "V";};

jmethodID TypedJNI::GetStaticMethodID(JNIEnv *env, const jclass cls, const std::string name, const std::string & signature) {
    jmethodID mid = env->GetStaticMethodID(cls, name.c_str(), signature.c_str());
    if (mid == NULL) {
        throw std::runtime_error("Failed to find static method '"+name+"' "+signature+".");
    }
    return mid;
}
jmethodID TypedJNI::GetMethodID(JNIEnv *env, const jclass cls, const std::string name, const std::string & signature) {
    jmethodID mid = env->GetMethodID(cls, name.c_str(), signature.c_str());
    if (mid == NULL) {
        throw std::runtime_error("Failed to find method '"+name+"' "+signature+".");
    }
    return mid;
}


TypedJNIObject::TypedJNIObject(JNIEnv *env, jclass cls, jobject obj) : env(env), cls(cls), obj(obj) {};


TypedJNIClass::TypedJNIClass(JNIEnv *env, jclass cls) : env(env), cls(cls) {
    //assert(cls);
};

TypedJNIEnv::TypedJNIEnv(JavaVMInitArgs vm_args) {
    jint res = JNI_CreateJavaVM(&vm, (void **)&env, &vm_args);
    if (res != JNI_OK) {
        throw std::runtime_error(std::string("Failed to create Java VM (error ")+std::to_string(res)+").");
    }
}
TypedJNIEnv::~TypedJNIEnv() {
    vm->DestroyJavaVM();
}
TypedJNIClass TypedJNIEnv::FindClass(std::string name) {
    jclass cls = env->FindClass(name.c_str());
    if (cls == NULL) {
        throw std::runtime_error("Failed to find class '"+name+"'.");
    }
    return TypedJNIClass(env, cls);
}
