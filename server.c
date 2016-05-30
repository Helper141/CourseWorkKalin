#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

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

void ToLower(char *s)
{
	int i;
	for (i=0;i<strlen(s);i++)
	{
		if ((s[i] >= 'A') && (s[i] <= 'Z'))
			s[i] += 32;
	}
}

void Allocate2DArray(char ***array, int size, int length)
{
	int i;
	*array = malloc(size * sizeof(char*));
	AllocationCheck(*array);
	for (i=0;i<size;i++)
	{
		(*array)[i] = malloc(length * sizeof(char));
		AllocationCheck((*array)[i]);
	}
}

void ReAllocate(char ***array, int newSize, int length, int i)
{
	*array = realloc(*array, newSize * sizeof(char*));
	AllocationCheck(*array);
	for(;i<newSize;i++)
	{
		(*array)[i] = malloc(length * sizeof(char));
		AllocationCheck((*array)[i]);
	}
}

void Add(char ***array, int i, int *size, char *add, int length)
{
	if (i == *size)
	{
		*size *= 2;
		ReAllocate(array, *size, length, i);
	}
	strcpy((*array)[i], add);
}

void Print(char **array, int size)
{
	int i;
	for (i=0;i<size;i++)
	{
		if (strlen(array[i]) > 0)
			printf("%s\n", array[i]);
	}
}

void ErrorMessage(int fd, char *message)
{
	int n;
	n = write(fd, message, strlen(message));
	ErrorCheck(n, "write");
}

void Logout(char *username, int *fd, char *fileName)
{
	int i = 0, n, j = 0, tempFile;
	char tempFileName[19] = {0}, temp[50] = {0}, c;
	strcpy(tempFileName, "/tmp/myFile-XXXXXX");
	tempFile = mkstemp(tempFileName);
	ErrorCheck(tempFile, "temporary file");
	unlink(tempFileName);
	while (n = read(*fd, &c, 1))
	{
		ErrorCheck(n, "read");
		if ((int)c != '\n')
			temp[i++] = c;
		else
		{
			if (strcmp(temp, username) != 0)
			{
				sprintf(temp, "%s\n", temp);
				n = write(tempFile, temp, strlen(temp));
				ErrorCheck(n, "write to temporary file");
			}
			i = 0;
			bzero(temp, strlen(temp));
		}
	}
	lseek(tempFile, 0, SEEK_SET);
	lseek(*fd, 0, SEEK_SET);
	ftruncate(*fd, 0);
	while(n = read(tempFile, &c, 1))
	{
		ErrorCheck(n, "read from temporary file");
		n = write(*fd, &c, 1);
		ErrorCheck(n, "write to temporary file");
	}
	close(tempFile);
	lseek(*fd, 0, SEEK_SET);
}

int IsLoggedIn(char *username, int fd)
{
	int i = 0, n;	
	char tempName[50] = {0}, c;
	while (n = read(fd, &c, 1))
	{
		ErrorCheck(n, "read");
		if ((int)c != '\n')
			tempName[i++] = c;
		else
		{
			if (strcmp(tempName, username) == 0)
			{
				lseek(fd, 0, SEEK_SET);
				return 1;
			}
			i = 0;
			bzero(tempName, strlen(tempName));
		}
	}
	return 0;
}

int NameCheck(char *name, int fd, int file)
{
	int i, n;
	if (strlen(name) > 50)
	{
		ErrorMessage(fd, "Username too long! Maximum 50 symbols.\n");
		return 1;
	}
	if (!(((name[0] >= 'A') && (name[0] <= 'Z')) || ((name[0] >= 'a') && (name[0] <= 'z'))))
	{
		ErrorMessage(fd, "Username must begin with a letter.\n");
		return 1;
	}
	for (i=0;i<strlen(name)-1;i++)
	{
		if (((name[i] < 'A') || (name[i] > 'Z')) && ((name[i] < 'a') || (name[i] > 'z')) && ((name[i] < '0') || (name[i] > '9')))
		{
			ErrorMessage(fd, "Username must contain only letters and numbers.\n");
			return 1;
		}
	}
	sscanf(name, "%s", name);
	if (IsLoggedIn(name, file) == 1)
	{
		ErrorMessage(fd, "User already logged in.\n");
		return 1;
	}
	ErrorMessage(fd, "1");
	return 0;
}

int TorrentCheck(int fd, char *torrent, char **userDownloads, int size, int *downloads_fd, int d_fd)
{
	int n, i;
	if (strlen(torrent) == 0)
		return -1;
	if (((int)torrent[0] == 32) || ((int)torrent[0] == 10))
	{
		ErrorMessage(fd, "First symbol must not be space or newline.\n");
		return 1;
	}
	if (strlen(torrent) > 256)
	{
		ErrorMessage(fd, "Torrent name too long! Maximum 256 symbols.\n");
		return 1;
	}
	for (i=0;i<strlen(torrent)-1;i++)
	{
		if (((int)torrent[i] < 32) && ((int)torrent[i] > 126))
		{
			ErrorMessage(fd, "Wrong torrent name.\n");
			return 1;
		}
	}
	for (i=0;i<size;i++)
	{
		if (strcmp(torrent, userDownloads[i]) == 0)
		{
			ErrorMessage(fd, "Torrent already downloaded.");
			return -2;
		}
	}
	ErrorMessage(fd, "1");
	return 0;
}

void WriteToFile(int d_fd, char *username, char *torrent)
{
	int n;
	n = write(d_fd, username, strlen(username));
	ErrorCheck(n, "write");
	n = write(d_fd, " ", 1);
	ErrorCheck(n, "write");
	n = write(d_fd, torrent, strlen(torrent));
	ErrorCheck(n, "write");
	n = write(d_fd, "\n", 1);
	ErrorCheck(n, "write");
}

void SendToUser(int sock, char *torrent)
{
	int n;
	n = write(sock, torrent, strlen(torrent));
	ErrorCheck(n, "write");
}

void TorrentDownload(int sock, char *username, char **userDownloads, int size, int *downloads_fd, int d_fd)
{
	char buffer[258];	
	int n, m, i;
	do
	{
		bzero(buffer, 258);
		n = read(sock, buffer, 257);
		ErrorCheck(n, "read");
		ToLower(buffer);
		n = TorrentCheck(sock, buffer, userDownloads, size, downloads_fd, d_fd);
		if (n == -1)
		{
			close(sock);
			break;
		}
		if (n == 1)
			printf("Wrong torrent name.\n");
		else if (n == -2)
			printf("Torrent '%s' already downloaded by user '%s'\n", buffer, username);
		else
		{
			WriteToFile(d_fd, username, buffer);
			//lseek(*downloads_fd, 0, SEEK_SET);
			WriteToFile(*downloads_fd, username, buffer);
			printf("Torrent '%s' downloaded by '%s'\n", buffer, username);
			lseek(*downloads_fd, 0, SEEK_SET);
			char OK;
			n = read(sock, &OK, 1);	
			ErrorCheck(n, "read");
			if (OK == '1')
				SendToUser(sock, buffer);
			else
				printf("Communication error.\n");
		}
	} while (n == 1);
}

void ReadToEndOfLine(int fd)
{
	char c;
	int n;
	while (n = read(fd, &c, 1))
	{
		ErrorCheck(n, "read");
		if ((int)c == '\n')
			break;
	}
}

void FillArrayDownloads(char ***array, int fd, char *username, int *size, int length)
{
	int i = 0, j = 0, n, k = 0;
	char c, tempName[50] = {0}, tempTorrent[256] = {0};
	while(n = read(fd, &c, 1))
	{
		ErrorCheck(n, "read");
		if ((int)c != 32)
			tempName[i++] = c;
		else
		{
			if (strcmp(tempName, username) == 0)
			{
				while(n = read(fd, &c, 1))
				{
					if ((int)c != '\n')
						tempTorrent[j++] = c;
					else
					{
						Add(array, k, size, tempTorrent, length);
						k++;
						j = 0;
						bzero(tempTorrent, strlen(tempTorrent));
						break;
					}
				}	
			}
			else
				ReadToEndOfLine(fd);
			i = 0;
			bzero(tempName, strlen(tempName));
		}
	}
	lseek(fd, 0, SEEK_SET);	
}

void ClientFunction(int sock, struct in_addr ip, int port, char *username, int i, char *fileName, int *users_fd, int *downloads_fd, int d_fd)
{
	printf("Client IP: %s\t", inet_ntoa(ip));
	printf("Port: %d\n", port);
	int n;
	char buffer[52];
	bzero(buffer, 52);
	do
	{
		n = read(sock, buffer, 51);
		ErrorCheck(n, "read");
		n = NameCheck(buffer, sock, *users_fd);
		if (n == 1)
			printf("Invalid username!\n");
		else
		{
			int m;
			sscanf(buffer, "%s", username);
			printf("User '%s' connected to the server.\n", username);
			printf("ID: %d\n", i);
			m = write(*users_fd, username, strlen(username));
			ErrorCheck(m, "write");
			m = write(*users_fd, "\n", 1);
			ErrorCheck(m, "write");
		}
		bzero(buffer, strlen(buffer));
	} while (n == 1);
	lseek(*users_fd, 0, SEEK_SET);

	char **userDownloads;
	int size = 20, length = 256;
	Allocate2DArray(&userDownloads, size, length);
	FillArrayDownloads(&userDownloads, *downloads_fd, username, &size, length);
	
	lseek(*downloads_fd, 0, SEEK_SET);
	TorrentDownload(sock, username, userDownloads, size, downloads_fd, d_fd);
	
	Logout(username, users_fd, fileName);
	for(i=0;i<size;i++)
		free(userDownloads[i]);
	free(userDownloads);
}

void FillTempDownloads(int *fd, int source)
{
	int n;
	n = lseek(source, 0, SEEK_END);
	lseek(source, 0, SEEK_SET);
	if (n > 0)
	{
		char *buffer = malloc(n);
		n = read(source, buffer, n);
		ErrorCheck(n, "read");
		n = write(*fd, buffer, strlen(buffer));
		ErrorCheck(n, "write");
		free(buffer);
	}
	lseek(*fd, 0, SEEK_SET);
}

int main()
{
	int sockfd, portno, newsockfd, clilen, pid;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i = 0;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	ErrorCheck(sockfd, "open");
	bzero((char*) &serv_addr, sizeof(serv_addr));
	portno = 2000;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	n = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	ErrorCheck(n, "bind");
	printf("The server has been established.\n");

	listen(sockfd, 5);
	clilen = sizeof(cli_addr);
	
	char tempName[24] = "/tmp/loggedUsers-XXXXXX";
	int loggedUsers_fd = mkstemp(tempName);
	ErrorCheck(loggedUsers_fd, "open temporary file loggedUsers");
	unlink(tempName);

	char downloads[22] = "/tmp/downloads-XXXXXX";
	int downloads_fd = mkstemp(downloads);
	ErrorCheck(downloads_fd, "open temporary file downloads");
	unlink(downloads);

	int fdDownloads = open("UserDownloads.txt", O_RDWR | O_APPEND | O_CREAT, 0777);
	ErrorCheck(fdDownloads, "open UserDownloads");

	FillTempDownloads(&downloads_fd, fdDownloads);

	while(1)
	{
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		ErrorCheck(newsockfd, "accept");
		pid = fork();
		ErrorCheck(pid, "fork");
		i++;
		if (pid == 0)
		{
			char username[52] = {0};
			close(sockfd);
			ClientFunction(newsockfd, cli_addr.sin_addr, (int) htons(cli_addr.sin_port), username, i-1, tempName, &loggedUsers_fd, &downloads_fd, fdDownloads);
			printf("''%s' disconnected from the server.\n", username);
			exit(0);
		}
		else
			close(newsockfd);
	}

	close(sockfd);
	close(loggedUsers_fd);
	close(downloads_fd);
	close(downloads_fd);
	return 0;
}
