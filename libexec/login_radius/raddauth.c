/*-
 * Copyright (c) 1996, 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI raddauth.c,v 1.6 1998/04/14 00:39:04 prb Exp
 */
/*
 * Copyright(c) 1996 by tfm associates.
 * All rights reserved.
 *
 * tfm associates
 * P.O. Box 1244
 * Eugene OR 97440-1244 USA
 *
 * This contains unpublished proprietary source code of tfm associates.
 * The copyright notice above does not evidence any 
 * actual or intended publication of such source code.
 * 
 * A license is granted to Berkeley Software Design, Inc. by
 * tfm associates to modify and/or redistribute this software under the
 * terms and conditions of the software License Agreement provided with this
 * distribution. The Berkeley Software Design Inc. software License
 * Agreement specifies the terms and conditions for redistribution.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <login_cap.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>


#define	MAXPWNETNAM		64	/* longest username */
#define MAXSECRETLEN		128	/* maximum length of secret */


#define AUTH_VECTOR_LEN			16
#define AUTH_HDR_LEN			20
#define	AUTH_PASS_LEN			(256 - 16)
#define	PW_AUTHENTICATION_REQUEST	1
#define	PW_AUTHENTICATION_ACK		2
#define	PW_AUTHENTICATION_REJECT	3
#define PW_ACCESS_CHALLENGE		11
#define	PW_USER_NAME			1
#define	PW_PASSWORD			2
#define	PW_CLIENT_ID			4
#define	PW_CLIENT_PORT_ID		5
#define PW_PORT_MESSAGE			18
#define PW_STATE			24

#ifndef	RADIUS_DIR
#define RADIUS_DIR		"/etc/raddb"
#endif
#define RADIUS_SERVERS		"servers"

char *radius_dir = RADIUS_DIR;
char auth_secret[MAXSECRETLEN+1];
int alt_retries;
int retries;
int sockfd;
int timeout;
jmp_buf timerfail;
u_long alt_server;
u_long auth_server;

typedef struct {
	u_char	code;
	u_char	id;
	u_short	length;
	u_char	vector[AUTH_VECTOR_LEN];
	u_char	data[4096 - AUTH_HDR_LEN];
} auth_hdr_t;

extern int md5_calc(u_char *, u_char *, u_int);

void servtimeout(int);
u_long get_ipaddr();
u_long gethost();
int rad_recv(char *, char *);
void parse_challenge(auth_hdr_t *, int, char *, char *);
void rad_request(pid_t, char *, char *, int, char *, char *);
void getsecret();

/*
 * challenge -- NULL for interactive service
 * password -- NULL for interactive service and when requesting a challenge
 */
int
raddauth(char *username, char *class, char *style, char *challenge,
    char *password, char **emsg)
{
	/*
	 * These are made static to get gcc to shut up
	 */
	static pid_t req_id;
	static char *userstyle, *passwd, *pwstate;
	static int auth_port;

	char vector[AUTH_VECTOR_LEN+1], _pwstate[1024], *p, *v;
	int i;
	login_cap_t *lc;
	short q;
	struct servent *svp;
	struct sockaddr_in sin;

	srandom((long)time(NULL) + getpid());
	bzero(_pwstate, sizeof(_pwstate));
	pwstate = password ? challenge : _pwstate;

	if ((lc = login_getclass(class)) == NULL) {
		snprintf(_pwstate, sizeof(_pwstate),
		    "%s: no such class", class);
		*emsg = _pwstate;
		return(1);
	}

	timeout = login_getcapnum(lc, "radius-timout", 2, 2);
	retries = login_getcapnum(lc, "radius-retries", 6, 6);
	if (timeout < 1)
		timeout = 1;
	if (retries < 2)
		retries = 2;

	if (challenge == NULL) {
		passwd = NULL;
		v = login_getcapstr(lc, "radius-challenge-styles",
		    NULL, NULL);
		i = strlen(style);
		while (v && (p = strstr(v, style)) != NULL) {
			if ((p == v || p[-1] == ',') &&
			    (p[i] == ',' || p[i] == '\0')) {
				passwd = "";
				break;
			}
			v = p+1;
		}
		if (passwd == NULL)
			passwd = getpass("Password:");
	} else if (password != NULL)
		passwd =  password;
	else
		passwd = "";

	if ((v = login_getcapstr(lc, "radius-server", NULL, NULL)) == NULL){
		*emsg = "radius-server not configured";
		return (1);
	}

	auth_server = get_ipaddr(v);

	if ((v = login_getcapstr(lc, "radius-server-alt", NULL, NULL)) == NULL)
		alt_server = 0;
	else {
		alt_server = get_ipaddr(v);
		alt_retries = retries/2;
		retries >>= 1;
	}

	/* get port number */
	svp = getservbyname ("radius", "udp");
	if (svp == NULL) {
		*emsg = "No such service: radius/udp";
		return (1);
	}

	/* get the secret from the servers file */
	getsecret();

	/* set up socket */
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		snprintf(_pwstate, sizeof(_pwstate), "%s", strerror(errno));
		*emsg = _pwstate;
		return(1);
	}

	/* set up client structure */
	bzero (&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = svp->s_port;

	req_id = getpid();
	auth_port = ttyslot();
	if (auth_port == 0)
		auth_port = (int)getppid();
	if (strcmp(style, "radius") != 0) {
		userstyle = malloc(strlen(username) + strlen(style) + 2);
		if(userstyle == NULL)
			err(1, NULL);
		sprintf(userstyle, "%s:%s", username, style);
	} else
		userstyle = username;

	/* generate random vector */

	for(i = 0; i < AUTH_VECTOR_LEN; i++) {
		q = (short)(((93.0 * random()) / LONG_MAX) + 33);
		vector[i] = *(char *)&q;
	}
	vector[AUTH_VECTOR_LEN] = '\0';

	signal(SIGALRM, servtimeout);
	setjmp(timerfail);

	while (retries > 0) {
		rad_request(req_id, userstyle, passwd, auth_port, vector,
		    pwstate);

		switch (i = rad_recv(_pwstate, challenge)) {
		case PW_AUTHENTICATION_ACK:
			/*
			 * Make sure we don't think a challenge was issued.
			 */
			if (challenge)
				*challenge = '\0';
			return (0);

		case PW_AUTHENTICATION_REJECT:
			return (1);

		case PW_ACCESS_CHALLENGE:
			/*
			 * If this is a response then reject them if
			 * we got a challenge.
			 */
			if (password)
				return (1);
			/*
			 * If we wanted a challenge, just return
			 */
			if (challenge) {
				if (strcmp(challenge, _pwstate) != 0)
					syslog(LOG_WARNING,
				    "challenge for %s does not match state",
				    userstyle);
				return (0);
			}
			req_id++;
			passwd = getpass("");
			break;

		default:
			snprintf(_pwstate, sizeof(_pwstate),
			    "invalid response type %d\n", i);
			*emsg = _pwstate;
			return(1);
		}
	}
	return (1);
}


/*
 *
 *	rad_request() -- build a radius authentication digest and
 *	submit it to the radius server
 *
 */

void
rad_request(pid_t id, char *name, char *password, int port, char *vector,
    char *state)
{
	auth_hdr_t auth;
	int i, len, secretlen, total_length, p;
	struct servent *rad_port;
	struct sockaddr_in sin;
	u_char md5buf[MAXSECRETLEN+AUTH_VECTOR_LEN], digest[AUTH_VECTOR_LEN],
	    pass_buf[AUTH_PASS_LEN], *pw, *ptr;
	u_int length;
	u_long ipaddr;

	auth.code = PW_AUTHENTICATION_REQUEST;
	auth.id = id;
	memcpy(auth.vector, vector, AUTH_VECTOR_LEN);
	total_length = AUTH_HDR_LEN;
	ptr = auth.data;

	*ptr++ = PW_USER_NAME;
	length = strlen(name);
	if(length > MAXPWNETNAM)
		length = MAXPWNETNAM;

	*ptr++ = length + 2;
	memcpy(ptr, name, length);
	ptr += length;
	total_length += length + 2;

	/* password */

	/* encrypt the password */

	length = strlen(password);
	if(length > AUTH_PASS_LEN)
		length = AUTH_PASS_LEN;

	p = (length + AUTH_VECTOR_LEN - 1) / AUTH_VECTOR_LEN;
	*ptr++ = PW_PASSWORD;
	*ptr++ = p * AUTH_VECTOR_LEN + 2;

	strncpy(pass_buf, password, AUTH_PASS_LEN);

	/* calculate the md5 digest */

	secretlen = strlen(auth_secret);
	memcpy(md5buf, auth_secret, secretlen);
	memcpy(md5buf + secretlen, auth.vector, AUTH_VECTOR_LEN);

	total_length += 2;

	/* xor the password into the md5 digest */
	pw = pass_buf;
	while (p-- > 0) {
		total_length += AUTH_VECTOR_LEN;
		md5_calc(digest, md5buf, secretlen + AUTH_VECTOR_LEN);
		for (i = 0; i < AUTH_VECTOR_LEN; ++i) {
			*ptr = digest[i] ^ *pw;
			md5buf[secretlen+i] = *ptr++;
			*pw++ = '\0';
		}
	}


	/* client id */

	*ptr++ = PW_CLIENT_ID;
	*ptr++ = sizeof(u_long) + 2;
	ipaddr = gethost();
	memcpy(ptr, &ipaddr, sizeof(u_long));
	ptr += sizeof(u_long);
	total_length += sizeof(u_long) + 2;

	/* client port */

	*ptr++ = PW_CLIENT_PORT_ID;
	*ptr++ = sizeof(u_long) + 2;
	port = htonl(port);
	memcpy(ptr, &port, sizeof(int));
	ptr += sizeof(int);
	total_length += sizeof(int) + 2;

	/* Append the state info */

	if((state != (char *)NULL) && (strlen(state) > 0)) {
		len = strlen(state);
		*ptr++ = PW_STATE;
		*ptr++ = len + 2;
		memcpy(ptr, state, len);
		ptr += len;
		total_length += len + 2;
	}

	auth.length = htons(total_length);

	/* get radius port number */

	rad_port = getservbyname("radius", "udp");
	if (rad_port == NULL)
		errx(1, "no such service: radius/udp");

	bzero (&sin, sizeof (sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = auth_server;
	sin.sin_port = rad_port->s_port;
	if (sendto(sockfd, &auth, total_length, 0, (struct sockaddr *)&sin,
	    sizeof(sin)) == -1)
		err(1, NULL);
}

/*
 *
 * rad_recv() -- receive udp responses from the radius server
 *
 */
int
rad_recv(char *state, char *challenge)
{
	auth_hdr_t auth;
	int nread, salen;
	struct sockaddr_in sin;

	salen = sizeof(sin);

	alarm(timeout);

	nread = recvfrom(sockfd, &auth, sizeof(auth), 0,
				(struct sockaddr *)&sin, &salen);
	alarm(0);

	if(sin.sin_addr.s_addr != auth_server)
		errx(1, "bogus authentication server");

	if (auth.code == PW_ACCESS_CHALLENGE)
		parse_challenge(&auth, nread, state, challenge);

	return(auth.code);
}


/*
 * gethost() -- get local hostname
 */
u_long
gethost()
{
	char hostname[MAXHOSTNAMELEN];

	if (gethostname(hostname, sizeof(hostname)))
		err(1, "gethost");
	return(get_ipaddr(hostname));
}

/*
 * get_ipaddr -- get an ip address in host long notation from a
 * hostname or dotted quad.
 */
u_long
get_ipaddr(char *host)
{
	struct hostent *hp;

	if ((hp = gethostbyname(host)) == NULL)
        	return(0);

	return (((struct in_addr *)hp->h_addr)->s_addr);
}

/*
 * get the secret from the servers file
 */
void
getsecret()
{
    	FILE *servfd;
	char *host, *secret, buffer[MAXPATHLEN];
	size_t len;

	snprintf(buffer, sizeof(buffer), "%s/%s",
		radius_dir, RADIUS_SERVERS);

	if ((servfd = fopen(buffer, "r")) == NULL) {
		syslog(LOG_ERR, "%s: %m", buffer);
		return;
	}

	secret = NULL;			/* Keeps gcc happy */

	while ((host = fgetln(servfd, &len)) != NULL) {
		if (*host == '#') {
			memset(host, 0, len);
			continue;
		}
		if (host[len-1] == '\n')
			--len;
		while (len > 0 && isspace(host[--len]))
			;
		host[len+1] = '\0';
		while (isspace(*host)) {
			++host;
			--len;
		}
		if (*host == '\0')
			continue;
		secret = host;
		while (*secret && !isspace(*secret))
			++secret;
		if (*secret)
			*secret++ = '\0';
		if (get_ipaddr(host) != auth_server) {
			memset(host, 0, len);
			continue;
		}
		while (isspace(*secret))
			++secret;
		if (*secret)
			break;
	}
	if (host) {
		strncpy(auth_secret, secret, MAXSECRETLEN);
		auth_secret[MAXSECRETLEN] = '\0';
		memset(host, 0, len);
	}
	fclose(servfd);
}

void
servtimeout(int signo)
{
	if (--retries <= 0) {
		/*
		 * If we ran out of tries but there is an alternate
		 * server, switch to it and try again.
		 */
		if (alt_retries) {
			auth_server = alt_server;
			retries = alt_retries;
			alt_retries = 0;
			getsecret();
		} else
			warnx("no response from authentication server");
	}
	longjmp(timerfail, 1);
}

void
parse_challenge(auth_hdr_t *authhdr, int length, char *state, char *challenge)
{
	int attribute, attribute_len;
	u_char *ptr;

	ptr = authhdr->data;
	length = ntohs(authhdr->length) - AUTH_HDR_LEN;

	*state = 0;

	while (length > 0) {
		attribute = *ptr++;
		attribute_len = *ptr++;
		length -= attribute_len;
		attribute_len -= 2;

		switch (attribute) {
		case PW_PORT_MESSAGE:
			if (challenge) {
				memcpy(challenge, ptr, attribute_len);
				challenge[attribute_len] = 0;
			} else
				printf("%.*s", attribute_len, ptr);
			break;
		case PW_STATE:
			memcpy(state, ptr, attribute_len);
			state[attribute_len] = 0;
			break;
		}
		ptr += attribute_len;
	}
}