/*
 * GenericShell.cc
 *
 * Generic interactive shell
 * Copyright (c) 2008, 2103 Linas Vepstas <linas@linas.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <opencog/server/ConsoleSocket.h>
#include <opencog/util/Logger.h>
#include <opencog/util/platform.h>

#include "GenericEval.h"
#include "GenericShell.h"

using namespace opencog;

// Some random RFC 854 characters
#define IAC 0xff  // Telnet Interpret As Command
#define IP 0xf4   // Telnet IP Interrupt Process
#define AO 0xf5   // Telnet AO Abort Output
#define EL 0xf8   // Telnet EL Erase Line
#define WILL 0xfb // Telnet WILL
#define DO 0xfd   // Telnet DO
#define TIMING_MARK 0x6 // Telnet RFC 860 timing mark

// Some random ASCII control characters (unix semantics)
#define EOT 0x4   // end    or ^D at keyboard.
#define SYN 0x16  // quit   or ^C at keyboard.
#define CAN 0x18  // cancel or ^X at keyboard.
#define ESC 0x1b  // ecsape or ^[ at keyboard.

GenericShell::GenericShell(void)
{
	show_output = true;
	show_prompt = true;
	normal_prompt = "> ";
	pending_prompt = "... ";
	abort_prompt = "asdf"; // simply reserve 4 chars
	abort_prompt[0] = IAC;
	abort_prompt[1] = WILL;
	abort_prompt[2] = TIMING_MARK;
	abort_prompt[3] = '\n';

	evaluator = NULL;
	socket = NULL;
	self_destruct = false;
}

GenericShell::~GenericShell()
{
	if (socket)
	{
		socket->SetShell(NULL);
		socket->OnRequestComplete();
		socket = NULL;
	}
}

void GenericShell::socketClosed(void)
{
	// As of right now, the only thing that calls methods on us is the
	// console socket. Thus, when the console socket closes, no one
	// else will ever call a method on this instance ever again. Thus,
	// we should self-destruct. Three remarks:
	// 1) This wouldn't be needed if we had garbage collection, and
	// 2) If this feels hacky to you, well, it is, but I simply do not
	//    see a solution that is easier/better/simpler within the
	//    confines of the current module/socket/request design. (I can
	//    envision all sorts of complicated solutions, but none easy).
	// 3) This is safe in the current threading design, since the thread
	//    that is calling eval() is the same thread that is calling this
	//    method. Thus, no locks. If, instead, it ever happened that the
	//    eval() method was called from a different thread than the socket
	//    closed method, then there would be a race leading to a horrible
	//    crash. The only cure for that would be a redesign of the
	//    socket/request layers. Again, this would not be needed if we
	//    had garbage collection. Wah wah wah.
	delete this;
}

/* ============================================================== */

void GenericShell::hush_output(bool hush)
{
	show_output = !hush;
}

void GenericShell::hush_prompt(bool hush)
{
	show_prompt = !hush;
}

const std::string& GenericShell::get_prompt(void)
{
	static const std::string empty_prompt = "";
	if (!show_prompt) return empty_prompt;

	// Use different prompts, depending on whether there is pending
	// input or not.
	if (evaluator->input_pending())
	{
		return pending_prompt;
	}
	else
	{
		return normal_prompt;
	}
}

/* ============================================================== */
/**
 * Register this shell with the console.
 */
void GenericShell::set_socket(ConsoleSocket *s)
{
	if (socket)
	{
		socket->SetShell(NULL);
		socket->OnRequestComplete();
	}

	socket = s;
	socket->SetShell(this);
}

/* ============================================================== */

void GenericShell::eval(const std::string &expr, ConsoleSocket *s)
{
	// XXX A subtle but important point: the way that socket handling
	// works in OpenCog is that socket-listen/accept happens in one
	// thread, while socket receive is in another. In particular, the
	// constructor for this class runs in a *different* thread than
	// this method does.
	if (NULL == socket)
	{
		socket = s;
	}
	const std::string &retstr = do_eval(expr);
	// logger().debug("[SchemeShell] response: [%s]", retstr.c_str());
	//
	socket->Send(retstr);

	// The user is exiting the shell. No one will ever call a method on
	// this instance ever again. So stop hogging space, and self-destruct.
	// We have to do this here; there is no other opportunity to call dtor.
	if (self_destruct)
	{
		socket->sendPrompt();
		delete this;
	}
}

/* ============================================================== */

/**
 * Evaluate the expression
 */
std::string GenericShell::do_eval(const std::string &expr)
{
	size_t len = expr.length();
	if (0 == len)
	{
		return get_prompt();
	}

	// Handle Telnet RFC 854 IAC format
	// Basically, we're looking for telnet-encoded abort or interrupt
	// characters, starting at the end of the input string. If they
	// are there, then don't process input, and clear out the evaluator.
	// Also, be sure to send telnet IAC WILL TIMING-MARK so that telnet
	// doesn't sit there flushing output forever.
	//
	// Search for IAC to at most 20 chars from the end of the string.
	int i = len-2;
	int m = len - 20;
	if (m < 0) m = 0;
	while (m <= i)
	{
		unsigned char c = expr[i];
		if (IAC == c)
		{
			c = expr[i+1];
			if ((IP == c) || (AO == c))
			{
				evaluator->clear_pending();
				return abort_prompt;
			}

			// Erase line -- just ignore this line.
			if (EL == c)
			{
				return get_prompt();
			}
		}
		i--;
	}

	// Don't evaluate if the line is terminated by
	// escape (^[), cancel (^X) or quit (^C)
	// These would typically be sent by netcat, and not telnet.
	unsigned char c = expr[len-1];
	if ((SYN == c) || (CAN == c) || (ESC == c))
	{
		evaluator->clear_pending();
		return "\n" + normal_prompt;
	}

	// Look for either an isolated control-D, or a single period on a line
	// by itself. This means "leave the shell". We leave the shell by
	// unsetting the shell pointer in the ConsoleSocket.
	// 0x4 is ASCII EOT, which is what ctrl-D at keybd becomes.
	if ((false == evaluator->input_pending()) &&
	    ((EOT == expr[len-1]) || ((1 == len) && ('.' == expr[0]))))
	{
		self_destruct = true;
		if (show_prompt) return "Exiting the shell\n";
		return "";
	}

	/* 
	 * Sometimes the newline gets cut !?!? This may not be true any
	 * longer, with the new whiz-bang socket code.
	 *
	 * Re-insert it; otherwise, comments within procedures will
	 * have the effect of commenting out the rest of the procedure,
	 * leading to garbage.
	 *
	 * (This is a pointless string copy, it should be eliminated)
	 */
	std::string input = expr + "\n";

	std::string result = evaluator->eval(input.c_str());

	if (evaluator->input_pending())
	{
		if (show_output && show_prompt)
			return pending_prompt;
		else
			return "";
	}

	if (show_output || evaluator->eval_error())
	{
		if (show_prompt) result += normal_prompt;
		return result;
	}
	else
	{
		return "";
	}

}

/* ===================== END OF FILE ============================ */
