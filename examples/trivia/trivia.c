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

#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <onion/log.h>
#include <onion/onion.h>
#include <onion/shortcuts.h>
#include "trivia.h"
#include "questions.h"

onion *o=NULL;

void onexit(int _){
	ONION_INFO("Exit");
	if (o)
		onion_listen_stop(o);
}

int add_landing_page(onion_url* urls)
{
	char r;
	char *page;
	if((r = asprintf(
		&page,
		"<html>\n"
		"<head>\n"
		" <title>%s</title>\n"
		"</head>\n"
		"\n"
		"%s\n"
		"<form method=\"POST\" action=\"%s\">\n"
		"<input type=\"submit\" value=\"Start\" autofocus>\n"
		"</form>\n"
		"\n"
		"</html>\n",
		trivia_title,
		trivia_welcome_message,
		trivia_questions->uri)) == -1) {
		printf("error: cannot format landing page");
		return r;
	}
	return onion_url_add_static(urls, "", page, HTTP_OK);
}

onion_connection_status handle_done(void *_, onion_request *req, onion_response *res)
{
	static int load_count = 0;
	load_count++;
	if(load_count == 1) {
		onion_response_printf(res, trivia_end.winner_message, load_count);
	} else {
		onion_response_printf(res, trivia_end.other_message, load_count);
	}
	return OCS_PROCESSED;
}

int add_done_page(urls)
{
	return onion_url_add(urls, trivia_end.uri, handle_done);
}

int add_question_page(onion_url *urls, trivia_question const* const q)
{
	char r;
	char *page;
	if((r = asprintf(
		&page,
		"<html>\n"
		"<head>\n"
		" <title>%s</title>\n"
		"</head>\n"
		"<div>"
		"\n"
		"%s\n"
		"<form method=\"POST\" action=\"%s_data\">\n"
		"<input type=\"answer\" name=\"answer\" autofocus>\n"
		"<input type=\"submit\">\n"
		"</form>\n"
		"</div>"
		"\n"
		"</html>\n",
		trivia_title,
		q->ask,
		q->uri)) == -1) {
		printf("error: cannot format question page: %s", q->ask);
		return r;
	}
	return onion_url_add_static(urls, q->uri, page, HTTP_OK);
}

int add_bad_answer_question_page(onion_url *urls, trivia_question* const q)
{
	char r;
	char *page;
	if((r = asprintf(&page,
		"<html>\n"
		"<head>\n"
		" <title>%s</title>\n"
		"</head>\n"
		"<div>"
		"\n"
		"%s\n"
		"</div>"
		"<div>"
		"\n"
		"%s\n"
		"<form method=\"POST\" action=\"%s_data\">\n"
		"<input type=\"answer\" name=\"answer\" autofocus>\n"
		"<input type=\"submit\">\n"
		"</form>\n"
		"</div>"
		"\n"
		"</html>\n",
		trivia_title,
		trivia_bad_answer_message,
		q->ask,
		q->uri)) == -1) {
		printf("error: cannot format bad answer page: %s", q->ask);
		return r;
	}
	if((r = asprintf(&q->again_uri, "%s_again", q->uri)) == -1) {
		return r;
	}
	return onion_url_add_static(urls, q->again_uri, page, HTTP_OK);
}

int add_question_pages(onion_url *urls, trivia_question* const q)
{
	int r;
	if((r = add_question_page(urls, q))) {
		return r;
	}
	if((r = add_bad_answer_question_page(urls, q))) {
		return r;
	}
	return 0;
}

char const* next_non_whitespace(char const* s)
{
	while((*s != '\0') && ((*s == '\t') || (*s == ' '))) {
		s++;
	}
	return s;
}

int strcmp_ignoring_case_and_whitespace(char const* a, char const* b)
{
	while(*a || *b) {
		a = next_non_whitespace(a);
		b = next_non_whitespace(b);
		if(tolower(*a) < tolower(*b)) {
			return -1;
		} else if(tolower(*a) > tolower(*b)) {
			return 1;
		} else {
			a++;
			b++;
		}
	}
	return 0;
}

onion_connection_status check_answer(void *privdata, onion_request *req, onion_response *res)
{
	trivia_question const* const q = privdata;
	if (onion_request_get_flags(req)&OR_HEAD){
		onion_response_write_headers(res);
		return OCS_PROCESSED;
	}
	const char *answer = onion_request_get_post(req,"answer");
	const char *uri;
	if(strcmp_ignoring_case_and_whitespace(q->correct_answer, answer) == 0) {
		uri = q->correct_uri;
	} else {
		uri = q->again_uri;
	}
	return onion_shortcut_response_extra_headers("<h1>302 - Moved</h1>", HTTP_REDIRECT, req, res, "Location", uri, NULL );
}

int add_answer_redirect_page(onion_url *urls, trivia_question* const q)
{
	int r;
	char *uri;
	if((r = asprintf(&uri, "%s_data", q->uri)) == -1) {
		return r;
	}
	return onion_url_add_handler(urls, uri, onion_handler_new(check_answer, q, NULL));
}

int add_questions(onion_url *urls)
{
	int i;
	trivia_question* prev_q = NULL;
	trivia_question* q = NULL;
	for(i = 0; i < sizeof(trivia_questions) / sizeof(trivia_questions[0]); i++) {
		q = trivia_questions + i;
		int r;
		if((r = add_question_pages(urls, q))) {
			return r;
		}
		if((r = add_answer_redirect_page(urls, q))) {
			return r;
		}
		if(prev_q) {
			prev_q->correct_uri = q->uri;
		}
		prev_q = q;
	}
	if(q) {
		q->correct_uri = trivia_end.uri;
	}
	return 0;
}

/**
 * This example creates a onion server and adds two urls: the base one is a static content with a form, and the
 * "data" URL is the post_data handler.
 */
int main(int argc, char **argv){
	o=onion_new(O_ONE_LOOP);
	onion_url *urls=onion_root_url(o);
	
	add_questions(urls);
	add_landing_page(urls);
	add_done_page(urls);

	signal(SIGTERM, onexit);	
	signal(SIGINT, onexit);	
	onion_listen(o);

	onion_free(o);
	return 0;
}
