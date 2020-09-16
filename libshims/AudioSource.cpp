#include <system/audio.h>
#include <utils/String16.h>

// Shim for frameworks/av/media/libstagefright/AudioSource

namespace android {

    // This is the constructor prototype for android::AudioSource in Eleven which has extra parameters
    extern "C" void _ZN7android11AudioSourceC1EPK18audio_attributes_tRKNS_8String16Ejjjjii28audio_microphone_direction_tf(
        const audio_attributes_t *attr, const String16 &opPackageName,
        uint32_t sampleRate, uint32_t channelCount, uint32_t outSampleRate,
        uid_t uid, pid_t pid, audio_port_handle_t selectedDeviceId,
        audio_microphone_direction_t selectedMicDirection,
        float selectedMicFieldDimension);

    // Define the missing constructor symbol
    extern "C" void _ZN7android11AudioSourceC1E14audio_source_tRKNS_8String16Ejjjji(audio_source_t inputSource, const String16 &opPackageName,
        uint32_t sampleRate, uint32_t channelCount, uint32_t outSampleRate, uid_t uid, pid_t pid)
    {
        // Invoke the Eleven android::AudioSource constructor with the extra parameters
        audio_attributes_t attr = AUDIO_ATTRIBUTES_INITIALIZER;
        attr.source = inputSource;
        _ZN7android11AudioSourceC1EPK18audio_attributes_tRKNS_8String16Ejjjjii28audio_microphone_direction_tf(&attr, opPackageName, sampleRate, channelCount, outSampleRate, uid, pid, 0, MIC_DIRECTION_UNSPECIFIED, 0.0f);
    }

}
