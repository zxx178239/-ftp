#ifndef __FUNC_H__
#define __FUNC_H__
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <dirent.h>
#define SER_CONF "/home/wd/wd/linux_study/ftp_new/conf/server.conf"
//send for the max_length
void send_buf(int sfd, char *buf, int len);
//recv for the max_length
void recv_buf(int sfd, char *buf, int len);
//send to one
int file_upload(int sfd, char *path);
//recv from one
int file_download(int sfd, char *path);
//handle conf file
int file_conf_read(char *ip, char *port);
//rm mkdir gets or puts handle
void handle_str(int sfd, int flag, char *cmd, char *server_work, char *client_work);
//handle the dir or txt send op
void handle_dir_txt_send(int sfd, char *chdir_path, char *path);
//handle the dir or txt recv op
void handle_dir_txt_recv(int sfd, char *chdir_path, char *path);
//handle rm op
void handle_rm(int sfd, char *path);
//verify if the path is dir
//if is , save the path to dir_name to mkdir or rm -rf
int verify_dir(char *chdir_path, char *path, char *dir_name);
#endif
