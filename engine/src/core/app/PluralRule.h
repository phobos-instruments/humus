// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

namespace hum {

class PluralRule {
public:
    PluralRule() = default;

    static PluralRule fromHeader(const std::string& pluralForms);

    int forms() const { return forms_; }
    int formFor(long long n) const;
    bool parsed() const { return parsed_; }

private:
    int forms_ = 2;
    std::string expression_ = "n != 1";
    bool parsed_ = false;
};

bool evaluatePluralExpression(const std::string& expression, long long n, long long& result);

}
