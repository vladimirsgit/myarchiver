#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

#define CHUNK_SIZE 4096

int ar(const char* arh, const char* src);

int arfile(FILE *arhfp, char* src);
void ardir(FILE *arhfp, char* path);

void readSrcFileAndWriteToArh(FILE *arhfp, FILE *srcfp);
int readSrcDir(DIR *srcdp);

void writeNameAndStatToArchive(FILE *arhfp, const char* src);
void writeFileContentToArchive(char *readBuff, const char* arh);

int unar(const char* arh, const char* dst);

void readArchiveDataAndUnar(FILE *arhfp);
char* createPath(FILE *arhfp);
struct stat createStat(FILE *arhfp);
char* readFileContentFromArchive(FILE *arhfp, off_t bytesToRead);
off_t processContentFromArchive(FILE *arhfp, FILE *dstfp, off_t fileSize, off_t bytesLeftToRead);
void createDirAndSetStat(char *path, struct stat *fileData);

int isFile(const char* path);
void checkError(void *fptr, char *path);



int main(int argc, char** argv)
{
	if(argc != 4){
		printf("Please call ./myar ar/unar archiveName src/dst\n");
		return 0;
	}

	if(strcmp(argv[1], "ar") == 0){
		ar(argv[2], argv[3]);

	} else if(strcmp(argv[1], "unar") == 0){
		unar(argv[2], argv[3]);
	} else {
		printf("Please call ./myar ar/unar archiveName src/dst\n");
		return 0;
	}

	return 0;
}


/*
  The ar function is used for archiving. It checks if it starts with a file or with a directory.
*/
int ar(const char* arh, const char* src){
	FILE *arhfp = fopen(arh, "wb");
	checkError(arhfp, arh);
	if(isFile(src)){
		char* src2 = calloc(strlen(src) + 1, sizeof(char));
		strcat(src2, src);
		arfile(arhfp, src2);
		free(src2);
	} else {
		char *path = calloc(strlen(src) + 1, sizeof(char));
		strcat(path, src);

		ardir(arhfp, path);
		free(path);
	}
	return 0;
}

/*
    The unar function is used for unarchiving an archive. First it creates the directory, then it changes the current working directory
        to be the destination directory.
*/
int unar(const char* arh, const char* dst){
	FILE *arhfp = fopen(arh, "rb");

	mkdir(dst, 0777);
	checkError(arhfp, arh);

	chdir(dst);
	readArchiveDataAndUnar(arhfp);

	return 0;
}


/*
    Here we read the data from our archive. The name of the file is until the first '\0', then its the stat data and after that we have the actual
        content of the file.
*/
void readArchiveDataAndUnar(FILE *arhfp){

	while(!feof(arhfp)){
		char *path = createPath(arhfp);
		if(strlen(path) == 0){
			break;
		}

		struct stat fileData = createStat(arhfp);
		if(S_ISREG(fileData.st_mode)){

			FILE *dstfp = fopen(path, "wb");

			printf("FILE: %s\n", path);
			off_t fileSize = fileData.st_size;

			printf("FILE SIZE: %ld\n", fileSize);
			if(fileSize){
					off_t totalBytesRead = 0;

					while(totalBytesRead < fileSize){

					totalBytesRead+=processContentFromArchive(arhfp, dstfp, fileSize, fileSize - totalBytesRead);

					}
					printf("TOTAL BYTES READ AND WRITTEN: %ld\n", totalBytesRead);

			}
			fclose(dstfp);
		} else {
			printf("DIR: %s\n", path);
			createDirAndSetStat(path, &fileData);
		}
		free(path);

	}

}

off_t processContentFromArchive(FILE *arhfp, FILE *dstfp, off_t fileSize, off_t bytesLeftToRead){

	off_t currentChunkSize = 0;
	off_t bytesRead = 0;

	char *content = calloc(CHUNK_SIZE, sizeof(char));


	if(bytesLeftToRead < CHUNK_SIZE){
		currentChunkSize = bytesLeftToRead;
	} else {
		currentChunkSize = CHUNK_SIZE;
	}

	bytesRead = fread(content, 1, currentChunkSize, arhfp);

	fwrite(content, 1, currentChunkSize, dstfp);

	free(content);
	return bytesRead;

}
void createDirAndSetStat(char *path, struct stat *fileData){
	if(mkdir(path, fileData->st_mode) != 0){
		perror("UNABLE TO CREATE DIR");
	}

}
char* createPath(FILE *arhfp){
	char *path = calloc(PATH_MAX, sizeof(char));
	char byte[1];
	size_t bytesRead = 1;
	int i = 0;

	while(bytesRead > 0){
		bytesRead = fread(byte, 1, 1, arhfp);

		path[i] = byte[0];

		if(byte[0] == '\0'){
			break;
		}
		i++;
	}

	return path;

}

struct stat createStat(FILE *arhfp){
	struct stat fileData;

	fread(&fileData, sizeof(struct stat), 1, arhfp);

	return fileData;
}


int arfile(FILE *arhfp, char* src){

	FILE *srcfp = fopen(src, "rb");
	if(srcfp == NULL){
		printf("ERROR TRYING TO OPEN %s\n", src);
		return 0;
	}
	checkError(arhfp, "nothing");
	//checkError(srcfp, src);


	writeNameAndStatToArchive(arhfp, src);

	readSrcFileAndWriteToArh(arhfp, srcfp);

	printf("FILE: %s\n", src);
	fclose(srcfp);

	return 0;
}


void readSrcFileAndWriteToArh(FILE *arhfp, FILE *srcfp){
	char readBuff[CHUNK_SIZE];
	size_t bytesRead = 1;

	while(bytesRead > 0){
		bytesRead = fread(readBuff, 1, CHUNK_SIZE, srcfp);
		fwrite(readBuff, sizeof(char), bytesRead, arhfp);
	}
}



void writeNameAndStatToArchive(FILE *arhfp, const char* src){
	size_t length = strlen(src) + 1;
	if(fwrite(src, sizeof(char), length, arhfp) != length){
		perror("Error writing name to archive");
		exit(-1);
	}

	struct stat pathData;

	if(stat(src, &pathData) != 0){
		perror("Error accessing path data");
		exit(-1);
	}

	if(fwrite(&pathData, sizeof(struct stat), 1, arhfp) != 1){
		perror("Error writing stat data to archive");
		exit(-1);
	}

}
void ardir(FILE *arhfp, char* path){


	DIR *srcdp = opendir(path);
	if(srcdp == NULL){
		printf("ERROR TRYING TO OPEN %s\n", path);
		return;
	}
	//checkError(srcdp, path);

	printf("DIR: %s\n", path);

	writeNameAndStatToArchive(arhfp, path);

	struct dirent *entry;
	entry = readdir(srcdp);

	while(entry != NULL){
		if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			entry = readdir(srcdp);
			continue;
		}
  size_t newLength = strlen(path) + strlen(entry->d_name) + 2;
        char *newPath = calloc(newLength, sizeof(char));
        strcat(newPath, path);
        strcat(newPath, "/");
        strcat(newPath, entry->d_name);

        if (entry->d_type == DT_DIR) {
            ardir(arhfp, newPath); // Recurse with the new path
        } else {
            arfile(arhfp, newPath);
        }

        free(newPath);
		entry = readdir(srcdp);
	}
	closedir(srcdp);
}


int isFile(const char* path){
	//returns true(1) if its regular file or false(0) if its directory, assuming we will only work with files and directories
	struct stat pathData;

	if(stat(path, &pathData) != 0){
		perror("Error accessing the path");
		printf("%s", path);
		exit(-1);
	}

	return S_ISREG(pathData.st_mode);
}


void checkError(void *fptr, char *path){
	if(fptr == NULL){
		perror("Error when opening file/directory");
		printf("%s\n", path);
		exit(-1);
	}

}

