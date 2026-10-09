// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/app/PluralRule.h"

#include <cctype>
#include <cstdlib>

namespace hum {

namespace {

class Evaluator {
public:
    Evaluator(const std::string& text, long long n) : text_(text), n_(n) {}

    bool run(long long& out) {
        out = ternary();
        skipSpace();
        return ok_ && pos_ == text_.size();
    }

private:
    void skipSpace() {
        while (pos_ < text_.size() && std::isspace((unsigned char) text_[pos_])) ++pos_;
    }

    bool take(const char* token) {
        skipSpace();
        const std::string t(token);
        if (text_.compare(pos_, t.size(), t) != 0) return false;
        if (t.size() == 1 && pos_ + 1 < text_.size()) {
            const char next = text_[pos_ + 1];
            if ((t == "<" || t == ">" || t == "!") && next == '=') return false;
            if (t == "&" && next == '&') return false;
            if (t == "|" && next == '|') return false;
        }
        pos_ += t.size();
        return true;
    }

    long long ternary() {
        const long long condition = logicalOr();
        if (!take("?")) return condition;
        const long long yes = ternary();
        if (!take(":")) { ok_ = false; return 0; }
        const long long no = ternary();
        return condition != 0 ? yes : no;
    }

    long long logicalOr() {
        long long v = logicalAnd();
        while (take("||")) { const long long r = logicalAnd(); v = (v != 0 || r != 0) ? 1 : 0; }
        return v;
    }

    long long logicalAnd() {
        long long v = equality();
        while (take("&&")) { const long long r = equality(); v = (v != 0 && r != 0) ? 1 : 0; }
        return v;
    }

    long long equality() {
        long long v = relation();
        for (;;) {
            if (take("==")) v = v == relation() ? 1 : 0;
            else if (take("!=")) v = v != relation() ? 1 : 0;
            else return v;
        }
    }

    long long relation() {
        long long v = sum();
        for (;;) {
            if (take("<=")) v = v <= sum() ? 1 : 0;
            else if (take(">=")) v = v >= sum() ? 1 : 0;
            else if (take("<")) v = v < sum() ? 1 : 0;
            else if (take(">")) v = v > sum() ? 1 : 0;
            else return v;
        }
    }

    long long sum() {
        long long v = product();
        for (;;) {
            if (take("+")) v += product();
            else if (take("-")) v -= product();
            else return v;
        }
    }

    long long product() {
        long long v = unary();
        for (;;) {
            if (take("*")) { v *= unary(); continue; }
            const bool divide = take("/");
            if (!divide && !take("%")) return v;
            const long long d = unary();
            if (d == 0) { ok_ = false; return 0; }
            v = divide ? v / d : v % d;
        }
    }

    long long unary() {
        if (take("!")) return unary() == 0 ? 1 : 0;
        return primary();
    }

    long long primary() {
        skipSpace();
        if (take("(")) {
            const long long v = ternary();
            if (!take(")")) ok_ = false;
            return v;
        }
        if (pos_ < text_.size() && text_[pos_] == 'n') { ++pos_; return n_; }
        if (pos_ < text_.size() && std::isdigit((unsigned char) text_[pos_])) {
            long long v = 0;
            while (pos_ < text_.size() && std::isdigit((unsigned char) text_[pos_]))
                v = v * 10 + (text_[pos_++] - '0');
            return v;
        }
        ok_ = false;
        return 0;
    }

    const std::string& text_;
    long long n_;
    size_t pos_ = 0;
    bool ok_ = true;
};

std::string fieldOf(const std::string& header, const std::string& name) {
    const auto at = header.find(name + "=");
    if (at == std::string::npos) return {};
    const auto from = at + name.size() + 1;
    const auto to = header.find(';', from);
    return header.substr(from, to == std::string::npos ? std::string::npos : to - from);
}

}

bool evaluatePluralExpression(const std::string& expression, long long n, long long& result) {
    return Evaluator(expression, n).run(result);
}

PluralRule PluralRule::fromHeader(const std::string& pluralForms) {
    PluralRule rule;
    const auto count = fieldOf(pluralForms, "nplurals");
    const auto expression = fieldOf(pluralForms, "plural");
    const int forms = count.empty() ? 0 : std::atoi(count.c_str());
    long long probe = 0;
    if (forms < 1 || forms > 6 || expression.empty() || !evaluatePluralExpression(expression, 1, probe))
        return rule;
    rule.forms_ = forms;
    rule.expression_ = expression;
    rule.parsed_ = true;
    return rule;
}

int PluralRule::formFor(long long n) const {
    long long form = 0;
    if (!evaluatePluralExpression(expression_, n, form)) form = n != 1 ? 1 : 0;
    if (form < 0 || form >= forms_) return forms_ - 1;
    return (int) form;
}

}
