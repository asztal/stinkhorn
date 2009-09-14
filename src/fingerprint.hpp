#ifndef B98_FINGERPRINT_HPP_INCLUDED
#define B98_FINGERPRINT_HPP_INCLUDED

#include "stinkhorn.hpp"

#include "boost/shared_ptr.hpp"
#include "boost/intrusive_ptr.hpp"
#include <cassert>
#include <typeinfo>
#include <map>
#include <vector>

namespace stinkhorn {
	namespace {
		const unsigned int
			//Cat's Eye fingerprints
			TIMER_FINGERPRINT = 0x48525449,
			NULL_FINGERPRINT = 0x4e554c4c,
			TOYS_FINGERPRINT = 0x544f5953,
			REFC_FINGERPRINT = 0x52454643,
			ORTH_FINGERPRINT = 0x4F525448,
			MODU_FINGERPRINT = 0x4D4F4455,
			ROMA_FINGERPRINT = 0x524f4d41,
			//RC/Funge-98 fingerprints
			// http://www.elf-emulation.com/funge/rcsfingers.html
			_3DSP_FINGERPRINT = 0x33445350,
			ARRY_FINGERPRINT = 0x41525259,
			BASE_FINGERPRINT = 0x42415345,
			BOOL_FINGERPRINT = 0x424F4F4C,
			CPLI_FINGERPRINT = 0x43504C49,
			DATE_FINGERPRINT = 0x44415445,
			DIRF_FINGERPRINT = 0x44495246,
			EMEM_FINGERPRINT = 0x454d454d,
			EVAR_FINGERPRINT = 0x45564152,
			EXEC_FINGERPRINT = 0x45584543,
			FILE_FINGERPRINT = 0x46494C45,
			FING_FINGERPRINT = 0x46494e47,
			FNGR_FINGERPRINT = 0x464E4752,
			FOBJ_FINGERPRINT = 0x464f424a,
			FORK_FINGERPRINT = 0x464F524B,
			FPDP_FINGERPRINT = 0x46504450,
			FPRT_FINGERPRINT = 0x46505254,
			FPSP_FINGERPRINT = 0x46505350,
			FIXP_FINGERPRINT = 0x46495850,
			FRTH_FINGERPRINT = 0x46525448,
			ICAL_FINGERPRINT = 0x4943414c,
			IIPC_FINGERPRINT = 0x49495043,
			IMAP_FINGERPRINT = 0x494D4150,
			IMTH_FINGERPRINT = 0x494d5448,
			INDV_FINGERPRINT = 0x494E4456,
			LONG_FINGERPRINT = 0x4c4f4e47,
			MACR_FINGERPRINT = 0x4d414352,
			MSGQ_FINGERPRINT = 0x4d534751, //Page lists as 0x44d534751 but is probably a typo
			MVRS_FINGERPRINT = 0x4d565253,
			RAND_FINGERPRINT = 0x52414e44,
			REXP_FINGERPRINT = 0x52455850,
			SETS_FINGERPRINT = 0x53455453,
			SMEM_FINGERPRINT = 0x534d454d,
			SMPH_FINGERPRINT = 0x534d5048,
			SOCK_FINGERPRINT = 0x534F434B,
			SORT_FINGERPRINT = 0x534f5254,
			STCK_FINGERPRINT = 0x5354434b,
			STRN_FINGERPRINT = 0x5354524E,
			SUBR_FINGERPRINT = 0x53554252,
			TIME_FINGERPRINT = 0x54494D45,
			TERM_FINGERPRINT = 0x5445524D,
			TRDS_FINGERPRINT = 0x54524453,
			TRGR_FINGERPRINT = 0x54524752,
			WIND_FINGERPRINT = 0x57494E44;
	}

	/**
	 * A fingerprint must only provide a method of calling an instruction
	 * from the fingerprint.
	 */
	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::IFingerprint {
		///returns true if it handles the instruction, returns false otherwise.
		virtual bool handleInstruction(CellT instruction, Context& ctx) = 0;
		virtual IdT id() = 0;

		///Returns a list of handled instructions in no particular order.
		///It's expected that this stays constant throughout the fingerprint's lifetime.
		///Only instructions in the range A-Z should be included.
		//The built-in fingerprints don't need to store/load semantics.
		virtual char const* handledInstructions() {
			return "";
		}

		//Most fingerprints won't overload instructions outside A-Z, so it's needless to add them
		//to the stack for non-semantic instructions.
		virtual bool onlySemantics() {
			return true;
		}

		virtual bool is(IdT id) { 
			return id == this->id();				
		}

		void addRef() { 
			referenceCount++;
			assert(referenceCount); //Perhaps this should be in release mode too
		}

		unsigned long release() { 
			unsigned long count = --referenceCount;
			if(count == 0)
				delete this;
			return count;
		};

		IFingerprint() : referenceCount(1) {}

		virtual ~IFingerprint() {}
	
	private:
		unsigned long referenceCount;
	};

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::Befunge93Fingerprint 
		: public IFingerprint
	{
		bool handleInstruction(CellT instruction, Context& ctx);
		CellT askForDivideByZero();
		IdT id();
		bool onlySemantics();
		bool is(IdT id);
	};

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::Befunge98Fingerprint
		: public Befunge93Fingerprint 
	{
		bool handleInstruction(CellT instruction, Context& ctx);
	private:
		void doInfo(int info, Context& ctx, bool all, std::size_t initial_top_stack_size);
	};

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::TrefungeFingerprint
		: public Befunge98Fingerprint
	{
		bool handleInstruction(CellT instruction, Context& ctx);
	};

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::TimerFingerprint
		: public IFingerprint
	{
		TimerFingerprint();
		~TimerFingerprint();

		bool handleInstruction(CellT instruction, Context& ctx);
		IdT id();
		char const* handledInstructions();

		bool mark();
		CellT elapsedTime();

	private:
		struct state;
		std::auto_ptr<state> self;
	};

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::NullFingerprint
		: public IFingerprint
	{
		bool handleInstruction(CellT instruction, Context& ctx);
		IdT id();
	};

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::RomaFingerprint
		: public IFingerprint
	{
		bool handleInstruction(CellT instruction, Context& ctx);
		IdT id();
		char const* handledInstructions();
	};

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::RefcFingerprint
		: public IFingerprint
	{
		bool handleInstruction(CellT instruction, Context& ctx);
		IdT id();
		char const* handledInstructions();

		struct State;
	};

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::ToysFingerprint
		: public IFingerprint 
	{
		bool handleInstruction(CellT instruction, Context& ctx);
		IdT id();
		char const* handledInstructions();

		void copy(Context& ctx, bool low_order, bool erase);
		void chicane(Context& ctx);
		void move_line(Context& ctx, const Vector& movement_direction, CellT magnitude);
	};

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::OrthFingerprint
		: public IFingerprint
	{
		bool handleInstruction(CellT instruction, Context& ctx);
		IdT id();
		char const* handledInstructions();
	};

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::ModuFingerprint
		: public IFingerprint 
	{
		bool handleInstruction(CellT instruction, Context& ctx);
		IdT id();
		char const* handledInstructions();
	};

	//RC/Funge-98 fingerprints start here.
	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::BoolFingerprint
		: public IFingerprint 
	{
		bool handleInstruction(CellT instruction, Context& ctx);
		char const* handledInstructions();
		IdT id() { return BOOL_FINGERPRINT; }
	};

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::SockFingerprint
		: public IFingerprint 
	{
		SockFingerprint();
		~SockFingerprint();

		bool handleInstruction(CellT instruction, Context& ctx);
		char const* handledInstructions();
		IdT id() { return SOCK_FINGERPRINT; }
	};

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::StrnFingerprint
		: public IFingerprint
	{

		bool handleInstruction(CellT instruction, Context& ctx);
		char const* handledInstructions();
		IdT id() { return STRN_FINGERPRINT; }
	};

	template<class CellT, int Dimensions>
	inline unsigned long makeFingerprint(unsigned char a, unsigned char b, unsigned char c, unsigned char d) {
		typedef unsigned long ulong;
		return (ulong(a) << 24 | 
				ulong(b) << 16 | 
				ulong(c) << 8 | 
				ulong(d));
	}
	
	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::IFingerprintSource {
		virtual IFingerprint* createFingerprint(IdT id) = 0;
	};

	//Creates the default fingerprints, and is installed in the interpreter by default.
	template<class CellT, int Dimensions>
	class Stinkhorn<CellT, Dimensions>::DefaultFingerprintSource 
		: public IFingerprintSource
	{
	public:
		IFingerprint* createFingerprint(IdT id);
	};

	template<class CellT, int Dimensions>
	struct Stinkhorn<CellT, Dimensions>::IFingerprintState {
		virtual ~IFingerprintState() { }
	};

	template<class CellT, int Dimensions>
	class Stinkhorn<CellT, Dimensions>::FingerprintRegistry {
	public:
		void addSource(IFingerprintSource* source);
		void removeSource(IFingerprintSource* source);

		IFingerprint* createFingerprint(IdT id);

		//These refer to interpreter-global state. For IP-local state, this is not needed.
		boost::shared_ptr<IFingerprintState> stateForType(std::type_info const& type);
		void setStateForType(std::type_info const& type, boost::shared_ptr<IFingerprintState> const& value);

	private:
		std::map<std::type_info const*, boost::shared_ptr<IFingerprintState> > states;
		std::vector<IFingerprintSource*> sources;
	};
}

#endif
