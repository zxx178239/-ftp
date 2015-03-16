/*************************************************************************
  > File Name: client.c
  > Author: zxx
  > Mail: zxx178239@163.com 
  > Created Time: Tue 03 Mar 2015 10:59:28 AM CST
 ************************************************************************/

#include "func.h"
int verify_cmd(char *buf);
void show_help();
void handle_cmd(int sfd, char *cmd, char *server_work, char *client_work);
int main(int argc, char *argv[])
{
	int fd_client;
	int verify_ret;
	char ip[64] = "";
	char port[64] = "";
	int len;
	char command[128];
	//socket
	fd_client = socket(AF_INET, SOCK_STREAM, 0);
	if(fd_client == -1)
	{
		perror("socket");
		exit(1);
	}
	//get ip and port from server.conf
	file_conf_read(ip, port);
	//connect
	struct sockaddr_in seraddr;
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(atoi(port));
	seraddr.sin_addr.s_addr = inet_addr(ip);
	if(connect(fd_client, (struct sockaddr *)&seraddr, 16) == -1)
	{
		perror("connect");
		exit(1);
	}
	char server_work_des[128];
	char *client_work_des;
	//get server work description
	memset(server_work_des, 0, sizeof(server_work_des));
	client_work_des = getcwd(NULL, 0);
	len = strlen(client_work_des);
	send(fd_client, &len, 4, 0);
	send_buf(fd_client, client_work_des, len);
	recv(fd_client, &len, 4, 0);
	recv_buf(fd_client, server_work_des, len);
	while(memset(command, 0, sizeof(command)), fgets(command, 128, stdin) != NULL)
	{
		verify_ret = verify_cmd(command);
		if(verify_ret == -1)
		{
			system("clear");
			printf("cmd is error!\n");
			continue;
		}
		if(strncmp(command, "help", 4) == 0)
		{
			show_help();
		}else
		{
			handle_cmd(fd_client, command, server_work_des, client_work_des);
		}
	}
}
int verify_cmd(char *buf)
{
	if(strstr(buf, "ls") == NULL \
			&& strstr(buf, "help") == NULL \
			&& strstr(buf, "cd") == NULL \
			&& strstr(buf, "pwd") == NULL \
			&& strstr(buf, "gets") == NULL \
			&& strstr(buf, "puts") == NULL \
			&& strstr(buf, "rm") == NULL \
			&& strstr(buf, "mkdir") == NULL
	  )
	{
		return -1;
	}else
	{
		return 0;
	}
}

void handle_cmd(int sfd, char *cmd, char *server_work, char *client_work)
{
	int len;
	char recv_msg[1024];
	char second_msg[128];
	if(strncmp(cmd, "ls", 2) == 0 || \
		strncmp(cmd, "pwd", 3) == 0 || \
		strncmp(cmd, "cd", 2) == 0 
		)
	{
		memset(recv_msg, 0, sizeof(recv_msg));
		send(sfd, cmd, strlen(cmd), 0);
		recv(sfd, &len, 4, 0);
		recv_buf(sfd, recv_msg, len);
		system("clear");
		printf("%s\n", recv_msg);
	}else if(strncmp(cmd, "rm", 2) == 0 || strncmp(cmd, "mkdir", 5) == 0)
	{
		send(sfd, cmd, strlen(cmd), 0);
		system("clear");
		while(1)
		{
			memset(recv_msg, 0, sizeof(recv_msg));
			recv(sfd, &len, 4, 0);
			if(len == 0)
				break;
			recv_buf(sfd, recv_msg, len);
			printf("%s\n", recv_msg);
		}
	}else
	{
		memset(second_msg, 0, sizeof(second_msg));
		sscanf(cmd, "%*s%s", second_msg);
		if(strcmp(second_msg, "") == 0)
		{
			system("clear");
			printf("command is error!\n");
			return;
		}
		send(sfd, cmd, strlen(cmd), 0);
		if(strncmp(cmd, "puts", 4) == 0 || strncmp(cmd, "gets", 4) == 0)
		{
			handle_str(sfd, 1, cmd, server_work, client_work);
		}
	}
}

void show_help()
{
	system("clear");
	printf("command as follow: \n");
	printf("\tls\n");
	printf("\tpwd\n");
	printf("\tcd [des]\n");
	printf("\trm [filename]\n");
	printf("\tgets [filename]\n");
	printf("\tputs [filename]\n");
	printf("\tmkdir [dirname]\n");
}
