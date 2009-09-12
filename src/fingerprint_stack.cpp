#include "fingerprint_stack.hpp"

#include <algorithm>
#include <iomanip>

using std::map;
using std::vector;
using std::setbase;
using std::setw;

namespace stinkhorn {
	template<class CellT, int Dimensions>
	Stinkhorn<CellT, Dimensions>::FingerprintStack::FingerprintStack(FingerprintRegistry& registry) 
		: registry(registry) 
	{
	}

	template<class CellT, int Dimensions>
	Stinkhorn<CellT, Dimensions>::FingerprintStack::~FingerprintStack() {
		//Unload all semantics
		for(int i = 0; i < 26; ++i) {
			while(!semantics[i].empty()) {
				release(semantics[i].top());
				semantics[i].pop();
			}
		}

		//Note: no re-allocation will occur here, since vector doesn't shrink
		while(!stack.empty()) {
			release(stack.back());
			stack.pop_back();
		}

		//'all' does not addRef its members
	}

	namespace {
		template<class IFingerprintT>
		struct id_equals {
			IdT id;

			id_equals(IdT id) : id(id) {}

			bool operator()(IFingerprintT* fp) {
				return fp->is(id);
			}
		};
	}

	template<class CellT, int Dimensions>
	typename Stinkhorn<CellT, Dimensions>::IFingerprint* 
	Stinkhorn<CellT, Dimensions>::FingerprintStack::get(IdT id) 
	{
		typename vector<IFingerprint*>::iterator itr = find_if(all.begin(), all.end(), id_equals<IFingerprint>(id));
		if(itr == all.end())
			return 0;

		return *itr;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::FingerprintStack::push(IFingerprint* builtin) {
		all.push_back(builtin);
		stack.push_back(builtin);
		builtin->addRef();
		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::FingerprintStack::push(IdT id) {
		IFingerprint* fp = get(id);

		if(!fp) {
			fp = this->registry.createFingerprint(id);

			if(fp)
				all.push_back(fp);
		}

		if(!fp)
			return false;

		if(!fp->onlySemantics()) {
			stack.push_back(fp);
			fp->addRef();
		}

		char const* instructions = fp->handledInstructions();
		for(; *instructions; instructions++) {
			int sem = *instructions - 'A';
			assert(sem >= 0 && sem < 26);
			semantics[sem].push(fp);
			fp->addRef();
		}

		std::vector<IFingerprint*> stack;
		release(fp);

		return true;
	}

	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::FingerprintStack::release(IFingerprint* fp) {
		char const* hi = fp->handledInstructions();
		IdT id = fp->id();
		unsigned long c = fp->release();
		if(c == 0) {
			//std::cerr << "[FP: 0x" << setbase(16) << setw(8) << id <<  " ] Releasing fingerprint (supported: " << hi << ")\n";
			typename vector<IFingerprint*>::iterator itr = remove(all.begin(), all.end(), fp);
			all.erase(itr, all.end());
		} else {
			//std::cerr << "[FP: 0x" << setbase(16) << setw(8) << id <<  " ] Not yet releasing fingerprint (supported: " << hi << ", ref count: " << c << " )\n";
		}
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::FingerprintStack::pop(IdT id) {
		IFingerprint* fp = get(id);

		//Spec says: ")" unloads semantics even if fingerprint was not loaded
		if(!fp)
			return true;

		char const* instructions = fp->handledInstructions();
		for(; *instructions; instructions++) {
			int sem = *instructions - 'A';
			assert(sem >= 0 && sem < 26);
			assert(!semantics[sem].empty());
			IFingerprint* boundFP = semantics[sem].top(); 
			semantics[sem].pop();
			release(boundFP); //Release the fingerprint at the top of the stack, not fp (they might be the same, might not).
		}

		//Remove fp from the stack if it's in there.
		typename vector<IFingerprint*>::iterator itr = std::remove(stack.begin(), stack.end(), fp);
		if(itr != stack.end())
			release(fp);
		stack.erase(itr, stack.end());

		return true;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::FingerprintStack::execute(CellT instruction, Context& ctx) {
		if(instruction >= 'A' && instruction <= 'Z') {
			int sem = static_cast<int>(instruction - 'A');
			if(!semantics[sem].empty()) {
				IFingerprint* fp = semantics[sem].top();
				if(fp->handleInstruction(instruction, ctx))
					return true;
			}
		}

		//Technically the behaviour of calling fingerprint_stack::push and returning false (unhandled)
		//isn't very well defined at all, but let's keep it uncrashing for now.
		//It should be warning-worthy though.
		typename vector<IFingerprint*>::size_type i = 0;
		while(i < stack.size()) {
			if(stack.rbegin()[i]->handleInstruction(instruction, ctx))
				return true;
			++i;
		}

		return false;
	}
}

INSTANTIATE(class, FingerprintStack);
