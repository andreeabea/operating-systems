#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

#define MAX_LENGTH 256
#define KEY 15966

void readReq(int fd_req, char *buf)
{
	read(fd_req, buf, 1);
	read(fd_req, buf + 1, (unsigned char)buf[0]);
	buf[(unsigned char)buf[0] + 1] = 0;
}

void writeResp(int fd_resp, char *buf, char *s)
{
	int len = strlen(s);
	buf[0] = len;
	strncpy(buf + 1, s, len);
	write(fd_resp, buf, len + 1);
}

int createShm(int fd_resp, char *buf, unsigned int dim)
{
	int shmId;
	shmId = shmget(KEY, dim, IPC_CREAT | 0664);
	writeResp(fd_resp, buf, "CREATE_SHM");
	if (shmId < 0)
	{
		perror("Could not aquire shm");
		writeResp(fd_resp, buf, "ERROR");
		return -1;
	}
	writeResp(fd_resp, buf, "SUCCESS");
	return shmId;
}

void writeShm(int shmId, unsigned int offset, unsigned int val, int fd_resp, char *buf, unsigned int dim)
{
	unsigned char *sharedChar = (unsigned char *)shmat(shmId, NULL, 0);
	if (sharedChar == (void *)-1 || offset + sizeof(val) > dim)
	{
		perror("Could not attach to shm");
		writeResp(fd_resp, buf, "ERROR");
		return;
	}
	sharedChar[offset] = (unsigned char)val;
	sharedChar[offset + 1] = (unsigned char)(val >> 8);
	sharedChar[offset + 2] = (unsigned char)(val >> 16);
	sharedChar[offset + 3] = (unsigned char)(val >> 24);

	writeResp(fd_resp, buf, "SUCCESS");
	shmdt(sharedChar);
}

char *mapFile(char *file, int fd_resp, char *buf, int *size)
{
	char fileName[MAX_LENGTH];
	fileName[0] = '.';
	fileName[1] = '/';
	fileName[2] = 0;
	strcat(fileName, file + 1);
	int fd = open(fileName, O_RDONLY);
	writeResp(fd_resp, buf, "MAP_FILE");

	if (fd == -1)
	{
		perror("Could not open input file");
		writeResp(fd_resp, buf, "ERROR");
		return NULL;
	}
	*size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	char *data = (char *)mmap(NULL, *size, PROT_READ, MAP_SHARED, fd, 0);
	if (data == (void *)-1)
	{
		perror("Could not map file");
		writeResp(fd_resp, buf, "ERROR");
		close(fd);
		return NULL;
	}
	writeResp(fd_resp, buf, "SUCCESS");
	//munmap(data, size);
	close(fd);
	return data;
}

int convertInt(unsigned char *s, int n) //converts a string to an integer
{
	int nr = 0;
	int power = 1;
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if (s[i] & 1)
				nr += power;
			power *= 2;
			s[i] = s[i] >> 1;
		}
	}
	return nr;
}

int getSectionOffset(char *data, unsigned int section_no, unsigned int offset, unsigned int no_of_bytes)
{
	unsigned char offs[4];
	unsigned char no_of_sections[1];
	unsigned char size[4];

	no_of_sections[0] = data[6];
	if (section_no > convertInt(no_of_sections, 1))
	{
		return -1;
	}

	for (int i = 0; i < 4; i++)
	{
		offs[i] = data[7 + (section_no - 1) * 22 + 14 + i];
	}
	int offset_section = convertInt(offs, 4);

	for (int i = 0; i < 4; i++)
	{
		size[i] = data[7 + (section_no - 1) * 22 + 18 + i];
	}
	if (offset + no_of_bytes > (unsigned int)convertInt(size, 4))
	{
		return -1;
	}
	return offset_section;
}

int getOffset(char *data, unsigned int logical_offset)
{
	unsigned char no_of_sections[1];
	no_of_sections[0] = data[6];
	int nb_sections = convertInt(no_of_sections, 1);

	unsigned char offset[5];
	unsigned char size[5];
	unsigned int section_no = 1;
	
	int nb_files = logical_offset / 2048 + 1;
	int last_offset = 0, inside_offset = logical_offset % 2048;
	int ssize = 0;

	while (section_no <= nb_sections)
	{
		for (int i = 0; i < 4; i++)
		{
			offset[i] = data[7 + (section_no - 1) * 22 + 14 + i];
			size[i] = data[7 + (section_no - 1) * 22 + 18 + i];
		}
		ssize += convertInt(size, 4) / 2048 + 1;
		if(ssize>=nb_files)
		{
			last_offset = convertInt(offset,4);
			break;
		}
		section_no++;
	}
	return last_offset + inside_offset;
}

int main(int argc, char **argv)
{
	char buf[MAX_LENGTH];
	int shmId;
	unsigned int dim;
	char *data = NULL;
	int size;

	if (mkfifo("RESP_PIPE_44485", 0664 | S_IFIFO) < 0)
	{
		printf("ERROR\ncannot create the response pipe\n");
		exit(1);
	}
	int fd_req = open("REQ_PIPE_44485", O_RDWR);
	if (fd_req < 0)
	{
		printf("ERROR\ncannot open the request pipe\n");
		exit(1);
	}
	int fd_resp = open("RESP_PIPE_44485", O_RDWR);
	if (fd_resp < 0)
	{
		printf("ERROR\ncannot open the response pipe\n");
		exit(1);
	}

	writeResp(fd_resp, buf, "CONNECT");

	readReq(fd_req, buf);

	while (strcmp(buf + 1, "EXIT") != 0)
	{
		if (strcmp(buf + 1, "PING") == 0)
		{
			writeResp(fd_resp, buf, "PING");
			writeResp(fd_resp, buf, "PONG");
			unsigned int nb = 44485;
			write(fd_resp, &nb, 4);
		}
		else if (strcmp(buf + 1, "CREATE_SHM") == 0)
		{
			read(fd_req, (void *)(&dim), 4);
			shmId = createShm(fd_resp, buf, dim);
		}
		else if (strcmp(buf + 1, "WRITE_TO_SHM") == 0)
		{
			unsigned int val, offset;
			read(fd_req, (void *)&offset, 4);
			read(fd_req, (void *)&val, 4);
			writeResp(fd_resp, buf, "WRITE_TO_SHM");

			if (offset > 0 && offset < dim)
			{
				writeShm(shmId, offset, val, fd_resp, buf, dim);
			}
			else
			{
				writeResp(fd_resp, buf, "ERROR");
			}
		}
		else if (strcmp(buf + 1, "MAP_FILE") == 0)
		{
			char fileName[MAX_LENGTH];
			readReq(fd_req, fileName);
			data = mapFile(fileName, fd_resp, buf, &size);
		}
		else if (strcmp(buf + 1, "READ_FROM_FILE_OFFSET") == 0)
		{
			unsigned int offset, no_of_bytes;
			read(fd_req, (void *)&offset, 4);
			read(fd_req, (void *)&no_of_bytes, 4);
			writeResp(fd_resp, buf, "READ_FROM_FILE_OFFSET");
			if (data != NULL)
			{
				unsigned char *sharedChar = (unsigned char *)shmat(shmId, NULL, 0);
				if (sharedChar == (void *)-1 || offset + no_of_bytes > size)
				{
					perror("Could not attach to shm");
					writeResp(fd_resp, buf, "ERROR");
				}
				else
				{
					for (int i = 0; i < no_of_bytes; i++)
						sharedChar[i] = data[offset + i];
					writeResp(fd_resp, buf, "SUCCESS");
					shmdt(sharedChar);
				}
			}
			else
			{
				writeResp(fd_resp, buf, "ERROR");
			}
		}
		else if (strcmp(buf + 1, "READ_FROM_FILE_SECTION") == 0)
		{
			unsigned int section_no, offset, no_of_bytes;
			read(fd_req, (void *)&section_no, 4);
			read(fd_req, (void *)&offset, 4);
			read(fd_req, (void *)&no_of_bytes, 4);
			writeResp(fd_resp, buf, "READ_FROM_FILE_SECTION");
			int offset_section = getSectionOffset(data, section_no, offset, no_of_bytes);
			if (offset_section != -1 && data != NULL)
			{
				unsigned char *sharedChar = (unsigned char *)shmat(shmId, NULL, 0);
				if (sharedChar == (void *)-1)
				{
					perror("Could not attach to shm");
					writeResp(fd_resp, buf, "ERROR");
				}
				else
				{
					for (int i = 0; i < no_of_bytes; i++)
						sharedChar[i] = data[offset_section + offset + i];
					writeResp(fd_resp, buf, "SUCCESS");
					shmdt(sharedChar);
				}
			}
			else
			{
				writeResp(fd_resp, buf, "ERROR");
			}
		}
		else if (strcmp(buf + 1, "READ_FROM_LOGICAL_SPACE_OFFSET") == 0)
		{
			unsigned int logical_offset, no_of_bytes;
			read(fd_req, (void *)&logical_offset, 4);
			read(fd_req, (void *)&no_of_bytes, 4);
			writeResp(fd_resp, buf, "READ_FROM_LOGICAL_SPACE_OFFSET");
			int offset = getOffset(data, logical_offset);
			if (data != NULL)
			{
				unsigned char *sharedChar = (unsigned char *)shmat(shmId, NULL, 0);
				if (sharedChar == (void *)-1 || offset + no_of_bytes > size)
				{
					perror("Could not attach to shm");
					writeResp(fd_resp, buf, "ERROR");
				}
				else
				{
					for (int i = 0; i < no_of_bytes; i++)
						sharedChar[i] = data[offset + i];
					writeResp(fd_resp, buf, "SUCCESS");
					shmdt(sharedChar);
				}
			}
			else
			{
				writeResp(fd_resp, buf, "ERROR");
			}
		}
		readReq(fd_req, buf);
	}
	unlink("RESP_PIPE_44485");
	printf("SUCCESS\n");

	return 0;
}
