# The JUCE 8.0.4 CoreAudio combiner race and its cousins - the full account
# is docs/dev/build.md, "JUCE, and the three patches". Idempotent per hunk;
# by hand: cmake -DJUCE_SOURCE_DIR=engine/build/_deps/juce-src -P <this file>

if(NOT DEFINED JUCE_SOURCE_DIR)
  message(FATAL_ERROR "pass -DJUCE_SOURCE_DIR=<juce checkout>")
endif()

set(_f "${JUCE_SOURCE_DIR}/modules/juce_audio_devices/native/juce_CoreAudio_mac.cpp")
if(NOT EXISTS "${_f}")
  message(FATAL_ERROR "not a JUCE tree: ${_f} missing")
endif()

file(READ "${_f}" _src)

# Per-hunk markers: an earlier-patched tree still picks up hunks added later.
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

set(_old_input "    void inputAudioCallback (const float* const* channels, int numChannels, int n, const AudioIODeviceCallbackContext& context) noexcept
    {
        auto& writePos = inputWrapper.sampleTime;")
set(_new_input "    void inputAudioCallback (const float* const* channels, int numChannels, int n, const AudioIODeviceCallbackContext& context) noexcept
    {
        // HUMUS-PATCH: a block we cannot hold is a dropout, never a write past
        // the end of scratchBuffer. See docs/dev/build.md.
        if (n > scratchBuffer.getNumSamples())
        {
            xrun();
            return;
        }

        auto& writePos = inputWrapper.sampleTime;")
hum_apply_hunk("HUMUS-PATCH: a block we cannot hold" "${_old_input}" "${_new_input}" "input block-size guard")

set(_old_chans "                const auto numActiveOutputChannels = outputWrapper.getActiveChannels().countNumberOfSetBits();
                jassert (numActiveOutputChannels <= scratchBuffer.getNumChannels());")
set(_new_chans "                // HUMUS-PATCH: an assertion is not a bound in a release build.
                const auto numActiveOutputChannels = jmin (outputWrapper.getActiveChannels().countNumberOfSetBits(),
                                                           scratchBuffer.getNumChannels());")
hum_apply_hunk("HUMUS-PATCH: an assertion is not a bound" "${_old_chans}" "${_new_chans}" "scratch channel-count bound")

set(_old_bodge "        updateDetailsFromDevice (ins, outs);
        sampleRate = newSampleRate;
        bufferSize = bufferSizeSamples;")
set(_new_bodge "        updateDetailsFromDevice (ins, outs);
        {
            // HUMUS-PATCH: the bodge below claims a block size the temp buffers
            // were not sized for. See docs/dev/build.md.
            const ScopedLock sl (callbackLock);
            sampleRate = newSampleRate;
            bufferSize = bufferSizeSamples;
            allocateTempBuffers();
        }")
hum_apply_hunk("HUMUS-PATCH: the bodge below" "${_old_bodge}" "${_new_bodge}" "reopen block-size bodge")

file(WRITE "${_f}" "${_src}")
