#pragma once
#include <cstdint>

namespace hum::phbank {

int sixOpFactoryCount();
void fillSixOpFactory(uint8_t bank[][128], int slots);

}
