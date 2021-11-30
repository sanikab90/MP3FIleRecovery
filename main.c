#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "ext3_fs.h"

//global variables

int SUPERBLOCK_OFFSET = 1024;
static int totalBlocks = 0;
static int blockSize = 0;
static int inodeSize = 0;
static int blocksPerGroup = 0;
int indBlock;
int firstDataBlock;
unsigned int dirArray[15];
unsigned int indBlockArr[1024];

//prototypes
int findHeader(int);
int findIndirect(int);
void makeArray(int);
void makeIndArray(int);
int writeToBlock(int, FILE*, int);
int compareHexValues(unsigned char[], unsigned char[], int);
void firstSuperblock(int);
char *decimalToHexStringInReverseOrder(int);

void makeIndArray(int fd){
    //put indirect blocks in array
    int offset;
	//Read in blocks in indirect block
	if(indBlock == 0){
        offset = dirArray[12] * blockSize;
	}
	else{
        offset = indBlock * blockSize;
	}

    lseek(fd, offset, SEEK_SET);
    unsigned char b[blockSize];
    long ret = read(fd, b, blockSize);

    int i = 0;
    while(i < SUPERBLOCK_OFFSET){
        offset = i * 4;
        //convert
        unsigned int blockNum = ((unsigned int)b[offset+3] << 24) | ((unsigned int)b[offset+2] << 16) | ((unsigned int)b[offset+1] << 8) | ((unsigned int)b[offset]);
        indBlockArr[i] = blockNum;
        i++;
    }

    //make sure function is correct
    // int i = 0;
    // while (i < SUPERBLOCK_OFFSET){
    //     printf("%d:\t%X\n", i+1, indBlockArr[i]);
    // }
}

void makeArray(int fd){
    for(int i = 0; i < 12; i++){
        dirArray[i] = firstDataBlock + i;
    }

    // make sure function prints correctly
    // for(int i = 0; i < 12; i++){
    //     printf("%d\t%d\n", i+1, dirArray[i]);
    // }
}

// char *decimalToHexStringInReverseOrder(int decimalNumber) {
//     char *signs = (char *) malloc(sizeof(char) * 4);
//     signs[0] = decimalNumber & 0xff;
//     signs[1] = (decimalNumber >> 8) & 0xff;
//     signs[2] = (decimalNumber >> 16) & 0xff;
//     signs[3] = (decimalNumber >> 24) & 0xff;

//     return signs;
// }

//get superblock values from first superblock
void firstSuperblock(int fd){
    struct ext3_super_block s;

    lseek64(fd, (off_t)SUPERBLOCK_OFFSET, SEEK_SET);
    read(fd, &s, sizeof(s));
    
    blockSize = 1024 << s.s_log_block_size;
    totalBlocks = s.s_blocks_count;
    inodeSize = s.s_inode_size;
    blocksPerGroup = s.s_blocks_per_group;
}

/*
This utility function comapres the hex values of two character arrays upto a certain length (n)
*/
int compareHexValues(unsigned char string1[], unsigned char string2[], int n) {

    int i;
    for (i = 0; i < n; i++) {
        if ((int) string1[i] == (int) string2[i]) {
            if (i == n - 1) {
                return 1;
            }
        } else {
            return 0;
        }
    }
    return 0;
}

/*
 * This function will scan the file descriptor for different
 * file type by theirs specific signature in the file header
 */
int findHeader(int fd)
{
    struct ext3_super_block s; //Not needed here later
    unsigned char header[2];
    unsigned char b[4096]; 
    int count = 0;
    printf("Searching for mp3 file...");
    header[0] = 0x49;
    header[1] = 0x44;

    lseek64(fd, 1024, SEEK_SET);
    read(fd, &s, sizeof(s));
    int totalBlocks = s.s_blocks_count;
    //printf("Total Blocks: %d\n", totalBlocks);
    
    lseek64(fd, 2048, SEEK_CUR); //Moves to the first block to start reading at group descriptor

    for (int i = 1; i <= totalBlocks; i++)
    {
        read(fd, b, 4096);
        if(compareHexValues(b, header, 2) == 1)
        {
            count = i;
            printf("Mp3 file header found! At %d\n", count);
            return count;
        }
    }
    return -1;
}

int findIndirect(int fd) {
    printf("\nSearching for indirect block...");
    int isDirect = 0;
	int blockBreak = 0;
    int ifound = 0;
    int offset2;
    int offset = 0;
	unsigned char b[blockSize];
	unsigned int blockNum;
	unsigned int nextBlock;
	
    //reset pointer
    lseek(fd, offset, SEEK_SET);

	//loop through all blocks
    int i = firstDataBlock;
    while(i < totalBlocks){
		isDirect = 0;
		blockBreak = 0;
		offset2 = i * blockSize;
		lseek(fd, offset2, SEEK_SET);
		long retVal = read(fd, b, blockSize);
		
        // printf("whileloop1");

		//convert
		blockNum = ((unsigned int)b[3] << 24) | ((unsigned int)b[2] << 16) | ((unsigned int)b[1] << 8) | ((unsigned int)b[0]);
		
		if(blockNum == 0){
            i++;
			continue;
		}
		
		//check for contigious blocks
        int j = 1;
        while(j < blockSize-4){
			offset = j * 4;
			
            // printf("whileloop2\n");
			
			nextBlock = ((unsigned int)b[offset+3] << 24) | ((unsigned int)b[offset+2] << 16) | ((unsigned int)b[offset+1] << 8) | ((unsigned int)b[offset]);
			
			if((blockNum+1) == nextBlock){
				isDirect = isDirect + 1;
			}
			else{
				blockBreak = blockBreak + 1;
				if(blockBreak == 2){
					break;
				}
			}
			
			if(isDirect == 15){
				printf("Found indirect block! At %d\n", i);
				ifound = 1;
				indBlock = i;
				break;
				
			}
			
			blockNum = nextBlock;
            j++;
			
		}
		
		if(ifound == 1){
			break;
		}

		i++;
	}
}

// flag = {0, 1}
// 0 => writeDirect
// 1 => writeInd 
int writeToBlock(int fd, FILE *fp, int flag) {
    int offset; 
    int blockNum;
    int limit;
    unsigned char b[blockSize];

    if (flag == 0){
        limit = 12;
    } else {
        limit = 1024;
    }

    for(int i = 0; i < limit; i++){
        if (flag == 0){
            //Navigate to block and read in file data
            blockNum = dirArray[i];
        } else {
            //Navigate to block and read in file data
            blockNum = indBlockArr[i];
        }

        //Check if at end of array
        if(flag == 1 && blockNum == 0){
            break;
        }

        offset = blockSize * blockNum;
        lseek(fd, offset, SEEK_SET);
        long retVal = read(fd, b, blockSize);

        //Write data to new file
        fwrite(b, sizeof(b), 1, fp);
    }

    //Set pointer back to start of file
    lseek(fd, 0, SEEK_SET);
}

//take in 2 arguments, disk and the name of the mp3 you want to recover
int main(int argc, char *argv[]){
    char *Disk = argv[1];

    //file descriptor to write to file
    FILE *wfp;
    wfp = fopen(argv[2], "wb");

    //make sure file pointer works
    if(wfp == NULL){
        perror("File opening failed");
        return 1;
    }

    //check for correct entry
    if(argc != 3){
        printf("Please enter the disk and the name of the mp3 file to recover\n");
        exit(2);
    }

    //open linux drive
    unsigned char b[4096];
    int fd = open(Disk, O_RDONLY);

    //make sure fd opened correctly
    if(fd < 0){
        fputs("memory error", stderr);
        exit(2);
    }

    firstSuperblock(fd);

    //find the first block of mp3
    firstDataBlock = findHeader(fd);

    //if file header isn't ifound exit
    if(firstDataBlock < 0){
        fputs("file header not ifound", stderr);
        exit(2);
    }

    // printf("block size %d\ntotal blocks %d\ninode size %d\n", blockSize, totalBlocks, inodeSize);
    
    indBlock = findIndirect(fd);

    // printf("%d\n%d\n", indBlock, indPointer);

    //make array for blocks - this is assuming all direct blocks are contiguous
    makeArray(fd);

    //make indirect block array
    makeIndArray(fd);

    //now recover by writing to file
    writeToBlock(fd, wfp, 0);
    writeToBlock(fd, wfp, 1);

    printf("\nYour file has been recovered as %s!\n", argv[2]);

    close(fd);
    close(fd);
}