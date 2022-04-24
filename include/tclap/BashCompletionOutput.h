// -*- Mode: c++; c-basic-offset: 4; tab-width: 4; -*-

/******************************************************************************
 *
 *  file:  BashCompletionOutput.h
 *
 *  Original Zsh version Copyright (c) 2006, Oliver Kiddle
 *  and Copyright (c) 2017 Google Inc.
 *  Modified for Bash in 2022 by VintagePC <http://github.com/vintagepc>
 *  All rights reserved.
 *
 *  See the file COPYING in the top directory of this distribution for
 *  more information.
 *
 *  THE SOFTWARE IS PROVIDED _AS IS_, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <map>

#include <tclap/CmdLineInterface.h>
#include <tclap/CmdLineOutput.h>
#include <tclap/XorHandler.h>
#include <tclap/Arg.h>
#include <tclap/sstream.h>

namespace TCLAP {

/**
 * A class that generates a Zsh completion function as output from the usage()
 * method for the given CmdLine and its Args.
 */
class BashCompletionOutput : public CmdLineOutput
{

	public:

		BashCompletionOutput();

		/**
		 * Prints the usage to stdout.  Can be overridden to
		 * produce alternative behavior.
		 * \param c - The CmdLine object the output is generated for.
		 */
		virtual void usage(CmdLineInterface& c);

		/**
		 * Prints the version to stdout. Can be overridden
		 * to produce alternative behavior.
		 * \param c - The CmdLine object the output is generated for.
		 */
		virtual void version(CmdLineInterface& c);

		/**
		 * Prints (to stderr) an error message, short usage
		 * Can be overridden to produce alternative behavior.
		 * \param c - The CmdLine object the output is generated for.
		 * \param e - The ArgException that caused the failure.
		 */
		virtual void failure(CmdLineInterface& c,
						     ArgException& e );

	protected:

		void basename( std::string& s );
		void quoteSpecialChars( std::string& s );

		std::string getMutexList( CmdLineInterface& _cmd, Arg* a );
		void printOption( Arg* it, std::string mutex );
		void printArg( Arg* it );

		std::map<std::string, std::string> common;
		char theDelimiter;
};

BashCompletionOutput::BashCompletionOutput()
: common(std::map<std::string, std::string>()),
  theDelimiter('=')
{
	common["host"] = "_hosts";
	common["hostname"] = "_hosts";
	common["file"] = "_files";
	common["filename"] = "_files";
	common["user"] = "_users";
	common["username"] = "_users";
	common["directory"] = "_directories";
	common["path"] = "_directories";
	common["url"] = "_urls";
}

inline void BashCompletionOutput::version(CmdLineInterface& _cmd)
{
	std::cout << _cmd.getVersion() << std::endl;
}

inline void BashCompletionOutput::usage(CmdLineInterface& _cmd )
{
	std::list<Arg*> argList = _cmd.getArgList();
	std::string progName = _cmd.getProgramName();
	std::string xversion = _cmd.getVersion();
	theDelimiter = _cmd.getDelimiter();
	basename(progName);

	std::cout << "#Bash completion for  " << progName << std::endl << std::endl <<
		"# " << progName << " version " << _cmd.getVersion() << std::endl << std::endl;

	std::cout << "_MK404_opts()\n{\n\tcase $prev in\n";

	for (ArgListIterator it = argList.begin(); it != argList.end(); it++)
	{
		if ( (*it)->shortID().at(0) == '<' )
			continue;
		if ( (*it)->getFlag() != "-" )
			printOption((*it), getMutexList(_cmd, *it));
	}

	std::cout << "\tesac\n\treturn 1\n}" << std::endl;
	std::cout << "_MK404()\n{\n";
	std::cout << "\tlocal cur prev words cword\n";
    std::cout << "\t_init_completion || return\n";
	std::cout << "\t_MK404_opts && return\n";
    std::cout << "\tif [[ \"$cur\" == -* ]]; then\n";
	std::cout << "\t\tCOMPREPLY=( $( compgen -W '$( _parse_help \"$1\" --help )' -- \"$cur\" ) )\n";
    std::cout << "\telse\n";
    std::cout << "\t\tCOMPREPLY=( \\\n\t\t";
	for (ArgListIterator it = argList.begin(); it != argList.end(); it++)
	{
		if ( (*it)->shortID().at(0) == '<' )
			printArg((*it));
	}
	std::cout << " )\n";
    std::cout << "\tfi\n";
	std::cout << "} &&\n";
	std::cout << "complete -F _MK404 MK404\n";
}

inline void BashCompletionOutput::failure( CmdLineInterface& _cmd,
				                ArgException& e )
{
	static_cast<void>(_cmd); // unused
	std::cout << e.what() << std::endl;
}

inline void BashCompletionOutput::quoteSpecialChars( std::string& s )
{
	size_t idx = s.find_last_of(':');
	while ( idx != std::string::npos )
	{
		s.insert(idx, 1, '\\');
		idx = s.find_last_of(':', idx);
	}
	idx = s.find_last_of('\'');
	while ( idx != std::string::npos )
	{
		s.insert(idx, "'\\'");
		if (idx == 0)
			idx = std::string::npos;
		else
			idx = s.find_last_of('\'', --idx);
	}
}

inline void BashCompletionOutput::basename( std::string& s )
{
	size_t p = s.find_last_of('/');
	if ( p != std::string::npos )
	{
		s.erase(0, p + 1);
	}
}

inline void BashCompletionOutput::printArg(Arg* a)
{
	auto arg = a->shortID();
	if ( arg.at(0) == '<' )
	{
		arg.erase(arg.length()-1);
		arg.erase(0, 1);
	}
	size_t p = arg.find('|');
	if ( p != std::string::npos )
	{
		do
		{
			arg.replace(p, 1, 1, ' ');
		}
		while ( (p = arg.find_first_of('|', p)) != std::string::npos );
		quoteSpecialChars(arg);
		std::cout << " $( compgen -W '" << arg << "' -- \"$cur\" ) ";
	}
}

inline void BashCompletionOutput::printOption(Arg* a, std::string mutex)
{

	if (! a->isValueRequired())
	{
		return;
	}

	std::string flag = a->flagStartChar() + a->getFlag();
	std::string name = a->nameStartString() + a->getName();
	std::string desc = a->getDescription();

	std::cout << "\t\t";
	if (! a->getFlag().empty() )
	{
		std::cout << "-" << a->getFlag() << "|";
	}

	std::cout << name << ")\n";

	std::string arg = a->shortID();
	// Example arg: "[-A <integer>] ... "
	size_t pos = arg.rfind(" ... ");
	if (pos != std::string::npos) {
		arg.erase(pos);
	}

	arg.erase(0, arg.find_last_of(theDelimiter) + 1);
	if ( arg.at(arg.length()-1) == ']' )
		arg.erase(arg.length()-1);
	if ( arg.at(arg.length()-1) == ']' )
	{
		arg.erase(arg.length()-1);
	}
	if ( arg.at(0) == '<' )
	{
		arg.erase(arg.length()-1);
		arg.erase(0, 1);
	}
	size_t p = arg.find("file:");
	if (p == 0) // prefix
	{
		arg.erase(0,5);
		std::cout << "\t\t\t_filedir \"" << arg << "\"\n";
		arg.clear();
	}
	p = arg.find('|');
	if ( p != std::string::npos )
	{
		std::cout << "\t\t\tCOMPREPLY=(";
		do
		{
			arg.replace(p, 1, 1, ' ');
		}
		while ( (p = arg.find_first_of('|', p)) != std::string::npos );
		quoteSpecialChars(arg);
		std::cout << " $( compgen -W '" << arg << "' -- \"$cur\") ";
		std::cout << ")\n";
	}
	std::cout << "\t\t\treturn\n\t\t\t;;\n";
}

inline std::string BashCompletionOutput::getMutexList( CmdLineInterface& _cmd, Arg* a)
{
	XorHandler xorHandler = _cmd.getXorHandler();
	std::vector< std::vector<Arg*> > xorList = xorHandler.getXorList();

	if (a->getName() == "help" || a->getName() == "version")
	{
		return "(-)";
	}

	ostringstream list;
	if ( a->acceptsMultipleValues() )
	{
		list << '*';
	}

	for ( int i = 0; static_cast<unsigned int>(i) < xorList.size(); i++ )
	{
		for ( ArgVectorIterator it = xorList[i].begin();
			it != xorList[i].end();
			it++)
		if ( a == (*it) )
		{
			list << '(';
			for ( ArgVectorIterator iu = xorList[i].begin();
				iu != xorList[i].end();
				iu++ )
			{
				bool notCur = (*iu) != a;
				bool hasFlag = !(*iu)->getFlag().empty();
				if ( iu != xorList[i].begin() && (notCur || hasFlag) )
					list << ' ';
				if (hasFlag)
					list << (*iu)->flagStartChar() << (*iu)->getFlag() << ' ';
				if ( notCur || hasFlag )
					list << (*iu)->nameStartString() << (*iu)->getName();
			}
			list << ')';
			return list.str();
		}
	}

	// wasn't found in xor list
	if (!a->getFlag().empty()) {
		list << "(" << a->flagStartChar() << a->getFlag() << ' ' <<
			a->nameStartString() << a->getName() << ')';
	}

	return list.str();
}

} //namespace TCLAP
