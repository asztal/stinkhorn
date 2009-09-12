#include "fingerprint.hpp"
#include "context.hpp"
#include <ctime>

#if defined(WIN32) && !defined(B98_WIN32_DONT_USE_QPC)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
namespace stinkhorn {

#if defined(WIN32) && !defined(B98_WIN32_DONT_USE_QPC)
	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::TimerFingerprint::state {
		LARGE_INTEGER frequency, marked;
		bool is_marked;
		bool is_supported;

		state() {
			is_marked = false;
			is_supported = !!::QueryPerformanceFrequency(&frequency);
		}
	};
	
	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::TimerFingerprint::mark() {
		if(!QueryPerformanceCounter(&self->marked))
			return false;

		self->is_marked = true;
		return true;
	}

	template<class CellT, int Dimensions>
	CellT Stinkhorn<CellT, Dimensions>::TimerFingerprint::elapsedTime() {
		if(!self->is_marked)
			return -1;
		
		LARGE_INTEGER now;
		if(!QueryPerformanceCounter(&now))
			return -1;
		
		//This will most likely be garbage if the difference is too high.
		//Shouldn't be a problem with 64-bit funge though :)
		LONGLONG value = 1000000 * (now.QuadPart - self->marked.QuadPart) / self->frequency.QuadPart;
		return static_cast<CellT>(value);
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::TimerFingerprint::handleInstruction(CellT instruction, Context& ctx) {    
		StackStackT& stack = ctx.stack();

		switch(instruction) {
			case 'G':
				{
					if(!self->is_supported) {
						ctx.cursor().reflect();
						return true;
					}

					CellT granularity = static_cast<CellT>(1000000 / self->frequency.QuadPart);
					if(granularity == 0)
						granularity = 1; //Granularity can be zero if the frequency > 1e6 (which it usually is)

					stack.push(granularity);
					return true;
				}

			case 'M': 
				{
					mark();
					return true;
				}

			case 'T': 
				{
					CellT time = elapsedTime();
					if(time == -1)
						ctx.cursor().reflect();
					else
						stack.push(time);
					return true;
				}

			case 'E':
				{
					self->is_marked = false;
					return true;
				}

			case 'S': 
				{
					LARGE_INTEGER now;
					if(!QueryPerformanceCounter(&now)) {
						ctx.cursor().reflect();
						return true;
					}

					stack.push(static_cast<CellT>((now.QuadPart * 1000000 / self->frequency.QuadPart) % 1000000));
					return true;
				}
		}

		return false;
	}

	#else

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::TimerFingerprint::state {
		clock_t marked;
		bool is_marked;

		state() {
			is_marked = false;
		}
	};
	
	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::TimerFingerprint::mark() {
		self->is_marked = true;
		self->marked = clock();
		return true;
	}

	template<class CellT, int Dimensions>
	CellT Stinkhorn<CellT, Dimensions>::TimerFingerprint::elapsedTime() {
		if(!self->is_marked)
			return -1;
		
		return 1000000 / CLOCKS_PER_SEC * (clock() - self->marked);
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::TimerFingerprint::handleInstruction(CellT instruction, Context& ctx) {    
		StackStackT& stack = ctx.stack();

		switch(instruction) {
			case 'G':
				{
					stack.push(std::min<CellT>(1, 1000000 / CLOCKS_PER_SEC));
					return true;
				}

			case 'M': 
				{
					mark();
					self->marked = clock();
					return true;
				}

			case 'T': 
				{
					CellT time = elapsedTime();
					if(time != -1)
						stack.push(time);
					else
						ctx.cursor().reflect();
					return true;
				}

			case 'E':
				{
					self->is_marked = false;
					return true;
				}

			case 'S': 
				{
					clock_t clocks = clock() % CLOCKS_PER_SEC;
					stack.push(clocks * 1000000 / CLOCKS_PER_SEC);
					return true;
				}
		}

		return false;
	}

	#endif

	template<class CellT, int Dimensions>
	Stinkhorn<CellT, Dimensions>::TimerFingerprint::TimerFingerprint() {
		self.reset(new state);
	}

	template<class CellT, int Dimensions>
	Stinkhorn<CellT, Dimensions>::TimerFingerprint::~TimerFingerprint() {
	}

	template<class CellT, int Dimensions>
	IdT Stinkhorn<CellT, Dimensions>::TimerFingerprint::id() {
		return TIMER_FINGERPRINT;
	}

	template<class CellT, int Dimensions>
	char const* Stinkhorn<CellT, Dimensions>::TimerFingerprint::handledInstructions() {
		return "GMTES";
	}

}

INSTANTIATE(struct, TimerFingerprint);
