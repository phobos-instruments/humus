// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

namespace hum {

class MasterTap {
public:
    virtual ~MasterTap() = default;
    virtual int channels() const = 0;
    virtual const float* channelData(int channel) const = 0;
    virtual int lastBlockLength() const = 0;
    virtual int firstChannel() const { return 0; }
};

class HardwareOut {
public:
    virtual ~HardwareOut() = default;
    virtual int channel() const = 0;
    virtual const float* channelData() const = 0;
    virtual int blockLength() const = 0;
};

class HardwareIn {
public:
    virtual ~HardwareIn() = default;
    virtual int channel() const = 0;
    virtual int channelCount() const { return 1; }
};

class LevelMeterSource {
public:
    virtual ~LevelMeterSource() = default;
    virtual int meterChannels() const = 0;
    virtual float meterLevel(int channel) const = 0;
};

class StereoFieldSource {
public:
    static constexpr int kFieldPairs = 256;
    virtual ~StereoFieldSource() = default;
    virtual int fieldRead(float* lr, int maxPairs) const = 0;
    virtual unsigned fieldStamp() const = 0;
    virtual float fieldCorrelation() const = 0;
};

class VuSource {
public:
    virtual ~VuSource() = default;
    virtual int vuChannels() const = 0;
    virtual float vuRms(int channel) const = 0;
    virtual float vuPeak(int channel) const = 0;
};

class ScopeSource {
public:
    static constexpr int kScopeSamples = 2048;
    virtual ~ScopeSource() = default;
    virtual int scopeRead(float* dst, int maxSamples) const = 0;
    virtual unsigned scopeStamp() const = 0;
    virtual double scopeRate() const = 0;
};

class PitchDetectSource {
public:
    virtual ~PitchDetectSource() = default;
    virtual float detectedHz() const = 0;
    virtual float detectClarity() const = 0;
    virtual float detectLevel() const = 0;
    virtual int detectedNote() const = 0;
};

class VisualSource {
public:
    virtual ~VisualSource() = default;
    virtual void setVisualTapEnabled(bool on) = 0;
    virtual int readVisualTap(float* dest, int maxSamples) const = 0;
};

class GainReductionSource {
public:
    virtual ~GainReductionSource() = default;
    virtual float grDb() const = 0;
};

class LatencyReporting {
public:
    virtual ~LatencyReporting() = default;
    virtual int latencySamples() const = 0;
    virtual bool takeLatencyChange() { return false; }
};

}
