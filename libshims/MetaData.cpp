#include <utils/String16.h>

// Shim for frameworks/av/media/libstagefright/foundation/MetaData.cpp
namespace android {

    // The android::MetaData class doesn't exist in Pie. Define the missing symbol returning false.
    extern "C" bool _ZN7android8MetaData9findInt64EjPx(int64_t *value __unused) { return false; }

}
