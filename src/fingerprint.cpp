#define _CRT_SECURE_NO_WARNINGS //I hate these messages :)

#include "fingerprint.hpp"
#include "context.hpp"
#include "octree.hpp"

#include <fstream>
#include <ctime>   //clock
#include <cctype>  //isupper
#include <cstring> //strstr
#include <algorithm>

using namespace boost;
using namespace std;

namespace stinkhorn {
	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::FingerprintRegistry::addSource(IFingerprintSource* source) {
		sources.push_back(source);
	}

	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::FingerprintRegistry::removeSource(IFingerprintSource* source) {
		typename vector<IFingerprintSource*>::iterator itr = remove(sources.begin(), sources.end(), source);
		sources.erase(itr, sources.end());
	}

	template<class CellT, int Dimensions>
	typename Stinkhorn<CellT, Dimensions>::IFingerprint* 
	Stinkhorn<CellT, Dimensions>::FingerprintRegistry::createFingerprint(IdT id) 
	{
		for(typename vector<IFingerprintSource*>::iterator itr = sources.begin();
			itr != sources.end();
			++itr)
		{
			IFingerprint* fp = (*itr)->createFingerprint(id);
			if(fp)
				return fp;
		}

		return 0;
	}

	template<class CellT, int Dimensions>
	shared_ptr<typename Stinkhorn<CellT, Dimensions>::IFingerprintState> 
	Stinkhorn<CellT, Dimensions>::FingerprintRegistry::stateForType(type_info const& type) {
		return this->states[&type];
	}

	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::FingerprintRegistry::setStateForType(type_info const& type, 
		shared_ptr<IFingerprintState> const& value) 
	{
		this->states[&type] = value;
	}

	template<class CellT, int Dimensions>
	typename Stinkhorn<CellT, Dimensions>::IFingerprint*
	Stinkhorn<CellT, Dimensions>::DefaultFingerprintSource::createFingerprint(IdT id) 
	{
		switch(id) {
			case TIMER_FINGERPRINT: return new TimerFingerprint;
			case NULL_FINGERPRINT: return new NullFingerprint;
			case ROMA_FINGERPRINT: return new RomaFingerprint;
			case TOYS_FINGERPRINT: return new ToysFingerprint;
			case ORTH_FINGERPRINT: return new OrthFingerprint;
			case MODU_FINGERPRINT: return new ModuFingerprint;
			case REFC_FINGERPRINT: return new RefcFingerprint;
			case BOOL_FINGERPRINT: return new BoolFingerprint;
			case SOCK_FINGERPRINT: return new SockFingerprint;
			case STRN_FINGERPRINT: return new StrnFingerprint;
		}
		return 0;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Befunge93Fingerprint::handleInstruction(CellT instruction, Context& ctx) {
		Cursor& cr = ctx.cursor();
		StackStack& stack = ctx.stack();

		switch(instruction) {
			case '@':
				ctx.setQuitFlag();
				return true;

			case '"':
				assert(!ctx.stringMode());
				ctx.stringMode(true);
				ctx.space(false);
				return true;

				//TODO: These will need tweaking to work in hover mode
			case '>':
				cr.direction(Vector(1, 0, 0));
				return true;

			case '<':
				cr.direction(Vector(-1, 0, 0));
				return true;

			case 'v':
				cr.direction(Vector(0, 1, 0));
				return true;

			case '^':
				cr.direction(Vector(0, -1, 0));
				return true;

			case '?': 
				{
					int r = rand();
					CellT mag = r % 2;
					mag = mag + mag - 1; //mag is now -1 or 1

					CellT d = (r >> 1) % Dimensions;
					if(d == 0)
						cr.direction(Vector(mag, 0, 0));
					else if(d == 1)
						cr.direction(Vector(0, mag, 0));
					else
						cr.direction(Vector(0, 0, mag));
					return true;
				}

			case '$':
				stack.pop();
				return true;

			case '#':
				cr.position(cr.position() + cr.direction());
				//cr.advance();
				return true;

			case '_':
				cr.direction(Vector( (stack.pop() ? -1 : 1 ), 0, 0) );
				return true;

			case '|':
				cr.direction(Vector( 0, (stack.pop() ? -1 : 1 ), 0) );
				return true;

			case '0': stack.push(0); return true;
			case '1': stack.push(1); return true;
			case '2': stack.push(2); return true;
			case '3': stack.push(3); return true;
			case '4': stack.push(4); return true;
			case '5': stack.push(5); return true;
			case '6': stack.push(6); return true;
			case '7': stack.push(7); return true;
			case '8': stack.push(8); return true;
			case '9': stack.push(9); return true;

			case '&': 
				{
					CellT x;
					cin >> x;

					if(cin)
						stack.push(x);
					else
						cr.reflect();

					return true;
				}

			case '~': 
				{
					CellT c = cin.get();
					if(cin)
						stack.push(c);
					else
						cr.reflect();
					return true;
				}

			case '.': 
				{
					cout << static_cast<long>(stack.pop()) << ' ';
					return true;
				}

			case ',':
				{
					cout << char(stack.pop());
					return true;
				}

			case '+':
				{
					//Note: CellT::operator+ should be commutative, or this is UB!
					stack.push(stack.pop() + stack.pop());
					return true;
				}

			case '*':
				{
					//Note: CellT::operator* should be commutative, or this is UB!
					stack.push(stack.pop() * stack.pop());
					return true;
				}

			case '-':
				{
					CellT b = stack.pop();
					stack.push(stack.pop() - b);
					return true;
				}

				/* Note: Befunge 98 will override this to return 0 on divide-by-zero. In
				 * befunge-93, the interpreter simply asks the user what he/she wants
				 * the answer to be. This applies to both / and %.
				 */
			case '/':
				{
					CellT b = stack.pop();
					if(b == 0) {
						stack.pop();
						stack.push(askForDivideByZero());
					} else {
						stack.push(stack.pop() / b);
					}
					return true;
				}

				/* Implementation note: the implementation of % is undefined if either
				 * argument is negative. Use the MODU fingerprint for defined semantics.
				 */
			case '%':
				{
					CellT b = stack.pop();
					if(b == 0) {
						stack.pop();
						stack.push(askForDivideByZero());
					} else {
						stack.push(stack.pop() % b);
					}
					return true;
				}

			case 'g':
				{
					CellT x, y;
					y = stack.pop() + ctx.storageOffset().y;
					x = stack.pop() + ctx.storageOffset().x;
					stack.push(cr.get(Vector(x, y, 0)));
					return true;
				}

			case 'p':
				{
					CellT x, y, value;
					y = stack.pop() + ctx.storageOffset().y;
					x = stack.pop() + ctx.storageOffset().x;
					value = stack.pop();
					cr.put(Vector(x, y, 0), value);
					return true;
				}

			case ':':
				{
					CellT c = stack.pop();
					stack.push(c);
					stack.push(c);
					return true;
				}

			case '\\':
				{
					CellT a, b;
					b = stack.pop();
					a = stack.pop();
					stack.push(b);
					stack.push(a);
					return true;
				}

			case '`':
				{
					CellT a, b;
					b = stack.pop();
					a = stack.pop();
					stack.push(a > b ? 1 : 0);
					return true;
				}

			case '!': 
				{
					stack.push(stack.pop() != 0 ? 0 : 1);
					return true;
				}
		}

		return false;
	}

	template<class CellT, int Dimensions>
	CellT Stinkhorn<CellT, Dimensions>::Befunge93Fingerprint::askForDivideByZero() {
		cerr << "A division by zero has occurred. What do you want the result of this calculation to be?\n";
		CellT x;
		cin >> x;
		return x;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Befunge93Fingerprint::onlySemantics() {
		return false;
	}

	template<class CellT, int Dimensions>
	IdT Stinkhorn<CellT, Dimensions>::Befunge93Fingerprint::id() {
		return 0;
	}

	//We don't want people to load '0' as a fingerprint and get a Befunge93Fingerprint back.
	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Befunge93Fingerprint::is(IdT id) {
		return false;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Befunge98Fingerprint::handleInstruction(CellT instruction, Context& ctx) {
		bool failure = false;
		Cursor& cr = ctx.cursor();
		StackStack& stack = ctx.stack();

		switch(instruction) {
			/* Now, I'm not entirely sure how # works in funge-98. The spec says:
			*     The # "Trampoline" instruction moves the IP one position beyond
			*     the next Funge-Space cell in its path.
			* So I'm just going to advance, then advance again.
			*/
			case '#':
				//cr.jump();
				cr.position(cr.position() + cr.direction());
				return true;

				/* A note about / and % in befunge 98. In befunge 93, when a division 
				* by zero occurred, the interpreter would ask the user what the result
				* should be. In befunge 98, the interpreter simply returns zero.
				*/
			case '/':
				{
					CellT b = stack.pop();
					if(b == 0) {
						stack.pop();
						stack.push(0);
					} else {
						stack.push(stack.pop() / b);
					}
					return true;
				}

				/* The behaviour of this operator is undefined when either of its arguments is negative.
				* Instead of being nasty and attempting to summon nasal demons, I shall just return do
				* it the easy way.
				*/
			case '%':
				{
					CellT b = stack.pop();
					if(b == 0) {
						stack.pop();
						stack.push(0);
					} else {
						stack.push(stack.pop() % b);
					}
					return true;
				}

			case '{':
				stack.pushStack(ctx.storageOffset(), Dimensions);

				//The spec says that this is what should be done. I'm not sure what the
				//execution engine will do if it lands on a cell with no instruction in it,
				//but I guess we'll find out. I think it will try to execute the space,
				//which won't happen. Then it will either ignore that or throw an exception
				//depending on what I decide to do.
				ctx.storageOffset(cr.direction() + cr.position());

				return true;

			case '}': 
				{
					Vector so;
					failure = !stack.popStack(so, Dimensions);
					ctx.storageOffset(so);

					if(!failure)
						return true;
					break;
				}

			case '\'':
				{
					cr.position(cr.position() + cr.direction());
					CellT c = cr.currentCharacter();
					stack.push(c);
					return true;
				}

			case 'a': stack.push(10); return true;
			case 'b': stack.push(11); return true;
			case 'c': stack.push(12); return true;
			case 'd': stack.push(13); return true;
			case 'e': stack.push(14); return true;
			case 'f': stack.push(15); return true;

				//This was implemented by advance()ing |n| steps (backwards if necessary)
				//But that may not have been the correct way to do it.
			case 'j': 
				{
					CellT n = stack.pop();
					bool backwards = n < 0;

					if(backwards) {
						n = -n;
						cr.direction(-cr.direction());
					}

					while(n --> 0)
						//cr.advance();
						cr.position(cr.position() + cr.direction()); // cr.advance(); //Changed to do what ccbi does.
					//Leaving as .advance() for now, because using normal move causes an infinite loop even earlier.

					if(backwards)
						cr.direction(-cr.direction());

					return true;
				}

				//This really, really needs tests, once I know what k *does*.
			case 'k': 
				{
					CellT count = stack.pop();
					if(count <= 0) {
						cr.position(cr.position() + cr.direction());
						return true;
					}

					Vector k_pos = cr.position();
					Vector initial_dir = cr.direction();

					//Possibly advance() depending on consensus
					Vector instruction_pos = cr.position() + cr.direction();
					CellT instruction = cr.get(cr.position() + cr.direction());

					Cursor find_cursor(cr);

					while(instruction == ' ' || instruction == ';') {
						find_cursor.advance();
						instruction = find_cursor.currentCharacter();
					}

					//This assert is relevant only if we advance() to the next instruction!
					//assert(instruction != ' ' && instruction != ';' && "checking argument to k");

					assert(cr.position() == k_pos);
					assert(instruction != ' ' && instruction != ';');

					while(count--)
						ctx.owner().execute(instruction);

					//This code is no longer necessary IF k does not move past the instruction.
					//if(k_pos == cr.position() && initial_dir == cr.direction())
					//	cr.position(instruction_pos);
					return true;
				}

			case 'n': 
				stack.clearTopStack();
				return true;

			//q quits every thread, but not necessarily every interpreter instance, 
			//which is why this could probably be a bit nasty.
			case 'q':
				//This does not call destructors, which might be a bad thing for fingerprints.
				//exit(static_cast<int>(stack.pop()));
				{
					QuitProgram q = { static_cast<int>(stack.pop()) };
					throw q;
				}

			case 'r':
				failure = true;
				break;

			case 's':
				{
					cr.advance();
					CellT c = stack.pop();
					cr.put(cr.position(), c);
					return true;
				}

			case 't': 
				{
					failure = true;
					break;
				}

			case 'u': 
				{
					CellT elements = stack.pop();
					if(stack.transfer(elements))
						return true;
					failure = true;
					break;
				}

			case 'x':
				{
					Vector delta;
					delta.y = stack.pop();
					delta.x = stack.pop();
					cr.direction(delta);
					return true;
				}

				//These should be handled by the trefunge98 fingerprint.
			case 'l': case 'm': case 'h':
				failure = true;
				break;

			case 'z':
				return true;

			case '[':
				{
					cr.direction(cr.leftwards90Z());
					return true;
				}

			case ']': 
				{
					cr.direction(cr.rightwards90Z());
					return true;
				}

			case 'w': 
				{
					CellT a, b = stack.pop();
					a = stack.pop();
					if(a > b)
						return cr.direction(cr.rightwards90Z()), true;
					else if(a < b)
						return cr.direction(cr.leftwards90Z()), true;
					return true;
				}

			case 'y': 
				{
					CellT info = stack.pop();
					size_t ss = stack.topStackSize();
					if(info <= 0) {
						int x = 20;
						while(x)
							doInfo(x--, ctx, true, ss);
						return true;
					} else {
						if(info >= 10) {
							int x = 20;
							while(x)
								doInfo(x--, ctx, true, ss);

							//info goes from 1 to whatever, nth is zero-based
							CellT save = stack.nth(static_cast<size_t>(info - 1)); //cast: info > 0
							stack.resizeTopStack(ss);
							stack.push(save);
							return true;
						} else {
							doInfo(static_cast<int>(info), ctx, false, 0); //cast: 0 < info <= 10
							return true;
						}
					}
				}

			case '(': 
				{
					IdT temp = 0;
					CellT count = stack.pop();

					if(count < 0) {
						failure = true;
						break;
					}

					while(count--) {
						temp *= 256;
						temp += stack.pop();
					}

					if(ctx.pushFingerprint(temp)) {
						stack.push(static_cast<CellT>(temp));
						stack.push(1);
					} else {
						failure = true;
						break;
					}

					return true;
				}

			case ')':
				{
					IdT temp = 0;
					CellT count = stack.pop();

					if(count < 0) {
						failure = true;
						break;
					}

					while(count--) {
						temp *= 256;
						temp += stack.pop();
					}

					//The corresponding ) "Unload Semantics" instruction unloads the semantics for a given fingerprint
					//from any or all of the instructions A to Z *(even if that fingerprint had never been loaded before)*.
					//Thus, unloading invalid fingerprints... is valid.
					if(temp == 0) {
						cr.reflect();
						return true;
					}

					if(ctx.popFingerprint(temp)) {
						return true;
					} else {
						failure = true;
					}
				}

			case 'i':
				{
					int tree_flags = 0, flags = 0;

					string filename;
					stack.readString(filename);

					flags = static_cast<int>(stack.pop()); //Only bottom few bits matter
					if(flags & 0x1)
						tree_flags |= Tree::FileFlags::binary;

					Vector location;
					if(Dimensions > 2)
						location.z = stack.pop();

					location.y = stack.pop();    
					location.x = stack.pop();
					
					//Always use binary, even if it's text mode, because we need to handle CR/CRLF/LF on all platforms.
					//Not sure how all ifstream implementations handle these.
					ios_base::openmode file_flags = ios_base::in | ios_base::binary;
					ifstream stream(filename.c_str(), file_flags);

					//Try include directories
					if(!stream.good() && !stream.eof()) {
						vector<string> const& dirs = ctx.owner().includeDirectories();
						vector<string>::const_iterator i = dirs.begin();
						failure = true;

						for(; i != dirs.end(); ++i) {
							string path = *i;
							if(path.size()) {
								if(path[path.length()-1] != '/')
									path += '/';

								path += filename;
								stream.clear(ios_base::goodbit);
								stream.open(path.c_str(), file_flags);
								if(stream.good()) {
									failure = false;
									break;
								}
							}
						}

						if(failure == true)
							break;    
					}

					Vector size;
					bool b = ctx.fungeSpace().read_file_into(location + ctx.storageOffset(), stream, tree_flags, size);
					stream.close();

					if(b) {
						//Now push the position/size onto the stack, minus {1, 1, 1}
						stack.push(size.x - 1);
						stack.push(size.y - 1);
						if(Dimensions > 2)
							stack.push(size.z - 1);

						stack.push(location.x);
						stack.push(location.y);
						if(Dimensions > 2)
							stack.push(location.z);

						return true;
					}

					failure = true;
				}

			case 'o':
				{
					Vector location, size(0, 0, 0);
					string filename;
					int flags = 0, tree_flags = 0;

					stack.readString(filename);
					flags = static_cast<int>(stack.pop());

					if(Dimensions > 2)
						location.z = stack.pop();
					location.y = stack.pop();    
					location.x = stack.pop();

					if(Dimensions > 2)
						size.z = stack.pop();
					size.y = stack.pop();    
					size.x = stack.pop();

					if(size.x < 0 || size.y < 0 || size.z < 0) {
						failure = true;
						break;
					}

					if(flags & 0x1)
						tree_flags |= Tree::FileFlags::binary;

					ios_base::openmode file_flags = 
						ios_base::out;
					if(flags & 0x1)
						file_flags |= ios_base::binary;

					ofstream stream(filename.c_str(), file_flags);

					if(!stream.good()) {
						//Try include directories
						vector<string> const& dirs = ctx.owner().includeDirectories();
						vector<string>::const_iterator i = dirs.begin();
						failure = true;

						for(; i != dirs.end(); ++i) {
							string path = *i;
							if(path.size()) {
								if(path[path.length()-1] != '/')
									path += '/';

								path += filename;
								stream.clear(ios_base::goodbit);
								stream.open(path.c_str(), file_flags);
								if(stream.good()) {
									failure = false;
									break;
								}
							}
						}

						if(failure == true)
							break;    
					}

					bool b = ctx.fungeSpace().write_file_from(location + ctx.storageOffset(), location + size, stream, tree_flags);
					stream.close();
					if(b)
						return true;

					failure = true;
				}

				//This should probably be moved to cursor::advance
			case ';': 
				{
					cr.teleport();
					return true;
				}

				//Also in the NULL fingerprint
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
			case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
			case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
			case 'V': case 'W': case 'X': case 'Y': case 'Z':
				failure = true;
				break;
		}

		if(failure) {
			cr.direction(-cr.direction());
			return true;
		}

		return this->Stinkhorn<CellT, Dimensions>::Befunge93Fingerprint::handleInstruction(instruction, ctx);
	}

	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::Befunge98Fingerprint::doInfo(int info, Context& ctx, bool all, size_t initial_top_stack_size) {
		StackStack& stack = ctx.stack();
		Cursor& cr = ctx.cursor();

		switch(info) {
			case 1: 
				{
					stack.push(0x2 | 0x4);
					return;
				}

			case 2: 
				{
					stack.push(sizeof(CellT) * CHAR_BIT / 8);
					return;
				}

			case 3: 
				{
					stack.push((CellT)0x5e5f5f5e); //^__^
					return;
				}

			case 4: 
				{
					//Allow 2 digits for minor version, 3 for revision: 9.99.999
					CellT version = B98_MAJOR_VERSION * 100000 + B98_MINOR_VERSION * 1000 + B98_REVISION;
					stack.push(version);
					return;
				}

			case 5: 
				{
					stack.push(0);
					return;
				}

			case 6: 
				{
	#ifdef WIN32
					stack.push('\\');
	#else
					stack.push('/');
	#endif
					return;
				}

			case 7: 
				{
					stack.push(Dimensions);
					return;
				}

			case 8: 
				{
					stack.push(1);
					return;
				}

			case 9: 
				{
					stack.push(0);
					return;
				}

			case 10: 
				{
					Vector p = cr.position();
					stack.push(p.x);
					stack.push(p.y);
					if(Dimensions > 2)
						stack.push(p.z);
					return;
				}

			case 11: 
				{
					Vector d = cr.direction();
					stack.push(d.x);
					stack.push(d.y);
					if(Dimensions > 2)
						stack.push(d.z);
					return;
				}

			case 12: 
				{
					Vector so = ctx.storageOffset();
					stack.push(so.x);
					stack.push(so.y);
					if(Dimensions > 2)
						stack.push(so.z);
					return;
				}

			case 13: 
				{
					Vector min, max;
					ctx.fungeSpace().get_minmax(min, max);

					stack.push(min.x);
					stack.push(min.y);
					if(Dimensions > 2)
						stack.push(min.z);
					return;
				}

			case 14: 
				{
					Vector min, max;
					ctx.fungeSpace().get_minmax(min, max);

					//The specification says that this point should be relative to the least point given by #13.
					//Before now, it had been giving absolute values, but the min of (-1, 1) in mycology masked
					//this bug.

					//It's supposed to be a size, however, so add 1 ({0, 0} -> {0, 0} is of size {1, 1}).
					//SPEC: Mycology seems to expect just using (max - min). Is this correct? The spec says it's
					//      "useful to give to the o instruction", so I'd assume it should be a size.
					Vector size = max - min; // + Vector(1, 1, 1);
					stack.push(size.x);
					stack.push(size.y);
					if(Dimensions > 2)
						stack.push(size.z);
					return;
				}

			case 15: 
				{
					time_t current_time = time(0);
					tm* local = localtime(&current_time);
					stack.push(
						local->tm_mday + 256 * ((local->tm_mon + 1) + 256 * (local->tm_year))
						);
					return;
				}

			case 16: 
				{
					time_t current_time = time(0);
					tm* local = localtime(&current_time);
					stack.push(
						local->tm_sec + 256 * (local->tm_min + 256 * (local->tm_hour))
						);
					return;
				}

			case 17: 
				{
					stack.push(CellT(stack.stackCount()));
					return;
				}

			case 18: 
				{
					vector<size_t> stack_sizes;
					stack.getStackSizes(back_inserter(stack_sizes));

					if(all)
						stack_sizes[0] = initial_top_stack_size;

					for(vector<size_t>::iterator i = stack_sizes.begin(); i != stack_sizes.end(); ++i)
						stack.push(CellT(*i));
					return;
				}

				
			case 19: 
				{
					stack.push(0);
					stack.push(0);

					vector<string> args;
					ctx.owner().interpreter().getArguments(args);
					for(string::size_type i = 0; i < args.size(); ++i) {
						string& arg = args[i];
						stack.push(0);
						for(string::size_type j = arg.length(); j; --j)
							stack.push(arg[j - 1]);
					}
					return;
				}

			//Environment variables (8-bit)
			case 20: 
				{
					char** envp = ctx.owner().interpreter().environment();
					
					stack.push(0);

					for(; *envp; ++envp) {
						char* entry = *envp;
						if(!entry)
							break;

						size_t length = strlen(entry);
						for(size_t i = 0; i <= length; ++i) //Include the null terminator.
							stack.push(entry[length - i]);
					}

					return;
				}
		}
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::TrefungeFingerprint::handleInstruction(CellT instruction, Context& ctx) {
		bool failure = false;
		Cursor& cr = ctx.cursor();
		StackStack& stack = ctx.stack();

		switch(instruction) {
			case 'h': cr.direction(Vector(0, 0, 1)); return true;
			case 'l': cr.direction(Vector(0, 0, -1)); return true;
			case 'm':
				{
					CellT c = stack.pop();
					cr.direction(Vector(0, 0, c ? 1 : -1));
					return true;
				}
		}

		if(failure) {
			cr.direction(-cr.direction());
			return true;
		}

		return this->Stinkhorn<CellT, Dimensions>::Befunge98Fingerprint::handleInstruction(instruction, ctx);
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::NullFingerprint::handleInstruction(CellT instruction, Context& ctx) {
		if(isupper(static_cast<int>(instruction))) {
			if(strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ", static_cast<int>(instruction))) {
				ctx.cursor().reflect();
				return true; 
			}
		}
		return false;
	}

	template<class CellT, int Dimensions>
	IdT Stinkhorn<CellT, Dimensions>::NullFingerprint::id() {
		return NULL_FINGERPRINT;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::RomaFingerprint::handleInstruction(CellT instruction, Context& ctx) {
		if(isupper(static_cast<int>(instruction))) { //cast: doesn't matter
			StackStack& stack = ctx.stack();

			switch(instruction) {
				case 'C':
					stack.push(100);
					return true;

				case 'D':
					stack.push(500);
					return true;

				case 'I':
					stack.push(1);
					return true;

				case 'L':
					stack.push(50);
					return true; 

				case 'M':
					stack.push(1000);
					return true;   

				case 'V':
					stack.push(5);
					return true;

				case 'X':
					stack.push(10);
					return true;
			}
		}

		return false;
	}

	template<class CellT, int Dimensions>
	IdT Stinkhorn<CellT, Dimensions>::RomaFingerprint::id() {
		return ROMA_FINGERPRINT;
	}

	template<class CellT, int Dimensions>
	char const* Stinkhorn<CellT, Dimensions>::RomaFingerprint::handledInstructions() {
		return "CDILMVX";
	}
}

INSTANTIATE(class, FingerprintRegistry);
INSTANTIATE(struct, NullFingerprint);
INSTANTIATE(struct, RomaFingerprint);
INSTANTIATE(struct, Befunge93Fingerprint);
INSTANTIATE(struct, Befunge98Fingerprint);
INSTANTIATE(struct, TrefungeFingerprint);
INSTANTIATE(struct, IFingerprint);
INSTANTIATE(struct, IFingerprintState);
INSTANTIATE(class, DefaultFingerprintSource);
