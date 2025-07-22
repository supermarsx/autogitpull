project = autogitpull
using cxx

cxx.std = 20
cxx.poptions += -I$src_root/include
cxx.libs += pthread

./: include/ src/ tests/
