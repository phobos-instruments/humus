// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/midi/MidiSource.h"
#include "core/params/RangeEnd.h"
#include "gui/editor/ParamLearners.h"
#include "gui/properties/ModeChips.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class QuickMapDialog : public juce::Component, private juce::Timer {
public:
    static void show(BrickHost& host, const std::string& organism, const std::string& param);
    static void close();
    static bool isOpen();

    QuickMapDialog(BrickHost& host, std::string organism, std::string param);
    ~QuickMapDialog() override;

    void resized() override;
    void paint(juce::Graphics& g) override { g.fillAll(Palette::panel); }

    int slotCountForTest() const { return (int) slots_.size(); }
    std::string slotNameForTest(int slot) const;
    void setModeForTest(RangeMode want);
    std::string slotSourceForTest(int slot) const;
    void learnSlotForTest(int slot);
    std::string slotBehaviourForTest(int slot) const;
    void pickTypeForTest(int slot, ControlType t);
    void okForTest() { commit(); }
    void cancelForTest() { revert(); }

private:
    struct Remembered {
        MidiSource source;
        std::string param;
        double min = 0.0, max = 1.0;
        ControlShape shape;
    };

    struct Slot {
        RangeEnd end = RangeEnd::Whole;
        std::string param;
        juce::Label head, sourceLabel, deviceLabel, numberLabel;
        juce::TextButton learn{"Learn"};
        juce::ComboBox device;
        juce::TextEditor number;
        juce::TextButton clear{"Clear"};
        juce::Label howLabel;
        ModeChips<ControlType> type;
        juce::ComboBox how;
    };

    void buildSlot(Slot& s, RangeEnd end);
    void buildSlots();
    RangeMode mode() const;
    static RangeMode modeOfId(int id);
    static int idOfMode(RangeMode m);
    const MidiMapEntry* entryFor(const std::string& param) const;
    void refresh();
    void armSlot(Slot& s);
    void applySlot(Slot& s);
    void showBehaviour(Slot& s, const ControlShape& shape);
    void pickType(Slot& s, ControlType t);
    void pickHow(Slot& s);
    void fitToSlots();
    void commit();
    void revert();
    void timerCallback() override;
    std::pair<double, double> bounds() const;

    BrickHost& host_;
    std::string organism_, param_;
    juce::Label paramLine_, rangeLine_, modeLabel_;
    juce::ComboBox mode_;
    bool ranged_ = false;
    std::vector<std::unique_ptr<Slot>> slots_;
    std::vector<Remembered> before_;
    std::vector<std::string> devices_;
    juce::TextButton okBtn_{"OK"}, cancelBtn_{"Cancel"};
    bool wasArmed_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickMapDialog)
};

}
