/*
 * Copyright (c) 1993 W. Richard Stevens.  All rights reserved.
 * Permission to use or modify this software and its documentation only for
 * educational purposes and without fee is hereby granted, provided that
 * the above copyright notice appear in all copies.  The author makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */

#include "sock.h"

			/* define global variables */
char	*host;
char	*port;

int		bindport;			/* 0 or TCP or UDP port number to bind */
int		broadcast;			/* SO_BROADCAST */
int		cbreak;				/* set terminal to cbreak mode */
int		client = 1;			/* acting as client is the default */
int		crlf;				/* convert newline to CR/LF & vice versa */
int		debug;				/* SO_DEBUG */
int		dofork;				/* concurrent server, do a fork() */
char	foreignip[32];		/* foreign IP address, dotted-decimal string */
int		foreignport;		/* foreign port number */
int		halfclose;			/* TCP half close option */
int		keepalive;			/* SO_KEEPALIVE */
long	linger = -1;		/* 0 or positive turns on option */
int		listenq = 5;		/* listen queue for TCP Server */
int		nodelay;			/* TCP_NODELAY (Nagle algorithm) */
int		nbuf = 1024;		/* number of buffers to write (sink mode) */
int		pauseclose;			/* seconds to sleep after recv FIN, before close */
int		pauseinit;			/* seconds to sleep before first read */
int		pauselisten;		/* seconds to sleep after listen() */
int		pauserw;			/* seconds to sleep before each read or write */
int		reuseaddr;			/* SO_REUSEADDR */
int		readlen = 1024;		/* default read length for socket */
int		writelen = 1024;	/* default write length for socket */
int		recvdstaddr;		/* IP_RECVDSTADDR option */
int		rcvbuflen;			/* size for SO_RCVBUF */
int		sndbuflen;			/* size for SO_SNDBUF */
char   *rbuf;				/* pointer that is malloc'ed */
char   *wbuf;				/* pointer that is malloc'ed */
int		server;				/* to act as server requires -s option */
int		sourcesink;			/* source/sink mode */
int		udp;				/* use UDP instead of TCP */
int		urgwrite;			/* write urgent byte after this write */
int		verbose;

static void	usage(const char *);

int
main(int argc, char *argv[])
{
	int		c, fd;
	char	*ptr;

	if (argc < 2)
		usage("");

	opterr = 0;		/* don't want getopt() writing to stderr */
	while ( (c = getopt(argc, argv, "b:cf:hin:p:q:r:suvw:ABCDEFKL:NO:P:Q:R:S:U:")) != EOF) {
		switch (c) {
		case 'b':
			bindport = atoi(optarg);
			break;

		case 'c':			/* convert newline to CR/LF & vice versa */
			crlf = 1;
			break;

		case 'f':			/* foreign IP address and port#: a.b.c.d.p */
			if ( (ptr = strrchr(optarg, '.')) == NULL)
				usage("invalid -f option");

			*ptr++ = 0;					/* null replaces final period */
			foreignport = atoi(ptr);	/* port number */
			strcpy(foreignip, optarg);	/* save dotted-decimal IP */
			break;

		case 'h':			/* TCP half-close option */
			halfclose = 1;
			break;

		case 'i':			/* source/sink option */
			sourcesink = 1;
			break;

		case 'n':			/* number of buffers to write */
			nbuf = atol(optarg);
			break;

		case 'p':			/* pause before each read or write */
			pauserw = atoi(optarg);
			break;

		case 'q':			/* listen queue for TCP server */
			listenq = atoi(optarg);
			break;

		case 'r':			/* read() length */
			readlen = atoi(optarg);
			break;

		case 's':			/* server */
			server = 1;
			client = 0;
			break;

		case 'u':			/* use UDP instead of TCP */
			udp = 1;
			break;

		case 'v':			/* output what's going on */
			verbose = 1;
			break;

		case 'w':			/* write() length */
			writelen = atoi(optarg);
			break;

		case 'A':			/* SO_REUSEADDR socket option */
			reuseaddr = 1;
			break;

		case 'B':			/* SO_BROADCAST socket option */
			broadcast = 1;
			break;

		case 'C':			/* set standard input to cbreak mode */
			cbreak = 1;
			break;

		case 'D':			/* SO_DEBUG socket option */
			debug = 1;
			break;

		case 'E':			/* IP_RECVDSTADDR socket option */
			recvdstaddr = 1;
			break;

		case 'F':			/* concurrent server, do a fork() */
			dofork = 1;
			break;

		case 'K':			/* SO_KEEPALIVE socket option */
			keepalive = 1;
			break;

		case 'L':			/* SO_LINGER socket option */
			linger = atol(optarg);
			break;

		case 'N':			/* SO_NODELAY socket option */
			nodelay = 1;
			break;

		case 'O':			/* pause before listen(), before first accept() */
			pauselisten = atoi(optarg);
			break;

		case 'P':			/* pause before first read() */
			pauseinit = atoi(optarg);
			break;

		case 'Q':			/* pause after receiving FIN, but before close() */
			pauseclose = atoi(optarg);
			break;

		case 'R':			/* SO_RCVBUF socket option */
			rcvbuflen = atoi(optarg);
			break;

		case 'S':			/* SO_SNDBUF socket option */
			sndbuflen = atoi(optarg);
			break;

		case 'U':			/* when to write urgent byte */
			urgwrite = atoi(optarg);
			break;

		case '?':
			usage("unrecognized option");
		}
	}

		/* check for options that don't make sense */
	if (udp && halfclose)
		usage("can't specify -h and -u");
	if (udp && debug)
		usage("can't specify -D and -u");
	if (udp && linger >= 0)
		usage("can't specify -L and -u");
	if (udp && nodelay)
		usage("can't specify -N and -u");
	if (udp == 0 && broadcast)
		usage("can't specify -B with TCP");
	if (udp == 0 && foreignip[0] != 0)
		usage("can't specify -f with TCP");

	if (client) {
		if (optind != argc-2)
			usage("missing <hostname> and/or <port>");
		host = argv[optind];
		port = argv[optind+1];

	} else {
			/* If server specifies host and port, then local address is
			   bound to the "host" argument, instead of being wildcarded. */
		if (optind == argc-2) {
			host = argv[optind];
			port = argv[optind+1];
		} else if (optind == argc-1) {
			host = NULL;
			port = argv[optind];
		} else
			usage("missing <port>");
	}

	if (client)
		fd = cliopen(host, port);
	else
		fd = servopen(host, port);

	if (sourcesink)
		sink(fd);			/* ignore stdin/stdout */
	else
		loop(fd);			/* copy stdin/stdout to/from socket */

	exit(0);
}

static void
usage(const char *msg)
{
	err_msg(
"使用: sock [ options ] <host> <port>              (for client; default)\n"
"       sock [ options ] -s [ <IPaddr> ] <port>     (for server)\n"
"       sock [ options ] -i <host> <port>           (for \"source\" client)\n"
"       sock [ options ] -i -s [ <IPaddr> ] <port>  (for \"sink\" server)\n"
"options: -b n 将n绑定为客户的本地端口号(在默认情况下,系统给客户分配一个临时的端口号).\n"
"         -c  将从标准输入读入的新行字符转换为一个回车符和一个换行符,类似地,当从网络中读数据时,将<回车,换行>序列转换为新行字符。很多因特网应用需要 NVT ASCII,它使用回车和换行来终止每一行.\n"
"         -f    a.b.c.d.port为一个UDP端点指明远端的IP地址(a.b.c.d)和远端的端口号(port).\n"
"         -h   实现TCP的半关闭机制,当在标准输入中读到一个文件结束符时并不终止.而是在TCP连接上发送一个半关闭报文,继续从网络中读报文直到对方关闭连接.\n"
"         -i   源客户或接收器服务器.向网络写数据(默认),或者如果和-s选项一起用,从网络读数据.-n选项可以指明写(或读)的缓存的数目,-w选项可以指明每次写的大小， -r 选项可以指明每次读的大小.\n"
"         -n n  当和-i选项一起使用时,n指明了读或写的缓存的数目.n的默认值是1024.\n"
"         -p n  指明每个读或写之间暂停的秒数.这个选项可以和源客户(-i)或接收器服务器(-is)一起使用作为每次对网络读写时的延迟.参考-P选项,实现在第1次读或写之前暂停.\n"
"         -q n  为TCP服务器指明挂起的连接队列的大小:TCP将为之进行排队的已经接受的连接的数目.默认值是5.\n"
);
  err_msg(
"         -r n  和-is选项一起使用,n指明每次从网络中读数据的大小.默认是每次读1024字节.\n"
"         -s    作为一个服务器启动,而不是一个客户.\n"
"         -u    使用UDP,而不是TCP.\n"
"         -v    详细模式.在标准差错上打印附加的细节信息(如客户和服务器的临时端口号)\n"
"         -w n  和-i选项一起使用,n指明每次从网络中写数据的大小.默认值是每次写1024字节.\n"
"         -A    SO_REUSEADDR接口选项.对于TCP,这个选项允许进程给自己分配一个处于2MSL等待的连接的端口号.对于UDP,这个选项支持多播,它允许多个进程使用同一个本地端口来接收广播或多播的数据报.\n"
"         -B    SO_BROADCAST接口选项,允许向一个广播IP地址发送UDP数据报.\n"
"         -C    设置终端为cbreak模式.\n"
"         -D    SO_DEBUG接口选项.这个选项使得内核为这个TCP连接维护另外的调试信息.以后可以运行trpt(8)程序输出这个信息.\n"
"         -E    如果实现支持,使能IP_RECVDSTADDR接口选项.这个选项用于UDP服务器,用来打印接收到的UDP数据报的目的IP地址.\n"
"         -F    指明一个并发的TCP服务器.即,服务器使用fork函数为每一个客户连接创建一个新的进程.\n"
"         -K    SO_KEEPALIVE 接口选项\n"
"         -L n  SO_LINGER 选项把一个TCP端点的拖延时间 (linger time)(SO_LINGER)设置为 n.一个为0的拖延时间意味着当网络连接关闭时，正在排队等着发送的任何数据都被丢弃，向对方发送一个重置报文.一个正的拖延时间（百分之一秒）是关闭网络连接必须等待的将所有正在排队等着发送的数据发送完并收到确认的时间。关闭网络连接时，如果这个拖延定时器超时，挂起的数据没有全部发送完并收到确认，关闭操作将返回一个差错信息.\n"
"         -N    设置TCP_NODELAY接口选项来禁止Nagle算法\n"
"         -O n  指明一个TCP服务器在接受第一个客户连接之前暂停的秒数.\n"
"         -P n  指明在第一次对网络进行读或写之前暂停的秒数。这个选项可以和接收器服务器(-is)一起使用,完成在接受了客户的连接请求之后但在执行从网络中第一次读之前的延迟。和接收源(-i)一起使用时,完成连接建立之后但第一次向网络写之前的延迟.参看-p选项,实现在接下来的每一次读或写之间进行暂停.\n"
"         -Q n  指明当一个TCP客户或服务器收到了另一端发来的一个文件结束符,在它关闭自己这一端的连接之前需要暂停的秒数.\n"
"         -R n  把接口的接收缓存(SO_RCVBUF接口选项)设置为n.这可以直接影响TCP通告的接收窗口的大小.对于UDP,这个选项指明了可以接收的最大的UDP数据报.\n"
"         -S n  把接口的发送缓存(SO_SNDBUF接口选项)设置为n.对于UDP,这个选项指明了可以发送的最大的 UDP数据报.\n"
"         -U n  在向网络写了数字n后进入TCP的紧急模式.写一个字节的数据以启动紧急模式.\n"
);

	if (msg[0] != 0)
		err_quit("%s", msg);
	exit(1);
}
