#include <chrono>
#include <thread>
#include <iostream>

#include "typedjni.hpp"

int main(int argc, char **argv)
{
    JavaVMInitArgs  vm_args;
    JavaVMOption jvmo;
    std::vector<JavaVMOption> options;
    //jvmo.optionString = const_cast<char*>("-verbose:gc"); options.push_back(jvmo);
    //jvmo.optionString = const_cast<char*>("-XX:+PrintGCTimeStamps"); options.push_back(jvmo);
    //jvmo.optionString = const_cast<char*>("-XX:+PrintGCDetails"); options.push_back(jvmo);
    vm_args.options = options.data();
    vm_args.nOptions = options.size();
    vm_args.version  = JNI_VERSION_1_8;
    TypedJNIEnv tenv(vm_args);
    TypedJNIClass javacls = tenv.find_class("TypedJNITest");
    javacls.GetStaticMethod<void()>("printHelloWorld");
    javacls.GetStaticMethod<void(jlong)>("printLong")(1);
    javacls.GetStaticMethod<void(jlong,jlong)>("print2Long")(1,2);
    {
        const long l = 1;
        std::cout << l << " incremented by Java is " << javacls.GetStaticMethod<jint(jint)>("increment")(l) << std::endl;
    }
    
    std::shared_ptr<TypedJNIObject> javaobj = javacls.GetConstructor<jstring>()(tenv.make_jstring("5"));
    {
        jint i = javaobj->GetMethod<jint(jint)>("incrementCounterBy")(2);
        std::cout << "After incrementing, counter is " << i << "." << std::endl;
    }
    javaobj->GetMethod<void(void)>("printCounter")();
    
    TypedJNIString s = tenv.make_jstring(std::string("Some words"));
    javaobj->GetMethod<void(jstring, jint)>("printInBackground")(s, 10);
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}
