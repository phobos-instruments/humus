# Steinberg ASIO SDK (vendored subset)

ASIO 2.3.4, from `ASIO-SDK_2.3.4_2025-10-15.zip`
(`https://www.steinberg.net/asiosdk`, sha256
`d5ebf0c20dd2c5f43771fd0c1418f4b361bf52434ee670097cfa6b3a335e2eca`).

**Taken under the GPLv3 arm of its dual licence**, not the proprietary one — see
`LICENSE.txt`, which offers both. AGPLv3 §13 permits combining a covered work
with GPLv3 code, which is what makes shipping this possible at all; the reasoning
is written up in `packaging/README.md` § ASIO.

Only three headers are here because JUCE's ASIO backend needs only the COM
interface: `juce_ASIO_windows.cpp` includes `<iasiodrv.h>`, which pulls in
`asiosys.h` and `asio.h`. None of the SDK's host or sample-driver sources
(`asio.cpp`, `asiodrivers.cpp`, `combase.cpp`, `driver/`, `host/`) are used, so
none are vendored. The files are copied verbatim from the zip above;
Git normalises their line endings as it does for the rest of the tree.

The GPL covers this code, not the ASIO name or logo. Logo use is optional and, if
used, must be unaltered and product-scoped; "ASIO" must not appear in a product
or company name. Naming a driver type "ASIO" in the device picker is functional
use and is what JUCE calls it.

To upgrade: download the SDK, copy those four files, and check `changes.txt` for
interface changes.
