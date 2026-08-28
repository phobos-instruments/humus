#include "core/AssistantProtocol.h"

namespace hum {

namespace {

juce::var obj() { return juce::var(new juce::DynamicObject()); }

void set(juce::var& v, const char* key, const juce::var& value) {
    v.getDynamicObject()->setProperty(key, value);
}

juce::var prop(const char* type, const char* description) {
    auto p = obj();
    set(p, "type", type);
    set(p, "description", description);
    return p;
}

juce::var tool(const char* name, const char* description,
               std::initializer_list<std::pair<const char*, juce::var>> props,
               std::initializer_list<const char*> required = {}) {
    auto properties = obj();
    for (auto& [k, v] : props) set(properties, k, v);
    auto schema = obj();
    set(schema, "type", "object");
    set(schema, "properties", properties);
    juce::Array<juce::var> req;
    for (auto* r : required) req.add(juce::String(r));
    set(schema, "required", req);
    auto t = obj();
    set(t, "name", name);
    set(t, "description", description);
    set(t, "input_schema", schema);
    return t;
}

}

juce::var assistantToolsSpec() {
    juce::Array<juce::var> tools;
    tools.add(tool("list_patch",
                   "The current patch: every organism (name, class, position) and all "
                   "audio/MIDI connections. Call this before editing anything.",
                   {}));
    tools.add(tool("list_classes",
                   "Every organism class that can be added, grouped by category.",
                   {}));
    tools.add(tool("describe",
                   "An organism's parameters: schema (name, range, default) and - when "
                   "`name` is a node in the patch - the current values.",
                   {{"name", prop("string", "A node name (e.g. 'SDelay_1') or a class name")}},
                   {"name"}));
    tools.add(tool("levels",
                   "Listen for ~0.6s while the transport plays and report every node's "
                   "output peak (linear, 1.0 = full scale) with dBFS. THE way to judge a "
                   "mix - never guess levels. The node feeding SoundOut at or above 1.0 "
                   "is clipping; a healthy master peaks around 0.5-0.7.",
                   {}));
    tools.add(tool("spectrum",
                   "Listen for ~0.7s while the transport plays and report each node's "
                   "energy in 8 named bands (sub/bass/low_mid/mid/high_mid/presence/"
                   "brilliance/air, dB, -80 = silent). Use it to find masking: two nodes "
                   "loud in the same band are fighting - carve one with a filter, duck "
                   "it, or narrow its width.",
                   {}));
    tools.add(tool("automix",
                   "Deterministic gain staging: measures every source's peak and sets "
                   "its level param so the anchor (a Kick if present, else the loudest "
                   "source) peaks at 0.5, other sources at 0.35, then stages the master "
                   "to 0.6. Run this FIRST for mix/balance requests, then refine with "
                   "spectrum and verify with levels.",
                   {}));
    tools.add(tool("add",
                   "Add an organism to the patch. Returns the ASSIGNED node name - "
                   "read it from the result before connecting (the first of a kind "
                   "is plain 'SSideChain', not 'SSideChain_1'; never guess).",
                   {{"class", prop("string", "Class name from list_classes")},
                    {"pod", prop("string", "Create inside this pod (the prefix of "
                                 "the nodes it should live beside: for 'Pod_1/Kick' "
                                 "pass 'Pod_1'). Omit for the top canvas.")},
                    {"x", prop("number", "Canvas x (default: an empty spot)")},
                    {"y", prop("number", "Canvas y")}},
                   {"class"}));
    tools.add(tool("remove", "Delete a node and its connections.",
                   {{"name", prop("string", "Node name")}}, {"name"}));
    tools.add(tool("rename", "Rename a node (references update).",
                   {{"name", prop("string", "Current node name")},
                    {"new_name", prop("string", "New name")}},
                   {"name", "new_name"}));
    tools.add(tool("replace",
                   "Swap a node's class in place: keeps name, position and compatible "
                   "connections/params. Returns the (unchanged) node name.",
                   {{"name", prop("string", "Node name")},
                    {"class", prop("string", "New class")}},
                   {"name", "class"}));
    tools.add(tool("connect",
                   "Patch a cord from src outlet to dst inlet (channels are mono; a stereo "
                   "link is two cords: outlet 0->inlet 0 and 1->1). midi=true patches the "
                   "dotted MIDI ports instead.",
                   {{"src", prop("string", "Source node")},
                    {"outlet", prop("integer", "Source outlet, 0-based")},
                    {"dst", prop("string", "Destination node")},
                    {"inlet", prop("integer", "Destination inlet, 0-based")},
                    {"midi", prop("boolean", "MIDI cord instead of audio (default false)")}},
                   {"src", "dst"}));
    tools.add(tool("disconnect", "Remove one cord (same arguments as connect).",
                   {{"src", prop("string", "Source node")},
                    {"outlet", prop("integer", "Source outlet, 0-based")},
                    {"dst", prop("string", "Destination node")},
                    {"inlet", prop("integer", "Destination inlet, 0-based")},
                    {"midi", prop("boolean", "MIDI cord instead of audio")}},
                   {"src", "dst"}));
    tools.add(tool("set_param",
                   "Set a numeric parameter (see describe for ranges). Booleans are 0/1, "
                   "enums their index.",
                   {{"name", prop("string", "Node name")},
                    {"param", prop("string", "Parameter name")},
                    {"value", prop("number", "New value (clamped to the range)")}},
                   {"name", "param", "value"}));
    tools.add(tool("set_param_text",
                   "Set a text parameter (e.g. a File path the user gave you).",
                   {{"name", prop("string", "Node name")},
                    {"param", prop("string", "Parameter name")},
                    {"text", prop("string", "New text")}},
                   {"name", "param", "text"}));
    tools.add(tool("set_tempo", "Set the transport tempo.",
                   {{"bpm", prop("number", "Beats per minute")}}, {"bpm"}));
    tools.add(tool("transport", "Start or stop the clock.",
                   {{"action", prop("string", "'play' | 'stop' | 'play_from_start'")}},
                   {"action"}));
    return tools;
}

juce::String assistantSystemPrompt() {
    return
        "You are the assistant inside Humus, a modular audio patcher: "
        "organisms (nodes) carry mono audio channels between outlets and inlets; MIDI "
        "flows on separate dotted cords. You edit the user's LIVE patch with the tools.\n"
        "Rules:\n"
        "- The ONLY way to change anything is calling the tools. There is no code or "
        "Python API: never write code blocks, and never describe an edit as done unless "
        "a tool call actually did it. A reply with prose but no tool calls changes "
        "NOTHING.\n"
        "- Call list_patch first when the request involves the existing patch; call "
        "describe before setting parameters you have not seen. Only add classes that "
        "list_classes actually offers.\n"
        "- Pods: a node named 'Pod_1/Kick' lives INSIDE pod 'Pod_1'. When a new node "
        "should sit beside pod members (a sidechain between 'Pod_1/Kick' and "
        "'Pod_1/Substrate'), pass pod='Pod_1' to add - otherwise it lands on the top "
        "canvas and cords across the boundary become strays.\n"
        "- add returns the node's REAL name and the first of a kind has no number "
        "('SSideChain', not 'SSideChain_1'). NEVER connect a freshly added node in "
        "the same reply as its add: add it alone, read the returned name, then wire "
        "it in the next round.\n"
        "- Audio chains normally end at the SoundOut node's inlets (0 = left, 1 = right); "
        "wire both channels of stereo paths.\n"
        "- Mixing and levels: never set gains by guesswork - measure. For mix/balance "
        "requests: start playback if stopped, run automix (deterministic gain staging), "
        "then spectrum to spot masking - two nodes loud in the same band - and carve it "
        "(high-pass what isn't bass, duck pads under the kick with SSideChain, narrow "
        "busy sources with StereoTool width). Finish with levels: fix clipping by "
        "CUTTING the loud sources rather than boosting quiet ones (parallel paths into "
        "one inlet sum), aim the node feeding SoundOut at a 0.5-0.7 peak, and re-check "
        "until it shows no clipping.\n"
        "- Prefer editing over rebuilding; never remove nodes the user did not ask about.\n"
        "- Everything you do in one reply is a single undo step - be decisive, the user "
        "can always undo.\n"
        "- Final reply: one to three plain sentences covering every change you made this "
        "turn, not just the last tool call. No markdown, no restating tool logs, no "
        "closing offers of further help.";
}

juce::String progressLine(const ToolCall& c) {
    const auto& in = c.input;
    const auto s = [&](const char* k) { return in[k].toString(); };
    const auto arrow = juce::String::fromUTF8(" \xe2\x86\x92 ");
    if (c.name == "list_patch") return "Reading the patch";
    if (c.name == "list_classes") return "Browsing the organism classes";
    if (c.name == "describe") return "Inspecting " + s("name");
    if (c.name == "levels") return "Listening to the levels";
    if (c.name == "spectrum") return "Analysing the spectrum";
    if (c.name == "automix") return "Auto-balancing the mix";
    if (c.name == "add")
        return "Adding a " + s("class")
               + (in.hasProperty("pod") && s("pod").isNotEmpty() ? " in " + s("pod")
                                                                 : juce::String());
    if (c.name == "remove") return "Removing " + s("name");
    if (c.name == "rename") return "Renaming " + s("name") + arrow + s("new_name");
    if (c.name == "replace") return "Turning " + s("name") + " into a " + s("class");
    if (c.name == "connect")
        return ((bool) in["midi"] ? "Patching MIDI " : "Patching ") + s("src") + arrow + s("dst");
    if (c.name == "disconnect")
        return ((bool) in["midi"] ? "Unpatching MIDI " : "Unpatching ") + s("src") + arrow
               + s("dst");
    if (c.name == "set_param")
        return "Setting " + s("name") + " " + s("param") + " to " + s("value");
    if (c.name == "set_param_text") return "Setting " + s("name") + " " + s("param");
    if (c.name == "set_tempo") return "Setting tempo to " + s("bpm") + " BPM";
    if (c.name == "transport")
        return s("action") == "stop" ? juce::String("Stopping the transport")
                                     : juce::String("Starting playback");
    return c.name;
}

bool looksLikePhantomEdits(const juce::String& text) {
    const int fence = text.indexOf("```");
    if (fence < 0) return false;
    const int close = text.indexOf(fence + 3, "```");
    const auto body = close > fence ? text.substring(fence + 3, close)
                                    : text.substring(fence + 3);
    return body.contains("(");
}

juce::String toolNudge() {
    return "Nothing in that reply executed - there is no code API and no edit "
           "happened. The patch is only changed by CALLING your tools (add, connect, "
           "set_param, automix, ...). Redo the work now with real tool calls, checking "
           "list_classes for what actually exists.";
}

bool claimsEdits(const juce::String& text) {
    const juce::String hay = " " + text.toLowerCase();
    static const char* markers[] = {
        " i've applied",  " i applied",  " i've added",    " i added",
        " i've set",      " i set ",     " i've adjusted",  " i adjusted",
        " i've connected"," i connected"," i've removed",   " i removed",
        " i've replaced", " i replaced", " i've inserted",  " i inserted",
        " i've ducked",   " i ducked",   " i've filtered",  " i filtered",
        " i've renamed",  " i renamed",  " i've patched",   " i patched",
        " i've created",  " i created",  " i've raised",    " i raised",
        " i've lowered",  " i lowered",  " i've rebalanced"," i've re-balanced",
        " i've staged",   " i've tuned", " i've improved",  " the patch now ",
    };
    for (const auto* m : markers)
        if (hay.contains(m)) return true;
    return false;
}

juce::String editClaimNudge() {
    return "Your last reply CLAIMS changes, but no editing tool ran this turn - "
           "the patch is untouched. Either make the edits now with real tool calls "
           "(add, connect, set_param, ...) or state plainly that you changed "
           "nothing. Never describe an edit as done unless a tool call did it.";
}

juce::String extractText(const juce::var& response) {
    juce::String text;
    if (const auto* blocks = response["content"].getArray())
        for (const auto& b : *blocks)
            if (b["type"].toString() == "text") text << b["text"].toString();
    return text;
}

std::vector<ToolCall> extractToolCalls(const juce::var& response) {
    std::vector<ToolCall> out;
    if (const auto* blocks = response["content"].getArray())
        for (const auto& b : *blocks)
            if (b["type"].toString() == "tool_use")
                out.push_back({b["id"].toString(), b["name"].toString(), b["input"]});
    return out;
}

bool wantsTools(const juce::var& response) {
    return response["stop_reason"].toString() == "tool_use";
}

juce::var userMessage(const juce::String& text) {
    auto m = obj();
    set(m, "role", "user");
    set(m, "content", text);
    return m;
}

juce::var assistantMessage(const juce::var& response) {
    auto m = obj();
    set(m, "role", "assistant");
    set(m, "content", response["content"]);
    return m;
}

juce::var toolResultsMessage(
        const std::vector<std::pair<juce::String, juce::String>>& idAndContent) {
    juce::Array<juce::var> blocks;
    for (const auto& [id, content] : idAndContent) {
        auto b = obj();
        set(b, "type", "tool_result");
        set(b, "tool_use_id", id);
        set(b, "content", content);
        blocks.add(b);
    }
    auto m = obj();
    set(m, "role", "user");
    set(m, "content", blocks);
    return m;
}

}
