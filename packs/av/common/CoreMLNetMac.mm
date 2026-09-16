// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
// Apple frameworks FIRST (the ApplicationServices `Pattern` collision -
// see HandsVisionMac.mm). Output MLMultiArrays are STRIDED: a GPU-backed
// array pads its rows, so the copy walks shape x strides - pinned by
// body-check, which goes red on a flat memcpy.
#import <CoreML/CoreML.h>
#import <Foundation/Foundation.h>

#include "common/CoreMLNet.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace hum {

namespace {

bool copyOut(id<MLFeatureProvider> result, NSString* name, std::vector<float>& out) {
    MLFeatureValue* v = [result featureValueForName:name];
    if (v == nil || v.multiArrayValue == nil) return false;
    MLMultiArray* a = v.multiArrayValue;
    const auto count = (size_t) a.count;
    out.resize(count);
    const int rank = (int) a.shape.count;
    long shape[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    long stride[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    if (rank > 8) return false;
    for (int d = 0; d < rank; ++d) {
        shape[d] = a.shape[(NSUInteger) d].longValue;
        stride[d] = a.strides[(NSUInteger) d].longValue;
    }
    const void* base = a.dataPointer;
    bool ok = true;
    auto copyAll = [&](auto readAt) {
        size_t i = 0;
        long idx[8] = {};
        while (i < count) {
            long off = 0;
            for (int d = 0; d < rank; ++d) off += idx[d] * stride[d];
            out[i++] = readAt(off);
            for (int d = rank - 1; d >= 0; --d) {
                if (++idx[d] < shape[d]) break;
                idx[d] = 0;
            }
        }
    };
    if (a.dataType == MLMultiArrayDataTypeFloat32)
        copyAll([&](long off) { return ((const float*) base)[off]; });
    else if (a.dataType == MLMultiArrayDataTypeDouble)
        copyAll([&](long off) { return (float) ((const double*) base)[off]; });
    else if (@available(macOS 12.0, *)) {
        if (a.dataType == MLMultiArrayDataTypeFloat16)
            copyAll([&](long off) { return (float) ((const __fp16*) base)[off]; });
        else
            ok = false;
    } else
        ok = false;
    return ok;
}

class MacCoreMLNet final : public CoreMLNet {
public:
    explicit MacCoreMLNet(MLModel* model) : model_(model) {}

    ~MacCoreMLNet() override {
        if (model_ != nil) [model_ release];
    }

    bool run(const float* in, int count, int side,
             std::vector<std::vector<float>>& outs, int numOuts) override {
        if (count != side * side * 3) return false;
        NSError* err = nil;
        MLMultiArray* input =
            [[MLMultiArray alloc] initWithShape:@[ @1, @(side), @(side), @3 ]
                                       dataType:MLMultiArrayDataTypeFloat32
                                          error:&err];
        if (input == nil) return false;
        std::memcpy(input.dataPointer, in, (size_t) count * sizeof(float));
        MLDictionaryFeatureProvider* feed = [[MLDictionaryFeatureProvider alloc]
            initWithDictionary:@{
                @"in0" : [MLFeatureValue featureValueWithMultiArray:input]
            }
                         error:&err];
        bool ok = false;
        if (feed != nil) {
            id<MLFeatureProvider> result = [model_ predictionFromFeatures:feed
                                                                    error:&err];
            ok = result != nil;
            outs.resize((size_t) numOuts);
            for (int o = 0; ok && o < numOuts; ++o) {
                NSString* name = [NSString stringWithFormat:@"out%d", o];
                ok = copyOut(result, name, outs[(size_t) o]);
            }
            [feed release];
        }
        [input release];
        return ok;
    }

private:
    MLModel* model_ = nil;
};

}   // namespace

std::unique_ptr<CoreMLNet> CoreMLNet::load(const juce::File& package) {
    const bool talk = getenv("HUMUS_BODY_PROFILE") != nullptr;
    if (getenv("HUMUS_NO_COREML") != nullptr) return nullptr;
    if (!package.exists()) {
        if (talk)
            std::fprintf(stderr, "coreml: missing %s\n",
                         package.getFullPathName().toRawUTF8());
        return nullptr;
    }
    NSString* path =
        [NSString stringWithUTF8String:package.getFullPathName().toRawUTF8()];
    if (path == nil) return nullptr;
    NSError* err = nil;
    NSURL* compiled = [MLModel compileModelAtURL:[NSURL fileURLWithPath:path]
                                           error:&err];
    if (compiled == nil) {
        if (talk)
            std::fprintf(stderr, "coreml: compile failed: %s\n",
                         err.localizedDescription.UTF8String);
        return nullptr;
    }
    MLModelConfiguration* cfg = [[MLModelConfiguration alloc] init];
    cfg.computeUnits = MLComputeUnitsAll;
    MLModel* m = [MLModel modelWithContentsOfURL:compiled configuration:cfg error:&err];
    [cfg release];
    if (m == nil) {
        if (talk)
            std::fprintf(stderr, "coreml: load failed: %s\n",
                         err.localizedDescription.UTF8String);
        return nullptr;
    }
    return std::make_unique<MacCoreMLNet>([m retain]);
}

}   // namespace hum
