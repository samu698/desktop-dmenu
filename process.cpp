#include "process.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include <streambuf>
#include <iostream>
#include <initializer_list>

#include <sys/wait.h>
#include <cstring>
#include <unistd.h>

namespace detail {

// ==========================================
// iopipes::PipeFds
// ==========================================

iopipes::PipeFds::PipeFds() { pipe(fds); }
void iopipes::PipeFds::close() { ::close(readEnd); ::close(writeEnd); }
iopipes::PipeFds::~PipeFds() { close(); }

// ==========================================
// iopipes
// ==========================================

iopipes::iopipes() : std::iostream(this) { setg(buffer, buffer, buffer); }
void iopipes::sendEof() { opipe.close(); }
void iopipes::close() { ipipe.close(); opipe.close(); }
void iopipes::closeUnneded() { ::close(ipipe.writeEnd); ::close(opipe.readEnd); }
void iopipes::dup() {
	dup2(ipipe.writeEnd, STDOUT_FILENO);
	dup2(opipe.readEnd, STDIN_FILENO);
	close();
}
iopipes::traits::int_type iopipes::overflow(traits::int_type c) {
	if (c == traits::eof()) return traits::eof();
	if (::write(opipe.writeEnd, &c, 1) != 1) return traits::eof();
	return c;
}
std::streamsize iopipes::xsputn(const char* data, std::streamsize len) {
	ssize_t writeret = ::write(opipe.writeEnd, data, len);
	return writeret == -1 ? traits::eof() : writeret;
}
int iopipes::underflow() {
	if (gptr() == egptr()) {
		ssize_t readCount = ::read(ipipe.readEnd, gptr(), sizeof buffer);
		if (readCount <= 0) return traits::eof();
		setg(buffer, buffer, buffer + readCount);
	}
	return traits::to_int_type(*gptr());
}

// ==========================================
// StringList
// ==========================================

StringList::StringList(std::vector<std::string_view> args) : strings(args) {
	cstrings.reserve(strings.size() + 1);
	for (int i = 0; i < strings.size(); i++) cstrings.push_back(strings[i].data());
	cstrings.push_back(nullptr);
}
const char** StringList::carray() { return cstrings.data(); }
}

// ==========================================
// Process
// ==========================================

std::vector<std::string_view> Process::prepend(std::vector<std::string_view> vec, std::string_view val) {
	vec.insert(std::begin(vec), val);
	return vec;
}
Process::Process(std::string_view file, std::string_view arg0, std::vector<std::string_view> args) : 
	file(file), argv(prepend(args, std::string(arg0))) {}
Process::Process(std::string_view file, std::vector<std::string_view> args) : 
	file(file), argv(prepend(args, std::string(file))) {}
void Process::run() {
	int exitCode;
	pid = fork();
	if (pid == 0) {
		pipes.dup();
		int ret = execvp(file.c_str(), (char**)argv.carray());
		exit(1);
	}
	pipes.closeUnneded();
}
void Process::exec() {
	execvp(file.c_str(), (char**)argv.carray());
	throw std::runtime_error("Cannot exec process");
}
detail::iopipes& Process::stream() {
	return pipes;
}
int Process::join() {
	int exitCode;
	pipes.close();
	waitpid(pid, &exitCode, 0);
	return exitCode;
}
