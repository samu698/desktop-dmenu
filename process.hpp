#pragma once

#include <streambuf>
#include "utils.hpp"

namespace detail {
class iopipes : std::streambuf, public std::iostream {
	using traits = std::streambuf::traits_type;

	union PipeFds {
		struct { int readEnd, writeEnd; };
		int fds[2];

		PipeFds();
		void close();
		~PipeFds();
	} ipipe, opipe;

	char buffer[1024];
public:
	iopipes();

	void sendEOF();
	void close();
	void closeUnneded();
	void dup();
protected:
	virtual traits::int_type overflow(traits::int_type c);
	virtual std::streamsize xsputn(const char* data, std::streamsize len);
	virtual int underflow();
};

class StringList {
	std::vector<std::string_view> strings;
	std::vector<const char*> cstrings;
public:
	StringList(std::vector<std::string_view> args);
	const char** carray();
};
}

class Process {
	std::string file;
	detail::StringList argv;
	int pid;
	detail::iopipes pipes;
public:
	Process(std::string_view file, std::string_view arg0, std::vector<std::string_view> args = {});
	Process(std::string_view file, std::vector<std::string_view> args = {});

	void run();
	void exec();
	detail::iopipes& stream();
	int join();
};
