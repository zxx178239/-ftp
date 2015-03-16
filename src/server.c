/*************************************************************************
	> File Name: server.c
	> Author: zxx
	> Mail: zxx178239@163.com 
	> Created Time: Tue 03 Mar 2015 09:23:51 AM CST
 ************************************************************************/

#include "func.h"
void handle_child(int sfd, char *buf, char *server_work, char *client_work);
int main(int argc, char *argv[])
{
	int fd_listen;
	char port[64] = "";
	char ip[64] = "";
	int len;
	//socket
	fd_listen = socket(AF_INET, SOCK_STREAM, 0);
	if(fd_listen == -1)
	{
		perror("socket");
		exit(1);
	}
	//open the file server.conf to get ip and port
	if(file_conf_read(ip, port) == -1)
	{
		perror("conf");
		exit(1);
	}
	//bind
	struct sockaddr_in seraddr;
	memset(&seraddr, 0, sizeof(seraddr));
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(atoi(port));
	seraddr.sin_addr.s_addr = inet_addr(ip);
	if(bind(fd_listen, (struct sockaddr *)&seraddr, 16) == -1)
	{
		perror("bind");
		exit(1);
	}
	//listen
	if(listen(fd_listen, 10) == -1)
	{
		perror("listen");
		exit(1);
	}
	//epoll
	//listen client for link
	int epoll_wait_ret, index;
	int fd_client;
	int recv_ret;
	char command[128];
	pid_t pt;
	int epoll_ret;
	char *server_work_des;
	char client_work_des[128];
	epoll_ret = epoll_create(1024);
	struct epoll_event my_event, my_events[1024];
	memset(&my_event, 0, sizeof(my_event));
	my_event.data.fd = fd_listen;
	my_event.events = EPOLLIN;
	epoll_ctl(epoll_ret, EPOLL_CTL_ADD, fd_listen, &my_event);
	while(1)
	{
		memset(my_events, 0, sizeof(my_events));
		epoll_wait_ret = epoll_wait(epoll_ret, my_events, 1024, -1);
		for(index = 0; index < epoll_wait_ret; ++ index)
		{
			if(my_events[index].data.fd == fd_listen)
			{//new client link
				memset(&my_event, 0, sizeof(my_event));
				fd_client = accept(fd_listen, NULL, NULL);
				my_event.data.fd = fd_client;
				my_event.events = EPOLLIN;
				epoll_ctl(epoll_ret, EPOLL_CTL_ADD, fd_client, &my_event);	
				//get server work description
				memset(client_work_des, 0, sizeof(client_work_des));
				server_work_des = getcwd(NULL, 0);
				recv(fd_client, &len, 4, 0);
				recv_buf(fd_client, client_work_des, len);
				len = strlen(server_work_des);
				send(fd_client, &len, 4, 0);
				send_buf(fd_client, server_work_des, len);
				pt = fork();
			}else
			{//recv msg from client
				if(pt == 0)
				{//child to handle
					while(1)
					{
						memset(command, 0, sizeof(command));
						recv_ret = recv(my_events[index].data.fd, command, sizeof(command), 0);
						if(recv_ret == 0)
						{
							epoll_ctl(epoll_ret, EPOLL_CTL_DEL, my_events[index].data.fd, NULL);
							close(fd_client);
						}else
						{
							handle_child(my_events[index].data.fd, command, server_work_des, client_work_des);
						}
					}
					exit(1);
				}
				//parent
				wait(NULL);
			}
		}
	}
}


void handle_child(int sfd, char *buf, char *server_work, char *client_work)
{
	struct dirent *dt;
	struct stat my_stat;
	char *path;
	char msg[128];
	char send_msg[2048];
	char second_msg[512];
	DIR *pDir;
	int len;
	if(strncmp(buf, "ls", 2) == 0)
	{
		memset(send_msg, 0, sizeof(send_msg));
		path = getcwd(NULL, 0);
		if((pDir = opendir(path)) == NULL)
		{
			perror("opendir");
			return;
		}
		while((dt = readdir(pDir)) != NULL)
		{
			if(strcmp(dt->d_name, ".") == 0 || strcmp(dt->d_name, "..") == 0)
			{
				continue;
			}
			memset(msg, 0, sizeof(msg));
			memset(&my_stat, 0, sizeof(my_stat));
			stat(dt->d_name, &my_stat);
			sprintf(msg, "%-20s%dB\n", dt->d_name, my_stat.st_size);
			strcat(send_msg, msg);
		}
		send_msg[strlen(send_msg) - 1] = '\0';
		len = strlen(send_msg);
		send(sfd, &len, 4, 0);
		send_buf(sfd, send_msg, len);
	}else if(strncmp(buf, "cd", 2) == 0)
	{
		memset(second_msg, 0, sizeof(second_msg));
		memset(send_msg, 0, sizeof(second_msg));
		sscanf(buf, "%*s%s", second_msg);
		if(strcmp(second_msg, "") == 0)	
		{
			path = getcwd(NULL, 0);
		}else
		{
			chdir(second_msg);
			path = getcwd(NULL, 0);
		}
		strcpy(send_msg, path);
		len = strlen(send_msg);
		send(sfd, &len, 4, 0);
		send_buf(sfd, send_msg, len);
		memset(server_work, 0, sizeof(server_work));
		strcpy(server_work, send_msg);
	}else if(strncmp(buf, "pwd", 3) == 0)
	{
		len = strlen(server_work);
		send(sfd, &len, 4, 0);
		send_buf(sfd, server_work, len);
	}else if(strncmp(buf, "gets", 4) == 0 || strncmp(buf, "puts", 4) == 0 || strncmp(buf, "rm", 2) == 0 || strncmp(buf, "mkdir", 5) == 0)
	{
		handle_str(sfd, 0, buf, server_work, client_work);
	}
}
