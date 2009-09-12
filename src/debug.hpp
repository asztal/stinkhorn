#ifndef B98_DEBUG_HPP_INCLUDED
#define B98_DEBUG_HPP_INCLUDED

#include "stinkhorn.hpp"

#include "interpreter.hpp"
#include <vector>
#include <map>
#include <deque>

using std::string;
using std::vector;

template<class CellT, int Dimensions>
class stinkhorn::Stinkhorn<CellT, Dimensions>::DebugInterpreter 
	: public Interpreter 
{
public:
	DebugInterpreter(Options& opts);
	~DebugInterpreter();

	//Necessary because no arguments are dependent on template parameters.
	using Interpreter::fungeSpace;

	void doRun();

protected:
	bool cmdStep(Thread& t, vector<string> const& args, string const& entire_args);
	bool cmdGo(Thread& t, vector<string> const& args, string const& entire_args);
	bool cmdUntil(Thread& t, vector<string> const& args, string const& entire_args);
	bool cmdShowTicks(Thread& t, vector<string> const& args, string const& entire_args);
	bool cmdTrace(Thread& t, vector<string> const& args, string const& entire_args);

	bool cmdShowPos(Thread& t, vector<string> const& args, string const& entire_args);
	bool cmdShowModes(Thread& t, vector<string> const& args, string const& entire_args);

	bool cmdListing(Thread& t, vector<string> const& args, string const& entire_args);
	bool cmdListingSize(Thread& t, vector<string> const& args, string const& entire_args);
	bool cmdDump(Thread& t, vector<string> const& args, string const& entire_args);
	bool cmdShowTrail(Thread& t, vector<string> const& args, string const& entire_args);

	//Stack visualisation stuff
	bool cmdShowStacks(Thread& t, vector<string> const& args, string const& entire_args);
	bool cmdShowTopStack(Thread& t, vector<string> const& args, string const& entire_args);
	bool cmdShowSecondStack(Thread& t, vector<string> const& args, string const& entire_args);

	//Non-destructive stack visualisation commands
	bool cmdShowStringFromToss(Thread& t, vector<string> const& args, string const& entire_args);

	bool cmdStackPop(Thread& t, vector<string> const& args, string const& entire_args);
	bool cmdStackPush(Thread& t, vector<string> const& args, string const& entire_args);
	bool cmdStackClear(Thread& t, vector<string> const& args, string const& entire_args);

	bool cmdExec(Thread& t, vector<string> const& args, string const& entire_args);

	//Breakpoint commands
	bool cmdBreakpoint(Thread& t, vector<string> const& args, string const& entire_args);

	bool cmdCommands(Thread& t, vector<string> const& args, string const& entire_args);

protected:
	bool prompt(Thread& t);
	void step(Thread& t);
	bool shouldBreak(Thread& t);

	struct QuitProgram {} ;

	bool doCommand(Thread& t, string const& command);

	//Probably not necessary, just use boost::lexical_cast
	int toInt(string const& str);

	typedef bool(DebugInterpreter::* CommandFunction)(Thread& t, vector<string> const& args, string const& entire_args);
	void registerCommand(string const& name, CommandFunction);

private:
	std::map<string, CommandFunction> functions;
	int ticks_until_break;
	int total_ticks;
	bool tracing;

	vector<Vector> positional_breakpoints;
	vector<vector<CellT> > lookahead_breakpoints;
	std::size_t until_breakpoint_id;
	CellT break_on_execute;
	Vector listing_context; //Amount of lines to show around the cursor

	std::deque<Vector> last_positions;

	//Signal handling
	void(*oldSignal)(int);
};

#endif
