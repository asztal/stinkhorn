#ifndef B98_OPTIONS_HPP_INCLUDED
#define B98_OPTIONS_HPP_INCLUDED

#include <string>
#include <vector>

namespace stinkhorn {
	struct Options {
		bool debug, warnings, befunge93, trefunge, shouldRun, showSourceLines, concurrent, sandbox, environmentSorted;
		int cellSize;
		int runCount;

		std::string sourceFile;
		std::string pathToSelf;
		std::vector<std::string> sourceLines;
		std::vector<std::string> include;

		char** environment;

		Options() {
			debug = warnings = befunge93 = trefunge = shouldRun = showSourceLines = sandbox = false;
			environmentSorted = false;
			concurrent = true;
			environment = 0;
			cellSize = 32;
			runCount = 1;
		}
	};

	void parseOptions(int argc, char* argv[], char** envp, Options& opts);
}

#endif
