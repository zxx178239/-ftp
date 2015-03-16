/*************************************************************************
  > File Name: func.c
  > Author: zxx
  > Mail: zxx178239@163.com 
  > Created Time: Tue 03 Mar 2015 10:11:11 AM CST
 ************************************************************************/

#include "func.h"

void send_buf(int sfd, char *buf, int len)
{
	int send_ret;
	int send_num = 0;
	while(send_num < len)
	{
		send_ret = send(sfd, buf + send_num, len - send_num, 0);
		send_num += send_ret;
	}
}

void recv_buf(int sfd, char *buf, int len)
{
	int recv_ret;
	int recv_num = 0;
	while(recv_num < len)
	{
		recv_ret = recv(sfd, buf + recv_num, len - recv_num, 0);
		recv_num += recv_ret;
	}
}
//send
int file_upload(int sfd, char *path)
{
	int fd;
	int send_len;
	char buf[1024];
	fd = open(path, O_RDONLY);
	if(fd == -1)
	{
		send_len = -1;
		send(sfd, &send_len, 4, 0);
		return -1;
	}
	while(memset(buf, 0, sizeof(buf)), (send_len = read(fd, buf, 1024)) != 0)
	{
		send(sfd, &send_len, 4, 0);
		send_buf(sfd, buf, send_len);
	}
	close(fd);
	send_len = 0;
	send(sfd, &send_len, 4, 0);
	printf("file upload success!\n");
	return 0;
}
//recv
int file_download(int sfd, char *path)
{
	int fd;
	int recv_len;
	char buf[1024];
	fd = open(path, O_WRONLY | O_CREAT, 0666);
	while(1)
	{
		memset(buf, 0, sizeof(buf));
		recv(sfd, &recv_len, 4, 0);
		if(recv_len == -1)
		{
			remove(path);
			return -1;
		}
		if(recv_len == 0)
		{
			printf("file download success!\n");
			close(fd);
			break;
		}
		recv_buf(sfd, buf, recv_len);
		write(fd, buf, recv_len);
	}
	return 0;
}
int file_conf_read(char *ip, char *port)
{
	FILE *ser_conf;	//the file description of server.conf
	int index = 0;
	int contain;
	char buf[128];
	ser_conf = fopen(SER_CONF, "r");
	if(ser_conf == NULL)
	{
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	while((contain = fgetc(ser_conf)) != '\n')
	{
		buf[index ++] = (char)contain;
	}
	strcpy(ip, buf);
	memset(buf, 0, sizeof(buf));
	index = 0;
	while((contain = fgetc(ser_conf)) != '\n')
	{
		buf[index ++] = (char)contain;
	}
	strcpy(port, buf);
	return 0;
}
//flag == 0 is server
//flag == 1 is client
void handle_str(int sfd, int flag, char *cmd, char *server_work, char *client_work)
{
	int start_pos = 0;
	int index = 0;
	char path[128];
	char first_str[128] = "";
	char file_op[128];
	char server_work_copy[128];	//use it to save to rm chdir
	//use for rm
	DIR *pDir;
	struct dirent *dt;
	//op for rm
	char dir_name[128];
	int len;
	//get op description
	memset(file_op, 0, sizeof(file_op));
	sscanf(cmd, "%s", file_op);
	start_pos = strlen(file_op);

	while(1)
	{
		for(index = 0; index < strlen(cmd) - start_pos - 1; ++ index)
		{
			cmd[index] = cmd[index + start_pos + 1];
		}
		cmd[index] = '\0';
		index = 0;
		memset(path, 0, sizeof(path));
		sscanf(cmd, "%s", path);
		if(strcmp(cmd, "") == 0)
		{
			if(strcmp(file_op, "rm") == 0 || strcmp(file_op, "mkdir") == 0)
			{
				len = 0;
				send(sfd, &len, 4, 0);
			}
			break;
		}
		if(strcmp(file_op, "mkdir") == 0)
		{
			char send_msg[128];
			int len;
			memset(send_msg, 0, sizeof(send_msg));
			if(mkdir(path, 0775) == 0)
			{
				strcpy(send_msg, "success");
			}else
			{
				strcpy(send_msg, "fail");
			}
			len = strlen(send_msg);
			send(sfd, &len, 4, 0);
			send_buf(sfd, send_msg, len);
		}else if(strcmp(file_op, "rm") == 0)
		{
			memset(dir_name, 0, sizeof(dir_name));
			memset(server_work_copy, 0, sizeof(server_work_copy));
			strcpy(server_work_copy, server_work);
			if(verify_dir(server_work_copy, path, dir_name) == 0)
			{
				pDir = opendir(server_work_copy);
				printf("server_work_copy is %s\n", server_work_copy);
				chdir(server_work_copy);
				while((dt = readdir(pDir)) != NULL)
				{
					if(strcmp(dt->d_name, ".") == 0 || strcmp(dt->d_name, "..") == 0)
					{
						continue;
					}
					handle_rm(sfd, dt->d_name);
				}
				chdir(server_work);
				handle_rm(sfd, dir_name);
			}else
			{
				handle_rm(sfd, path);
			}
		}else if(strcmp(file_op, "gets") == 0 && flag == 0)
		{//server recv gets, server send to client
			handle_dir_txt_send(sfd, server_work, path);
		}else if(strcmp(file_op, "gets") == 0 && flag == 1)
		{//client send gets, client recv from server
			handle_dir_txt_recv(sfd, client_work, path);
		}else if(strcmp(file_op, "puts") == 0 && flag == 0)
		{//server recv puts, server recv from client
			handle_dir_txt_recv(sfd, server_work, path);
		}else
		{//client send puts, client send to server
			handle_dir_txt_send(sfd, client_work, path);
		}
		memset(first_str, 0, sizeof(first_str));
		sscanf(cmd, "%s", first_str);
		start_pos = strlen(first_str);
	}
}

void handle_dir_txt_send(int sfd, char *chdir_path, char *path)
{
	DIR *pDir;
	struct dirent *dt;
	char chdir_path_copy[128];
	char send_msg[1024];
	char dir_name[128];
	int len;
	memset(chdir_path_copy, 0, sizeof(chdir_path_copy));
	memset(dir_name, 0, sizeof(dir_name));
	strcpy(chdir_path_copy, chdir_path);
	if(verify_dir(chdir_path_copy, path, dir_name) == 0)
	{
		//change work description
		chdir(chdir_path_copy);
		//send to client "yes" for dir
		memset(send_msg, 0, sizeof(send_msg));
		strcpy(send_msg, "yes");
		len = strlen(send_msg);
		send(sfd, &len, 4, 0);
		send_buf(sfd, send_msg, len);
		//send dir name
		len = strlen(dir_name);
		send(sfd, &len, 4, 0);
		send_buf(sfd, dir_name, len);
		pDir = opendir(chdir_path_copy);
		while((dt = readdir(pDir)) != NULL)
		{
			//only one dir 
			if(strcmp(dt->d_name, ".") == 0 || strcmp(dt->d_name, "..") == 0)
			{
				continue;
			}
			memset(path, 0, sizeof(path));
			strcpy(path, dt->d_name);
			len = strlen(path);
			send(sfd, &len, 4, 0);
			send_buf(sfd, path, len);
			if(file_upload(sfd, path) == -1)
			{
				printf("file download failed!\n");
			}
		}
		len = -2;
		send(sfd, &len ,4, 0);
		chdir(chdir_path);
	}else
	{
		memset(send_msg, 0, sizeof(send_msg));
		strcpy(send_msg, "no");
		len = strlen(send_msg);
		send(sfd, &len, 4, 0);
		send_buf(sfd, send_msg, len);
		if(file_upload(sfd, path) == -1)
		{
			printf("file download failed!\n");
		}
	}

}
void handle_dir_txt_recv(int sfd, char *chdir_path, char *path)
{
	int len;
	char send_msg[1024];
	char chdir_path_copy[128];
	recv(sfd, &len, 4, 0);
	//cur use send_msg to recv yes or no msg
	memset(send_msg, 0, sizeof(send_msg));
	memset(chdir_path_copy, 0, sizeof(chdir_path_copy));
	strcpy(chdir_path_copy, chdir_path);
	recv_buf(sfd, send_msg, len);
	if(strcmp(send_msg, "yes") == 0)
	{
		//get dir name and mkdir
		memset(send_msg, 0, sizeof(send_msg));
		recv(sfd, &len, 4, 0);
		recv_buf(sfd, send_msg, len);
		mkdir(send_msg, 0775);
		strcat(chdir_path_copy, "/");
		strcat(chdir_path_copy, send_msg);
		chdir(chdir_path_copy);
		while(1)
		{
			memset(send_msg, 0, sizeof(send_msg));
			recv(sfd, &len, 4, 0);
			if(len == -2)
			{
				break;
			}
			recv_buf(sfd, send_msg, len);
			memset(path, 0, sizeof(path));
			strcpy(path, send_msg);
			if(file_download(sfd, path) == -1)
			{
				printf("file upload failed!\n");
			}

		}
		chdir(chdir_path);
	}else
	{
		if(file_download(sfd, path) == -1)
		{
			printf("file upload failed!\n");
		}
	}

}

void handle_rm(int sfd, char *path)
{
	char send_msg[128];
	int len;
	memset(send_msg, 0, sizeof(send_msg));
	if(remove(path) == 0)
	{
		strcpy(send_msg, "success");
	}else
	{
		strcpy(send_msg, "fail");
	}
	len = strlen(send_msg);
	send(sfd, &len, 4, 0);
	send_buf(sfd, send_msg, len);
}
//chdir_path save the path to use chdir
//path save the dir or txt name for strcat and chdir_path
int verify_dir(char *chdir_path, char *path, char *dir_name)
{
	struct stat my_stat;
	memset(&my_stat, 0, sizeof(my_stat));
	strcat(chdir_path, "/");
	strcat(chdir_path, path);
	stat(chdir_path, &my_stat);
	if(S_ISDIR(my_stat.st_mode))
	{
		strcpy(dir_name, path);
		return 0;
	}else
	{
		return -1;
	}
}
