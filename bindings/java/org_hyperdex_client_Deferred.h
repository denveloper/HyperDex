/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_hyperdex_client_Deferred */

#ifndef _Included_org_hyperdex_client_Deferred
#define _Included_org_hyperdex_client_Deferred
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_hyperdex_client_Deferred
 * Method:    create
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_hyperdex_client_Deferred_create
  (JNIEnv *, jobject);

/*
 * Class:     org_hyperdex_client_Deferred
 * Method:    destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_hyperdex_client_Deferred_destroy
  (JNIEnv *, jobject);

/*
 * Class:     org_hyperdex_client_Deferred
 * Method:    waitForIt
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_hyperdex_client_Deferred_waitForIt
  (JNIEnv *, jobject);

/*
 * Class:     org_hyperdex_client_Deferred
 * Method:    callback
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_hyperdex_client_Deferred_callback
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
