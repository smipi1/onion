/*
    Onion HTTP server trivia example
    Copyright (C) 2015 Pieter Smith

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */
#ifndef QUESTIONS_H
#define QUESTIONS_H

#include "trivia.h"

#ifdef __cplusplus
extern "C"{
#endif

char* trivia_title = "Trivia: Linux in a light bulb";

char* trivia_start_message = "Welcome to the Linux in a light bulb trivia:";

char* trivia_bad_answer_message = "Nope! Sorry... Try again if you like:";

trivia_question trivia_questions[] = {
	{
		"sZcUjd9Zdr3sBdPJeRmiHJfC95Ml2UI3",
		"In what state is a traditional light bulb most noticeable and usually "
		"results in an unexpected trip to the store?",
		"broken",
	},
	{
		"o0EVEnQxK50aMi5RO16ItvYdFYqqxRCa",
		"Disorders associated with which under-appreciated 1/3 of your life "
		"can easily be treated with light?",
		"sleep",
	},
	{
		"dhyVbMdHtrcbjOScJkWeGvEeerq7nNdL",
		"Lacking a bigger SoC, what device can traditionally placed between your "
		"things and the cloud?",
		"gateway",
	},
	{
		"4Rthgpq1mRjYswGv0SOBFZmYr3LnZdql",
		"What type of memory is maliciously being used by SoC vendors to store "
		"networking stacks in a deliberate attempt to shorten the life-expectancy "
		"of developers tasked with supporting their products for years to come?",
		"rom",
	},
	{
		"9jtywjlypN4eC4t56xY9fD20OGOAmh4l",
		"Security cannot be treated as a state (strangely enough Arkansas can), "
		"it should be treated as a ___.",
		"process",
	},
	{
		"WEP4sIRfLF2f1TpSuYysv11lhQYrlBaQ",
		"Unlike autistic programmers, what has traditionally been Linux's most "
		"compelling feature?",
		"networking",
	},
	{
		"DuNeCi4gLneprqkMk3wxBxK6y7sfYHFg",
		"The internals of a Philips lamp runs at ___ deg C.",
		"100",
	},
	{
		"7osjb0E5QgqUkErmAwiwrqQxEoZaWTOW",
		"Back when modems ruled the world, Linux was distributed on a "
		"___ disc.",
		"floppy",
	},
	{
		"zIOhpJdEWvSQLZ60hkg3PwqldWWa1qr1",
		"Efforts with regard to death and taxes have failed, so kernel tinification "
		"hackers have turned their attention to making more of Linux ___.",
		"optional",
	},
	{
		"otI5C3HR6Uv0eOwwZFqPgzfHuJc559p4",
		"Which of Josh Triplett's pet projects has Python running in GRUB?",
		"BITS",
	},
	{
		"nXHAtNDyasjBdLCB0otTjZqqNKzWmXfy",
		"Which system call that gives advice about use of memory is now optional "
		"in mainline due to some spectacularly successful tinification efforts?",
		"madvise",
	},
};


/*
	{
		"26QTwRbw5x7OZ3lJqWhb0htdDLjJSJSC",
		"?",
		"",
	},
	{
		"JlA2bT6EcIh8r2Mb1aiut5bGOn8seYsP",
		"?",
		"",
	},
	{
		"8ncZVkv4VvGgaMt0dRiMi4zsxonE0BEB",
		"?",
		"",
	},
	{
		"bNLiWFCXi2DY7sAYHJd0eHld3Y1Yeqys",
		"?",
		"",
	},
	{
		"mI5aXbGMKJjHef5k7V2Bin0sP9TH6axj",
		"?",
		"",
	},
	{
		"M7uNcUagrERV6tPOuKasWhI8CJdBzmkR",
		"?",
		"",
	},
	{
		"SQvHnGNWQu16HLKh7Rx2OOHthyO36sQ7",
		"?",
		"",
	},
	{
		"VssvqHGAgHQVrwKaxbJNOAyRZl1WhEA7",
		"?",
		"",
	},
	{
		"7A7qY85AE3NXgS7ai8aAdwPQFdRtpNip",
		"?",
		"",
	},
	{
		"6WTjFzI972cMRnT81VWezsGa2uYJCGBP",
		"?",
		"",
	},
	{
		"qILKrxotSPwVH9SoYgzaRIDPdMjNcMKa",
		"?",
		"",
	},
	{
		"3i6FxtRSyIYLEKym1a4nGO8ckLhoa34b",
		"?",
		"",
	},
	{
		"9SpVeW8o2w7LLGfhOUY1dUB5yWbbDJ9w",
		"?",
		"",
	},
*/

trivia_done trivia_end =
{
	"xC24pN7ovSBey05rEslXBIBI8hGRfSr9",
	"Congratulations, you are the winner!",
	"Unfortunately you were beaten. You are number %d.",
};

#ifdef __cplusplus
}
#endif

#endif
