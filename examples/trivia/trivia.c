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
static int done_count = 0;

onion *o=NULL;

#define PAGE_HEAD \
"<html>" \
"<head>" \
"<link rel=\"stylesheet\" href=\"s.css\">" \
"<title>%s</title>" \
"</head>" \
"<body>" \
"<h1>"

#define PAGE_TAIL \
"</h1>" \
"</body>" \
"</html>\n"

#define ANSWER_AND_SUBMIT(URL_POSTFIX) \
"<form class=\"button-primary\" method=\"POST\" action=\"%s" URL_POSTFIX "\">" \
"<input type=\"text\" name=\"answer\" autofocus>" \
"<input type=\"submit\">" \
"</form>"

void onexit(int sig){
	ONION_INFO("Exit");
	kill(child, sig);
	if (o)
		onion_listen_stop(o);
}

void trivia_response_print_head(onion_response* res)
{
	onion_response_printf(res, PAGE_HEAD, trivia_title);
}

void trivia_response_print_tail(onion_response* res)
{
	onion_response_printf(res, PAGE_TAIL);
}

void trivia_response_print_button(onion_response* res, char *name, char *dest)
{
	onion_response_printf(res,
		"<form class=\"button-primary\" method=\"POST\" action=\"%s\">"
		"<input type=\"submit\" value=\"%s\" autofocus>"
		"</form>",
		dest,
		name);
}

void trivia_response_form_with_button(onion_response* res, char *name, char *dest, char *dest_postfix)
{
	onion_response_printf(res,
		"<form class=\"button-primary\" method=\"POST\" action=\"%s%s\">"
		"<input type=\"text\" name=\"answer\" autofocus>"
		"<input type=\"submit\" value=\"%s\">"
		"</form>",
		dest, dest_postfix,
		name);
}

int add_css_page(onion_url *urls)
{
	char page[] = ".column,.columns,.container{box-sizing:border-box;width:100%}body,h6{line-height:1.6}.container{position:relative;max-width:960px;margin:0 auto;padding:0 20px}ol,p,ul{margin-top:0}.column,.columns{float:left}@media (min-width:400px){.container{width:85%;padding:0}}html{font-size:62.5%}body{font-size:1.5em;font-weight:400;font-family:Raleway,HelveticaNeue,\"Helvetica Neue\",Helvetica,Arial,sans-serif;color:#222}h1,h2,h3,h4,h5,h6{margin-top:0;margin-bottom:2rem;font-weight:300}h1{font-size:4rem;line-height:1.2;letter-spacing:-.1rem}h2{font-size:3.6rem;line-height:1.25;letter-spacing:-.1rem}h3{font-size:3rem;line-height:1.3;letter-spacing:-.1rem}h4{font-size:2.4rem;line-height:1.35;letter-spacing:-.08rem}h5{font-size:1.8rem;line-height:1.5;letter-spacing:-.05rem}h6{font-size:1.5rem;letter-spacing:0}@media (min-width:550px){.container{width:80%}.column,.columns{margin-left:4%}.column:first-child,.columns:first-child{margin-left:0}.one.column,.one.columns{width:4.66666666667%}.two.columns{width:13.3333333333%}.three.columns{width:22%}.four.columns{width:30.6666666667%}.five.columns{width:39.3333333333%}.six.columns{width:48%}.seven.columns{width:56.6666666667%}.eight.columns{width:65.3333333333%}.nine.columns{width:74%}.ten.columns{width:82.6666666667%}.eleven.columns{width:91.3333333333%}.twelve.columns{width:100%;margin-left:0}.one-third.column{width:30.6666666667%}.two-thirds.column{width:65.3333333333%}.one-half.column{width:48%}.offset-by-one.column,.offset-by-one.columns{margin-left:8.66666666667%}.offset-by-two.column,.offset-by-two.columns{margin-left:17.3333333333%}.offset-by-three.column,.offset-by-three.columns{margin-left:26%}.offset-by-four.column,.offset-by-four.columns{margin-left:34.6666666667%}.offset-by-five.column,.offset-by-five.columns{margin-left:43.3333333333%}.offset-by-six.column,.offset-by-six.columns{margin-left:52%}.offset-by-seven.column,.offset-by-seven.columns{margin-left:60.6666666667%}.offset-by-eight.column,.offset-by-eight.columns{margin-left:69.3333333333%}.offset-by-nine.column,.offset-by-nine.columns{margin-left:78%}.offset-by-ten.column,.offset-by-ten.columns{margin-left:86.6666666667%}.offset-by-eleven.column,.offset-by-eleven.columns{margin-left:95.3333333333%}.offset-by-one-third.column,.offset-by-one-third.columns{margin-left:34.6666666667%}.offset-by-two-thirds.column,.offset-by-two-thirds.columns{margin-left:69.3333333333%}.offset-by-one-half.column,.offset-by-one-half.columns{margin-left:52%}h1{font-size:5rem}h2{font-size:4.2rem}h3{font-size:3.6rem}h4{font-size:3rem}h5{font-size:2.4rem}h6{font-size:1.5rem}}a{color:#1EAEDB}a:hover{color:#0FA0CE}.button,button,input[type=submit],input[type=reset],input[type=button]{display:inline-block;height:8rem;padding:0 1rem;color:#555;text-align:center;font-size:4rem;line-height:1.2;letter-spacing:-.1rem;font-weight:600;text-transform:uppercase;text-decoration:none;white-space:nowrap;background-color:transparent;border-radius:2rem;border:1px solid #bbb;cursor:pointer;box-sizing:border-box}fieldset,hr{border-width:0}.button:focus,.button:hover,button:focus,button:hover,input[type=submit]:focus,input[type=submit]:hover,input[type=reset]:focus,input[type=reset]:hover,input[type=button]:focus,input[type=button]:hover{color:#333;border-color:#888;outline:0}.button.button-primary,button.button-primary,input[type=submit].button-primary,input[type=reset].button-primary,input[type=button].button-primary{color:#FFF;background-color:#33C3F0;border-color:#33C3F0}.button.button-primary:focus,.button.button-primary:hover,button.button-primary:focus,button.button-primary:hover,input[type=submit].button-primary:focus,input[type=submit].button-primary:hover,input[type=reset].button-primary:focus,input[type=reset].button-primary:hover,input[type=button].button-primary:focus,input[type=button].button-primary:hover{color:#FFF;background-color:#1EAEDB;border-color:#1EAEDB}input[type=tel],input[type=url],input[type=password],input[type=email],input[type=number],input[type=search],input[type=text],select,textarea{display:inline-block;height:8rem;padding:1 1rem;color:#000;font-size:4rem;line-height:1.2;letter-spacing:-.1rem;font-weight:600;text-decoration:none;white-space:nowrap;background-color:#fff;border-radius:2rem;border:1px solid #D1D1D1;box-sizing:border-box;box-shadow:none}input[type=tel],input[type=url],input[type=password],input[type=email],input[type=number],input[type=search],input[type=text],textarea{-webkit-appearance:none;-moz-appearance:none;appearance:none}textarea{min-height:65px;padding-top:6px;padding-bottom:6px}input[type=tel]:focus,input[type=url]:focus,input[type=password]:focus,input[type=email]:focus,input[type=number]:focus,input[type=search]:focus,input[type=text]:focus,select:focus,textarea:focus{border:1px solid #33C3F0;outline:0}label,legend{display:block;margin-bottom:.5rem;font-weight:600}fieldset{padding:0}input[type=checkbox],input[type=radio]{display:inline}label>.label-body{display:inline-block;margin-left:.5rem;font-weight:400}ul{list-style:circle inside}ol{list-style:decimal inside}ol,ul{padding-left:0}ol ol,ol ul,ul ol,ul ul{margin:1.5rem 0 1.5rem 3rem;font-size:90%}.button,button,li{margin-bottom:1rem}code{padding:.2rem .5rem;margin:0 .2rem;font-size:90%;white-space:nowrap;background:#F1F1F1;border:1px solid #E1E1E1;border-radius:4px}pre>code{display:block;padding:1rem 1.5rem;white-space:pre}td,th{padding:12px 15px;text-align:left;border-bottom:1px solid #E1E1E1}td:first-child,th:first-child{padding-left:0}td:last-child,th:last-child{padding-right:0}fieldset,input,select,textarea{margin-bottom:1.5rem}blockquote,dl,figure,form,ol,p,pre,table,ul{margin-bottom:2.5rem}.u-full-width{width:100%;box-sizing:border-box}.u-max-full-width{max-width:100%;box-sizing:border-box}.u-pull-right{float:right}.u-pull-left{float:left}hr{margin-top:3rem;margin-bottom:3.5rem;border-top:1px solid #E1E1E1}.container:after,.row:after,.u-cf{content:\"\";display:table;clear:both}";
	return onion_url_add_static(urls, "s.css", page, HTTP_OK);
}

onion_connection_status handle_start(void *_, onion_request *req, onion_response *res)
{
	trivia_response_print_head(res);
	onion_response_printf(res, trivia_start_message);
	trivia_response_print_button(res, "Start", trivia_questions->uri);
	trivia_response_print_tail(res);
	return OCS_PROCESSED;
}

int add_start_page(onion_url* urls)
{
	return onion_url_add(urls, "", handle_start);
}

onion_connection_status handle_done(void *_, onion_request *req, onion_response *res)
{
	done_count++;
	trivia_response_print_head(res);
	if(done_count == 1) {
		lamp->have_winner = 1;
		onion_response_printf(res, trivia_end.winner_message, done_count);
	} else {
		onion_response_printf(res, trivia_end.other_message, done_count);
	}
	trivia_response_print_tail(res);
	return OCS_PROCESSED;
}

int add_done_page(onion_url* urls)
{
	return onion_url_add(urls, trivia_end.uri, handle_done);
}

onion_connection_status handle_reset(void *_, onion_request *req, onion_response *res)
{
	done_count = 0;
	lamp->have_winner = 0;
	lamp->red = 0;
	lamp->green = 0;
	lamp->blue = 0;
	lamp->white = 0;
	return OCS_PROCESSED;
}

int add_reset_page(onion_url* urls)
{
	return onion_url_add(urls, "reset", handle_reset);
}

onion_connection_status handle_question(void *data, onion_request *req, onion_response *res)
{
	trivia_question const* const q = data;
	trivia_response_print_head(res);
	onion_response_printf(res, q->ask);
	trivia_response_form_with_button(res, "Submit", q->uri, ".data");
	trivia_response_print_tail(res);
	return OCS_PROCESSED;
}

int add_question_page(onion_url *urls, trivia_question* const q)
{
	return onion_url_add_with_data(urls, q->uri, handle_question, q, NULL);
}

onion_connection_status handle_bad_answer_question(void *data, onion_request *req, onion_response *res)
{
	trivia_question const* const q = data;
	trivia_response_print_head(res);
	onion_response_printf(res, "%s<br>", trivia_bad_answer_message);
	onion_response_printf(res, q->ask);
	trivia_response_form_with_button(res, "Submit", q->uri, ".data");
	trivia_response_print_tail(res);
	return OCS_PROCESSED;
}

int add_bad_answer_question_page(onion_url *urls, trivia_question* const q)
{
	int r;
	if((r = asprintf(&q->again_uri, "%s.again", q->uri)) == -1) {
		printf("error: cannot format again uri: %s", q->ask);
		return r;
	}
	return onion_url_add_with_data(urls, q->again_uri, handle_bad_answer_question, q, NULL);
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

char const* first_non_whitespace(char const* s)
{
	while((*s != '\0') && ((*s == '\t') || (*s == ' '))) {
		s++;
	}
	return s;
}

char const* next_non_whitespace(char const* s)
{
	while(*s != '\0') {
		s++;
		if((*s != '\t') && (*s != ' ')) {
			break;
		}
	}
	return s;
}

int strcmp_ignoring_case_and_whitespace(char const* a, char const* b)
{
	a = first_non_whitespace(a);
	b = first_non_whitespace(b);
	while(*a || *b) {
		printf("%c:%c\n", *a, *b);
		if(tolower(*a) < tolower(*b)) {
			return -1;
		} else if(tolower(*a) > tolower(*b)) {
			return 1;
		}
		a = next_non_whitespace(a);
		b = next_non_whitespace(b);
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
	if((r = asprintf(&uri, "%s.data", q->uri)) == -1) {
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

uint16_t tw(uint32_t const period, uint32_t x)
{
	uint32_t const hp = period / 2;
	x %= period;
	if(x > hp) {
		x = period - x;
	}
	return (65535 * x) / hp;
}

void fade_colors(void)
{
	if(lamp->have_winner) {
		static uint32_t ctr = 0;
		lamp->red = tw(23, ctr);
		lamp->green = tw(29, ctr);
		lamp->blue = tw(37, ctr);
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

	printf("adding css skeleton\n");
	if(add_css_page(urls)) {
		printf("error: add_questions(): %s\n", strerror(errno));
		exit(1);
	}

	printf("adding questions\n");
	if(add_questions(urls)) {
		printf("error: add_questions(): %s\n", strerror(errno));
		exit(1);
	}
	printf("adding landing page\n");
	if(add_start_page(urls)) {
		printf("error: add_landing_page(): %s\n", strerror(errno));
		exit(1);
	}
	printf("adding done page\n");
	if(add_done_page(urls)) {
		printf("error: add_done_page(): %s\n", strerror(errno));
		exit(1);
	}
	printf("adding reset page\n");
	if(add_reset_page(urls)) {
		printf("error: add_landing_page(): %s\n", strerror(errno));
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
