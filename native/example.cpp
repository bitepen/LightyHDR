#include <jni.h>
#include <android/log.h>
#include <string.h>
#include <dlfcn.h>
#include "zygisk.hpp"
#include "dobby.h"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "LightyZygisk", __VA_ARGS__)

// 1. 官方函数素描
typedef void (*setDataSpace_t)(void* transaction, void* surface_control, int32_t data_space);
setDataSpace_t orig_ASurfaceTransaction_setDataSpace = nullptr;

// 2. 咱们的拦截函数
void my_ASurfaceTransaction_setDataSpace(void* transaction, void* surface_control, int32_t data_space) {
    if (data_space != 142671872 && data_space != 0) {
        LOGE("🚨 叮当底层拦截：抓到 Threads 开启 HDR！强行镇压为 SRGB！💥");
        data_space = 142671872; 
    }
    if (orig_ASurfaceTransaction_setDataSpace != nullptr) {
        orig_ASurfaceTransaction_setDataSpace(transaction, surface_control, data_space);
    }
}

class LightyNoHDRModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        const char *process = env->GetStringUTFChars(args->nice_name, nullptr);
        if (process != nullptr && strcmp(process, "com.instagram.barcelona") == 0) {
            is_threads = true;
            LOGE("✨ 光光的木马已成功潜入 Threads 的大门！");
            api->setOption(zygisk::Option::FORCE_DENYLIST_UNMOUNT); 
        }
        env->ReleaseStringUTFChars(args->nice_name, process);
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
        if (is_threads) {
            void* libandroid = dlopen("libandroid.so", RTLD_NOW);
            if (libandroid != nullptr) {
                void* target_func = dlsym(libandroid, "ASurfaceTransaction_setDataSpace");
                if (target_func != nullptr) {
                    DobbyHook(target_func, (void*)my_ASurfaceTransaction_setDataSpace, (void**)&orig_ASurfaceTransaction_setDataSpace);
                    LOGE("✅ 核心拦截成功！底层 HDR 物理封印完毕！");
                }
                dlclose(libandroid);
            }
        }
    }

private:
    zygisk::Api *api;
    JNIEnv *env;
    bool is_threads = false;
};

REGISTER_ZYGISK_MODULE(LightyNoHDRModule)