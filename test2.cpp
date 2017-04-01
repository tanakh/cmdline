/*
Copyright (c) 2009, Hideyuki Tanaka
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the <organization> nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY <copyright holder> ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "cmdline.h"

#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
	cmdline::parser a;
	a.add<string>("host", 'h', "host name", true, "");
	a.add<int>("port", 'p', "port number", false, 80, cmdline::range(1, 65535));
	a.add<string>("type", 't', "protocol type", false, "http", cmdline::oneof<string>("http", "https", "ssh", "ftp"));
	a.add("help", 0, "print this message");
	a.footer("filename ...");
	a.set_program_name("test");

	bool ok = a.parse(argc, argv);

	if (argc == 1 || a.exist("help")) {
		cerr << a.usage();
		return 0;
	}

	if (!ok) {
		cerr << a.error() << endl << a.usage();
		return 0;
	}

	cout << a.get<string>("host") << ":" << a.get<int>("port") << endl;

	for (size_t i = 0; i < a.rest().size(); i++)
		cout << "- " << a.rest()[i] << endl;

	return 0;
}
