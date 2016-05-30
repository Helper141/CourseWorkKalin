#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>

void ErrorCheck(int n, char *message)
{
	if (n < 0)
	{
		perror(message);
		exit(1);
	}
}

void AllocationCheck(void *memory)
{
	if (memory == NULL)
	{
		perror("Memory allocation");
		exit(1);
	}
}

int CheckDirectory(char *dir)
{
	if (strlen(dir) > 1024)
	{
		printf("Directory path too long! Maximum 1024 symbols.\n");
		return 1;
	}
	sscanf(dir, "%s", dir);
	struct stat s = {0};
	int err = stat(dir, &s);
	if (-1 == err)
	{
		printf("The selected directory does not exist.\n");
		printf("The file will be downloaded to the current directory.\n");
		return 1;
	}
	return 0;
}

void EnterDirectory(char **dir)
{
	char c[2], ch;
	int n;
	do
	{
		bzero(c, strlen(c));
		printf("Choose different directory? Y/n: ");
		fgets(c, 2, stdin);
		while((ch = getchar()) != '\n' && ch != EOF); 
	} while ((c[0] != 'y') && (c[0] != 'Y') && (c[0] != 'n') && (c[0] != 'N'));
	if ((c[0] == 'y') || (c[0] == 'Y'))
	{
		printf("Enter directory: ");
		fgets(*dir, 1026, stdin);
		if (strlen(*dir) > 1024)
			while((ch = getchar()) != '\n' && ch != EOF); 
		n = CheckDirectory(*dir);
	}
}

void Enter(int sockfd, char *array, int type, int length)
{
	char c, message[50] = {0}, ch;
	int n;
	do
	{
		bzero(array, strlen(array));
		if (type == 0)
			printf("Enter username: ");
		else
			printf("Enter torrent name: ");
		fgets(array, length + 2, stdin);
		if (strlen(array) > length)
			while((ch = getchar()) != '\n' && ch != EOF); 
		if (type == 1)
			sscanf(array, "%[^\t\n]", array);
		n = write(sockfd, array, strlen(array));
		ErrorCheck(n, "write");
		n = read(sockfd, message, 49);
		ErrorCheck(n, "read");
		if (strcmp(message, "1") == 0)
		{
			if (type == 0)
				printf("You have connected to the server.\n");
			else
				printf("Torrent successfully downloaded.\n");
			n = 0;
		}
		else
		{
			printf("Error: %s\n", message);
			if (strcmp(message, "Torrent already downloaded.") == 0)
				exit(1);
			n = 1;
		}
		bzero(message, strlen(message));
	} while (n == 1);
}

void Write(int fd, char *a)
{
	int n;
	n = write(fd, a, strlen(a));
	ErrorCheck(n, "write");
}

void SaveFile(int fd, int sock, char *serverName, int serverPort)
{
	int n, size;
	char buffer[1024];
	n = write(sock, "1", 1);
	ErrorCheck(n, "write");
	n = read(sock, buffer, 1023);
	ErrorCheck(n, "read");
	size = strlen(buffer);
	Write(fd, "File content: ");
	Write(fd, buffer);
	bzero(buffer, strlen(buffer));
	Write(fd, "\nServer: ");
	Write(fd, serverName);
	Write(fd, "\nPort: ");
	sprintf(buffer, "%d\nFile size: %d\n", serverPort, size);
	Write(fd, buffer);		
}

char* RemoveSpaces(char *s)
{
	int i, j = 0;
	char *buffer = malloc(256);
	AllocationCheck(buffer);
	for(i=0;i<strlen(s);i++)
	{
		if ((int)s[i] != 32)
			buffer[j++] = s[i];
	}
	bzero(s, strlen(s));	
	strcpy(s, buffer);
	free(buffer);
	return s;
}

void Download(int sockfd, char *serverName, int serverPort)
{
	char c, message[50] = {0}, username[52] = {0}, torrent[258] = {0}, *dir;
	int n;
	dir = malloc(1026);
	AllocationCheck(dir);
	
	Enter(sockfd, username, 0, 50);
	
	bzero(message, strlen(message));
	
	EnterDirectory(&dir);
	Enter(sockfd, torrent, 1, 256);
	
	char *path = malloc(2048);
	AllocationCheck(path);
	if (strlen(path) > 0)
	{
		strcpy(path, dir);	
		strcat(path, "/");
	}
	strcat(path, torrent);
	strcpy(path, RemoveSpaces(path));
	int dwFile = open(path, O_WRONLY | O_CREAT, 0777);
	ErrorCheck(dwFile, "open");
	
	SaveFile(dwFile, sockfd, serverName, serverPort);
	
	close(dwFile);
	free(dir);
	free(path);
}

int main()
{
	int sockfd, portno, n = 0;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	
	portno = 2000;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	ErrorCheck(sockfd, "open");
	server = gethostbyname("localhost");
	if (server == NULL)
	{
		perror("server");
		return 1;
	}
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	n = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	ErrorCheck(n, "connect");
	
	printf("Connected to '%s', port: %d\n", server->h_name, serv_addr.sin_port);
	Download(sockfd, server->h_name, serv_addr.sin_port);
	return 0;
}
