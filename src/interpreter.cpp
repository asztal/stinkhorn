#include "config.hpp"
#include "octree.hpp"
#include "interpreter.hpp"
#include "fingerprint.hpp"
#include "thread.hpp"
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <sstream>

using std::cerr;
using std::string;
using std::vector;
using std::auto_ptr;
using std::ostream_iterator;
using std::stringstream;
using std::ifstream;
using std::istream;

namespace stinkhorn {
	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::Interpreter::PrivateData {
		Tree tree;
		FingerprintRegistry registry;
		DefaultFingerprintSource default_source;

		Options& options;

		PrivateData(Options& options) 
			: options(options)
		{
			registry.addSource(&default_source);
		}
	};

	template<class CellT, int Dimensions>
	Stinkhorn<CellT, Dimensions>::Interpreter::Interpreter(Options& options) {
		self = new PrivateData(options);
	}

	template<class CellT, int Dimensions>
	Stinkhorn<CellT, Dimensions>::Interpreter::Interpreter() {
		static Options default_options;
		self = new PrivateData(default_options);
	}

	template<class CellT, int Dimensions>
	Stinkhorn<CellT, Dimensions>::Interpreter::~Interpreter() {
		delete self;
	}

	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::Interpreter::run() {
		assert(self && (!self->options.sourceFile.empty() || !self->options.sourceLines.empty()));

		//todo: catch exceptions and beautify them

		stringstream lines_stream;
		ifstream file_stream;
		istream* stream = 0;

		if(!self->options.sourceFile.empty()) {
			//Load the source file now
			file_stream.open(self->options.sourceFile.c_str(), std::ios_base::binary | std::ios_base::in);
			if(!file_stream.good())
				throw std::runtime_error("Unable to open source file for reading");
			stream = &file_stream;
		} else {
			std::copy(self->options.sourceLines.begin(), self->options.sourceLines.end(),
				std::ostream_iterator<string>(lines_stream, "\n"));

			stream = &lines_stream;
		}

		if(self->options.showSourceLines) {
			std::ostream& os = std::cerr;
			os << "Source dump:\n";
			std::copy(self->options.sourceLines.begin(), self->options.sourceLines.end(), ostream_iterator<string>(os, "\n"));
		}

		assert(stream);

		Vector size;
		//TODO: Why no_form_feeds?
		self->tree.read_file_into(Vector(), *stream, Tree::FileFlags::no_form_feeds, size);

		if(stream == &file_stream)
			file_stream.close();
		stream = 0;

		this->doRun();
	}

	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::Interpreter::doRun() {
		//Just one thread for now.
		auto_ptr<Thread> t (new Thread(*this, self->tree));
		while(t->advance())
			;
	}

	template<class CellT, int Dimensions>
	vector<string> const& Stinkhorn<CellT, Dimensions>::Interpreter::includeDirectories() {
		return self->options.include;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Interpreter::isBefunge93() {
		return self->options.befunge93;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Interpreter::isTrefunge() {
		return self->options.trefunge;
	}

	template<class CellT, int Dimensions>
	typename Stinkhorn<CellT, Dimensions>::Tree& Stinkhorn<CellT, Dimensions>::Interpreter::fungeSpace() {
		return self->tree;
	}

	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::Interpreter::getArguments(vector<string>& args) {
		args.push_back(self->options.sourceFile);
	}

	template<class CellT, int Dimensions>
	char** Stinkhorn<CellT, Dimensions>::Interpreter::environment() {
		return self->options.environment;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Interpreter::debug() {
		return self->options.debug;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Interpreter::warnings() {
		return self->options.warnings;
	}

	template<class CellT, int Dimensions>
	typename Stinkhorn<CellT, Dimensions>::FingerprintRegistry& 
	Stinkhorn<CellT, Dimensions>::Interpreter::registry() 
	{
		return self->registry;
	}
}

INSTANTIATE(class, Interpreter);
