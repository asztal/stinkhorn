#include "config.hpp"
#include "options.hpp"
#include "interpreter.hpp"
#include "debug.hpp"
#include "cursor.hpp"

#include "fingerprint.hpp" //for TimerFingerprint

#include <iostream>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <ctime>

using namespace stinkhorn;
using namespace std;

int main(int argc, char** argv, char** envp) {
	Options opts;
	
	srand(static_cast<unsigned int>(time(0)));


#ifdef B98_NO_64BIT_CELLS
	Stinkhorn<int32, 2>::TimerFingerprint timer;
#else
	Stinkhorn<int64, 2>::TimerFingerprint timer;
#endif
	timer.mark();

	int returnCode = 0, runCount = 0;

	try {
		parseOptions(argc, argv, envp, opts);

		if(!opts.shouldRun)
			return 0;
		
		//TODO: support multiple cell sizes
		for(int i = 0; opts.runCount == -1 ? (timer.elapsedTime() < 2000000) : i < opts.runCount; ++i) {
			try { 
				if(opts.debug) {
#ifndef B98_NO_64BIT_CELLS
					if(opts.cellSize == 64) 
#ifndef B98_NO_TREFUNGE
						if(trefunge)
							Stinkhorn<int64, 3>::DebugInterpreter(opts).run();
						else
#endif
							Stinkhorn<int64, 2>::DebugInterpreter(opts).run();
					else
#endif
#ifndef B98_NO_TREFUNGE
						if(trefunge)
							Stinkhorn<int32, 3>::DebugInterpreter(opts).run();
						else
#endif
							Stinkhorn<int32, 2>::DebugInterpreter(opts).run();
				} else {
#ifndef B98_NO_64BIT_CELLS
					if(opts.cellSize == 64) 
#ifndef B98_NO_TREFUNGE
						if(trefunge)
							Stinkhorn<int64, 3>::Interpreter(opts).run();
						else
#endif
							Stinkhorn<int64, 2>::Interpreter(opts).run();
					else
#endif
#ifndef B98_NO_TREFUNGE
						if(trefunge)
							Stinkhorn<int32, 3>::Interpreter(opts).run();
						else
#endif
							Stinkhorn<int32, 2>::Interpreter(opts).run();
				}
				++runCount;
			} catch(QuitProgram&) {
				++runCount;
				if(opts.runCount == 1)
					throw;
			}
		}
	} catch (std::exception& e) {
		cerr << argv[0] << ": " << e.what() << "\n";
		returnCode = 1;
	} catch (QuitProgram& q) {
		returnCode = q.returnCode;
	} catch(int n) {
		//This is such a stupid idea.
		cerr << "Warning: something is doing silly things\n";
		returnCode = n;
	} catch (...) {
		cerr << "** Unknown internal failure! **\n";
		returnCode = 2;
	}
	
	if(opts.runCount != 1) {
		float time = (timer.elapsedTime() / 1000) / 1000.0f;
		if(time >= 0)
			fprintf(stderr, "Time: average %0.3fs for %d runs (%0.3fs total)\n", time / runCount, runCount, time);
		else
			std::cerr << "Time: HRTI timer failed or returned negative number :(\n";
	}

	return returnCode;
}
