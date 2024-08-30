#include "LibWindows/Platform.h"
#include "Lib/Assert.h"
#include "Lib/Filesystem.h"
#include "Lib/String.h"
#include "Lib/Time.h"
#include <iostream>

BUFF_NAMESPACE_BEGIN

static int run(const ArrayView<const char*> arguments) {
    ExecuteProcessParams dummyParams;
    for (auto& i : arguments) {
        dummyParams.args.pushBack(i);
    }
    String cmdline = dummyParams.getCommandLine();
    BUFF_ASSERT(cmdline.startsWith("\"\"")); // quoted dummy command to run - remove it
    cmdline = cmdline.getSubstring(2);
    const Timer timer;
    const int   res = std::system(cmdline.asCString());
    std::cout << "\n[BENCHMARK UTILITY] Elapsed time: " << timer.getElapsed().getUserReadable() << ".\n";
    return res;
}

BUFF_NAMESPACE_END

// ReSharper disable once CppInconsistentNaming
int main(const int argc, const char** argv) {
    return Buff::run(Buff::ArrayView<const char*>(argv, argc).getSub(1));
}
