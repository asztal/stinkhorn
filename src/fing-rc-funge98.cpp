#include "fingerprint.hpp"
#include "context.hpp"
#include "octree.hpp"

#ifdef B98_WINDOWS
#	include <winsock.h> //Note: boost.mutex (used in stack pooling) includes winsock.h, but not winsock2.h
#	ifdef B98_MSVC
#	pragma comment(lib, "ws2_32.lib")
#	endif
typedef int socklen_t;
#else
//This will probably fail for now due to missing stuff. I will fix this later.
#include <sys/socket.h> 
#include <netinet/ip.h> 
#include <netinet/tcp.h> 
#include <arpa/inet.h>
typedef int SOCKET;
#endif

#include <boost/scoped_array.hpp>
#include <iostream>
#include <string>
#include <sstream>

namespace stinkhorn {
	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::BoolFingerprint::handleInstruction(CellT instruction, Context& ctx) 
	{
		StackStackT& stack = ctx.stack();
		switch(instruction) {
			case 'A': stack.push(stack.pop() && stack.pop()); return true;
			case 'N': stack.push(!stack.pop()); return true;
			case 'O': stack.push(stack.pop() || stack.pop()); return true;
			case 'X': stack.push((!!stack.pop()) ^ (!!stack.pop())); return true;
		}
		return false;
	}

	template<class CellT, int Dimensions>
	char const* Stinkhorn<CellT, Dimensions>::BoolFingerprint::handledInstructions() {
		return "ANOX";
	}

	template<class CellT, int Dimensions>
	Stinkhorn<CellT, Dimensions>::SockFingerprint::SockFingerprint() {
#ifdef B98_WINDOWS
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
	}

	template<class CellT, int Dimensions>
	Stinkhorn<CellT, Dimensions>::SockFingerprint::~SockFingerprint() {
#ifdef B98_WINDOWS
		WSACleanup();
#endif
	}

	template<class CellT, int Dimensions>
	char const* Stinkhorn<CellT, Dimensions>::SockFingerprint::handledInstructions() {
		return "ABCIKLORSW";
	}

	namespace {
		//TODO: These should probably do something slightly more clever, surely?
		//(That is, in the case when CellT and SOCKET are different sizes)
		template<class CellT>
		SOCKET socketFromCell(CellT cell) {
			return cell;
		}

		template<class CellT>
		CellT cellFromSocket(SOCKET socket) {
			return socket;
		}
	}

	//Notes:
	//Some validation is performed by the fingerprint itself (e.g. port > 65535), but most is left to the OS to handle.
	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::SockFingerprint::handleInstruction(CellT instruction, Context& ctx) 
	{
		StackStackT& stack = ctx.stack();
		Cursor& cr = ctx.cursor();
		Vector storage_offset = ctx.storageOffset();

		switch(instruction) {
			case 'A':
			{
				SOCKET socket = socketFromCell(stack.pop());
				sockaddr_in addr = {0};
				socklen_t addrlen = sizeof(addr);
				socket = ::accept(socket, reinterpret_cast<sockaddr*>(&addr), &addrlen);
				if(socket != -1) {
					stack.push(addr.sin_port);
					stack.push(addr.sin_addr.s_addr);
					stack.push(cellFromSocket<CellT>(socket));
				} else {
					cr.reflect();
				}
				return true;
			}

			case 'B': case 'C':
			{
				sockaddr_in addr = {0};
				CellT family, port, address = stack.pop();			
				
				port = stack.pop();
				if(port < 0 || port > 65535) 
					{ cr.reflect(); return true; }
				
				family = stack.pop();
				if(family == 1)
					addr.sin_family = AF_UNIX;
				else if(family == 2)
					addr.sin_family = AF_INET;
				else
					{ cr.reflect(); return true; }
				
				SOCKET socket = socketFromCell(stack.pop());
				
				addr.sin_family = family;
				addr.sin_port = port;
				addr.sin_addr.s_addr = address;

				if(-1 == (instruction == 'B' ? ::bind : ::connect)(socket, reinterpret_cast<sockaddr*>(&addr), sizeof addr))
					cr.reflect();
				return true;
			}

			case 'I':
			{
				std::string address;
				stack.readString(address);

#ifdef B98_WINDOWS
				unsigned long addr = inet_addr(address.c_str());
				//inet_addr can return either of these depending on what the OS is.
				if(addr == INADDR_NONE || addr == INADDR_ANY)
					cr.reflect();
				else
					stack.push(static_cast<CellT>(addr));

#else
                sockaddr_in addr = { 0 };
				if(1 == ::inet_pton(AF_INET, address.c_str(), &addr))
					stack.push(addr.sin_addr.s_addr);
				else
					cr.reflect();
#endif
				return true;
			}

			case 'K':
			{
				SOCKET socket = socketFromCell(stack.pop());
				int rv = 
#ifdef B98_WINDOWS
				::closesocket(socket);
#else
				::close(socket);
#endif
				if(rv != 0)
					cr.reflect();
				return true;
			}

			case 'L':
			{
				SOCKET socket = socketFromCell(stack.pop());
				int backlog = stack.pop();
				if(0 != ::listen(socket, static_cast<int>(backlog)))
					cr.reflect();
				return true;
			}

			case 'O':
			{
				SOCKET socket = socketFromCell(stack.pop());
				int option = static_cast<int>(stack.pop());
				int value = static_cast<int>(stack.pop());

				if(option == 1) option = SO_DEBUG;
				else if(option == 2) option = SO_REUSEADDR;
				else if(option == 3) option = SO_KEEPALIVE;
				else if(option == 4) option = SO_DONTROUTE;
				else if(option == 5) option = SO_BROADCAST;
				else if(option == 6) option = SO_OOBINLINE;
				else { cr.reflect(); return true; }
				
				//It should be void* but windows uses const char*...
				if(-1 == setsockopt(socket, SOL_SOCKET, option, reinterpret_cast<const char*>(&value), sizeof(value)))
					cr.reflect();
				return true;
			}

			case 'R':
			{
				SOCKET socket = socketFromCell(stack.pop());
				int length = static_cast<int>(stack.pop());
				Vector to = stack.popVector(Dimensions) + storage_offset;

				if(length < 0)
					{ cr.reflect(); return true; }

				boost::scoped_array<unsigned char> data(new unsigned char[length]);
				int bytes = recv(socket, reinterpret_cast<char*>(data.get()), length, 0);

				if(bytes >= 0) {
					stack.push(bytes);
					Cursor putter(cr);
					putter.position(to);
					for(int i = 0; i < bytes; ++i) {
						putter.put(putter.position(), data[i]);
						putter.position(putter.position() + Vector(1, 0, 0));
					}
				} else 
					cr.reflect();

				return true;
			}

			case 'S':
			{
				int protocol = static_cast<int>(stack.pop());
				int type = static_cast<int>(stack.pop());
				int pf = static_cast<int>(stack.pop());

				if(pf == 1) pf = PF_UNIX;
				else if(pf == 2) pf = PF_INET;
				else { cr.reflect(); return true; }

				if(type == 1) type = SOCK_DGRAM;
				else if(type == 2) type = SOCK_STREAM;
				else { cr.reflect(); return true; }

				if(protocol == 1) protocol = IPPROTO_TCP;
				else if(protocol == 2) protocol = IPPROTO_UDP;
				else { cr.reflect(); return true; }

				SOCKET socket = ::socket(pf, type, protocol);
				if(socket != -1)
					stack.push(cellFromSocket<CellT>(socket));
				else
					cr.reflect();
				return true;
			}

			case 'W': 
			{
				SOCKET socket = socketFromCell(stack.pop());
				int length = static_cast<int>(stack.pop());
				Vector from = stack.popVector(Dimensions) + storage_offset;

				boost::scoped_array<unsigned char> data(new unsigned char[length]);
				Cursor getter(cr);
				getter.position(from);
				for(int i = 0; i < length; ++i) {
					data[i] = getter.get(getter.position());
					getter.position(getter.position() + Vector(1, 0, 0));
				}

				int bytes = ::send(socket, reinterpret_cast<char*>(data.get()), length, 0);
				if(bytes < 0)
					cr.reflect();
				else
					stack.push(bytes);
				return true;
			}
		}

		return false;
	}

	template<class CellT, int Dimensions>
	char const* Stinkhorn<CellT, Dimensions>::StrnFingerprint::handledInstructions() 
	{
		return "ACDFGILMNPRSV";
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::StrnFingerprint::handleInstruction(CellT instruction, Context& ctx) {
		StackStackT& stack = ctx.stack();
		Cursor& cr = ctx.cursor();

		switch(instruction) {
			case 'A': 
			{
				//This is probably quite optimisable by sticking it inside of the StackStack instead of here.
				String first, second;
				stack.readString(first);
				stack.readString(second);
				first.append(second);
				stack.pushString(first);
				return true;
			}

			case 'C':
			{
				String first, second;
				stack.readString(first);
				stack.readString(second);
				stack.push(first.compare(second));
				return true;
			}

			case 'D':
			{
				String str;
				stack.readString(str);
				writeString(std::cout, str);
				return true;
			}

			case 'F':
			{
				String first, second;
				stack.readString(first);
				stack.readString(second);
				typename String::size_type i = first.find(second);
				if(i == String::npos) {
					stack.push(0);
				} else {
					stack.pushString(first.substr(i));
				}
				return true;
			}

			case 'G': 
			{
				Cursor getter(cr);
				getter.position(stack.popVector(Dimensions) + ctx.storageOffset());
				String str;
				Vector min, max;
				ctx.fungeSpace().get_minmax(min, max);

				while(true) {
					Vector pos = getter.position();

					//Guard against going off the edge of funge-space.
					//Required to get to the end of mycology, even though it is UNDEF.
					if(pos.x > max.x) {
						cr.reflect();
						return true;
					}

					CellT c = getter.currentCharacter();
					if(c)
						str += c;
					else
						break;
					getter.position(pos + Vector(1, 0, 0));
				}
				stack.pushString(str);
				return true;
			}

			case 'I': 
			{
				std::string in;
				getline(std::cin, in);
				if(!std::cin) {
					cr.reflect();
				} else {
					String str(in.begin(), in.end());
					stack.pushString(str);
				}
				return true;
			}

			case 'L':
			{
				String str;
				CellT n = stack.pop();
				stack.readString(str);
				if(n < 0) {
					cr.reflect();
				} else {
					str = str.substr(0, n);
					stack.pushString(str);
				}
				return true;
			}

			case 'M':
			{
				String str;
				CellT n = stack.pop();
				CellT s = stack.pop();
				stack.readString(str);

				//000M is self-contradictory - it should both return 0 (because we are requesting zero characters)
				//and reflect (because the start is beyond the end of the string). However, since the behaviour is
				//not terribly well-defined anyway, it's probably better to choose returning the empty string.
				if(n < 0 || s < 0 || (!str.empty() && s >= (CellT)str.size())) {
					cr.reflect();
				} else {
					str = str.substr(s, n);
					stack.pushString(str);
				}
				return true;
			}

			case 'N':
			{
				stack.push(stack.strnGetLength());
				return true;
			}

			case 'P':
			{
				Vector to = stack.popVector(Dimensions);
				String str;
				stack.readString(str);
				
				Cursor putter(cr);
				putter.position(to + ctx.storageOffset());

				for(typename String::size_type i = 0; i < str.size(); ++i) {
					putter.put(putter.position(), str[i]);
					putter.position(putter.position() + Vector(1, 0, 0));
				}
				putter.put(putter.position(), 0);
				return true;
			}

			case 'R':
			{
				CellT n = stack.pop();
				String str;
				stack.readString(str);

				if(n < 0) {
					cr.reflect();
				} else {
					str = str.substr(std::max<typename String::size_type>(str.size() - n, 0));
					stack.pushString(str);
				}

				return true;
			}

			case 'S':
			{
				std::ostringstream stream;
				//Promote from CellT to long, in case CellT is char (in which case, it would be formatted differently)
				stream << stack.pop() + 0L;
				std::string str = stream.str(); //std::ctype<int> might not exist
				stack.pushString(String(str.begin(), str.end()));
				return true;
			}

			case 'V':
			{
				String str;
				stack.readString(str);
				std::string str_char(str.begin(), str.end()); //std::ctype<int> might not exist?
				std::istringstream stream(str_char);
				CellT x;
				stream >> x;
				if(stream)
					stack.push(x);
				else
					cr.reflect();
				return true;
			}
		}

		return false;
	}
}

INSTANTIATE(struct, BoolFingerprint);
INSTANTIATE(struct, SockFingerprint);
INSTANTIATE(struct, StrnFingerprint);
