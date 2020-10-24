# TypedJNI

This module wraps the variardic JNI functions using C++ templates for offering type-safe and compact alternative.

Instead of the lengthy and error-prone

    jclass jcls = (*env)->FindClass(env, "SomeClass");
    if (jcls == NULL) {
        return; // handle error
    }
    jmethodID constructor = (*env)->GetMethodID(env, jcls, "<init>", "(JLjava/lang/String;)V");
    if (constructor == NULL) {
        return; // handle error
    }
    jstring jsomestring = (*env)->NewStringUTF(env, somestring);
    if (jsomestring == NULL) {
        return; // handle error
    }
    jlong jsomelong = somelong; // explicitly convert
    jobject jobj = (*env)->NewObject(env, jcls, constructor, jsomelong, jsomestring);
    if (jobj == NULL) {
        return; // handle error
    }
    jmethodID method = (*env)->GetMethodID(env, jcls, "someMethod", "()V");
    if (method == NULL) {
        return; // handle error
    }
    (*env)->CallVoidMethod(env, jobj, method);
    (*env)->DeleteLocalRef(jsomestring); // cleanup
    (*env)->DeleteLocalRef(jobj);
    

you can now write

    try {
        TypedJNIClass psclass = tenv.find_class("SomeClass").
            GetConstructor<jlong,jstring>()(
                somelong,  // implicit conversion where possible
                *tenv.make_jstring(somestring) // explicit conversion with automated clean-up
            ).
            GetMethod<void()>("someMethod")();
    } catch (std::exception & e) {
        // handle error
    }
    
.
