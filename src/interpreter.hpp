#ifndef B98_INTERPETER_HPP_INCLUDED
#define B98_INTERPETER_HPP_INCLUDED

#include "stinkhorn.hpp"

#include "config.hpp"
#include "vector.hpp"
#include "options.hpp"

#include <string>
#include <vector>

struct QuitProgram { int returnCode; };

template<class CellT, int Dimensions>
class stinkhorn::Stinkhorn<CellT, Dimensions>::Interpreter {
	struct PrivateData;
	PrivateData* self;

	Interpreter(Interpreter const&);

public:
	Interpreter(Options& opts);
	Interpreter(); //Use the default options
	~Interpreter();

	Tree& fungeSpace();

	void run();

	void getArguments(std::vector<std::string>& args) const;
	char** environment() const;

	void spawnThread(const Vector& position, const Vector& direction, const Vector& storageOffset, const StackStackT& stack);

	bool isBefunge93() const;
	bool isTrefunge() const;
	bool isConcurrent() const;
	bool debug() const;
	bool warnings() const;

	std::vector<std::string> const& includeDirectories() const;
	FingerprintRegistry& registry();

protected:
	virtual void doRun();
};

#endif
