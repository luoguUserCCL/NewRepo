#include <jni.h>
#include <calc/calc_engine.h>
#include <calc/number_format.h>
#include <string>
#include <cstring>

// Helper: convert Java string to std::string
static std::string jstringToString(JNIEnv* env, jstring jstr) {
    if (!jstr) return "";
    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return result;
}

// Helper: convert std::string to Java string
static jstring stringToJstring(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}

// Helper: create CalcResult Java object
static jobject createCalcResult(JNIEnv* env, bool isError, const std::string& value, const std::string& error) {
    jclass resultClass = env->FindClass("com/calc/CalcResult");
    if (!resultClass) return nullptr;

    jmethodID method;
    if (isError) {
        method = env->GetStaticMethodID(resultClass, "error",
            "(Ljava/lang/String;)Lcom/calc/CalcResult;");
        jstring errMsg = stringToJstring(env, error);
        return env->CallStaticObjectMethod(resultClass, method, errMsg);
    } else {
        method = env->GetStaticMethodID(resultClass, "success",
            "(Ljava/lang/String;)Lcom/calc/CalcResult;");
        jstring valStr = stringToJstring(env, value);
        return env->CallStaticObjectMethod(resultClass, method, valStr);
    }
}

// ==================== JNI Method Implementations ====================

extern "C" {

/**
 * Create a native CalcEngine instance.
 */
JNIEXPORT jlong JNICALL Java_com_calc_CalcEngine_nativeCreate(JNIEnv* env, jobject obj) {
    calc::CalcEngine* engine = new calc::CalcEngine();
    return reinterpret_cast<jlong>(engine);
}

/**
 * Destroy a native CalcEngine instance.
 */
JNIEXPORT void JNICALL Java_com_calc_CalcEngine_nativeDestroy(JNIEnv* env, jobject obj, jlong handle) {
    auto* engine = reinterpret_cast<calc::CalcEngine*>(handle);
    delete engine;
}

/**
 * Evaluate an expression.
 */
JNIEXPORT jobject JNICALL Java_com_calc_CalcEngine_nativeEvaluate(
        JNIEnv* env, jobject obj, jlong handle, jstring input) {
    auto* engine = reinterpret_cast<calc::CalcEngine*>(handle);
    if (!engine) {
        return createCalcResult(env, true, "", "Engine not initialized");
    }

    std::string inputStr = jstringToString(env, input);
    auto result = engine->evaluate(inputStr);

    if (!engine->lastError().empty()) {
        return createCalcResult(env, true, "", engine->lastError());
    }

    std::string resultStr = engine->formatResult(result);
    return createCalcResult(env, false, resultStr, "");
}

/**
 * Set the number format mode.
 */
JNIEXPORT void JNICALL Java_com_calc_CalcEngine_nativeSetFormatMode(
        JNIEnv* env, jobject obj, jlong handle, jstring mode) {
    auto* engine = reinterpret_cast<calc::CalcEngine*>(handle);
    if (!engine) return;

    std::string modeStr = jstringToString(env, mode);
    auto& config = engine->formatConfig();

    if (modeStr == "scientific") {
        config.mode = calc::FormatMode::SCIENTIFIC;
    } else if (modeStr == "fixed") {
        config.mode = calc::FormatMode::FIXED_POINT;
    } else if (modeStr == "significant") {
        config.mode = calc::FormatMode::SIGNIFICANT_DIGITS;
    } else {
        config.mode = calc::FormatMode::FLOATING_POINT;
    }
}

/**
 * Set the number base.
 */
JNIEXPORT void JNICALL Java_com_calc_CalcEngine_nativeSetBase(
        JNIEnv* env, jobject obj, jlong handle, jint base) {
    auto* engine = reinterpret_cast<calc::CalcEngine*>(handle);
    if (!engine) return;

    int b = static_cast<int>(base);
    if (b == 2 || b == 8 || b == 10 || b == 16) {
        engine->formatConfig().base = b;
    }
}

/**
 * Set the precision.
 */
JNIEXPORT void JNICALL Java_com_calc_CalcEngine_nativeSetPrecision(
        JNIEnv* env, jobject obj, jlong handle, jint precision) {
    auto* engine = reinterpret_cast<calc::CalcEngine*>(handle);
    if (!engine) return;

    int p = static_cast<int>(precision);
    if (p >= 32 && p <= 4096) {
        calc::BigDecimal::setDefaultPrecision(p);
    }
}

/**
 * Reset the engine.
 */
JNIEXPORT void JNICALL Java_com_calc_CalcEngine_nativeReset(
        JNIEnv* env, jobject obj, jlong handle) {
    auto* engine = reinterpret_cast<calc::CalcEngine*>(handle);
    if (engine) engine->reset();
}

} // extern "C"
