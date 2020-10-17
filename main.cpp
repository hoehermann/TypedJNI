#include "typedjni.hpp"

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
    javacls.GetStaticMethod<void()>("printHelloWorld");
    javacls.GetStaticMethod<void(jlong)>("printLong")(1);
    javacls.GetStaticMethod<void(jlong,jlong)>("print2Long")(1,2);
    {
        const long l = 1;
        std::cout << l << " incremented by Java is " << javacls.GetStaticMethod<jint(jint)>("increment")(l) << std::endl;
    }
    
    TypedJNIObject javaobj = javacls.GetConstructor<>()();
    {
        jint i = javaobj.GetMethod<jint(jint)>("incrementCounterBy")(2);
        std::cout << "After incrementing, counter is " << i << "." << std::endl;
    }
    javaobj.GetMethod<void(void)>("printCounter")();

    return 0;
}
