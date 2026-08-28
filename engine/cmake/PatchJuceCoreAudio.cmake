# Fixes a fatal JUCE 8.0.4 race on macOS: with different input and output
# devices, CoreAudio's device-properties-changed listener fires on a
# background dispatch thread and AudioIODeviceCombiner::restartAsync would
# close() the combiner RIGHT THERE - racing the message thread's own
# open/close of the same object (our boot always restarts the device to apply
# the saved buffer size, so the app crashed at every launch: use-after-free
# inside shutdown's device-wrapper loop). Upstream only truly fixed this by
# rewriting the whole CoreAudio layer in JUCE 9.
#
# The patch defers the close to the combiner's existing 100ms timer, which
# fires on the message thread - where every other open/close of the device
# already runs.
#
# Hunk 5 is the one that actually cost the Bluetooth launch crash: reopen()
# overwrites bufferSize with the size it asked for, having already sized the
# temp buffers to the size the device reported, and the IO proc then writes past
# the end of them. AddressSanitizer named it outright.
#
# That deferral then exposed the second fault, which cost a launch crash with
# Bluetooth headphones: the combiner's scratchBuffer is sized to the buffer
# size open() agreed, and nothing bounds the incoming block against it. A
# device renegotiating its block size delivers the bigger one before the
# reopen, and the callback writes past the end - detected much later, and
# somewhere else, as a malloc checksum abort in allocateTempBuffers.
#
# Idempotent (guarded by the HUMUS-PATCH marker). Runs as FetchContent's
# PATCH_COMMAND on fresh checkouts, and can be run by hand against an
# already-populated _deps tree:
#   cmake -DJUCE_SOURCE_DIR=engine/build/_deps/juce-src -P engine/cmake/PatchJuceCoreAudio.cmake

if(NOT DEFINED JUCE_SOURCE_DIR)
  message(FATAL_ERROR "pass -DJUCE_SOURCE_DIR=<juce checkout>")
endif()

set(_f "${JUCE_SOURCE_DIR}/modules/juce_audio_devices/native/juce_CoreAudio_mac.cpp")
if(NOT EXISTS "${_f}")
  message(FATAL_ERROR "not a JUCE tree: ${_f} missing")
endif()

file(READ "${_f}" _src)

# Each hunk is applied only if its own marker is absent, so a tree patched by an
# earlier version of this file still picks up a hunk added later.
macro(hum_apply_hunk marker old new what)
  if(NOT _src MATCHES "${marker}")
    string(FIND "${_src}" "${old}" _at)
    if(_at EQUAL -1)
      message(FATAL_ERROR "${what} anchor not found - JUCE version changed, revisit this patch")
    endif()
    string(REPLACE "${old}" "${new}" _src "${_src}")
    message(STATUS "JUCE CoreAudio: applied ${what}")
  endif()
endmacro()

# 1. restartAsync: record the request; never close() on this (listener) thread.
set(_old_restart "    void restartAsync() override
    {
        {
            const ScopedLock sl (closeLock);

            if (active)
            {
                if (callback != nullptr)
                    previousCallback = callback;

                close();
            }
        }

        startTimer (100);
    }")
set(_new_restart "    void restartAsync() override
    {
        // HUMUS-PATCH: never close() here - this runs on the CoreAudio
        // property-listener thread and races the message thread's own
        // open/close of this combiner (use-after-free in shutdown). The
        // close is deferred to timerCallback on the message thread.
        restartPending = true;
        startTimer (100);
    }")
hum_apply_hunk("HUMUS-PATCH: never close" "${_old_restart}" "${_new_restart}" "restartAsync deferral")

# 2. The combiner's timerCallback (anchored by the members right above it):
#    perform the deferred close, then restart as before.
set(_old_timer "    double sampleRateRequested = 44100;
    int bufferSizeRequested = 512;

    void timerCallback() override
    {
        stopTimer();

        restart (previousCallback);
    }")
set(_new_timer "    double sampleRateRequested = 44100;
    int bufferSizeRequested = 512;
    std::atomic<bool> restartPending { false };   // HUMUS-PATCH (see restartAsync)

    void timerCallback() override
    {
        stopTimer();

        if (restartPending.exchange (false))
        {
            const ScopedLock sl (closeLock);

            if (active)
            {
                if (callback != nullptr)
                    previousCallback = callback;

                close();
            }
        }

        restart (previousCallback);
    }")
hum_apply_hunk("std::atomic<bool> restartPending" "${_old_timer}" "${_new_timer}" "deferred close in timerCallback")

# 3. inputAudioCallback: bound the block against the scratch buffer open() sized.
#    Nothing upstream does, so a device that renegotiates its block size - every
#    Bluetooth headset does, on connect - delivers the bigger block before the
#    combiner has reopened and the callback writes n floats into a buffer holding
#    bufferSize. The overrun surfaces much later as a malloc checksum abort in
#    CoreAudioInternal::allocateTempBuffers, which is why it reads as a JUCE bug
#    somewhere else entirely. Hunk 1's 100ms deferral widens the window from
#    unlikely to certain, so this is the other half of that fix.
set(_old_input "    void inputAudioCallback (const float* const* channels, int numChannels, int n, const AudioIODeviceCallbackContext& context) noexcept
    {
        auto& writePos = inputWrapper.sampleTime;")
set(_new_input "    void inputAudioCallback (const float* const* channels, int numChannels, int n, const AudioIODeviceCallbackContext& context) noexcept
    {
        // HUMUS-PATCH: a block we cannot hold is a dropout, never a write past
        // the end of scratchBuffer. See PatchJuceCoreAudio.cmake hunk 3.
        if (n > scratchBuffer.getNumSamples())
        {
            xrun();
            return;
        }

        auto& writePos = inputWrapper.sampleTime;")
hum_apply_hunk("HUMUS-PATCH: a block we cannot hold" "${_old_input}" "${_new_input}" "input block-size guard")

# 4. The channel count of that same buffer is only jasserted, so a device that
#    gains channels writes through pointers past the end of the array in a
#    release build.
set(_old_chans "                const auto numActiveOutputChannels = outputWrapper.getActiveChannels().countNumberOfSetBits();
                jassert (numActiveOutputChannels <= scratchBuffer.getNumChannels());")
set(_new_chans "                // HUMUS-PATCH: an assertion is not a bound in a release build.
                const auto numActiveOutputChannels = jmin (outputWrapper.getActiveChannels().countNumberOfSetBits(),
                                                           scratchBuffer.getNumChannels());")
hum_apply_hunk("HUMUS-PATCH: an assertion is not a bound" "${_old_chans}" "${_new_chans}" "scratch channel-count bound")

# 5. reopen()'s "bodge": after asking the device for a new rate and block size,
#    JUCE overwrites bufferSize with the size it REQUESTED, because some devices
#    do not report the new one straight away. updateDetailsFromDevice has already
#    sized the temp buffers to the size the device DID report, and nothing
#    resizes them afterwards - so the IO proc's copy loop, which runs bufferSize
#    times, writes past the end of audioBuffer. A device slow to catch up (a
#    Bluetooth headset renegotiating) makes that a certainty. Caught by
#    AddressSanitizer at juce_CoreAudio_mac.cpp:795, writing into the block
#    allocated at :361. Reallocate for the size we are about to claim, under the
#    lock the callback takes, so the two can never disagree.
set(_old_bodge "        updateDetailsFromDevice (ins, outs);
        sampleRate = newSampleRate;
        bufferSize = bufferSizeSamples;")
set(_new_bodge "        updateDetailsFromDevice (ins, outs);
        {
            // HUMUS-PATCH: the bodge below claims a block size the temp buffers
            // were not sized for. See PatchJuceCoreAudio.cmake hunk 5.
            const ScopedLock sl (callbackLock);
            sampleRate = newSampleRate;
            bufferSize = bufferSizeSamples;
            allocateTempBuffers();
        }")
hum_apply_hunk("HUMUS-PATCH: the bodge below" "${_old_bodge}" "${_new_bodge}" "reopen block-size bodge")

file(WRITE "${_f}" "${_src}")
