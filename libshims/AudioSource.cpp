#include <system/audio.h>
#include <utils/String16.h>

// Shim for frameworks/av/media/libstagefright/AudioSource

namespace android {

    // This is the constructor prototype for android::AudioSource in Ten which has an extra parameter
    extern "C" void _ZN7android11AudioSourceC1E14audio_source_tRKNS_8String16Ejjjjii28audio_microphone_direction_tf(audio_source_t inputSource, const String16 &opPackageName,
            uint32_t sampleRate, uint32_t channels, uint32_t outSampleRate, uid_t uid, pid_t pid,
            audio_port_handle_t selectedDeviceId, int32_t selectedMicDirection, float selectedMicFieldDimension);

    // Define the missing constructor symbol
    extern "C" void _ZN7android11AudioSourceC1E14audio_source_tRKNS_8String16Ejjjji(audio_source_t inputSource, const String16 &opPackageName,
        uint32_t sampleRate, uint32_t channelCount, uint32_t outSampleRate, uid_t uid, pid_t pid)
    {
        // Invoke the Ten android::AudioSource constructor with the extra parameter
        _ZN7android11AudioSourceC1E14audio_source_tRKNS_8String16Ejjjjii28audio_microphone_direction_tf(inputSource, opPackageName, sampleRate, channelCount, outSampleRate, uid, pid, 0, 0, 0.0f);
    }

}
