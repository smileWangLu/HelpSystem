#include "common.h"


int get_conf_value(char* pathname, const char* key_name, char *value) {

	FILE *fp = NULL;
	char *line = NULL, *substr = NULL;
	size_t len = 0, tmplen = 0;
	ssize_t read;
	//memset(value, 0, sizeof(char)*MAX_SIZE);

	if ( key_name == NULL || value == NULL) {
		DBG("paramer is invaild!\n");
		exit(-1);
	}

	fp = fopen(pathname,"r");
	if (fp == NULL) {
		DBG("Open config file error!\n");
		exit(-1);
	}

	while (( read = getline(&line, &len,fp)) != -1) {
		substr = strstr(line, key_name);
		if (substr == NULL) 
			continue;
		else {
			tmplen = strlen(key_name);
			if (line[tmplen] == '=') {
				strncpy(value, &line[tmplen + 1], (int)read - tmplen + 1);
				tmplen = strlen(value);
				*(value + tmplen - 1) = '\0';
				break;
			}
			else {
				DBG("Maybe there is something wrong with config file!\n");
				continue;
			}
		}
	}

	if (substr == NULL) {
		DBG("%s not found in config file!\n", key_name);
		fclose(fp);
		exit(-1);
	}

	//DBG("%s=%s\n", key_name, value);
	free(line);
	fclose(fp);
	return 0;
}



/*
*创建监听套接字
*返回套接字描述符，错误返回-1
*/

int socket_create(int port){
	int sockfd;
	int yes = 1;
	struct sockaddr_in sock_addr;

	struct linger m_sLinger;
	m_sLinger.l_onoff = 1;
	m_sLinger.l_linger = 0;
	

	//创建套接字
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket() error");
		return -1;
	}

	//设置本地套接字地址
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port); //转化为网络字节序
	sock_addr.sin_addr.s_addr = htonl(INADDR_ANY); //0.0.0.0

	//设置本地套接字
	setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const char*)&m_sLinger,sizeof(struct linger));

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		close(sockfd);
		perror("setsockopt() error\n");
		return -1;
	}

	//绑定本地套接字到套接字
	if (bind(sockfd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) < 0) {
		close(sockfd);
		perror("bind() error");
		return -1;
	}

	//将套接字设置为监听状态
	if (listen(sockfd, 20) < 0) {
		close(sockfd);
		perror("listen() error");
		return -1;
	}
	return sockfd;
}


/*
*接受套接字请求
*返回新的套接字描述符，错误返回-1
*/
int socket_accept(int sock_listen) {
	int sockfd;
	struct sockaddr_in client_addr, server_addr;
	char buffer[MAX_SIZE];
	socklen_t len = sizeof(client_addr); 
	sockfd = accept(sock_listen, (struct sockaddr *) &client_addr, &len);
	getsockname(sockfd, (struct sockaddr *) &server_addr, &len);
	if (sockfd < 0){
		perror("accept() error");
		return -1;
	}
	sprintf(buffer, "%s:%d --> You have connected to Server!", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
	send(sockfd, buffer, strlen(buffer), 0);
	DBG("%s:%d Login Server!\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	return sockfd;
}


/*
*接受套接字请求
*返回新的套接字描述符，错误返回-1
*/

int socket_connect(int port, char *host) {
	int sockfd;
	struct sockaddr_in dest_addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket() error");
		return -1;
	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	dest_addr.sin_addr.s_addr = inet_addr(host);

	DBG("Connetion TO %s:%d\n",host,port);
	//fflush(stdout);
	if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
		//perror("connect() error");
		//DBG("connect() error : %s!\n", stderror(errno));
		return -1;
	}
	return sockfd;

}


int connect_nonblock(int port, char *host, long timeout) {
	int sockfd;
	struct sockaddr_in dest_addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket() error");
		return -1;
	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	dest_addr.sin_addr.s_addr = inet_addr(host);

	DBG("Connetion TO %s:%d\n",host,port);


	int error = -1, len;
	len = sizeof(int);
	struct timeval tm;
	fd_set set;
	unsigned long ul = 1;
	ioctl(sockfd, FIONBIO, &ul);

	bool ret = false; 
	//fflush(stdout);
	if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
		tm.tv_sec = 0;
		tm.tv_usec = timeout;
		FD_ZERO(&set);
		FD_SET(sockfd, &set);
		if( select(sockfd+1, NULL, &set, NULL, &tm) > 0)
		{
			getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
			if(error == 0) {
				ret = true;
			}
			else ret = false;
		} 
		else ret = false;		
	}
    if (!ret) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int connect_sock(struct sockaddr_in addr){
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        DBG("%s\n", strerror(errno));
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        DBG("%s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    close(sockfd);
    return 0;
}

bool check_connect(struct sockaddr_in addr, long timeout){
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        DBG("%s\n", strerror(errno));
        return false;
    }
    int error = -1, len;
	len = sizeof(int);
	struct timeval tm;
	fd_set set;
	unsigned long ul = 1;
	ioctl(sockfd, FIONBIO, &ul);

	bool ret = false;
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		tm.tv_sec = 0;
		tm.tv_usec = timeout;
		FD_ZERO(&set);
		FD_SET(sockfd, &set);
		if( select(sockfd + 1, NULL, &set, NULL, &tm) > 0)
		{
			getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
			if(error == 0) {
				ret = true;
			}
			else ret = false;
		} 
		else ret = false;		
    } else {
    	ret = true;
    }

    close(sockfd);
    return ret;
}



int recv_data(int sockfd, char* buf, int bufsize) {
	size_t num_bytes;
	memset(buf, 0, bufsize);

	num_bytes = recv(sockfd, &buf, bufsize, 0);

	if (num_bytes < 0) {
		return -1;
	}
	return num_bytes;
}



int send_response(int sockfd, int rq) {
	if (send(sockfd, &rq, sizeof(rq), 0) <= 0 ) {
		perror("error sending rq");
		return -1;
	}
	return 0;
}


int recv_response(int sockfd) {
	int res_recv;
	if ((recv(sockfd, &res_recv, sizeof(int), 0)) <= 0) {
		//perror("recv response error:");
		return -1;
	}
	return res_recv;
}

int generate_logname(int code, char *logname, char *log_dir) {
	strcpy(logname, log_dir);
	switch (code) {
		case 100:
			strcat(logname, "/cpu.log");
			break;
		case 101:
			strcat(logname, "/mem.log");
			break;
		case 102:
			strcat(logname, "/disk.log");
			break;
		case 103:
			strcat(logname, "/proc.log");
			break;
		case 104:
			strcat(logname, "/sysinfo.log");
			break;
		case 105:
			strcat(logname, "/users.log");
			break;
		default:
			break;
	}
	return 0;
}



int check_size(char *filename, int size, char *dir) {
	struct stat st;
	int flag;
	char cmd_1[50] = {0};
	char cmd[100] = {0};
	char basename[10] = {0};
	time_t _time;
	struct tm *lt;
 	stat(filename, &st);
	int size_real = st.st_size / 1048576;
	if (size_real >= size) {
		flag = 0;
	} else {
		return 0;
	}

	sprintf(cmd_1, "basename %s", filename);
	FILE *stream = popen(cmd_1, "r");

	fgets(basename, sizeof(basename), stream);

	time(&_time);
	lt = localtime(&_time);

	sprintf(cmd, "cp -a %s %s/%s_%d%d%d%d", filename, dir, basename, lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour);
	system(cmd);
	return 0;
}



int write_pi_log (char *PiHealthLog, const char *format, ...) {  
    va_list arg;  
    int done;  
    FILE* pFile = fopen(PiHealthLog, "a+");
  
    va_start (arg, format);  
  
    time_t time_log = time(NULL);  
    struct tm* tm_log = localtime(&time_log);  
    fprintf(pFile, "%04d-%02d-%02d %02d:%02d:%02d ", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec);  
  
    done = vfprintf (pFile, format, arg);  
    va_end (arg);  
  
    fflush(pFile);
    fclose(pFile);  
    return done;  
} 
//宿船长
