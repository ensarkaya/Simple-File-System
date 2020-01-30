#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "simplefs.h"
#include <sys/time.h>
#define DISKNAME "vdisk1.bin"

struct FCB
{
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

int main()
{
    int ret;
    int fd, fd2, fd3,fd4; 
    int i; 
    char buffer[1024];
    char bufferr[1024];
    char buffer5[1024];
    char buffer2[8] = {87, 87, 87, 87, 87, 87, 87, 87};
    char buffer3[8] = {80, 80, 80, 80, 80, 80, 80, 80};
    int size;
    char c; 

    printf ("started\n"); 
    
    // ****************************************************
    // if this is the first running of app, we can
    // create a virtual disk and format it as below
    ret  = create_vdisk (DISKNAME, 24); // size = 16 MB
    if (ret != 0) {
		printf ("there was an error in creating the disk\n");
		exit(1); 
    }
	
    ret = sfs_format (DISKNAME);
	if (ret != 0) {
		printf ("there was an error in format\n");
		exit(1); 
    }
    // ****************************************************
	//printf("1234");
    ret = sfs_mount (DISKNAME); 
    if (ret != 0) {
		printf ("could not mount \n");
		exit (1); 
    }
	struct timeval startTime, endTime;    	
	long elapsedTime;
	gettimeofday(&startTime, NULL);
    sfs_create ("file1.bin");
    gettimeofday(&endTime, NULL);
	elapsedTime = (endTime.tv_sec - startTime.tv_sec) * 1000000 +
	endTime.tv_usec - startTime.tv_usec;
	printf("create in ms: %ld\n",  elapsedTime);
    sfs_create ("file2.bin");
    sfs_create ("file3.bin");
    
    
    gettimeofday(&startTime, NULL);
    fd = sfs_open("file1.bin", MODE_APPEND);
    gettimeofday(&endTime, NULL);
	elapsedTime = (endTime.tv_sec - startTime.tv_sec) * 1000000 +
	endTime.tv_usec - startTime.tv_usec;
	printf("open in ms: %ld\n",  elapsedTime);
 	fd2 = sfs_open("file2.bin", MODE_APPEND);   
    //fd3 = sfs_open("file3.bin", MODE_APPEND);
    gettimeofday(&startTime, NULL);
	for(int i=0;i<320000;i++){
		//printf("i: %d ", i);
		memcpy (buffer, buffer2, 8); // just to show memcpy
		sfs_append(fd, (void *) buffer, 8); 
	}
	gettimeofday(&endTime, NULL);
	elapsedTime = (endTime.tv_sec - startTime.tv_sec) * 1000000 +
	endTime.tv_usec - startTime.tv_usec;
	printf("append in ms: %ld\n",  elapsedTime);
	//printf("\n");
	for(int i=0;i<126;i++){
		memcpy (bufferr, buffer3, 8); // just to show memcpy
		sfs_append(fd2, (void *) bufferr, 8); 
	}
	gettimeofday(&startTime, NULL);
	sfs_close(fd);
	gettimeofday(&endTime, NULL);
	elapsedTime = (endTime.tv_sec - startTime.tv_sec) * 1000000 +
	endTime.tv_usec - startTime.tv_usec;
	printf("close in ms: %ld\n",  elapsedTime);
	sfs_close(fd2);
    fd = sfs_open("file1.bin", MODE_READ);
    fd2 = sfs_open("file2.bin", MODE_READ);
    
    size = sfs_getsize (fd);
    //printf("size: %d", size);
    size = 320000*8;
    int size2=126*8;
    char bu[size];
    gettimeofday(&startTime, NULL);
    sfs_read (fd, (void *) bu, size);
    gettimeofday(&endTime, NULL);
	elapsedTime = (endTime.tv_sec - startTime.tv_sec) * 1000000 +
	endTime.tv_usec - startTime.tv_usec;
	printf("read in ms: %ld\n",  elapsedTime);
    for (int i = 0; i < size; i++) {	
		c = (char) bu[i];
		//printf("c1: %d %c\n",i,c);
    }
    //sfs_close (fd); 
    char bu2[size2];
    sfs_read (fd2, (void *) bu2, size2);
    for (int i = 0; i < size2; i++) {	
		c = (char) bu2[i];
		//printf("c2: %d %c\n",i,c);
    }
    gettimeofday(&startTime, NULL);
    sfs_delete("file1.bin");
    gettimeofday(&endTime, NULL);
	elapsedTime = (endTime.tv_sec - startTime.tv_sec) * 1000000 +
	endTime.tv_usec - startTime.tv_usec;
	printf("delete in ms: %ld\n",  elapsedTime);
    fd3 = sfs_open("file3.bin", MODE_APPEND);
    for(int i=0;i<139;i++){
		//printf("i: %d ", i);
		memcpy (buffer5, buffer2, 8); // just to show memcpy
		sfs_append(fd3, (void *) buffer5, 8); 
	}
	
	sfs_close(fd3);
    fd3 = sfs_open("file3.bin", MODE_READ);
    char bu3[139*8];
    sfs_read (fd3, (void *) bu3, 139*8);
    for (int i = 0; i < 139*8; i++) {	
		c = (char) bu3[i];
		//printf("c3: %d %c\n",i,c);
    }
    
/*	
    sfs_close (fd); 
    sfs_close (fd2);
    
    char block2[1024];
    char block3[1024];
    char block4[1024];
  //  sfs_append(fd2, (void *) buffer3, 8); 
//	sfs_append(fd3, (void *) buffer3, 8); 
    read_block((void *)block2,1032);
    read_block((void *)block3,1033);
    read_block((void *)block4,1034);
	for(int i =0;i<1024;i++){
		printf("i1 %d :  v: %c \n",i, block2[i]);
	}
   	for(int i =0;i<1024;i++){
   		printf("i2 %d:  v: %c \n",i, block3[i]);
	}
	for(int i =0;i<1024;i++){
   		printf("i3 %d:  v: %c \n",i, block4[i]);
	}
 */
   /* sfs_create ("file4.bin");
    fd = sfs_open("file1.bin", MODE_APPEND);
	printf("fd: %d\n",fd);
	
	fd2 = sfs_open("file2.bin", MODE_APPEND);
	printf("fd2: %d\n",fd2);
	
	printf("fd close: %d\n",sfs_close(fd));
	fd3= sfs_open("file3.bin", MODE_APPEND);
	printf("fd3: %d\n",fd3);
	printf("fd2 close: %d\n",sfs_close(fd2));
	fd4= sfs_open("file4.bin", MODE_APPEND);
	printf("fd4: %d\n",fd4);
	*/
	//printf("fd close: %d\n",sfs_close(fd));
    /*char buffer3[1024];
    char buffer4[1024];
    
    for(int i = 0; i < 1024; i++){
    	buffer3[i] = (char) 87;
    }
    printf("faruk");
    int as = write_block((void *) buffer3, 1);
    printf("ensar");
    
    int sa = read_block((void *) buffer4, 2);
    printf("furkan");
    
    int count = 0;
    for(int i = 0; i < 1024; i++){
    	printf("--%c\n", (char) buffer4[i]);
    	count++;
    }
	printf("%d\n", count);
	
	struct directEntry bb;
	
	printf("%d\n", sizeof(bb));
	
    printf ("creating files\n"); 
    sfs_create ("file1.bin");
    sfs_create ("file2.bin");
    sfs_create ("file3.bin");

    fd1 = sfs_open ("file1.bin", MODE_APPEND);
    fd2 = sfs_open ("file2.bin", MODE_APPEND); 
    for (i = 0; i < 10000; ++i) {
		buffer[0] =   (char) 65;  
		sfs_append (fd1, (void *) buffer, 1);
    }

    for (i = 0; i < 10000; ++i) {
		buffer[0] = (char) 70;
		buffer[1] = (char) 71;
		buffer[2] = (char) 72;
		buffer[3] = (char) 73;
		sfs_append(fd2, (void *) buffer, 4);
    }
    
    sfs_close(fd1); 
    sfs_close(fd2); 

    fd = sfs_open("file3.bin", MODE_APPEND);
    for (i = 0; i < 10000; ++i) {
		memcpy (buffer, buffer2, 8); // just to show memcpy
		sfs_append(fd, (void *) buffer, 8); 
    }
    sfs_close (fd); 

    fd = sfs_open("file3.bin", MODE_READ);
    size = sfs_getsize (fd);
    for (i = 0; i < size; ++i) {
		sfs_read (fd, (void *) buffer, 1);
		c = (char) buffer[0];
    }
    sfs_close (fd); 
    */
    ret = sfs_umount();
}
