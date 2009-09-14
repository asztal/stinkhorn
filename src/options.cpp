#include "config.hpp"
#include "options.hpp"
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <sstream>

#include <boost/lexical_cast.hpp>

using stinkhorn::Options;
using std::string;
using std::runtime_error;

void completeArg(string& arg);
void showHelp();
void showVersion();
void showWarrantyInfo(std::ostream&);

void stinkhorn::parseOptions(int argc, char* argv[], char** envp, Options& opts) {
	assert(*argv);
	opts.pathToSelf = *argv++;

	opts.shouldRun = true;
	opts.environment = envp;

	for(; *argv; ++argv, --argc) {
		string arg(*argv);

		completeArg(arg);

		if(arg == "--help" || arg == "-h") {
			opts.shouldRun = false;
			showHelp();
		}
		else 
			if(arg == "--version" || arg == "-v") {
				opts.shouldRun = false;
				showVersion();
			} 
		else 
			if(arg == "--debug" || arg == "-d")
				opts.debug = true;
		else 
			if(arg == "--warnings" || arg == "-w")
				opts.warnings = true;
		else
			if(arg == "--trefunge" || arg == "-3") {
				opts.trefunge = true;
			std::cerr << "warning: " << arg << " doesn't do anything yet\n";
		} else 
			if(arg == "--befunge-93" || arg == "-93") {
				opts.befunge93 = true;
			std::cerr << "warning: " << arg << " doesn't change the funge-space wrapping yet\n";
		} else 
			if(arg == "--show-source-lines") 
				opts.showSourceLines = true;
		else 
			if(arg == "--source-line" || arg == "-S") {
				if(!*++argv)
					throw runtime_error("expected an argument for " + arg);
				argc--;

				opts.sourceLines.push_back(*argv);
			} 
		else 
			if(arg == "--cell-size" || arg == "-B") {
				if(!*++argv)
					throw runtime_error("expected an argument for " + arg);
				argc--;

				if(*argv == string("16"))
					opts.cellSize = 16;
				else if(*argv == string("32"))
					opts.cellSize = 32;
				else if(*argv == string("64"))
					opts.cellSize = 64;
				else
					throw runtime_error("cell size: expected 16, 32 or 64");

				std::cerr << "warning: --cell-size doesn't do anything yet\n";
			}
		else
			if(arg == "--no-concurrent" || arg == "-N")
				opts.concurrent = false;
		else 
			if(arg == "--include-directory" || arg == "-I") {
				if(!*++argv)
					throw runtime_error("expected an argument for " + arg);
				argc--;

				opts.include.push_back(*argv);
			}
		else
			if(arg == "--bench" || arg == "-b") {
				opts.runCount = -1;
			}
		else
			if(arg == "--benchn") {
				try {
					if(!*++argv)
						throw runtime_error("expected an argument for --benchn");
					opts.runCount = boost::lexical_cast<int>(*argv);
					if(opts.runCount <= 0)
						throw runtime_error("argument to --benchn was nonpositive");
				} catch (boost::bad_lexical_cast&) {
					throw runtime_error("argument to --benchn was incorrect");
				}
			}
		else {
			if(!opts.sourceFile.empty()) {
				if(arg.find("--") == 0) {
					std::cerr << "unknown argument: " << arg << '\n';
				} else
					throw runtime_error("source file specified twice");
			}
			
			opts.sourceFile = arg;
		}
	}

	if(opts.shouldRun && opts.sourceFile.empty() && opts.sourceLines.empty())
		throw runtime_error("source file not specified");
}

void showHelp() {
	std::ostream& os = std::cout;

	std::size_t left_margin = 25;
	std::size_t width = 79;

	struct option {
		string short_name, long_name, desc;
		bool arg; //whether or not the argument takes a little baby argument of its own

		option(string short_name, string long_name, string desc, bool arg) : 
			short_name(short_name), long_name(long_name), desc(desc), arg(arg)
		{}
	};

	option opts[] = {
		option("-h", "--help", "produce help message", false),
		option("-v", "--version", "produce version message", false),
		option("-w", "--warnings", "turn on warnings", false),
		option("-93", "--befunge-93", "befunge-93 compatibility", false),
		option("-N", "--no-concurrent", "disable concurrency", false),
		option("-B", "--cell-size", "change the cell size (default 32)", true),
		option("-3", "--trefunge", "use trefunge instead of befunge", false),
		option("-S", "--source-line", "specifies the source code inline, instead of reading from a file. May be specified again to specify the next line of the source. Note: ^, <, > and \" must usually be escaped.", false),
		option("", "--show-source-lines", "useful for debugging --source-line", false),
		option("-d", "--debug", "attach debugger", false),
		option("-b", "--bench", "benchmark by running until 2 seconds has elapsed", false),
		option("", "--benchn", "benchmark by running the given number of times", true)
	};

	os << "General options:\n";

	for(int i = 0; i < sizeof(opts)/sizeof(opts[0]); ++i) {
		option& opt = opts[i];

		std::size_t name_length = 2 + opt.long_name.length();
		os << "  " << opt.long_name;

		if(!opt.short_name.empty()) {
			os << " [ " << opt.short_name << " ] ";
			name_length += 6 + opt.short_name.length();
		}

		if(opt.arg) {
			os << " arg";
			name_length += 4;
		}

		//Calculate where to start writing the command
		std::size_t left = left_margin;
		if(name_length > left_margin)
			os << "\n";
		else {
			os << string(left_margin - name_length, ' ');
		}

		std::size_t x = left;
		bool first_word_on_line = true;

		assert(x < width);
		std::istringstream ss(opt.desc);
		string word;
		while(ss >> word) {
			if(x + word.length() + (!first_word_on_line ? 1 : 0) > width) {
				os << "\n" << string(left, ' ');
				x = left;
				first_word_on_line = true;
			}

			if(!first_word_on_line) {
				x++;
				os << " ";
			}
			os << word;

			first_word_on_line = false;
			x += word.length();
		}
		os << std::endl;
	}

	os << std::endl;
	showWarrantyInfo(os);		
}

void showWarrantyInfo(std::ostream& os) {
	using std::endl;
	os << "This program comes with no warranty! Please forward bugs to ";
	os << "lee";
	os << "\x40";
	os << "chunkybacon.";
	os << "org\n";
}

void showVersion() {
	std::cout << "stinkhorn-" << B98_MAJOR_VERSION << "." << B98_MINOR_VERSION << "." << B98_REVISION << " " __DATE__ " " __TIME__ 
#if defined(B98_ARCH)
	" " B98_ARCH
#endif
#if defined(B98_PLATFORM)
	" " B98_PLATFORM
#endif
#if defined(B98_COMPILER)
	" " B98_COMPILER
#endif
#if defined(DEBUG)
	" debug"
#endif
	<< std::endl;
}

//Used as a function object, checks every string given to it for the amount of equal
//characters at the beginning of each string. It also checks for ambiguous arguments or 
//exact matches (which may otherwise be recorded as ambiguities).
struct compare_argument_similarity {
	compare_argument_similarity
		(string& compare_to, std::vector<string>& values, int& max, bool& ambiguous, bool& exact_match) 
		: compare_to(compare_to),
		values(values),
		ambiguous(ambiguous),
		max(max),
		exact_match(exact_match)
	{
	}

	void operator()(string const& str) {
		if(str == compare_to) { 
			exact_match = true;
			return;
		}

		string::size_type i = 0;
		int n = 0;

		while(i < compare_to.size() && i < str.size()) {
			if(compare_to[i] != str[i]) {
				//str contained a character that is not in compare_to, so it is not a match.
				return;
			}

			++n;
			++i;
		}

		update_max(n, str);
	}

private:
	void update_max(int n, string const& str) {
		if(exact_match)
			return;

		if(n == max) {
			ambiguous = true;
			values.push_back(str);
		} else if(n > max) {
			this->max = n;
			values.clear();
			values.push_back(str);
			ambiguous = false;
		}
	}

	string& compare_to;
	std::vector<string>& values;
	int& max;
	bool& ambiguous;
	bool& exact_match;
};

void completeArg(string& arg) {
	//Only arguments beginning with -- are completed.
	//It seems silly, though. What if someone has a file named "--debug"?
	if(arg.size() < 2 || arg[0] != '-' || arg[1] != '-')
		return;

	string list[] = {
		"--debug", "--warnings", "--trefunge", "--befunge93", 
		"--help", "--version", "--show-source-lines", "--include-directory", "--cell-size",
		"--source-line", "--bench", "--benchn", "--no-concurrent"
	};

	//Can't really declare these inside the predicate
	//it seems the predicate is copied for some reason (I thought that didn't happen)
	std::vector<string> values;
	int max = 0;
	bool ambiguous = true;
	bool exact_match = false;

	{
		compare_argument_similarity comparer(arg, values, max, ambiguous, exact_match);
		string* list_end = list + (sizeof(list)/sizeof(*list));
		std::for_each(list, list_end, comparer);
	}

	if(max <= 0)
		return;

	if(exact_match)
		return;

	if(ambiguous) {
		string msg = "ambiguous argument "  + arg + ": could be: ";

		std::vector<string>::const_iterator itr = values.begin(), end = values.end();
		for(; itr != end; ++itr) {
			if(itr != values.begin())
				msg += ", ";

			msg += *itr;
		}

		throw runtime_error(msg);
	}

	assert(!values.empty());
	arg = values.front();
}
