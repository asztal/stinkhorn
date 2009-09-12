#include "fingerprint.hpp"
#include "cursor.hpp"
#include "context.hpp"
#include <vector>
#include <limits>

namespace stinkhorn {
	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::RefcFingerprint::State : IFingerprintState {
		std::vector<Vector> vectors;
	};

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::RefcFingerprint::handleInstruction(CellT instruction, Context &ctx) {
		FingerprintRegistry& registry = ctx.owner().interpreter().registry();
		boost::shared_ptr<IFingerprintState> state_ptr(registry.stateForType(typeid(State)));

		if(!state_ptr.get()) {
			state_ptr.reset(new State);
			registry.setStateForType(typeid(State), state_ptr);
		}

		State& state = dynamic_cast<State&>(*state_ptr);

		if(instruction == 'D') {
			//Take a scalar and convert it to a vector: O(1)
			CellT id = ctx.stack().pop();
			if((size_t)id > state.vectors.size() || id < 0)
				ctx.cursor().reflect();
			else
				ctx.stack().pushVector(state.vectors[static_cast<std::size_t>(id)], Dimensions);
		} else if(instruction == 'R') {
			//Take a vector and return a unique ID which can be used to retrieve that vector
			Vector vector = ctx.stack().popVector(Dimensions);

			std::size_t id = state.vectors.size();

			//Check if the new ID can fit into a cell, if not, reflect.
			if(CellT(id) > std::numeric_limits<CellT>::max()) {
				ctx.cursor().reflect();
			} else {
				state.vectors.push_back(vector);
				ctx.stack().push(static_cast<CellT>(id));
			}
		} else {
			return false;
		}

		return true;
	}

	template<class CellT, int Dimensions>
	IdT Stinkhorn<CellT, Dimensions>::RefcFingerprint::id() {
		return REFC_FINGERPRINT;
	}

	template<class CellT, int Dimensions>
	char const* Stinkhorn<CellT, Dimensions>::RefcFingerprint::handledInstructions() {
		return "DR";
	}
}

INSTANTIATE(struct, RefcFingerprint);
