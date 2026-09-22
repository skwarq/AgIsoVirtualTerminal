#include <jni.h>

extern "C" JNIEXPORT jstring JNICALL
Java_com_openagriculture_agisovirtualterminal_MainActivity_nativeBuildStatus(JNIEnv *env, jclass)
{
	return env->NewStringUTF("Native C++/NDK loaded");
}
