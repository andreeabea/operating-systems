#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>

#define __DEBUG

#ifdef __DEBUG
void debug_info (const char *file, const char *function, const int line)
{
        fprintf(stderr, "DEBUG. ERROR PLACE: File=\"%s\", Function=\"%s\", Line=\"%d\"\n", file, function, line);
}

#define ERR_MSG(DBG_MSG) { \
        perror(DBG_MSG); \
        debug_info(__FILE__, __FUNCTION__, __LINE__); \
}

#else                   // with no __DEBUG just displays the error message

#define ERR_MSG(DBG_MSG) { \
        perror(DBG_MSG); \
}

#endif

#define MAX_PATH_LEN 256

bool checkPermissions(struct stat fileMetadata, char* permissions)
{
	if(*permissions==0)
		return true;
	//user's permissions
	if(fileMetadata.st_mode & S_IRUSR)
    	{
    		if(permissions[0]!='r')	return false;
    	}
    	else if(permissions[0]=='r') return false;

	if(fileMetadata.st_mode & S_IWUSR)
	{    
		if(permissions[1]!='w') return false;
	}
    	else if(permissions[1]=='w') return false;

	if(fileMetadata.st_mode & S_IXUSR)
	{    
		if(permissions[2]!='x') return false;
	}
    	else if(permissions[2]=='x') return false;
	//group permissions
	if(fileMetadata.st_mode & S_IRGRP)
    	{
    		if(permissions[3]!='r')	return false;
    	}
    	else if(permissions[3]=='r') return false;

	if(fileMetadata.st_mode & S_IWGRP)
	{    
		if(permissions[4]!='w') return false;
	}
    	else if(permissions[4]=='w') return false;

	if(fileMetadata.st_mode & S_IXGRP)
	{    
		if(permissions[5]!='x') return false;
	}
    	else if(permissions[5]=='x') return false;
	//others' permissions
	if(fileMetadata.st_mode & S_IROTH)
    	{
    		if(permissions[6]!='r')	return false;
    	}
    	else if(permissions[6]=='r') return false;

	if(fileMetadata.st_mode & S_IWOTH)
	{    
		if(permissions[7]!='w') return false;
	}
    	else if(permissions[7]=='w') return false;

	if(fileMetadata.st_mode & S_IXOTH)
	{    
		if(permissions[8]!='x') return false;
	}
    	else if(permissions[8]=='x') return false;

	return true;
}

int convertInt(unsigned char* s, int n) //converts a string to an integer
{
	int nr=0;
	int power=1;
	for(int i=0;i<n;i++)
	{
		for(int j=0;j<8;j++)
		{
			if(s[i]&1)
			   nr+=power;
			power*=2;
			s[i]=s[i]>>1;
		}
	}
	return nr;
}

//get option arguments that can be in any order, for list command
void getListOptions(int argc, char** argv, char* name_start, char* perm, char* path, int *recursive)
{
	for(int i=2;i<argc;i++)
		     {
			if(strcmp(argv[i], "recursive")==0)
				*recursive=1;
			else if(strstr(argv[i], "name_starts_with=")!=NULL)
				strcpy(name_start,argv[i]+17);
			else if(strstr(argv[i], "permissions=")!=NULL)
				strcpy(perm,argv[i]+12);
			else if(strstr(argv[i],"path=")!=NULL)
				strcpy(path,argv[i]+5);
		     }	
}

//get option arguments that can be in any order, for extract command
void getExtractOptions(int argc, char** argv, char* path, int* section, int* line)
{
	for(int i=2;i<argc;i++)
	{
		if(strstr(argv[i],"section=")!=NULL)
		{
			*section=atoi(argv[i]+8);
		}
		else if(strstr(argv[i],"line=")!=NULL)
		{
			*line=atoi(argv[i]+5);
		}
		else if(strstr(argv[i],"path=")!=NULL)
			strcpy(path,argv[i]+5);
	}
}

typedef struct s
{
	unsigned char name[14];
	int type;
	int offset;
	int size;
}section;

//checks if a file is a SF file and returns its section headers
section* checkSF(int fd, int print, int* nb_sections)
{
	int nr;
	char magic[3];
	unsigned char header_size[2], version[2], no_of_sections[1];
	int v;
	section* section_header = NULL;
	if((nr = read(fd, magic, 2)) != 0)
	{
		magic[2]=0;
		if(strcmp(magic,"6P")!=0)
		{
			if(print==1)
				printf("ERROR\nwrong magic");
			else if(print==0) printf("ERROR\ninvalid file");
			return NULL;
		}
	}
	else if(nr<0)
	{
		if(print==1)
			printf("Error\nreading from source file");
		else if(print==0) printf("ERROR\ninvalid file");
        	return NULL;
	}

	if((nr = read(fd, header_size, 2)) < 0)
	{
		if(print==1)
			printf("Error\nwrong header size");
		else if(print==0)printf("ERROR\ninvalid file");
        	return NULL;
	}
	if((nr=read(fd,version,2)) !=0)
	{
		v = convertInt(version,2);
		if(v<40 || v>139)
		{
			if(print==1)
				printf("ERROR\nwrong version");
			else if(print==0) printf("ERROR\ninvalid file");
			return NULL;
		}
	}
	else if(nr<0)
	{
		if(print==1)
			printf("Error\nreading from source file");
		else if(print==0)printf("ERROR\ninvalid file");
        	return NULL;
	}
	if((nr=read(fd,no_of_sections,1)) !=0)
	{
		*nb_sections = convertInt(no_of_sections,1);
		if(*nb_sections<5 || *nb_sections>14)
		{
			if(print==1)
				printf("ERROR\nwrong sect_nr");
			else if(print==0)printf("ERROR\ninvalid file");			
			return NULL;
		}
	}
	else if(nr<0)
	{	
		if(print==1)
			printf("Error\nreading from source file");
		else if(print==0)printf("ERROR\ninvalid file");
        	return NULL;
	}

	section_header = (section*)calloc(*nb_sections,sizeof(section));

	for(int i=0;i<*nb_sections;i++)
	{
		if((nr = read(fd, section_header[i].name, 13)) < 0)
		{
			free(section_header);
			if(print==1)
				printf("Error\nwrong section name");
			else if(print==0)printf("ERROR\ninvalid file");
        		return NULL;
		}
		section_header[i].name[13]=0;
		
		unsigned char t[1];
		if((nr=read(fd,t,1)) !=0)
		{
			section_header[i].type = convertInt(t,1);
			if(section_header[i].type!=47 && section_header[i].type!=60 && section_header[i].type!=81 && section_header[i].type!=73 && section_header[i].type!=49 && section_header[i].type!=23)
			{
				free(section_header);
				if(print==1)
					printf("ERROR\nwrong sect_types");
				else if(print==0)printf("ERROR\ninvalid file");
				return NULL;
			}
		}
		else if(nr<0)
		{	
			free(section_header);
			if(print==1)
				printf("Error\nreading from source file");
			else if(print==0)printf("ERROR\ninvalid file");
        		return NULL;
		}

		unsigned char offset[4];
		if((nr = read(fd, offset, 4)) < 0)
		{
			free(section_header);
			if(print==1)
				printf("Error\nwrong header offset");
			else if(print==0)printf("ERROR\ninvalid file");
        		return NULL;
		}
		section_header[i].offset=convertInt(offset,4);

		unsigned char size[4];
		if((nr = read(fd, size, 4)) < 0)
		{
			free(section_header);
			if(print==1)
				printf("Error\nwrong header size");
			else if(print==0)printf("ERROR\ninvalid file");
        		return NULL;
		}
		section_header[i].size=convertInt(size,4);
	}
	
	if(print==1)
	{
		printf("SUCCESS\nversion=%d\nnr_sections=%d\n",v,*nb_sections);
		for(int i=0;i<*nb_sections;i++)
		{
			printf("section%d: %s %d %d\n",i+1,section_header[i].name, section_header[i].type, section_header[i].size);
		}
	}
	return section_header;
}

//extracts a line from a section in a SF file
//if find parameter is 0, the function is used for the extract command, if it is 1, it returns whether a specific line was found or not
bool extractLine(int fd,int sect,int line,section* section_headers,int nb_sections,int find)
{
	int nr;
	if(sect>nb_sections)
	{
		if(find==0)
		{
			printf("ERROR\ninvalid section\n");
            	 	exit(-1);
		}
		else return false;
	}

	section s = section_headers[sect-1];
	char* buf = (char*)calloc(s.size,sizeof(char));
	
	if((nr=lseek(fd,s.offset,SEEK_SET))<0)
	{
		free(buf);
		if(find==0)
		{
			printf("ERROR\ninvalid file");
        		exit(-1);
		}
		else return false;
	}
	if((nr = read(fd, buf, s.size)) != 0)
	{
		int i=0, count=0, length=0;
		char* l =(char*)calloc(s.size,sizeof(char));
		while(i<s.size-2)
		{
			if(buf[i]==0x0D && buf[i+1]==0x0A)
			{
				count++;
				if(count==line && find==0)
				{
					l[length]=0;
					printf("SUCCESS\n%s\n",l);
					break;	
				}
				else if(count>line)
				{
					free(buf);
					if(find==0)
					{
						printf("ERROR\ninvalid line");
						break;
					}
					else return false;
				}
				else 
				{
					i+=2;
					length=0;
				}
			}
			else
			{
				l[length++]=buf[i];
				i++;
			}
		}
		free(l);		
	}
	else if(nr<0)
	{
		free(buf);
		if(find==0)
		{
			printf("ERROR\ninvalid file");
        		exit(-1);
		}
		else return false;
	}
	free(buf);
	return true;
}

//function used for the list command, to list everything in a directory (if sf parameter is 0)
//if sf is 1, all SF files in a directory are listed (for findall command)
void listDir(char *dirName, int recursive, char* name_start, char* permissions, int sf)
{
	DIR* dir;
	struct dirent *dirEntry;
	struct stat inode;
	char name[MAX_PATH_LEN];

	dir = opendir(dirName);
	if (dir == 0) 
	{
		printf("ERROR\nerror opening directory\n");
		exit(-1);
	}

	// iterate the directory contents
	while ((dirEntry=readdir(dir)) != 0) {
		// build the complete path to the element in the directory
		snprintf(name, MAX_PATH_LEN + 1, "%s/%s",dirName,dirEntry->d_name);
		
		// get info about the directory's element
		lstat (name, &inode);

		char* sub=(char*)calloc(MAX_PATH_LEN, sizeof(char));
		strcpy(sub, dirName);
		if(sub[strlen(sub)-1]!='/')
		     strcat(sub,"/");
		strcat(sub,dirEntry->d_name);

		if((name_start!=NULL && strncmp(dirEntry->d_name,name_start,strlen(name_start))==0) && checkPermissions(inode,permissions)==true && 			strcmp(dirEntry->d_name,".")!=0 && strcmp(dirEntry->d_name,"..")!=0)
		{
			if(sf==0)
			{
		 		printf("%s\n", sub);
			}
			else if(sf==1)
			{	
				section* section_headers;
				int nb_sections=0, fd;
				if ((fd = open(sub, O_RDONLY)) < 0) {
       					 printf("Error\nopening the source file");
        				 exit(-1);
    				 }
				 else section_headers=checkSF(fd,-1,&nb_sections);
			
				bool ok=false;
				if(section_headers!=NULL && nb_sections>0)
				{
					for(int i=0;i<nb_sections && ok==false;i++)
					{
					       	ok=extractLine(fd,i,13,section_headers,nb_sections,1);
					}
					if(ok==true)
						printf("%s\n", sub);
				}
				free(section_headers);
			}
		}

		if (S_ISDIR(inode.st_mode) && recursive==1 && strcmp(dirEntry->d_name,".")!=0 && strcmp(dirEntry->d_name,"..")!=0)
		{
			listDir(sub,1, name_start, permissions,sf);
		}
		free(sub);
	}
	free(dirEntry);
	closedir(dir);
}

int main(int argc, char **argv){

    char *path=(char*)calloc(MAX_PATH_LEN, sizeof(char));
    struct stat fileMetadata;
    section* section_headers = NULL;
    int nb_sections=0;

    if(argc >= 2){
        if(strcmp(argv[1], "variant") == 0){
            printf("44485\n");
        }
    }
    if(argc >= 3)
    {
	//listing directory contents - list cmd
	if(strcmp(argv[1], "list") == 0)
	{
		int recursive=0;
		char* name_start=(char*)calloc(MAX_PATH_LEN,sizeof(char));
	        char* perm=(char*)calloc(9,sizeof(char));

		getListOptions(argc,argv,name_start,perm,path,&recursive);

		if (stat(path, &fileMetadata) < 0)
		{  // get info
            	     printf("ERROR\ninvalid directory path\n");
		     free(path);
    		     free(section_headers);
		     free(name_start);
		     free(perm);
            	     exit(-1);
        	}

       		if (S_ISDIR(fileMetadata.st_mode))
        	{ // it is a directory list directory's contents
            	     printf("SUCCESS\n");
            	     
		     listDir(path,recursive,name_start,perm,0);
        	}
        	else
        	{
            	     printf("ERROR\ninvalid directory path\n");
		     free(path);
    		     free(section_headers);
		     free(name_start);
		     free(perm);
            	     exit(-1);
        	}
		free(name_start);
		free(perm);

	}
	//identifying and parsing section files - parse cmd
	else if(strcmp(argv[1], "parse") == 0 && argc==3)
	{
		int fd;
		
		if(strstr(argv[2],"path=") != NULL)
	     	{
		     strcpy(path,argv[2]+5);
	        }

		if (stat(path, &fileMetadata) < 0)
		{  // get info
            	     printf("ERROR\ninvalid directory path\n");
		     free(path);
            	     exit(-1);
        	}
		if (S_ISREG(fileMetadata.st_mode))
		{     
			if((fileMetadata.st_mode & S_IRUSR) || (fileMetadata.st_mode & S_IRGRP) || (fileMetadata.st_mode & S_IROTH))
			{
				 if ((fd = open(path, O_RDONLY)) < 0) {
       					 printf("Error\nopening the source file");
					 free(path);
        				 exit(-1);
    				 }
				 else section_headers=checkSF(fd,1,&nb_sections);
			}	
			else    
			{
				printf("ERROR\npermissions to read file denied");
				free(path);
				exit(-1);
			}
		}
		else
        	{
            	     printf("ERROR\ninvalid file path\n");
		     free(path);
            	     exit(-1);
        	}
	}
	//working with sections - extract cmd
	else if(strcmp(argv[1],"extract")==0)
	{
		int sect=-1, line=-1, fd;
		getExtractOptions(argc,argv,path,&sect,&line);

		if (stat(path, &fileMetadata) < 0)
		{  // get info
            	     printf("ERROR\ninvalid file\n");
		     free(path);
    		     free(section_headers);
            	     exit(-1);
        	}
		if(sect<0)
		{
			printf("ERROR\ninvalid section");
			free(path);
    		        free(section_headers);
			exit(-1);
		}
		if(line<0)
		{
			printf("ERROR\ninvalid line");
			free(path);
    		        free(section_headers);
			exit(-1);
		}
		if (S_ISREG(fileMetadata.st_mode))
		{     
			if((fileMetadata.st_mode & S_IRUSR) || (fileMetadata.st_mode & S_IRGRP) || (fileMetadata.st_mode & S_IROTH))
			{
				 if ((fd = open(path, O_RDONLY)) < 0) {
       					 ERR_MSG("Error\nopening the source file");
					 free(path);
    		     			 free(section_headers);
        				 exit(-1);
    				 }
				 else section_headers=checkSF(fd,0,&nb_sections);

				 if(section_headers!=NULL && nb_sections>0)
				{	
					extractLine(fd,sect,line,section_headers,nb_sections,0);
				}
			}	
			else    
			{
				printf("ERROR\npermissions to read file denied");
				free(path);
    		     		free(section_headers);
				exit(-1);
			}
		}
		else
        	{
            	     printf("ERROR\ninvalid file");
		     free(path);
    		     free(section_headers);
            	     exit(-1);
        	}
		
	}
	//sections filtering - findall cmd
	else if(strcmp(argv[1],"findall")==0 && argc==3)
	{
		
		if(strstr(argv[2],"path=") != NULL)
	     	{
		     strcpy(path,argv[2]+5);
	        }

		if (stat(path, &fileMetadata) < 0)
		{  // get info
            	     printf("ERROR\ninvalid directory path\n");
		     free(path);
    		     free(section_headers);
            	     exit(-1);
        	}

		if (S_ISDIR(fileMetadata.st_mode))
        	{ // it is a directory list directory's contents
            	     printf("SUCCESS\n");
            	     char* name_start=(char*)calloc(MAX_PATH_LEN,sizeof(char));
	             char* perm=(char*)calloc(9,sizeof(char));
		     listDir(path,1,name_start,perm,1);
		     free(name_start);
		     free(perm);        	
		}
        	else
        	{
            	     printf("ERROR\ninvalid directory path\n");
		     free(path);
    		     free(section_headers);
            	     exit(-1);
        	}
	}
    }
    free(path);
    free(section_headers);
    return 0;
}

