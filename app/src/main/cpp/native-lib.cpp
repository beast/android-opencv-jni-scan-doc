#include <jni.h>
#include <string>

extern "C"
jstring
Java_my_fin_scanner_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
