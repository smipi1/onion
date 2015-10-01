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
#ifndef TRIVIA_H
#define TRIVIA_H

#ifdef __cplusplus
extern "C"{
#endif

struct trivia_question_t {
	char* uri;
	char* ask;
	char* correct_answer;
	char* correct_uri;
	char* again_uri;
};

struct trivia_done_t {
	char* uri;
	char* winner_message;
	char* other_message;
};

typedef struct trivia_question_t trivia_question;
typedef struct trivia_done_t trivia_done;

extern char* trivia_title;
extern char* trivia_start_message;
extern char* trivia_bad_answer_message;
extern trivia_question trivia_questions[];
extern trivia_done trivia_end;

#ifdef __cplusplus
}
#endif

#endif
