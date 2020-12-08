#include "typedjni.hpp"

TypedJNIError::TypedJNIError(const std::string& what_arg) : std::runtime_error(what_arg) {};

template <> std::string TypedJNI::GetTypeString<void>(){return "V";};
template <> std::string TypedJNI::GetTypeString<jboolean>(){return "Z";};
template <> std::string TypedJNI::GetTypeString<jint>(){return "I";};
template <> std::string TypedJNI::GetTypeString<jlong>(){return "J";};
template <> std::string TypedJNI::GetTypeString<jfloat>(){return "F";};
template <> std::string TypedJNI::GetTypeString<jstring>(){return "Ljava/lang/String;";};

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
    jstring jstr = env->NewStringUTF(str.c_str());
    if (jstr == NULL) {
        throw TypedJNIError("NewStringUTF failed for string '"+str+"'.");
    }
    jstrptr = std::shared_ptr<_jstring>(jstr, [env](jstring s){env->DeleteLocalRef(s);});
}

TypedJNIString::operator jstring() const {
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
