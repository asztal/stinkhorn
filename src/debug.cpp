#include "debug.hpp"
#include "thread.hpp"
#include "context.hpp"
#include "octree.hpp"

#ifdef WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <iomanip>
#include <cctype>
#include <stdexcept>
#include <fstream>

#include <boost/lexical_cast.hpp>
#include <signal.h>

using std::cerr;
using std::endl;
using std::size_t;
using std::string;
using std::vector;

namespace stinkhorn {
	namespace signals { namespace {
		bool gInterrupt = false;

		void interruptHandler(int) {
			gInterrupt = true;
			signal(SIGINT, interruptHandler);
		}
	}}

	template<class CellT, int Dimensions>
	Stinkhorn<CellT, Dimensions>::DebugInterpreter::DebugInterpreter(stinkhorn::Options& options)
		: Interpreter(options) 
	{
		options.concurrent = false;
		ticks_until_break = 0;
		total_ticks = 0;
		until_breakpoint_id = -1;

		listing_context = Vector(35, 8, 0);

		//Stepping. (Perhaps some more intelligent step functions would be nice.)
		registerCommand("s", &DebugInterpreter::cmdStep);
		registerCommand("step", &DebugInterpreter::cmdStep);

		registerCommand("t", &DebugInterpreter::cmdTrace);
		registerCommand("trace", &DebugInterpreter::cmdTrace);

		registerCommand("go", &DebugInterpreter::cmdGo);
		registerCommand("until", &DebugInterpreter::cmdUntil);

		registerCommand("ticks", &DebugInterpreter::cmdShowTicks);

		registerCommand("list", &DebugInterpreter::cmdListing);
		registerCommand("l", &DebugInterpreter::cmdListing);
		registerCommand("dump", &DebugInterpreter::cmdDump);

		//Perhaps ls is better as something else.
		registerCommand("ls", &DebugInterpreter::cmdListingSize);
		registerCommand("listsize", &DebugInterpreter::cmdListingSize);
		registerCommand("trail", &DebugInterpreter::cmdShowTrail);
		registerCommand("tr", &DebugInterpreter::cmdShowTrail);

		registerCommand("pos", &DebugInterpreter::cmdShowPos);
		registerCommand("modes", &DebugInterpreter::cmdShowModes);

		registerCommand("stacks", &DebugInterpreter::cmdShowStacks);
		registerCommand("toss", &DebugInterpreter::cmdShowTopStack);
		registerCommand("soss", &DebugInterpreter::cmdShowSecondStack);

		registerCommand("spop", &DebugInterpreter::cmdStackPop);
		registerCommand("spush", &DebugInterpreter::cmdStackPush);
		registerCommand("sclear", &DebugInterpreter::cmdStackClear);

		//Different (doesn't actually modify stack stack)
		registerCommand("sstring", &DebugInterpreter::cmdShowStringFromToss);

		registerCommand("exec", &DebugInterpreter::cmdExec);

		registerCommand("b", &DebugInterpreter::cmdBreakpoint);
		registerCommand("breakpoint", &DebugInterpreter::cmdBreakpoint);

		registerCommand("?", &DebugInterpreter::cmdCommands);
		registerCommand("help", &DebugInterpreter::cmdCommands);
		registerCommand("commands", &DebugInterpreter::cmdCommands);

		oldSignal = signal(SIGINT, signals::interruptHandler);
	}

	template<class CellT, int Dimensions>
	Stinkhorn<CellT, Dimensions>::DebugInterpreter::~DebugInterpreter() {
		if(oldSignal != SIG_ERR)
			signal(SIGINT, oldSignal);
	}

	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::DebugInterpreter::doRun() {
		std::auto_ptr<Thread> t (new Thread(*this, this->fungeSpace(), 1));

		while(true) {
			step(*t);

			if(shouldBreak(*t)) {
				tracing = false;

				if(until_breakpoint_id != -1) {
					//Even if we didn't hit the required expression, erase it.
					if(until_breakpoint_id >= 0 && until_breakpoint_id < lookahead_breakpoints.size())
						lookahead_breakpoints.erase(lookahead_breakpoints.begin() + until_breakpoint_id);
					until_breakpoint_id = -1;
				}

				if(!prompt(*t))
					break;
			} else {
				if(tracing) {
					vector<string> v;
					cmdShowPos(*t, v, "");
				}
			}
			
			if(!t->advance())
				break;

			if(last_positions.size() >= 25)
				last_positions.pop_front();
			last_positions.push_back(t->topContext().cursor().position());

			total_ticks++;
		}
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::shouldBreak(Thread& t) {
		if(signals::gInterrupt) {
			signals::gInterrupt = false;
			cerr << "CTRL-C interrupt!\n";
			ticks_until_break = 0;
			return true;
		}

		if(ticks_until_break == 0) {
			//cerr << t.topContext().cursor_.position() << "\n";
			return true;
		}

		for(size_t i = 0; i < positional_breakpoints.size(); ++i) {
			if(positional_breakpoints[i] == t.topContext().cursor().position()) {
				cerr << "hit breakpoint #" << i << " at " << positional_breakpoints[i] << "\n";
				return true;
			}
		}
		
		for(size_t i = 0; i < lookahead_breakpoints.size(); ++i) {
			vector<CellT>& v = lookahead_breakpoints[i];
			Cursor cr(t.topContext().cursor());

			bool match = true;
			for(size_t j = 0; j < v.size(); ++j) {
				//TODO? Do simple interpretation of directional commands?
				if(v[j] != cr.get(cr.position())) {
					match = false;
					break;
				}
				cr.advance();
			}

			if(!match)
				continue;

			cerr << "hit lookahead breakpoint #" << i << " at " << t.topContext().cursor().position() << "\n";
			cerr << "breakpoint data: ";
			for(size_t j = 0; j < v.size(); ++j) {
				if(v[j] >= 32)
					cerr << (char)v[j];
				else
					cerr << '\xFA'; //\mdot

				if(j > 50) {
					cerr << "[Truncated]\n";
					return true;
				}
			}
			cerr << "\n";
			return true;
		}
		return false;
	}

	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::DebugInterpreter::step(Thread& t) {
		if(ticks_until_break > 0)
			ticks_until_break--;
	}

	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::DebugInterpreter::registerCommand(string const& name, CommandFunction fn) {
		functions[name] = fn;
	}

	namespace {
	#ifdef WIN32
		int setConsoleColour() {
			HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			int old_attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
			if(GetConsoleScreenBufferInfo( hConsole, &csbi)) {
				old_attributes = csbi.wAttributes;
			}

			SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
			return old_attributes;
		}

		void resetConsoleColour(int attributes) {
			HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(hConsole, attributes);
		}

		void setDisplayMode(int mode) {
			HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

			WORD attr = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
			if((mode & 16) == 16) //Current cell (yellow)
				attr = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
			else if((mode & 4) == 4) //Next to be executed (dark yellow)
				attr = FOREGROUND_GREEN | FOREGROUND_RED;
			else if((mode & 2) == 2) //IP Trail is purple
				attr = FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY;
			
			if((mode & 8) == 8) //Next to be executed is green
				attr = ((attr & 0xF0) >> 8) | ((attr & 0xF) << 8);

			SetConsoleTextAttribute(hConsole, attr);
			
			/*CONSOLE_SCREEN_BUFFER_INFO csbi;
			if(GetConsoleScreenBufferInfo(hConsole, &csbi)) {
				WORD attr = (csbi.wAttributes & ~0xFF) | ((csbi.wAttributes & 0xF) << 8) | ((csbi.wAttributes & 0xF0) >> 8);
				attr ^= FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;
				SetConsoleTextAttribute(hConsole, csbi.wAttributes ^ 0x77); 
			}*/
		}
	#else
		//TODO: ANSI codes?
		int setConsoleColour() {
			return 0;
		}

		void resetConsoleColour(int c) {
		}

        void setDisplayMode(int mode) {
        }
	#endif
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::prompt(Thread& t) {
		string command;

		while(true) {
			int i = setConsoleColour();
			cerr << t.topContext().cursor().position() << " ";
			cerr << "sdb% ";
			resetConsoleColour(i);
			getline(std::cin, command);

			try {
				if(!doCommand(t, command))
					return true;
			} catch ( QuitProgram& ) {
				return false;
			} catch ( std::exception& e ) {
				cerr << "** " << e.what() << std::endl;
			}
		}

		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::doCommand(Thread& t, string const& entire) {
		if(entire == "q" || entire == "quit")
			throw QuitProgram();

		string cmd;
		std::stringstream ss(entire);
		ss >> cmd;

		if(!ss)
			throw std::runtime_error("parse error");

		string entire_arguments;
		vector<string> args;
		while(ss) {
			string arg;

			while(std::isspace(ss.peek()))
				ss.ignore(1);

			if(args.empty()) {
				//Now, after the whitespace is gone, get the string.
				if(size_t(ss.tellg()) < entire.length() && ss.tellg() > 0)
					entire_arguments = entire.substr(ss.tellg());
			}

			if(ss.eof())
				break;

			if(ss.peek() == '\"') {
				ss.ignore(1);
				while(true) {
					int c = ss.get();
					if(c == std::char_traits<char>::eof())
						break;

					if(c == '\"')
						break;

					arg += char(c);
				}
			} else {
				ss >> arg;
			}

			args.push_back(arg);
		}

		if(!ss && !ss.eof())
			throw std::runtime_error("parse error");

		typename std::map<string, CommandFunction>::iterator itr = 
			functions.find(cmd);

		if(itr != functions.end()) {
			return (this->*itr->second)(t, args, entire_arguments);
		} else {
			throw std::runtime_error("what is " + cmd + "?");
		}
	}

	template<class CellT, int Dimensions>
	int Stinkhorn<CellT, Dimensions>::DebugInterpreter::toInt(string const& str) {
		std::stringstream ss(str);
		int n;
		ss >> std::skipws >> n;
		if(!ss || !ss.eof())
			throw std::runtime_error("\"" + str + "\" is not a number");
		return n;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdStep(Thread& t, vector<string> const& args, string const& entire_args) {
		if(args.empty()) {
			ticks_until_break = 1;
		} else {
			ticks_until_break = toInt(args.at(0));
		}

		return false;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdTrace(Thread& t, vector<string> const& args, string const& entire_args) {
		tracing = true;
		return cmdStep(t, args, entire_args);
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdGo(Thread& t, vector<string> const& args, string const& entire_args) {
		ticks_until_break = -1;

		return false;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdUntil(Thread& t, vector<string> const& args, string const& entire_args) {
		lookahead_breakpoints.push_back(vector<CellT>());
		vector<CellT>& v = lookahead_breakpoints.back();
		for(size_t i = 0; i < entire_args.size(); ++i)
			v.push_back(entire_args[i]);
		until_breakpoint_id = lookahead_breakpoints.size() - 1;
		ticks_until_break = -1;
		return false;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdShowTicks(Thread& t, vector<string> const& args, string const& entire_args) {
		std::cerr << "Total ticks: " << this->total_ticks << std::endl;
		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdShowStacks(Thread& t, vector<string> const& args, string const& entire_args) {
		StackStackT& stack = t.topContext().stack();

		vector<std::size_t> sizes;
		stack.getStackSizes(std::back_inserter(sizes));

		cerr << "stack sizes (bottom to top): [ ";
		copy(sizes.begin(), sizes.end(), std::ostream_iterator<std::size_t>(std::cerr, " "));
		cerr << " ]\n" << stack.stackCount() << " stacks.\n";
		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdShowTopStack(Thread& t, vector<string> const& args, string const& entire_args) {
		StackStackT stack(t.topContext().stack());
		stack.queueMode(true); //Pick bottom entries first :)

		size_t size = stack.topStackSize();

		cerr << "stack entries (bottom to top): [ ";
		while(stack.topStackSize())
			cerr << stack.pop() << " ";
		cerr << "]\n";

		cerr << size << " entries\n";
		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdShowSecondStack(Thread& t, vector<string> const& args, string const& entire_args) {
		StackStackT stack(t.topContext().stack());
		stack.queueMode(true); //Pick bottom entries first :)

		if(stack.stackCount() == 1)
			throw std::range_error("there is no second stack");

		stack.popStackNoSemantics();
		size_t size = stack.topStackSize();

		cerr << "stack entries (bottom to top): [ ";
		while(stack.topStackSize())
			cerr << stack.pop() << " ";
		cerr << "]\n";

		cerr << size << " entries\n";
		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdShowStringFromToss(Thread& t, const vector<string>& args, string const& entire_args) {
		StackStackT stack(t.topContext().stack());
		
		String s;
		stack.readString(s);
		cerr << s.length() << " chars: \"";
		writeString(cerr, s);
		cerr << "\"\n";

		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdShowModes(Thread& t, const vector<string>& args, string const& entire_args) {
		Context& ctx = t.topContext();

		cerr << "String mode: " << std::boolalpha << ctx.stringMode() << endl;
		cerr << "Invert mode: " << std::boolalpha << ctx.stack().invertMode() << endl;
		cerr << "Queue mode: " << std::boolalpha << ctx.stack().queueMode() << endl;
		cerr << "Storage offset: " << ctx.storageOffset() << endl;
		if(ctx.quitFlag())
			cerr << "Quit flag is set" << endl;

		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdShowPos(Thread& t, const vector<string> &args, string const& entire_args) {
		Cursor& cr = t.topContext().cursor();

		cerr << "at " << cr.position() << ", delta is " << cr.direction() << " on '" << char(cr.currentCharacter()) << "' instruction\n";
		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdListingSize(Thread& t, const vector<string> &args, string const& entire_args) {
		Cursor cr(t.topContext().cursor());

		Vector middle = cr.position();
		cmdShowPos(t, args, entire_args);

		CellT cx = boost::lexical_cast<CellT>(args.at(0));
		CellT cy = boost::lexical_cast<CellT>(args.at(1));
		listing_context = Vector(cx, cy, 0);
		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdListing(Thread& t, const vector<string> &args, string const& entire_args) {
		Cursor cr(t.topContext().cursor());

		Vector middle = cr.position();
		cmdShowPos(t, args, entire_args);

		CellT cx = listing_context.x, cy = listing_context.y;
		if(args.size() >= 2) {
			middle.x = toInt(args[0]);
			middle.y = toInt(args[1]);
			if(args.size() >= 3 && Dimensions == 3)
				middle.z = toInt(args[2]);
		}

		cx = std::abs(cx);
		cy = std::abs(cy);

		//TODO: does this assume there's a valid character to jump to?
		bool hasNext = cr.advance();
		Vector next = cr.position();

		setDisplayMode(0);
		for(CellT y = -cy; y <= cy; ++y) {
			for(CellT x = -cx; x <= cx; ++x) {
				cr.position(middle + Vector(x, y, 0));
				CellT c = cr.currentCharacter();
				char ch = static_cast<char>(c);
				if(ch < 32)
					ch = '\xFA'; //Is this a good idea? (Middle dot)

				int mode = 0;
				if(x == 0 || y == 0)
					mode = 1;
				if(count(last_positions.begin(), last_positions.end(), cr.position()) > 0)
					mode |= 2;
				if(hasNext && next == cr.position())
					mode |= 4;
				if(x == 0 && y == 0)
					mode |= 16;
				
				if(mode)
					setDisplayMode(mode);
				cerr << ch;
				if(mode)
					setDisplayMode(0);
			}

			cerr << '\n';
		}

		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdShowTrail(Thread& t, const vector<string> &args, string const& entire_args) {
		Cursor cr(t.topContext().cursor());

		cerr << "Trail (" << last_positions.size() << " entries, most recent shown last)\n";
		for(size_t i = 0; i < last_positions.size(); ++i) {
			Vector const& pos = last_positions[i];
			cerr << i << ": " << pos << "\n";
		}

		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdDump(Thread& t, const vector<string> &args, string const& entire_args) {
		string filename = entire_args;
		if(args.empty())
			filename = "dump.txt";
		std::ofstream stream(filename.c_str());
		stream.exceptions(std::ios::badbit);
		Vector min, max;
		fungeSpace().get_minmax(min, max);
		fungeSpace().write_file_from(min, max + Vector(1, 1, 1) /*exclusive*/, stream, 1); //Linear mode: don't write unnecessary spaces
		stream.close();
		cerr << "dumped funge space to " << filename << '\n';

		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdStackPop(Thread& t, const vector<string> &args, string const& entire_args) {
		cerr << t.topContext().stack().pop() << endl;
		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdStackPush(Thread& t, const vector<string> &args, string const& entire_args) {
		t.topContext().stack().push(toInt(args.at(0)));
		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdStackClear(Thread& t, const vector<string> &args, string const& entire_args) {
		cerr << "cleared " << t.topContext().stack().topStackSize() << " cells\n";
		t.topContext().stack().clearTopStack();
		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdExec(Thread& t, const vector<string> &args, string const& entire_args) {
		string str = args.at(0);
		for(size_t i = 0; i < str.length(); ++i)
			t.execute(str[i]);

		return true;
	}

	//More types of breakpoints are needed, such as data breakpoints, lookahead breakpoints, and so on.
	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdBreakpoint(Thread& t, const vector<string> &args, string const& entire_args) {
		if(args.empty()) {
			//List breakpoints.
			for(size_t i = 0; i < positional_breakpoints.size(); ++i)
				cerr << i << "\t@ " << positional_breakpoints[i] << '\n';
		} else {
			string subcmd = args.at(0);
			if(subcmd == "add") {
				Vector pos;
				pos.x = boost::lexical_cast<CellT>(args.at(1));
				pos.y = boost::lexical_cast<CellT>(args.at(2));
				if(Dimensions == 3)
					pos.z = boost::lexical_cast<CellT>(args.at(3));

				positional_breakpoints.push_back(pos);
				cerr << "added (" << positional_breakpoints.size() - 1 << ")\n";
			} else if(subcmd == "rm" || subcmd == "remove") {
				size_t index = toInt(args[1]);
				if(index < 0 || index >= positional_breakpoints.size())
					cerr << "invalid breakpoint ID\n";
				else
					positional_breakpoints.erase(positional_breakpoints.begin() + index);
			} else {
				cerr << "unknown breakpoint command \"" << args.at(0) << "\"\n";
			}
		}

		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::DebugInterpreter::cmdCommands(Thread& t, const vector<string> &args, string const& entire_args) {
		if(args.empty()) {
			cerr << "available commands:";
			for(typename std::map<string, CommandFunction>::iterator begin = functions.begin(), end = functions.end();
				begin != end; ++begin)
			{
				cerr << ' ' << begin->first;
			}
			cerr << '\n';
		} else {
			string cmd = args.at(0);
			string desc;
			if(cmd == "s" || cmd == "step")
				desc = "step <n=1>: step forward <n> instructions.";
			if(cmd == "t" || cmd == "trace")
				desc = "trace <n=1>: step forward <n> instructions, printing the position on each tick.";
			else if(cmd == "go")
				desc = "go: run until breakpoint or end of program.";
			else if(cmd == "until")
				desc = "until <str>: run until the code <str> is about to be executed. <>^v arrows are not yet followed.";
			else if(cmd == "ticks")
				desc = "ticks: show the amount of ticks since the program/Thread started.";
			else if(cmd == "list" || cmd == "listing" || cmd == "l")
				desc = "list [<x> <y>]: show the code surrounding the IP, or (<x>, <y>) if they are given.";
			else if(cmd == "dump")
				desc = "dump <file=dump.txt>: dump the entire funge space to dump.txt.";
			else if(cmd == "listsize" || cmd == "ls")
				desc = "listsize <x> <y>: set the default size for the list command (x is half of the width, y is half of the height).";
			else if(cmd == "trail" || cmd == "tr")
				desc = "trail: show the most recent cells passed through by the current IP.";
			else if(cmd == "pos")
				desc = "pos: show the current IP's position and delta.";
			else if(cmd == "modes")
				desc = "modes: show the status of the IP's various modes (string mode, hover mode, queue mode...).";
			else if(cmd == "stacks")
				desc = "stacks: list the stacks on the current IP's stack stack.";
			else if(cmd == "toss")
				desc = "toss: show the current IP's top-of-stack-stack.";
			else if(cmd == "soss")
				desc = "soss: show the current IP's second-on-stack-stack.";
			else if(cmd == "spop")
				desc = "spop: pop a value from the current IP's top-of-stack-stack.";
			else if(cmd == "spush <value>")
				desc = "spop: push <value> onto the current IP's top-of-stack-stack.";
			else if(cmd == "sclear")
				desc = "sclear: empty the current IP's top-of-stack-stack.";
			else if(cmd == "sstring")
				desc = "sstring: read a string from the stack until a zero is found, without actually modifying the stack.";
			else if(cmd == "b" || cmd == "breakpoint")
				desc = "b: list breakpoints\nb add <x> <y> (<z>): add a breakpoint\nb rm: remove a breakpoint by ID";
			else if(cmd == "?" || cmd == "help" || cmd == "commands")
				desc = "help: list commands\nhelp <command>: show information about a particular command and its parameters.";

			if(!desc.empty()) {
				cerr << desc << "\n";
			} else {
				cerr << "either the command isn't known, or there is no help text available for it.\n";
			}
		}
		return true;
	}
}

INSTANTIATE(class, DebugInterpreter);
