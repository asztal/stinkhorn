#include "config.hpp"
#include "octree.hpp"
#include "interpreter.hpp"
#include "context.hpp"
#include "fingerprint.hpp"
#include "thread.hpp"
#include <list>
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
		typename std::list<Thread*> threads;
		CellT nextThreadID;

		PrivateData(Options& options) 
			: options(options)
		{
			nextThreadID = 1;
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
		while(!self->threads.empty()) {
			delete self->threads.front();
			self->threads.pop_front();
		}

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
		self->threads.push_back(new Thread(*this, self->tree, self->nextThreadID++));
		while(!self->threads.empty()) {
			// This is only valid if the advance() operation doesn't remove the thread itself from the list.
			typename std::list<Thread*>::iterator itr = self->threads.begin();
			while(itr != self->threads.end()) {
				if(!(*itr)->advance())
					itr = self->threads.erase(itr);
				else
					itr++;
			}
		}
	}

	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::Interpreter::spawnThread(const Vector& pos, const Vector& dir, const Vector& storageOffset, const StackStackT& stack) {
		Thread* thread = new Thread(*this, self->tree, self->nextThreadID++);
		thread->topContext().cursor().position(pos);
		thread->topContext().cursor().direction(dir);
		thread->topContext().storageOffset(storageOffset);
		thread->topContext().stack() = stack;
		thread->topContext().cursor().advance();

		self->threads.push_front(thread);
	}

	template<class CellT, int Dimensions>
	vector<string> const& Stinkhorn<CellT, Dimensions>::Interpreter::includeDirectories() const {
		return self->options.include;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Interpreter::isBefunge93() const {
		return self->options.befunge93;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Interpreter::isTrefunge() const {
		return self->options.trefunge;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Interpreter::isConcurrent() const {
		return self->options.concurrent;
	}

	template<class CellT, int Dimensions>
	typename Stinkhorn<CellT, Dimensions>::Tree& Stinkhorn<CellT, Dimensions>::Interpreter::fungeSpace() {
		return self->tree;
	}

	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::Interpreter::getArguments(vector<string>& args) const {
		args.push_back(self->options.sourceFile);
	}

	template<class CellT, int Dimensions>
	char** Stinkhorn<CellT, Dimensions>::Interpreter::environment() const {
		return self->options.environment;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Interpreter::debug() const {
		return self->options.debug;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Interpreter::warnings() const {
		return self->options.warnings;
	}

	template<class CellT, int Dimensions>
	typename Stinkhorn<CellT, Dimensions>::FingerprintRegistry& 
	Stinkhorn<CellT, Dimensions>::Interpreter::registry() 
	{
		return self->registry;
	}

	template<class CellT, int Dimensions>
	const Options& Stinkhorn<CellT, Dimensions>::Interpreter::options() const {
		return self->options;
	}
}

INSTANTIATE(class, Interpreter);
