#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "simplefs.h"

//global variables
int vdisk_fd; // global virtual disk file descriptor
              // will be assigned with the sfs_mount call
              // any function in this file can use this.
char disk_name[128];   // name of virtual disk file
int  disk_size;        // size in bytes - a power of 2
int  disk_blockcount;  // block count on disk

struct FCB{
	int startBlock;
	int size;
	int sizeBytes;	 // asıl size
};
struct directEntry{
	struct FCB fcb;
	int available;
	char fileName[MAXFILENAMESIZE];
	char extra[80];	
};
struct openFileTableEntry{	// element of open file table
	int available;
	struct FCB * fcb;
	int position;
	char fileName[MAXFILENAMESIZE];		
	int openCount;
	int status;
};
struct FAT{
	int next;
	int data;//hash value
};

struct directEntry directory[MAXFILECOUNT];
struct FAT fatBlock[1024*128];
struct openFileTableEntry openFileTable[MAXOPENFILES];
int hash(char * file)
{
	int i=0;
	int cnt=0;
	while(file[i])
	{
		cnt+=(int)file[i];
		i++;
	}
	int a=3465;
	int b=6846;
	return ((a*cnt+b)%23197)%9973;	
}
// This function is simply used to a create a virtual disk
// (a simple Linux file including all zeros) of the specified size.
// You can call this function from an app to create a virtual disk.
// There are other ways of creating a virtual disk (a Linux file)
// of certain size. 
// size = 2^m Bytes
int create_vdisk (char *vdiskname, int m)
{
    char command[BLOCKSIZE]; 
    int size;
    int num = 1;
    int count; 
    size  = num << m;
    count = size / BLOCKSIZE;
    printf ("%d %d", m, size);
    sprintf (command, "dd if=/dev/zero of=%s bs=%d count=%d",
	     vdiskname, BLOCKSIZE, count);
    printf ("executing command = %s\n", command); 
    system (command);
    return (0); 
}

// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk. 
int read_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = read (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
		printf ("read error\n");
		return -1;
    }
    return (0); 
}

// write block k into the virtual disk. 
int write_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = write (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
		printf ("write error\n");
		return (-1);
    }
    return 0; 
}


/**********************************************************************
   The following functions are to be called by applications directly. 
***********************************************************************/

int sfs_format (char *vdiskname){

	strcpy (disk_name, vdiskname); // CAN CHANGE	
	
	vdisk_fd = open (disk_name, O_RDWR); 
	if (vdisk_fd == -1) {
		printf ("disk open error %s\n", disk_name); 
		return(-1); 
	}
	
	//perform your format operations here. 
	printf ("formatting disk=%s\n", disk_name);
	
	for (int i = 0; i < MAXFILECOUNT; i++){
		directory[i].available = 1;
		directory[i].fcb.size = 0;
		directory[i].fcb.startBlock = -1;//todo
		directory[i].fcb.sizeBytes = 0;
		//todo filename initialization 
	}
	
	for(int i = 0; i < 1024*128; i++){
		fatBlock[i].next = 0;
		fatBlock[i].data = -1;
	}	
	char block[BLOCKSIZE];
	for(int i = 0; i < 7; i++){
		memcpy(block, &directory[8*i], 8*(sizeof(struct directEntry)));	
		int fd = write_block((void *) block, i+1);
		//printf("\n");
	}	
	char fBlock[BLOCKSIZE];
	for(int i=0; i<1024;i++){
		memcpy(fBlock, &fatBlock[128*i], 128*(sizeof(struct FAT)));	
		write_block((void *)fBlock, i+8);
	}	
	fsync(vdisk_fd);
	close(vdisk_fd);
	
	for(int i=0;i<MAXOPENFILES;i++)
	{
		openFileTable[i].available=1;
	/*	openFileTable[i].position=-1;
		openFileTable[i].openCount=0;
		openFileTable[i].status=-1;
		openFileTable[i].fcb->size = 0;
		openFileTable[i].fcb->startBlock = -1;
		openFileTable[i].fcb->sizeBytes = 0;*/
	}

	
    return (0); 
}

int sfs_mount (char *vdiskname)
{
    // simply open the Linux file vdiskname and in this
    // way make it ready to be used for other operations.
    // vdisk_fd is global; hence other function can use it. 
    vdisk_fd = open(vdiskname, O_RDWR); 
    return(0);
}

int sfs_umount ()
{
    fsync (vdisk_fd); 
    close (vdisk_fd);
    return (0); 
}


int sfs_create(char *filename)
{
	if(sizeof(filename)>MAXFILENAMESIZE)	
  {	return (-1);}
    //efficiency bak
    for(int i=0; i<MAXFILECOUNT; i++)
    {
    	if(strcmp(directory[i].fileName, filename) == 0){
    		printf("the file already exits\n");
    		return(-1);
    		//return(sfs_open(filename, ));	
    	}
    }
    
    //find an empyt dir
    //!!!!!!!!!!!!!!!!!!
    int fnd=0;
    for(int i=0; (i<MAXFILECOUNT)&& (fnd==0); i++){
    	if(directory[i].available == 1){
    		fnd=1;
    		directory[i].available = 0;
    		directory[i].fcb.sizeBytes = 0;
    		directory[i].fcb.size = 0;
    		strcpy(directory[i].fileName,filename);
    		//printf("i: %d filename: %s \n",i, directory[i].fileName);
    		return (0);
    	}
    }
    return (-1);
}

int sfs_open(char *file, int mode)
{
	//file already open
	int found = 0;
	for(int i=0; (i < MAXOPENFILES) &&(found==0); i++){
		if((openFileTable[i].available==0) && (strcmp(openFileTable[i].fileName,file)==0)){
			found = 1;
			//todo
			//openFileTable[i].openCount++;
			//same
			if(openFileTable[i].status == mode)
			{
				printf("file already open\n");
				return i;
			}
			else{
				printf("You can either open a file in Read or Append mode\n");		
				return -1;
			}	
		}		
	}	
	int fileFound=0;
	for(int i = 0; (i<MAXFILECOUNT)&&(fileFound ==0); i++){
		if((strcmp(directory[i].fileName,file)==0)&& directory[i].available==0 ){
			fileFound=1;
			int spaceFound=0;
			for(int j =0;(j<MAXOPENFILES)&&(spaceFound==0);j++)
			{
				if(openFileTable[j].available==1)
				{
					spaceFound=1;
					openFileTable[j].available=0;
					openFileTable[j].position=0;
					openFileTable[j].openCount =1;
					openFileTable[j].status = mode;
					strcpy(openFileTable[j].fileName, file);
					openFileTable[j].fcb=&(directory[i].fcb);
					return j;
				}
			}
		}
	}
    return (-1); 
}

int sfs_close(int fd){
	if(openFileTable[fd].available == 0){
		openFileTable[fd].openCount--;
		if(openFileTable[fd].openCount < 1){
			openFileTable[fd].available = 1;
		}
		return 0;
	}
	else{
		printf("there is no such opened file at index %d\n", fd);	 
		return (-1); 
	}
 
}

int sfs_getsize (int  fd)
{
	if(openFileTable[fd].available == 1){
		printf("The index %d is empty \n", fd);
		return (-1);
	}
	else if(openFileTable[fd].available == 0)
	{
		return openFileTable[fd].fcb->sizeBytes;
	}
    return (-1); 
}

int sfs_read(int fd, void *buf, int n){
	int number=-1;
	if(openFileTable[fd].available == 1){
		printf("The index %d is empty \n",fd);
		return -1;
	}
	else{
		if(openFileTable[fd].fcb->startBlock == -1)
		{
			printf("There is no data in the file!\n");
			return -1;
		}
		else{
			//blockReadAdress
			int fatAddress = openFileTable[fd].fcb->startBlock;
			//blockRead Position
			int blockPosition= openFileTable[fd].position / BLOCKSIZE;
			for(int i=0;i<blockPosition; i++){
				fatAddress = fatBlock[fatAddress].next;
			}
			//todo if ekleyebiliriz Blocksize ı check etmek için
			int disp=openFileTable[fd].position % BLOCKSIZE;
			char block[BLOCKSIZE];
			read_block((void *)block,fatAddress+1032);
			
			number = 0;
			int blockNum = openFileTable[fd].fcb->sizeBytes / BLOCKSIZE;
			int tempBuffCnt = 0;
			char tempBuff[n];
			int x=openFileTable[fd].fcb->sizeBytes;
			//printf("num:%d n:%d block:%d x:%d\n",number,n,blockNum,x);
			while((number<n)&&(blockNum>=0)&&(number<x)){	
			//printf("fat adres: %d\n", fatAddress+1032);				
				tempBuff[tempBuffCnt]=block[disp];
				if(block[disp] == -4373)
					break;
				//printf("b: %c\n", block[disp]);
				disp++;
				number++;
				openFileTable[fd].position++;
				tempBuffCnt++;
				if(disp>=BLOCKSIZE){
					blockNum--;
					fatAddress = fatBlock[fatAddress].next;
					disp=0;
					read_block((void *)block,fatAddress+1032);
				}
			}
		/*	void *lolo=(void *) tempBuff;
			for(int k=0;k<n;k++)
				buf[k]=(void *)lolo[k];
			*/

			memcpy(buf,tempBuff,n);
			return number;	
		}
		return -1;
	}
    return -1; 
}

int sfs_append(int fd, void *buf, int n)
{
	int cnt = 0;
	if(openFileTable[fd].available == 1){
		printf("The index %d is empty\n",fd);
		return -1;
	}
	else{
		char tempBuffer[n];
		char *lol=(char *) buf;
		for(int l=0;l<n;l++)
			tempBuffer[l]=lol[l];
		//memcpy(tempBuffer,buf,n);
		int sb=openFileTable[fd].fcb->sizeBytes;
		//int disp=openFileTable[fd].fcb->sizeBytes % BLOCKSIZE;
		//first write
		int found=0;
		for(int i=0; (i<FATBLOCK*128)&&(found==0) ;i++){
			int hashVal=hash(openFileTable[fd].fileName);
			if(fatBlock[i].data == -1 || (fatBlock[i].data == hashVal)){
				if(cnt >= n)
					return cnt;
				if(openFileTable[fd].fcb->sizeBytes == 0)
					openFileTable[fd].fcb->startBlock = i;
				int address = openFileTable[fd].fcb->startBlock;
				for(int x=0; x<(openFileTable[fd].fcb->sizeBytes/BLOCKSIZE);x++){
					if(fatBlock[address].next !=0){
						address=fatBlock[address].next;}
				}
				int disp=openFileTable[fd].fcb->sizeBytes % BLOCKSIZE;
				found = 1;
				//openFileTable[fd].fcb->startBlock = address;
				fatBlock[address].data = hashVal;//block is not empty anymore
				char block[BLOCKSIZE];
				read_block((void *)block,address+1032);
				int blockCtrl=0;
				int xyz = 0;
				for(int j = 0; (j < n) && (blockCtrl ==0); j++){
					/*printf("address: %d j: %d sb: %d n: %d \n",address,j,openFileTable[fd].fcb->sizeBytes,n);*/
					block[openFileTable[fd].fcb->sizeBytes % BLOCKSIZE]=tempBuffer[j];
					//memcpy(&(block[j]),&(tempBuffer[j]),1);
					openFileTable[fd].fcb->sizeBytes++;
					cnt++;
					disp++;
					
					if((disp % BLOCKSIZE) == 0 ){//block doldu
						//printf("burdayım\n");
						found=0;
						blockCtrl=1;
						xyz=1;
						
						//write_block((void *)block,address+1032);
						int founded=0;
						for(int k=0; (k<FATBLOCK*128)&&(founded==0) ;k++)
						{
							if(fatBlock[k].data == -1 ){
								fatBlock[address].next=k;
								founded=1; 
							}
						}													
					}
					//printf("xyz %d\n", xyz);
				}
				
				write_block((void *)block, address+1032);
				xyz = 0;
				/*char block2[1024];
				read_block((void *)block2, i+1032);*/
			}
			else{
			//printf("girmedim\n"); 
			}
		}
		return cnt;
	}	
    return cnt; 
}

int sfs_delete(char *filename){
	int ret=-1;
	for(int i = 0; i < MAXOPENFILES; i++){
		if(strcmp(openFileTable[i].fileName,filename)==0 && openFileTable[i].available ==0){
			ret=0;
			int blockNum = openFileTable[i].fcb->sizeBytes / BLOCKSIZE;
			directory[i].available=1;
			int fatAddress = openFileTable[i].fcb->startBlock;
			for(int j=0;j<blockNum+1;j++){
				char block[1024];
				for(int k=0;k<1024;k++){
					block[k]=-4373;
				}
				write_block((void *)block,fatAddress+1032);		
				fatAddress = fatBlock[fatAddress].next;	
				directory[i].fcb.startBlock=-1;
				fatBlock[fatAddress+j].data=-1;		
			}
			break;//todo	
		}
	}
    return ret; 
}

