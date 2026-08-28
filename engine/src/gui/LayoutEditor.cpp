#include "gui/LayoutEditor.h"

#include <cmath>
#include <cstdlib>

#include "core/PackRegistry.h"
#include "gui/Mappable.h"

namespace hum {

namespace {
class WaveGlyphButton : public juce::Button {
public:
    WaveGlyphButton(const juce::String& name, WaveGlyph g, juce::Colour lit)
        : juce::Button(name), glyph_(g), lit_(lit) {}

    juce::String glyphName() const { return waveGlyphDescription(glyph_); }

    void paintButton(juce::Graphics& g, bool over, bool) override {
        const auto r = getLocalBounds().toFloat();
        g.setColour(getToggleState() ? lit_
                    : over            ? Palette::panelLight.brighter(0.15f)
                                      : Palette::panelLight);
        g.fillRect(r);
        g.setColour(Palette::border);
        g.drawRect(r, 1.0f);

        drawWaveGlyph(g, glyph_, r,
                      getToggleState() ? Palette::background : Palette::textDim);
    }

private:
    WaveGlyph glyph_;
    juce::Colour lit_;
};

class ListArrow : public juce::Button {
public:
    explicit ListArrow(bool forward) : juce::Button({}), fwd_(forward) {
        setRepeatSpeed(400, 140);
    }
    void paintButton(juce::Graphics& g, bool over, bool down) override {
        const auto r = getLocalBounds().toFloat();
        const float w = 6.0f, h = 8.0f;
        const float x = r.getCentreX() - w * 0.5f, y = r.getCentreY() - h * 0.5f;
        juce::Path p;
        if (fwd_) p.addTriangle(x, y, x, y + h, x + w, y + h * 0.5f);
        else      p.addTriangle(x + w, y, x + w, y + h, x, y + h * 0.5f);
        g.setColour(!isEnabled() ? Palette::border.brighter(0.08f)
                    : (down || over) ? Palette::text : Palette::textDim);
        g.fillPath(p);
    }

private:
    bool fwd_;
};
}

void LayoutEditor::build() {
    const int famProp = (int) familyOf(className());
    for (const auto& s : spec_.controls) {
        Control c;
        c.param = s.param;
        c.dimWhen = s.extraOr("dim-when");
        c.clearWhen = s.extraOr("clear-when");
        const int childrenBefore = getNumChildComponents();
        const auto tip = s.extraOr("tooltip");

        if (s.type == LayoutSpec::ControlType::Label) {
            auto lbl = std::make_unique<juce::Label>();
            lbl->setText(juce::String::fromUTF8(s.label.c_str()), juce::dontSendNotification);
            lbl->setJustificationType(juce::Justification::centredLeft);
            lbl->setFont(juce::FontOptions(11.0f));
            lbl->setColour(juce::Label::textColourId, Palette::text);
            addAndMakeVisible(*lbl);
            c.label = std::move(lbl);
            if (!tip.empty())
            for (int k = childrenBefore; k < getNumChildComponents(); ++k)
                if (auto* t = dynamic_cast<juce::SettableTooltipClient*>(getChildComponent(k)))
                    t->setTooltip(juce::String::fromUTF8(tip.c_str()));
        controls_.push_back(std::move(c));
            continue;
        }

        using CT = LayoutSpec::ControlType;
        const bool vertical = s.type == CT::Knob || s.type == CT::VSlider
                            || s.type == CT::RangeVSlider || s.type == CT::RotarySwitch;
        const bool ownsLabel = s.type == CT::Toggle || s.type == CT::MiniToggle
                             || s.type == CT::FaderBank || s.type == CT::Waveform
                             || s.type == CT::MidiKeyboard || s.type == CT::Deck
                             || s.type == CT::DeckPitch || s.type == CT::DeckControls
                             || s.type == CT::Momentary || s.type == CT::TapTempo
                             || s.type == CT::LooperTracks
                             || s.type == CT::StepGrid || s.type == CT::PatternGrid
                             || s.type == CT::PianoRoll || s.type == CT::SoundMap
                             || s.type == CT::MidiLog || s.type == CT::OscLog || s.type == CT::LevelBars
                             || s.type == CT::ThresholdMeter
                             || s.type == CT::PitchReadout
                             || s.type == CT::Camera
                             || s.type == CT::GainShapeCurve
                             || s.type == CT::PictureField
                             || s.type == CT::SequenceGrid
                             || s.type == CT::HandGestures
                             || s.type == CT::Sigil
                             || s.type == CT::DnaBases
                             || s.type == CT::DnaStrand
                             || s.type == CT::WaveDraw
                             || s.type == CT::VuMeter
                             || s.type == CT::FieldScope
                             || s.type == CT::SpectrumScope
                             || s.type == CT::LfoScope
                             || s.type == CT::Formula
                             || s.type == CT::TextField
                             || s.type == CT::GainReduction;
        if (!s.label.empty() && !ownsLabel) {
            auto lbl = std::make_unique<juce::Label>();
            lbl->setText(juce::String::fromUTF8(s.label.c_str()), juce::dontSendNotification);
            lbl->setJustificationType(vertical ? juce::Justification::centred
                                               : juce::Justification::centredRight);
            lbl->setFont(juce::FontOptions(10.0f));
            lbl->setColour(juce::Label::textColourId, Palette::textDim);
            lbl->setMinimumHorizontalScale(0.6f);
            addAndMakeVisible(*lbl);
            c.label = std::move(lbl);
        }

        if (buildBrick(s, c)) {
            if (!tip.empty())
            for (int k = childrenBefore; k < getNumChildComponents(); ++k)
                if (auto* t = dynamic_cast<juce::SettableTooltipClient*>(getChildComponent(k)))
                    t->setTooltip(juce::String::fromUTF8(tip.c_str()));
        controls_.push_back(std::move(c));
            continue;
        }

        double lo = 0.0, hi = 1.0;
        bool paramIsInt = false;
        Unit paramUnit = Unit::None;
        auto schema = schemaFor(className());
        for (auto& d : schema) {
            if (d.name == s.param) {
                lo = d.min; hi = d.max; paramIsInt = d.isInt;
                paramUnit = unitResolve(d.name, d.unit, d.min, d.max);
                break;
            }
        }

        const std::string pn = s.param;
        const std::string cn = name_;

        switch (s.type) {
            case LayoutSpec::ControlType::Knob:
            case LayoutSpec::ControlType::VSlider:
            case LayoutSpec::ControlType::HSlider: {
                const bool fader = s.type != LayoutSpec::ControlType::Knob;
                auto style = s.type == LayoutSpec::ControlType::Knob
                                 ? juce::Slider::RotaryVerticalDrag
                                 : (s.type == LayoutSpec::ControlType::VSlider
                                        ? juce::Slider::LinearVertical
                                        : juce::Slider::LinearHorizontal);
                auto textPos = fader ? juce::Slider::NoTextBox : juce::Slider::TextBoxBelow;
                auto slider = std::make_unique<ParamSlider>(style, textPos);
                slider->getProperties().set("family", famProp);
                slider->paramLabel = juce::String::fromUTF8((s.label.empty() ? s.param : s.label).c_str());
                slider->setRange(lo, hi, 0.0);
                if (s.logarithmic) slider->configureScaling();
                slider->setNumDecimalPlacesToDisplay(paramIsInt ? 0 : s.decimalPlaces);
                slider->setUnit(paramUnit);
                if (const auto m = s.extraOr("meter"); !m.empty()) {
                    c.meterChannel = std::atoi(m.c_str());
                    meters_ = true;
                }
                if (fader) slider->setPopupDisplayEnabled(true, true, this);
                slider->setValue(modelValue(pn), juce::dontSendNotification);
                enableDoubleClickReset(*slider, host_, cn, pn);
                auto* sp = slider.get();
                const auto sv = s.extraOr("show-value");
                c.showValue = (sv == "1" || sv == "true") && c.label != nullptr;
                auto* lb = c.showValue ? c.label.get() : nullptr;
                if (lb) lb->setText(sp->getTextFromValue(sp->getValue()),
                                    juce::dontSendNotification);
                slider->onValueChange = [this, sp, cn, pn, lb] {
                    host_.editParam(cn, pn, sp->getValue());
                    if (lb) lb->setText(sp->getTextFromValue(sp->getValue()),
                                        juce::dontSendNotification);
                };
                slider->onDragStart = [this, cn, pn] { host_.beginParamDrag(cn, pn); };
                slider->onDragEnd = [this] { host_.endParamDrag(); };
                sp->setParamId(cn, pn);
                sp->onPopup = [this, cn, pn](juce::Point<int> pos) {
                    showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
                };
                addAndMakeVisible(*slider);
                c.slider = std::move(slider);
                break;
            }
            case LayoutSpec::ControlType::RotarySwitch: {
                auto slider = std::make_unique<ParamSlider>(juce::Slider::RotaryVerticalDrag,
                                                            juce::Slider::TextBoxBelow);
                slider->getProperties().set("family", famProp);
                slider->paramLabel = juce::String::fromUTF8((s.label.empty() ? s.param : s.label).c_str());
                slider->setRange(lo, hi, 1.0);
                const auto opts = s.options;
                slider->textFromValueFunction = [opts](double v) {
                    const int i = (int) std::lround(v);
                    return i >= 0 && i < (int) opts.size()
                               ? juce::String::fromUTF8(opts[(size_t) i].c_str())
                               : juce::String(i);
                };
                slider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, 56, 16);
                slider->setColour(juce::Slider::textBoxTextColourId, Palette::text);
                slider->setValue(modelValue(pn), juce::dontSendNotification);
                enableDoubleClickReset(*slider, host_, cn, pn);
                auto* sp = slider.get();
                slider->onValueChange = [this, sp, cn, pn] { host_.editParam(cn, pn, sp->getValue()); };
                slider->onDragStart = [this, cn, pn] { host_.beginParamDrag(cn, pn); };
                slider->onDragEnd = [this] { host_.endParamDrag(); };
                sp->setParamId(cn, pn);
                sp->onPopup = [this, cn, pn](juce::Point<int> pos) {
                    showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
                };
                addAndMakeVisible(*slider);
                c.slider = std::move(slider);
                break;
            }
            case LayoutSpec::ControlType::RangeVSlider: {
                auto range = std::make_unique<RangeSlider>(host_, cn, pn, lo, hi, s.decimalPlaces,
                                                           s.logarithmic);
                range->onAutomationChanged = [this] { if (onAutomationChanged) onAutomationChanged(); };
                addAndMakeVisible(*range);
                c.range = std::move(range);
                break;
            }
            case LayoutSpec::ControlType::Toggle: {
                auto toggle = std::make_unique<Mappable<juce::ToggleButton>>(
                    juce::String::fromUTF8(s.label.c_str()));
                toggle->setColour(juce::ToggleButton::textColourId, Palette::text);
                toggle->setToggleState(modelValue(pn) >= 0.5, juce::dontSendNotification);
                toggle->onStateChange = [this, toggle = toggle.get(), cn, pn] {
                    host_.editParam(cn, pn, toggle->getToggleState() ? 1.0 : 0.0);
                    applyDims();
                };
                toggle->onRightClick = [this, cn, pn](juce::Point<int> pos) {
                    showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
                };
                addAndMakeVisible(*toggle);
                c.toggle = std::move(toggle);
                break;
            }
            case LayoutSpec::ControlType::MiniToggle: {
                auto tb = std::make_unique<Mappable<juce::TextButton>>(
                    juce::String::fromUTF8(s.label.c_str()));
                tb->setClickingTogglesState(true);
                tb->setColour(juce::TextButton::buttonColourId, Palette::panelLight);
                tb->setColour(juce::TextButton::buttonOnColourId, Palette::accent);
                tb->setColour(juce::TextButton::textColourOffId, Palette::textDim);
                tb->setColour(juce::TextButton::textColourOnId, Palette::background);
                tb->setToggleState(modelValue(pn) >= 0.5, juce::dontSendNotification);
                if (const auto onLabel = s.extraOr("on-label", ""); !onLabel.empty()) {
                    auto sync = [p = tb.get(), on = juce::String::fromUTF8(onLabel.c_str()),
                                 off = juce::String::fromUTF8(s.label.c_str())] {
                        p->setButtonText(p->getToggleState() ? on : off);
                    };
                    tb->onStateChange = sync;
                    sync();
                }
                tb->onClick = [this, p = tb.get(), cn, pn] {
                    host_.editParam(cn, pn, p->getToggleState() ? 1.0 : 0.0);
                    applyDims();
                };
                tb->onRightClick = [this, cn, pn](juce::Point<int> pos) {
                    showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
                };
                addAndMakeVisible(*tb);
                c.textToggle = std::move(tb);
                break;
            }
            case LayoutSpec::ControlType::EnumButtons: {
                const bool waveIcons = s.extraOr("icons") == "waves";
                std::vector<juce::Button*> row;
                const int cur = (int) std::lround(modelValue(pn));
                for (size_t i = 0; i < s.options.size(); ++i) {
                    const auto label = juce::String::fromUTF8(s.options[i].c_str());
                    std::unique_ptr<juce::Button> btn;
                    auto mapMenu = [this, cn, pn](juce::Point<int> pos) {
                        showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
                    };
                    if (waveIcons) {
                        auto wb = std::make_unique<Mappable<WaveGlyphButton>>(
                            label, waveGlyphFor(label),
                            Palette::familyAccent((Family) famProp));
                        wb->setTooltip(wb->glyphName());
                        wb->onRightClick = mapMenu;
                        btn = std::move(wb);
                    } else {
                        auto tb = std::make_unique<Mappable<juce::TextButton>>(label);
                        tb->setColour(juce::TextButton::buttonColourId, Palette::panelLight);
                        tb->setColour(juce::TextButton::buttonOnColourId,
                                      Palette::familyAccent((Family) famProp));
                        tb->setColour(juce::TextButton::textColourOffId, Palette::textDim);
                        tb->setColour(juce::TextButton::textColourOnId, Palette::background);
                        tb->setConnectedEdges(
                            (i > 0 ? juce::Button::ConnectedOnLeft : 0)
                            | (i + 1 < s.options.size() ? juce::Button::ConnectedOnRight : 0));
                        tb->onRightClick = mapMenu;
                        btn = std::move(tb);
                    }
                    btn->setToggleState((int) i == cur, juce::dontSendNotification);
                    row.push_back(btn.get());
                    addAndMakeVisible(*btn);
                    c.radioRow.push_back(std::move(btn));
                }
                for (size_t i = 0; i < row.size(); ++i)
                    row[i]->onClick = [this, cn, pn, i, row] {
                        host_.editParam(cn, pn, (double) i);
                        for (size_t j = 0; j < row.size(); ++j)
                            row[j]->setToggleState(j == i, juce::dontSendNotification);
                    };
                break;
            }
            case LayoutSpec::ControlType::RhythmicUnit: {
                auto ru = std::make_unique<RhythmicUnitPicker>(host_, cn, s.param, s.param2);
                addAndMakeVisible(*ru);
                c.rhythmic = std::move(ru);
                break;
            }
            case LayoutSpec::ControlType::SoundFile: {
                auto sf = std::make_unique<SoundFileSlot>(
                    host_, cn, pn, s.extraOr("filter"), s.extraOr("title"),
                    s.extraOr("kind"));
                addAndMakeVisible(*sf);
                c.file = std::move(sf);
                break;
            }
            case LayoutSpec::ControlType::ScaleFile: {
                auto sf = std::make_unique<ScaleFileSlot>(host_, cn, pn);
                addAndMakeVisible(*sf);
                c.scaleFile = std::move(sf);
                break;
            }
            case LayoutSpec::ControlType::BankFile: {
                auto bf = std::make_unique<BankFileSlot>(
                    host_, cn, pn,
                    banks::Slot{s.extraOr("kind", "Samples"), s.extraOr("filter"),
                                s.extraOr("factory")});
                addAndMakeVisible(*bf);
                c.bankFile = std::move(bf);
                break;
            }
            case LayoutSpec::ControlType::FileTransport: {
                auto tr = std::make_unique<FileTransport>(host_, cn);
                addAndMakeVisible(*tr);
                c.transport = std::move(tr);
                break;
            }
            case LayoutSpec::ControlType::Combo: {
                auto combo = std::make_unique<juce::ComboBox>();
                if (const auto key = s.extraOr("reclass"); !key.empty()) {
                    const std::string cls = className();
                    const size_t d0 = cls.find_first_of("0123456789");
                    const size_t d1 = cls.find_first_not_of("0123456789", d0);
                    if (key == "inputs" && d0 == std::string::npos) break;
                    const std::string current = key == "inputs"
                        ? cls.substr(d0, d1 - d0) : cls.substr(0, 1);
                    const bool bare = key == "mode"
                        && PackRegistry::instance().classManifest("S" + cls) != nullptr
                        && [&] {
                               for (const auto& o : s.options)
                                   if (o.substr(0, 1) == current) return false;
                               return true;
                           }();
                    for (size_t i = 0; i < s.options.size(); ++i) {
                        combo->addItem(juce::String::fromUTF8(s.options[i].c_str()), (int) i + 1);
                        const auto& opt = s.options[i];
                        if (bare ? opt.substr(0, 1) == "M"
                                 : (key == "inputs" ? opt == current
                                                    : opt.substr(0, 1) == current))
                            combo->setSelectedId((int) i + 1, juce::dontSendNotification);
                    }
                    combo->setColour(juce::ComboBox::textColourId, Palette::text);
                    auto* cb = combo.get();
                    combo->onChange = [this, cb, cn, key] {
                        const std::string opt = cb->getText().toStdString();
                        const std::string cur = className();
                        if (opt.empty() || cur.empty()) return;
                        std::string next;
                        if (key == "inputs") {
                            const size_t a = cur.find_first_of("0123456789");
                            const size_t b = cur.find_first_not_of("0123456789", a);
                            if (a == std::string::npos) return;
                            next = cur.substr(0, a) + opt
                                   + (b == std::string::npos ? std::string() : cur.substr(b));
                        } else {
                            next = opt.substr(0, 1) + cur.substr(1);
                            if (PackRegistry::instance().classManifest(next) == nullptr
                                && PackRegistry::instance().classManifest(opt.substr(0, 1) + cur)
                                       != nullptr)
                                next = opt.substr(0, 1) + cur;
                        }
                        if (next == cur) return;
                        host_.replaceOrganism(cn, next);
                        if (host_.onTopologyChanged)
                            juce::MessageManager::callAsync([cb2 = host_.onTopologyChanged] { cb2(); });
                    };
                    addAndMakeVisible(*combo);
                    c.combo = std::move(combo);
                    break;
                }
                c.comboIdsAreValues = !s.extraOr("source").empty();
                if (c.comboIdsAreValues) {
                    auto live = std::make_unique<LiveSourceCombo>();
                    auto* lc = live.get();
                    auto fill = [this, lc, pn, cn, src = s.extraOr("source")] {
                        const int current = (int) modelValue(pn);
                        lc->clear(juce::dontSendNotification);
                        bool listed = false;
                        for (const auto& item : host_.choiceItems(src, cn)) {
                            lc->addItem(juce::String::fromUTF8(item.second.c_str()),
                                        item.first);
                            listed |= item.first == current;
                        }
                        if (!listed && current > 0)
                            lc->addItem(std::to_string(current) + " (unavailable)", current);
                        lc->setSelectedId(current, juce::dontSendNotification);
                    };
                    fill();
                    lc->refreshItems = std::move(fill);
                    combo = std::move(live);
                } else {
                    for (size_t i = 0; i < s.options.size(); ++i)
                        combo->addItem(juce::String::fromUTF8(s.options[i].c_str()), (int) i + 1);
                }
                combo->setColour(juce::ComboBox::textColourId, Palette::text);
                const int off = c.comboIdsAreValues ? 0 : 1;
                combo->setSelectedId((int) modelValue(pn) + off, juce::dontSendNotification);
                auto* cb = combo.get();
                combo->onChange = [this, cb, cn, pn, off] { host_.editParam(cn, pn, cb->getSelectedId() - off); };
                addAndMakeVisible(*combo);
                if (!s.extraOr("steppers").empty()) {
                    auto make = [this, cb, cn, pn, off](bool fwd) {
                        auto b = std::make_unique<ListArrow>(fwd);
                        b->onClick = [this, cb, cn, pn, off, fwd] {
                            if (auto* live = dynamic_cast<LiveSourceCombo*>(cb))
                                if (live->refreshItems) live->refreshItems();
                            const int n = cb->getNumItems();
                            if (n <= 0) return;
                            const int cur = std::max(0, cb->getSelectedItemIndex());
                            const int next = (cur + (fwd ? 1 : -1) + n) % n;
                            cb->setSelectedItemIndex(next, juce::dontSendNotification);
                            host_.editParam(cn, pn, (double) (cb->getItemId(next) - off));
                        };
                        addAndMakeVisible(*b);
                        return b;
                    };
                    c.stepPrev = make(false);
                    c.stepNext = make(true);
                }
                c.combo = std::move(combo);
                break;
            }
            case LayoutSpec::ControlType::IntSpinner:
            case LayoutSpec::ControlType::DoubleSpinner: {
                auto style = juce::Slider::IncDecButtons;
                auto textPos = juce::Slider::TextBoxLeft;
                auto slider = std::make_unique<ParamSlider>(style, textPos);
                slider->paramLabel = juce::String::fromUTF8((s.label.empty() ? s.param : s.label).c_str());
                slider->setColour(juce::Slider::textBoxTextColourId, Palette::text);
                slider->setColour(juce::Slider::textBoxBackgroundColourId, Palette::background);
                slider->setColour(juce::Slider::textBoxOutlineColourId, Palette::border);
                {
                    const juce::Font boxFont(juce::FontOptions(14.0f));
                    for (double v : {lo, hi, (lo + hi) * 0.5})
                        c.textBoxWant = juce::jmax(c.textBoxWant, 8 + (int) std::ceil(
                            juce::TextLayout::getStringWidth(
                                boxFont, unitFormat(paramUnit, v, lo, hi))));
                }
                slider->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 48, 20);
                const bool ints = s.type == LayoutSpec::ControlType::IntSpinner;
                slider->setRange(lo, hi, ints ? 1.0 : 0.0);
                if (!ints) {
                    std::vector<double> ladder;
                    juce::StringArray rungs;
                    rungs.addTokens(juce::String(s.extraOr("steps")), " ,", "");
                    for (const auto& r : rungs)
                        if (r.trim().isNotEmpty()) ladder.push_back(r.getDoubleValue());
                    slider->setStepping((hi - lo) / 100.0, std::move(ladder));
                }
                slider->setIncDecButtonsMode(juce::Slider::incDecButtonsDraggable_Vertical);
                slider->setNumDecimalPlacesToDisplay(
                    (paramIsInt || s.type == LayoutSpec::ControlType::IntSpinner)
                        ? 0 : s.decimalPlaces);
                slider->setUnit(paramUnit);
                slider->setValue(modelValue(pn), juce::dontSendNotification);
                enableDoubleClickReset(*slider, host_, cn, pn);
                auto* sp = slider.get();
                slider->onValueChange = [this, sp, cn, pn] { host_.editParam(cn, pn, sp->getValue()); };
                slider->onDragStart = [this, cn, pn] { host_.beginParamDrag(cn, pn); };
                slider->onDragEnd = [this] { host_.endParamDrag(); };
                sp->setParamId(cn, pn);
                sp->onPopup = [this, cn, pn](juce::Point<int> pos) {
                    showAutomateMenu(host_, cn, pn, pos, [this] { if (onAutomationChanged) onAutomationChanged(); });
                };
                addAndMakeVisible(*slider);
                c.slider = std::move(slider);
                break;
            }
            default:
                break;
        }
        if (!tip.empty())
            for (int k = childrenBefore; k < getNumChildComponents(); ++k)
                if (auto* t = dynamic_cast<juce::SettableTooltipClient*>(getChildComponent(k)))
                    t->setTooltip(juce::String::fromUTF8(tip.c_str()));
        controls_.push_back(std::move(c));
    }
    applyDims();
}

void LayoutEditor::resized() {
    using CT = LayoutSpec::ControlType;
    const bool stretch = spec_.resize == LayoutSpec::Resize::Stretch;
    const double kx = stretch ? (spec_.width > 0 ? (double) getWidth() / spec_.width : 1.0)
                              : scale(getWidth());
    const double ky = stretch ? 1.0 : kx;
    const int xOff = stretch ? 0
                             : juce::jmax(0, (getWidth() - (int) std::lround(spec_.width * kx)) / 2);
    auto scx = [kx](int v) { return (int) std::lround(v * kx); };
    auto scy = [ky](int v) { return (int) std::lround(v * ky); };
    const int labelH = juce::jlimit(11, 18, (int) std::lround(14 * ky));
    for (size_t i = 0; i < controls_.size(); ++i) {
        const auto& s = spec_.controls[i];
        auto& c = controls_[i];
        juce::Rectangle<int> bounds(xOff + scx(s.x), collarHeight() + scy(s.y),
                                    scx(s.w), scy(s.h));

        if (s.type == CT::Label) {
            if (c.label) c.label->setBounds(bounds);
            continue;
        }

        if (c.label) {
            const bool vertical = s.type == CT::Knob || s.type == CT::VSlider ||
                                  s.type == CT::RangeVSlider || s.type == CT::RotarySwitch;
            if (vertical) {
                c.label->setBounds(bounds.removeFromTop(labelH));
                bounds.removeFromTop(2);
            } else {
                const int want = 2 + (int) std::ceil(juce::TextLayout::getStringWidth(
                                     c.label->getFont(), c.label->getText()));
                const int lw = juce::jlimit(bounds.getWidth() / 5, bounds.getWidth() / 2,
                                            juce::jmin(want, scx(72)));
                c.label->setBounds(bounds.removeFromLeft(lw));
                bounds.removeFromLeft(4);
            }
        }
        if (c.slider) {
            if (c.textBoxWant > 48)
                c.slider->setTextBoxStyle(
                    juce::Slider::TextBoxLeft, false,
                    juce::jlimit(48, juce::jmax(48, bounds.getWidth() - 40), c.textBoxWant),
                    scy(20));
            c.slider->setBounds(bounds);
        }
        if (c.stepPrev && c.stepNext) {
            const int a = std::min(18, bounds.getWidth() / 6);
            c.stepPrev->setBounds(bounds.removeFromLeft(a));
            c.stepNext->setBounds(bounds.removeFromRight(a));
        }
        if (c.combo) c.combo->setBounds(bounds);
        if (c.range) c.range->setBounds(bounds);
        if (c.toggle) c.toggle->setBounds(bounds);
        if (c.textToggle) c.textToggle->setBounds(bounds);
        if (!c.radioRow.empty()) {
            auto row = bounds;
            const int w = row.getWidth() / (int) c.radioRow.size();
            for (size_t j = 0; j < c.radioRow.size(); ++j)
                c.radioRow[j]->setBounds(j + 1 == c.radioRow.size() ? row : row.removeFromLeft(w));
        }
        if (c.rhythmic) c.rhythmic->setBounds(bounds);
        if (c.file) c.file->setBounds(bounds);
        if (c.scaleFile) c.scaleFile->setBounds(bounds);
        if (c.bankFile) c.bankFile->setBounds(bounds);
        if (c.transport) c.transport->setBounds(bounds);
        if (c.deck) c.deck->setBounds(bounds);
        if (c.deckPitch) c.deckPitch->setBounds(bounds);
        if (c.deckControls) c.deckControls->setBounds(bounds);
        if (c.keyboard) c.keyboard->setBounds(bounds);
        if (c.faderBank) c.faderBank->setBounds(bounds);
        if (c.waveform) c.waveform->setBounds(bounds);
        if (c.momentary) c.momentary->setBounds(bounds);
        if (c.tap) c.tap->setBounds(bounds);
        if (c.looperTracks) c.looperTracks->setBounds(bounds);
        if (c.stepGrid) c.stepGrid->setBounds(bounds);
        if (c.gainShape) c.gainShape->setBounds(bounds);
        if (c.seqGrid) c.seqGrid->setBounds(bounds);
        if (c.handGestures) c.handGestures->setBounds(bounds);
        if (c.rich) c.rich->setBounds(bounds);
    }
}

}
