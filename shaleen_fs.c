/*
  FUSE File System Assignment

  Note-: Code needs /fusedata directory in root folder of machine to create freeblocks

  To Compile:
  gcc -Wall shaleen_fs.c `pkg-config fuse --cflags --libs` -o shaleen_fs
*/

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

//Structure to store File Inode Information
struct file
{
  char name[50];
  int size;
  int uid;
  int gid;
  int mode;
  int linkcount;
  time_t atime;
  time_t ctime;
  time_t mtime;
  int indirect;
  int location;
};
typedef struct file file;

//Structure to store Directory Inode Information
struct directory
{
  char name[50];
  int size;
  int uid;
  int gid;
  int mode;
  time_t atime;
  time_t ctime;
  time_t mtime;
  int linkcount;
  int parentBlock;
  char type[400];
  char filename[400][50];
  int indexBlock[400];

};

typedef struct directory directory;
directory root;

//This function is used to intialize all Directory fields with initial value.
void defaultDirectory(directory* dir, char name[])
{
  sprintf(dir->name,"%s",name);
  dir->size = 4096;
  dir->uid = 1000;
  dir->gid = 1000;
  dir->mode = 1687;
  dir->atime = time(0);
  dir->ctime = time(0);
  dir->mtime = time(0);
  dir->linkcount = 0;
}

//This function is used to intialize all File fields with initial value.
void defaultFile(file* f, char name[], int location)
{
  sprintf(f->name,"%s",name);
  f->size = 4096;
  f->uid = 1000;
  f->gid = 1000;
  f->mode = 1687;
  f->atime = time(0);
  f->ctime = time(0);
  f->mtime = time(0);
  f->linkcount = 0;
  f->indirect = 0;
  f->location = location;
}

//This function is used to add inode information to the parent Directory's filename_toinode_dict
void setInode(directory* cwd,char type, char name[], int indexBlock)
{
  cwd->type[cwd->linkcount] = type;
  sprintf(cwd->filename[cwd->linkcount],"%s",name);
  cwd->indexBlock[cwd->linkcount] = indexBlock;
  cwd->linkcount = cwd->linkcount + 1;
}

//This function is used to write the free memory block numbers
void createFreeBlocks()
{
FILE *file;
file = fopen("/fusedata/fusedata.10000","w");
int x;
for (x=27; x<10000; x++)
  {
      fprintf(file,"%d,", x);
  }
fclose(file);
}

//This function is used to create the 10,000 free blocks
void createBlocks()
{
  FILE *file;
  int i;
  for(i=0;i<10000;i++)
  {
    char filename[50];
    sprintf(filename,"%s%d","/fusedata/fusedata.",i);
    file = fopen(filename,"wb+");
    //char t[10] = "Blank Data";
    //fwrite(t,sizeof(t),1,file);
    fclose(file);
  }
  createFreeBlocks();
}

//This function is used to print the File Inode information. It is used only for debugging.
void printFile(file* dir)
{
  printf("\nPrinting File\n");
  printf("\nSize = %d",dir->size);
  printf("\nName = %s",dir->name);
  printf("\nLinkCount = %d",dir->linkcount);
  printf("\natime = %ld",dir->atime);
  printf("\nmtime = %ld",dir->mtime);
  printf("\nctime = %ld",dir->ctime);
  printf("\nLocation = %d",dir->location);
}

//This function is used to print the Directory Inode information. It is used only for debugging.
void printDirectory(directory* dir)
{
  printf("\nSize = %d",dir->size);
  printf("\nName = %s",dir->name);
  printf("\nLinkCount = %d",dir->linkcount);
  printf("\natime = %ld",dir->atime);
  printf("\nmtime = %ld",dir->mtime);
  printf("\nctime = %ld",dir->ctime);
  printf("\nFilename_to_inode_dict = ");
  int i;
  for(i = 0; i<dir->linkcount; i++)
    {
      printf("%c ",dir->type[i]);
      printf("%s ",dir->filename[i]);
      printf("%d \n",dir->indexBlock[i]);
    }
}

//This function is used to get the last name of the file or directory in the path.
char* getLast(const char *path)
{
  char *token;
  char *last = (char*)malloc(sizeof(path)) ;
  char *rest = (char*)path;
  while(1)
  {
    token = strtok_r(rest,"/",&rest);
    if(token == NULL)
      {
        break;
      }
    strcpy(last,token);
  }
  return last;
}

//This function is used to write Directory Inode information into its indexBlock.
void writeToFile(directory* dir,int indexBlock)
{
  FILE *file;
  char filename[50];
  sprintf(filename,"%s%d","/fusedata/fusedata.",indexBlock);
  file = fopen(filename,"wb+");
  fwrite(dir,sizeof(directory),1,file);
  printf("\nwrite to file IndexBlock = %d",indexBlock);
  printDirectory(dir);
  fclose(file);
}

//This function is used to read Directory Inode information from its indexBlock.
void readFromFile(directory* dir,int indexBlock)
{
  FILE *file;
  char filename[50];
  sprintf(filename,"%s%d","/fusedata/fusedata.",indexBlock);
  file = fopen(filename,"rb");
  fread(dir,sizeof(directory),1,file);
  fclose(file);
}

//This function is used to write File Inode information into its indexBlock.
void writeFile(file* f,int indexBlock)
{
  FILE *fout;
  char filename[50];
  sprintf(filename,"%s%d","/fusedata/fusedata.",indexBlock);
  fout = fopen(filename,"wb+");
  fwrite(f,sizeof(file),1,fout);
  printf("\nwrite to file IndexBlock = %d",indexBlock);
  fclose(fout);
}

//This function is used to read File Inode information from its indexBlock.
void readFile(file* f,int indexBlock)
{
  FILE *fin;
  char filename[50];
  sprintf(filename,"%s%d","/fusedata/fusedata.",indexBlock);
  fin = fopen(filename,"rb");
  fread(f,sizeof(file),1,fin);
  fclose(fin);
}

/*This function is used to get no of free memory block available.
 *It returns -1 if no block is available.
 */
int* getFreeBlocks(int noOfBlocks)
{
  char buf[50000];
  FILE *file;
  file = fopen("/fusedata/fusedata.10000","r");
  fgets(buf,50000, file);
  fclose(file);
  char *token;
  char *rest = (char *)buf;
  int i;
  int* freeBlockList = (int*) malloc(noOfBlocks * sizeof(int));
  int* noFreeBlock = (int*) malloc(sizeof(int));
  noFreeBlock[0] = -1;
  for(i=0;i<noOfBlocks;i++)
  {
    token = strtok_r(rest,",",&rest);
     if(token == NULL)
     {
        return noFreeBlock;
     }
     else
     {
        freeBlockList[i] = atoi(token);
     }
  }
  char outputBuffer[50000];
  int j = 0;
  while(1)
  {
     token = strtok_r(rest,",",&rest);
     if(token == NULL)
        break;
     if(j == 0)
     {
        strcpy(outputBuffer,token);
        strcat(outputBuffer,",");
        j = 1;
     }
     else
     {
        strcat(outputBuffer,token);
        strcat(outputBuffer,",");
     }
  }
  FILE *f;
  f = fopen("/fusedata/fusedata.10000","w");
  fprintf(f,"%s", outputBuffer);
  fclose(f);
  return freeBlockList;
}

//This function is called when fuse file system is mounted
void init()
{
  FILE *fileCheck = NULL;
  fileCheck = fopen("/fusedata/fusedata.0","rb");
  //struct stat st;
  //int result = stat("/fusedata/fusedata.0", &st);
  if(fileCheck == NULL)
  {
     printf("Files not found");
     createBlocks();
     defaultDirectory(&root,"root");
     writeToFile(&root,26);
  }
  else
  {
     fclose(fileCheck);
     printf("\nFiles found");
     directory* head = (directory*)malloc(sizeof(directory));
     readFromFile(head,26);
     root = *(head);
     printf("\nHEAD");
     printDirectory(head);
     printf("\nROOT");
     printDirectory(&root);
  }

}

/*This function is used to create a new directory Inode and
 *update the parent directory's inode information.
 */
int makedir(directory* cwd,char name[])
{
  directory* dir = (directory*)malloc(sizeof(directory));
  int* freeBlocks;
  freeBlocks = getFreeBlocks(1);
  int indexBlock = freeBlocks[0];
  if(indexBlock == -1)
     return -1;
  setInode(cwd,'d',name,indexBlock);
  defaultDirectory(dir, name);
  writeToFile(dir,indexBlock);
  printDirectory(dir);
  free(dir);
  return 0;
}

//This function is invoked when mkdir command is fired from terminal.
static int makeDirectory(const char *path, mode_t mode)
{
  directory *parent = &root;
  printf("\nInside mkdir");
  int indexBlock = 26;
  char *token;
  char *rest = (char*)path;
  char last[100];
  directory *tempdir = (directory*) malloc(sizeof(directory));
  while(1)
  {
    token = strtok_r(rest,"/",&rest);
    if(token == NULL)
        break;
    printf("\nmakedir Token = %s\n", token);
    int i;
    for(i=0; i<parent->linkcount; i++)
    {
      if(strcmp(token,parent->filename[i]) == 0)
      {
        printf("\nmakedir Entry Matched");
        indexBlock = parent->indexBlock[i];
        readFromFile(tempdir,indexBlock);
        parent = tempdir;
        printDirectory(parent);
        break;
      }
    }
    strcpy(last,token);
    printf("\nmakedir Last = %s\n", token);
  }
  int result = 0;
  result = makedir(parent,last);
  if(result == -1)
    return -ENOMEM;
  writeToFile(parent,indexBlock);
  free(tempdir);
  return 0;
}

/*This function is invoked for almost every terminal command.
 *It checks whether the file or directory is present or not.
 *If present it returns 0 otherwise it returns an error
 */
static int getAttribute(const char *path, struct stat *stbuf)
{
  memset(stbuf, 0, sizeof(struct stat));
  int found = 0;
  int indexBlock = 26;
  printf("\nInside GetAttr\n");
  printf("\nPATH = %s",path);
  directory *cwd = &root;
  directory *parent = &root;
  directory *tempdir = (directory*) malloc(sizeof(directory));
  directory *p = (directory*) malloc(sizeof(directory));
  file *fin = (file*) malloc(sizeof(file));
  char *token;
  char lastDirectory[100];
  char pathCopy[100];
  strcpy(pathCopy,path);
  strcpy(lastDirectory,getLast(pathCopy));
  printf("\nLast Directory = %s", lastDirectory);
  char *rest = (char*)path;
  printf("\nAfter token initialized PATH = %s",path);
  while(1)
  {
    token = strtok_r(rest,"/",&rest);
    if(token == NULL)
        break;
    printf("\nToken = %s",token);
    int i;
    for(i=0; i<cwd->linkcount; i++)
    {
      if(strcmp(token,cwd->filename[i]) == 0)
      {
        printf("\nGetattr Entry Matched = %s", token);
        if(parent->type[i] == 'd')
        {
          printf("\nInside d\n");
          indexBlock = cwd->indexBlock[i];
          readFromFile(tempdir,indexBlock);
          cwd = tempdir;
        }
        else if(parent->type[i] == 'f')
        {
          printf("\nInside f\n");
          indexBlock = cwd->indexBlock[i];
          readFile(fin,indexBlock);
        }
        if(strcmp(lastDirectory, token) == 0)
        {
          if(parent->type[i] == 'd')
          {
            printf("\nInside if d\n");
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            stbuf->st_uid = cwd->uid;
            stbuf->st_gid = cwd->gid;
            stbuf->st_blocks = 16;
            stbuf->st_size = cwd->size;
            stbuf->st_atime = cwd->atime;
            stbuf->st_mtime = cwd->mtime;
            stbuf->st_ctime = cwd->ctime;
          }
          else if(parent->type[i] == 'f')
          {
            printf("\nInside if f\n");
            stbuf->st_mode = S_IFREG | 0644;
            stbuf->st_nlink = 1;
            stbuf->st_uid = fin->uid;
            stbuf->st_gid = fin->gid;
            stbuf->st_blocks = 16;
            stbuf->st_size = fin->size;
            stbuf->st_atime = fin->atime;
            stbuf->st_mtime = fin->mtime;
            stbuf->st_ctime = fin->ctime;
          }
          printf("\nGetattr Found and Completed = %s", token);
          printf("\ntesting testing\n");
          found = 1;
          break;
        }
        readFromFile(p,indexBlock);
        parent = p;
        break;
      }
    }
  }
  free(tempdir);
  free(p);
  if(found == 0)
    return -ENOENT;
  else
    return 0;
}

/*This function is called when ls command is fired from terminal.
 *It lists the directories and files present in the parent directory.
 */
static int readDirectory(const char *path, void *buf, fuse_fill_dir_t filler,
       off_t offset, struct fuse_file_info *fi)
{
  (void) offset;
  (void) fi;

  printf("\nInside readdir\n");
  printf("\nreaddir path = %s\n", path);
  directory* cwd = &root;
  int indexBlock = 26;
  char *token;
  char last[100];
  char *rest = (char*)path;
  directory *tempdir = (directory*) malloc(sizeof(directory));
  while(1)
  {
    token = strtok_r(rest,"/",&rest);
    if(token == NULL)
        break;
    int i;
    for(i=0; i<cwd->linkcount; i++)
    {
      if(strcmp(token,cwd->filename[i]) == 0)
      {
        printf("\nReaddir Entry Matched");
        indexBlock = cwd->indexBlock[i];
        readFromFile(tempdir,indexBlock);
        cwd = tempdir;
        printf("\nReaddir Structure After Reading\n");
        printDirectory(cwd);
        break;
      }
    }
    strcpy(last,token);
  }
  int i;
  for(i=0;i<cwd->linkcount;i++)
  {
     printf("\n Readdir Matching Name = %s",cwd->filename[i]);
     filler(buf,cwd->filename[i] , NULL, 0);
  }
  free(tempdir);
  return 0;
}


static int openFile(const char *path, struct fuse_file_info *fi)
{
  printf("\nInside open\n");
  //if (strcmp(path, hello_path) != 0)
  //  return -ENOENT;

  //if ((fi->flags & 3) != O_RDONLY)
  //  return -EACCES;

  return 0;
}

//This function is used to create a new file
int makefile(directory* cwd, char name[])
{
   file* newFile = (file*)malloc(sizeof(file));
   int* freeBlocks;
   freeBlocks = getFreeBlocks(2);
   int indexBlock = freeBlocks[0];
   if(indexBlock == -1)
      return -1;
   int location = freeBlocks[1];
   printf("\nindexblock = %d",indexBlock);
   setInode(cwd,'f',name,indexBlock);
   defaultFile(newFile, name,location);
   writeFile(newFile,indexBlock);
   printFile(newFile);
   free(newFile);
   return 0;
}

//This function is called when touch command is fired on linux terminal
static int createFile(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  printf("\nInside create path = %s",path);
  printf("\ncreate path = %s",path);
  directory *parent = &root;
  printf("\nPath passed to create file = %s",path);
  int indexBlock = 26; //Root is at indexblock 26
  char *token;
  char *rest = (char*)path;
  char last[100];
  directory *tempdir = (directory*) malloc(sizeof(directory));
  while(1)
  {
    token = strtok_r(rest,"/",&rest);
    if(token == NULL)
        break;
    printf("\ncreate Token = %s\n", token);
    int i;
    for(i=0; i<parent->linkcount; i++)
    {
      if(strcmp(token,parent->filename[i]) == 0)
      {
        printf("\ncreate parent Entry Matched");
        indexBlock = parent->indexBlock[i];
        readFromFile(tempdir,indexBlock);
        parent = tempdir;
        printf("\n======Parent Directory Matched = %s =======\n",parent->name);
        printf("\nParent Directory index block = %d",indexBlock);
        printDirectory(parent);
        break;
      }
    }
    strcpy(last,token);
    printf("\ncreate Last = %s\n", token);
    //token = strtok(NULL,path);
  }
  //printf("\nFound\n");
  int result = 0;
  result =makefile(parent,last);
  if(result == -1)
     return -ENOMEM;
  writeToFile(parent,indexBlock);
  free(tempdir);
  return 0;
}

static int readfile(const char *path, char *buf, size_t size, off_t offset,
          struct fuse_file_info *fi)
{
  //size_t len;
  //(void) fi;
  //if(strcmp(path, hello_path) != 0)
  //  return -ENOENT;

  /*len = strlen(hello_str);
  if (offset < len) {
    if (offset + size > len)
      size = len - offset;
    memcpy(buf, hello_str + offset, size);
  } else
    size = 0;*/

  return size;
}

static int chmodFile(const char *path, mode_t mode)
{
  printf("\nInside chmod");
  /*int res;

  res = chmod(path, mode);
  if (res == -1)
    return -errno;*/

  return 0;
}

static int chownFile(const char *path, uid_t uid, gid_t gid)
{
  //int res;
  printf("\nInside chown");
  /*res = lchown(path, uid, gid);
  if (res == -1)
    return -errno;*/

  return 0;
}
static int truncateFile(const char *path, off_t size)
{
  printf("\nInside truncate");
  /*int res;

  res = truncate(path, size);
  if (res == -1)
    return -errno;*/

  return 0;
}

static int utimensFile(const char *path, const struct timespec ts[2])
{
  printf("\nInside utimens");
  /*int res;

  res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
  if (res == -1)
    return -errno;*/

  return 0;
}

static int writefile(const char *path, const char *buf, size_t size,
         off_t offset, struct fuse_file_info *fi)
{
  printf("\nPath = %s",path);
  printf("\nBuffer = %s",buf);
  printf("\nSize = %ld",size);
  int found = 0;
  int indexBlock = 26;
  printf("\nPATH = %s",path);
  directory *cwd = &root;
  directory *parent = &root;
  directory *tempdir = (directory*) malloc(sizeof(directory));
  directory *p = (directory*) malloc(sizeof(directory));
  file *fin = (file*) malloc (sizeof(file));
  char *token;
  char lastDirectory[100];
  char pathCopy[100];
  strcpy(pathCopy,path);
  strcpy(lastDirectory,getLast(pathCopy));
  printf("\nLast Filename = %s", lastDirectory);
  char *rest = (char*)path;
  while(1)
  {
    token = strtok_r(rest,"/",&rest);
    if(token == NULL)
        break;
    int i;
    for(i=0; i<cwd->linkcount; i++)
    {
      if(strcmp(token,cwd->filename[i]) == 0)
      {
        if(parent->type[i] == 'd')
        {
           indexBlock = cwd->indexBlock[i];
           readFromFile(tempdir,indexBlock);
           cwd = tempdir;
        }
        else if(parent->type[i] == 'f')
        {
            indexBlock = cwd->indexBlock[i];
            readFile(fin,indexBlock);
        }
        if(strcmp(lastDirectory, token) == 0)
        {
           FILE *fout;
           char filename[50];
           sprintf(filename,"%s%d","/fusedata/fusedata.",fin->location);
           fout = fopen(filename,"w+");
           fputs(buf,fout);
           printf("\nFile written at location = %d",fin->location);
           fclose(fout);
           found = 1;
           break;
        }
        readFromFile(p,indexBlock);
        parent = cwd;
        break;
      }
    }
  }
  free(tempdir);
  free(fin);
  if(found == 0)
      return -errno;
  else
      return 1;

  /*(void) fi;
  fd = open(path, O_WRONLY);
  if (fd == -1)
    return -errno;

  res = pwrite(fd, buf, size, offset);
  if (res == -1)
    res = -errno;

  close(fd);*/
  //return res;
}

static int flushFile(const char *path, struct fuse_file_info *fi)
{
    printf("\nIniside flush");
    return 0;
}
static int getxattrFile(const char *path, const char *name, char *value,
      size_t size)
{
  printf("\nIniside getXattr");
  return 0;
  //int res = lgetxattr(path, name, value, size);
  //if (res == -1)
  //  return -errno;
  //return res;
}

static struct fuse_operations operationList = {
  .getattr	= getAttribute,
  .readdir	= readDirectory,
  .open		  = openFile,
  .read		  = readfile,
  .mkdir    = makeDirectory,
  .create   = createFile,
  .chmod		= chmodFile,
  .chown		= chownFile,
  .truncate	= truncateFile,
  .utimens	= utimensFile,
  .write = writefile,
  .flush = flushFile,
  .getxattr = getxattrFile,
};

int main(int argc, char *argv[])
{
  init();
  return fuse_main(argc, argv, &operationList, NULL);
}
