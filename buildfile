project = autogitpull
using cxx
using bash

cxx.std = 20
cxx.poptions += -I$src_root/include
cxx.libs += pthread

./: src/ tests/

# Lint helper invoked with `b lint`
lint: bash{scripts/run_lint.sh}
