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
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <onion/log.h>
#include <onion/onion.h>
#include <onion/http.h>
#include <onion/shortcuts.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include "trivia.h"
#include "questions.h"

struct rgbw_t {
	uint16_t red;
	uint16_t green;
	uint16_t blue;
	uint16_t white;
	int have_winner;
};

typedef struct rgbw_t rgbw;

rgbw *lamp;
int child;

onion *o=NULL;

void onexit(int sig){
	ONION_INFO("Exit");
	kill(child, sig);
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
		lamp->have_winner = 1;
		onion_response_printf(res, trivia_end.winner_message, load_count);
	} else {
		onion_response_printf(res, trivia_end.other_message, load_count);
	}
	return OCS_PROCESSED;
}

int add_done_page(onion_url* urls)
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
		lamp->green = 65535;
		uri = q->correct_uri;
	} else {
		lamp->red = 65535;
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

int setup_device_for_lamp(int fd, char *device)
{
	struct termios tty = { 0 };
	if(tcgetattr(fd, &tty) != 0) {
		printf("error: tcgetattr(%s): %s\n", device, strerror(errno));
		return -1;
	}
	cfsetospeed(&tty, B115200);
	cfsetispeed(&tty, B115200);
	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
	tty.c_iflag &= ~IGNBRK;         // disable break processing
	tty.c_lflag = 0;                // no signaling chars, no echo,
	                                // no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN] = 0;             // read doesn't block
	tty.c_cc[VTIME] = 0;            // read timeout disabled

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD); // ignore modem controls,
	                                 // enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	return tcsetattr (fd, TCSANOW, &tty);
}

int timed_out(int c, int *ms_idle)
{
	if(c != 0) {
		(*ms_idle) = 0;
		return 0;
	} else if((*ms_idle) > 100) {
		return 1;
	} else {
		(*ms_idle)++;
		usleep(1000);
		return 0;
	}
}

int get_frame(int fd, char *device, char **frame)
{
	static char buf[1024];
	int size = 0;
	int ms_idle = 0;
	while(1) {
		int c = read(fd, buf, 1);
		if(timed_out(c, &ms_idle)) {
			buf[0] = '\0';
			return 0;
		}
		if(c == -1) {
			printf("error: read(%s): %s\n", device, strerror(errno));
			return c;
		}
		if(c == 0) {
			continue;
		}
		if(buf[0] == '[')
			break;
	}
	while(1) {
		int i;
		int c = read(fd, buf + size, sizeof(buf) - size);
		if(timed_out(c, &ms_idle)) {
			printf("timeout\n");
			buf[0] = '\0';
			return 0;
		}
		if(c == -1) {
			printf("error: read(%s): %s\n", device, strerror(errno));
			return c;
		}
		if(c == 0) {
			continue;
		}
		int const new_size = size + c;
		for(i = size; i < new_size; i++) {
			if(buf[i] == ']') {
				buf[i] = '\0';
				*frame = buf;
				return i;
			}
		}
		size = new_size;
	}
	return -1;
}

int send_frame(int fd, char *format, ...)
{
	int r;
	{
		va_list debug_args;
		va_start(debug_args, format);
		if(printf("tx:[") == -1) {
			r = -1;
		} else if((r = vprintf(format, debug_args)) == -1) {
			r = -1;
		} else if(printf("]\r\n") == -1) {
			r = -1;
		}
		va_end(debug_args);
	}
	va_list args;
	va_start(args, format);
	if(dprintf(fd, "[") == -1) {
		r = -1;
	} else if((r = vdprintf(fd, format, args)) == -1) {
		r = -1;
	} else if(dprintf(fd, "]\r") == -1) {
		r = -1;
	}
	va_end(args);
	return r;
}

void send_colors(int fd)
{
	send_frame(fd, "ColorTest,SetDutyCycles,%d,%d,%d,%d",
				lamp->red,
				lamp->green,
				lamp->blue,
				lamp->white);
}

void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
{
	int i;
	float f, p, q, t;
	if( s == 0 ) {
		// achromatic (grey)
		*r = *g = *b = v;
		return;
	}
	h /= 60;			// sector 0 to 5
	i = floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );
	switch( i ) {
		case 0:
			*r = v;
			*g = t;
			*b = p;
			break;
		case 1:
			*r = q;
			*g = v;
			*b = p;
			break;
		case 2:
			*r = p;
			*g = v;
			*b = t;
			break;
		case 3:
			*r = p;
			*g = q;
			*b = v;
			break;
		case 4:
			*r = t;
			*g = p;
			*b = v;
			break;
		default:		// case 5:
			*r = v;
			*g = p;
			*b = q;
			break;
	}
}

void fade_colors(void)
{
	if(lamp->have_winner) {
		static uint32_t ctr = 0;
		float r, g, b;
		float h = (ctr * 5) % 360;
		float s = 1.0f;
		float v = 1.0f;
		HSVtoRGB(&r, &g, &b, h, s, v);
		lamp->red = floor(r * 65535.0);
		lamp->green = floor(g * 65535.0);
		lamp->blue = floor(b * 65535.0);
		lamp->white = 0;
		ctr++;
	} else {
		int const p = 970;
		int const p_div = 1000;
		lamp->red = lamp->red * p / p_div;
		lamp->green = lamp->green * p / p_div;
		lamp->blue = lamp->blue * p / p_div;
		lamp->white = lamp->white * p / p_div;
	}
}

void init_color_test(int fd)
{
	send_frame(fd, "ColorTest,Init");
}

void reset_to_factory_test(int fd)
{
	send_frame(fd, "TH,Reset,FactoryTest");
}

int lamp_control_task(char *device)
{
	int fd;
	fd = open(device, O_RDWR);
	if(fd == -1) {
		printf("error: open lamp device (%s): %s\n", device, strerror(errno));
		exit(1);
	}
	if(setup_device_for_lamp(fd, device) == -1) {
		printf("error: setup_device_for_lamp(%s): %s\n", device, strerror(errno));
		exit(1);
	}
	tcflush(fd, TCIOFLUSH);
	reset_to_factory_test(fd);
	while(1) {
		char *frame;
		static int color_init_attempts = 0;
		int size;
		if((size = get_frame(fd, device, &frame)) == -1) {
			printf("error: get_frame(%s): %s\n", device, strerror(errno));
			return 1;
		}
		if(!size) {
			printf("rx:timeout\n");
			reset_to_factory_test(fd);
			continue;
		}
		printf("rx:[%s]\n", frame);
		if(strcmp("TH,Ready,0", frame) == 0) {
			sleep(1);
			init_color_test(fd);
			color_init_attempts++;
		} else if((strcmp("SYS,Error,Incorrect format", frame) == 0) ||
				  (strcmp("SYS,Error,Unknown component", frame) == 0)) {
			if((color_init_attempts > 0) && (color_init_attempts < 5)) {
				init_color_test(fd);
				color_init_attempts++;
			} else {
				color_init_attempts = 0;
				reset_to_factory_test(fd);
			}
		} else if(strcmp("ColorTest,Init,0", frame) == 0) {
			send_colors(fd);
		} else if(strcmp("ColorTest,SetDutyCycles,0", frame) == 0) {
			send_colors(fd);
			fade_colors();
			usleep(33333);
		} else if(strstr(frame, "ColorTest") != NULL) {
			reset_to_factory_test(fd);
		}
	}
	return 1;
}

int trivia(void)
{
	printf("creating web-server\n");
	o=onion_new(O_ONE_LOOP);

	printf("obtaining root url\n");
	onion_url *urls=onion_root_url(o);

	printf("adding questions\n");
	if(add_questions(urls)) {
		printf("error: add_questions(): %s\n", strerror(errno));
		exit(1);
	}
	printf("adding landing page\n");
	if(add_landing_page(urls)) {
		printf("error: add_landing_page(): %s\n", strerror(errno));
		exit(1);
	}

	printf("adding done page\n");
	if(add_done_page(urls)) {
		printf("error: add_done_page(): %s\n", strerror(errno));
		exit(1);
	}

	printf("adding signal handlers\n");
	signal(SIGTERM, onexit);
	signal(SIGINT, onexit);

	printf("adding listening point\n");
	onion_add_listen_point(o, NULL, "80", onion_http_new());

	printf("listening\n");
	onion_listen(o);

	printf("freeing\n");
	onion_free(o);
	return 0;
}

int main(int argc, char **argv)
{
	lamp = mmap(NULL, sizeof *lamp, PROT_READ | PROT_WRITE,
	            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(lamp == MAP_FAILED) {
		printf("error: mmap(): %s\n", strerror(errno));
		exit(1);
	}
	child = fork();
	switch(child) {
	case -1: // Errors
		printf("error: fork(): %s\n", strerror(errno));
		return 1;
	case 0:  // Child process
		return lamp_control_task("/dev/ttyO1");
	default: // Parent process
		return trivia();
	}
}
