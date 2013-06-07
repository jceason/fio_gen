//#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NONSTDC_NO_WARNINGS

#include <windows.h>
#include <direct.h>
#endif				/* 
				 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef LINUX
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#endif				/* 
				 */

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifdef WIN32
#ifdef __cplusplus
extern "C" {

#endif				/* 
				 */

    int gettimeofday(struct timeval *tv, struct timezone *tz);

#ifdef __cplusplus
}
#endif				/* 
				 */
#endif				/* 
				 */
#define VERSION "1.03"
#define FILES_PER_DIR 1024
unsigned BLOCK_SIZE = 4096;

char *BLOCK = NULL;

#define PATTERN_SIZE 32
char PATTERNX[PATTERN_SIZE];

typedef struct teststat {

    unsigned long write_size;

    unsigned long read_size;

    double write_avg;

    double read_avg;

    unsigned long num_files;

    unsigned long num_directorys;

    unsigned long largest_file;

} teststat_t;


typedef struct thread_config {

    unsigned long iterations;

    unsigned long size_in_MB;

    unsigned long max_files;

    char path[1024];

    int write_only;

    int keep;

#ifdef LINUX
    unsigned long self;

#else				/* 
				 */
    HANDLE self;

#endif				/* 
				 */
    teststat_t stat;

    int result;

} thread_config_t;


/***********************************************/
/***********************************************/
void init_data()
{

    unsigned int i = 0;

    BLOCK = (char *) malloc(BLOCK_SIZE + 1);

    if (BLOCK == NULL) {

	printf("ERROR !! Unable to allocated block size %u\n", BLOCK_SIZE);

	exit(1);

    }


    strncpy(PATTERNX, "ABCDEFGHIJK*abcdefghijK012456789", 32);

    strncpy(BLOCK, PATTERNX, PATTERN_SIZE);

    BLOCK[PATTERN_SIZE] = '\0';

    for (i = 1; i < BLOCK_SIZE / PATTERN_SIZE; i++) {

	strncat(BLOCK, PATTERNX, PATTERN_SIZE);

    }

    printf("BLOCK_SIZE %u\n", BLOCK_SIZE);

}				/*end of init PATTERN */



void free_data()
{

    if (BLOCK != NULL) {

	free(BLOCK);

    }

}


/***********************************************/
/***********************************************/
int write_data(unsigned long size_in_MB, char *path, unsigned long max_files, teststat_t * stat)
{

    int rc = 0;

    int i, k;

    unsigned int j;

    FILE *outp;


    char tmppath[1024];

    char tmpname[1024];

    unsigned long write_total_KB = 0;


    unsigned long increment = 1;

    if (BLOCK_SIZE > 1024) {

	increment = BLOCK_SIZE / 1024;

    }

    unsigned long current_write_size_KB = increment;


    unsigned long num_files = 0;

    unsigned long total_size_KB = (size_in_MB * 1024);

    struct timeval start;

    struct timeval current;

    double elapsed;

#ifdef LINUX
    unsigned long threadID = pthread_self();

#else				/* 
				 */
    unsigned long threadID = (unsigned long) GetCurrentThreadId();

#endif				/* 
				 */
    k = 0;

#ifdef LINUX
    sprintf(tmppath, "%s/fiodir%i/", path, k);

    rc = mkdir(tmppath, 0777);

#else				/* 
				 */
    sprintf(tmppath, "%s\\fiodir%i\\", path, k);

    rc = mkdir(tmppath);

#endif				/* 
				 */

    if (rc != 0) {

	//continue if directory exists
	if (errno != EEXIST) {

	    printf("ERROR unable to create %s rc %i errno %i\n", tmppath, rc, errno);

	    return rc;

	}

	rc = 0;

    }

    stat->num_directorys++;

    gettimeofday(&start, NULL);

    while (1) {

	for (i = 0; i < FILES_PER_DIR; i++) {

	    sprintf(tmpname, "%sblfile%i", tmppath, i);


	    outp = fopen(tmpname, "w");


	    if (outp == NULL) {

		printf("ERROR unable to open %s errno %i\n", tmpname, errno);

		return 1;

	    }

	    /*Write the data */
	    for (j = 0; j < (current_write_size_KB * 1024 / BLOCK_SIZE); j++) {

		fwrite(BLOCK, BLOCK_SIZE, 1, outp);

	    }


	    stat->num_files++;

	    write_total_KB += current_write_size_KB;

	    stat->write_size += current_write_size_KB;

	    current_write_size_KB += increment;


	    fclose(outp);

	    num_files++;


	    if (write_total_KB >= total_size_KB || (max_files != 0 && num_files >= max_files)) {

		gettimeofday(&current, NULL);

		elapsed = (current.tv_sec - start.tv_sec);

		elapsed += ((current.tv_usec - start.tv_usec) / 1000.0) / 1000.0;

		printf("Thread %lu Total Write %lu MB %lf MB/s \n", threadID, write_total_KB / 1024,
		       (write_total_KB / 1024) / elapsed);

		stat->write_avg = stat->write_avg + ((write_total_KB / 1024) / elapsed);

		return 0;

	    }

	}

	k++;


	stat->num_directorys++;

	gettimeofday(&current, NULL);

	elapsed = (current.tv_sec - start.tv_sec);

	elapsed += ((current.tv_usec - start.tv_usec) / 1000.0) / 1000.0;

	printf("Thread %lu Current Write %lu MB %lf MB/s \n", threadID, write_total_KB / 1024, (write_total_KB / 1024) / elapsed);

#ifdef LINUX
	sprintf(tmppath, "%s/fiodir%i/", path, k);

	rc = mkdir(tmppath, 0777);

#else				/* 
				 */
	sprintf(tmppath, "%s\\fiodir%i\\", path, k);

	rc = mkdir(tmppath);

#endif				/* 
				 */

	if (rc != 0) {

	    if (errno != EEXIST) {

		printf("ERROR unable to create %s rc %i errno %i\n", tmppath, rc, errno);

		return rc;

	    }

	    rc = 0;

	}

    }				/*end of while */

    return rc;

}				/*end of write_data */




int verify_data(unsigned long size_in_MB, char *path, unsigned long max_files, teststat_t * stat)
{

    int rc = 0;

    int i, k;

    unsigned int j;

    FILE *inp;


    char tmppath[1024];

    char tmpname[1024];

    char *read_data;

    read_data = (char *) malloc(BLOCK_SIZE);

    if (read_data == NULL) {

	printf("ERROR unable to malloc read_data  errno %i\n", errno);

	return 1;

    }

    unsigned long read_total_KB = 0;

    unsigned long num_files = 0;

#ifdef LINUX
    unsigned long threadID = pthread_self();

#else				/* 
				 */
    unsigned long threadID = (unsigned long) GetCurrentThreadId();

#endif				/* 
				 */

    unsigned long increment = 1;

    if (BLOCK_SIZE > 1024) {

	increment = BLOCK_SIZE / 1024;

    }

    unsigned long current_read_size_KB = increment;


    unsigned long total_size_KB = size_in_MB * 1024;

    struct timeval start;

    struct timeval current;

    double elapsed;

    k = 0;

#ifdef LINUX
    sprintf(tmppath, "%s/fiodir%i/", path, k);

#else				/* 
				 */
    sprintf(tmppath, "%s\\fiodir%i\\", path, k);

#endif				/* 
				 */
    gettimeofday(&start, NULL);

    while (1) {

	for (i = 0; i < FILES_PER_DIR; i++) {

	    sprintf(tmpname, "%sblfile%i", tmppath, i);

	    if ((inp = fopen(tmpname, "r")) == NULL) {

		printf("ERROR unable to read %s rc %i errno %i\n", tmpname, rc, errno);
		free(read_data);

		return 1;

	    }

	    /*Check the data */
	    for (j = 0; j < (current_read_size_KB * 1024 / BLOCK_SIZE); j++) {

		rc = fread(read_data, BLOCK_SIZE, 1, inp);

		if (1 != rc) {

		    printf("ERROR unable to read %s rc %i errno %i\n", tmpname, rc, errno);
		    fclose(inp);
		    free(read_data);

		    return 1;

		}

		if (strncmp(read_data, BLOCK, BLOCK_SIZE) != 0) {
		    printf("ERROR read miscompare %s \n", tmpname);
		    fclose(inp);
		    free(read_data);
		    return 1;

		}

	    }

	    fclose(inp);

	    num_files++;

	    read_total_KB += current_read_size_KB;

	    stat->read_size += current_read_size_KB;

	    current_read_size_KB += increment;


	    if (read_total_KB >= total_size_KB || (max_files != 0 && num_files >= max_files)) {

		gettimeofday(&current, NULL);

		elapsed = (current.tv_sec - start.tv_sec);

		elapsed += ((current.tv_usec - start.tv_usec) / 1000.0) / 1000.0;

		printf("Thread %lu Total Read %lu MB %lf MB/s \n", threadID, read_total_KB / 1024,
		       (read_total_KB / 1024) / elapsed);

		stat->read_avg = stat->read_avg + ((read_total_KB / 1024) / elapsed);

		free(read_data);

		return 0;

	    }

	}			/*end of files per dir */

	k++;

#ifdef LINUX
	sprintf(tmppath, "%s/fiodir%i/", path, k);

#else				/* 
				 */
	sprintf(tmppath, "%s\\fiodir%i\\", path, k);

#endif				/* 
				 */
	gettimeofday(&current, NULL);

	elapsed = (current.tv_sec - start.tv_sec);

	elapsed += ((current.tv_usec - start.tv_usec) / 1000.0) / 1000.0;

	printf("Thread %lu Current Read %lu MB %lf MB/s \n", threadID, read_total_KB / 1024, (read_total_KB / 1024) / elapsed);

    }				/*end of while */


    free(read_data);


    return rc;

}				/*end of verify_data */




/***********************************************/
/***********************************************/
int remove_data(unsigned long size_in_MB, char *path, unsigned long max_files)
{

    int rc = 0;

    int i, k;

    char tmppath[1024];

    char tmpname[1024];

    unsigned long num_files = 0;

    unsigned long delete_total_KB = 0;


    unsigned long increment = 1;

    if (BLOCK_SIZE > 1024) {

	increment = BLOCK_SIZE / 1024;

    }

    unsigned long current_delete_size_KB = increment;


    unsigned long total_size_KB = size_in_MB * 1024;

    k = 0;

#ifdef LINUX
    sprintf(tmppath, "%s/fiodir%i/", path, k);

#else				/* 
				 */
    sprintf(tmppath, "%s\\fiodir%i\\", path, k);

#endif				/* 
				 */
    while (1) {

	for (i = 0; i < FILES_PER_DIR; i++) {

	    sprintf(tmpname, "%sblfile%i", tmppath, i);

	    rc = unlink(tmpname);

	    if (rc != 0) {

		printf("ERROR deleting file %s rc %i errno %i \n", tmpname, rc, errno);

		return rc;

		rc = 0;

	    }
	    num_files++;

	    delete_total_KB += current_delete_size_KB;

	    current_delete_size_KB += increment;


	    if (delete_total_KB >= total_size_KB || (max_files != 0 && num_files >= max_files)) {

		rc = rmdir(tmppath);

		if (rc != 0) {

		    if (errno != ENOTEMPTY) {

			printf("ERROR deleting dir %s rc %i errno %i complete\n", tmppath, rc, errno);

			return rc;

		    }

		}

		return 0;

	    }

	}			/*end of for */

	rc = rmdir(tmppath);

	if (rc != 0) {

	    if (errno != ENOTEMPTY) {

		printf("ERROR deleting dir %s rc %i errno %i next dir\n", tmppath, rc, errno);

		return rc;

	    }

	    rc = 0;

	}

	k++;

#ifdef LINUX
	sprintf(tmppath, "%s/fiodir%i/", path, k);

#else				/* 
				 */
	sprintf(tmppath, "%s\\fiodir%i\\", path, k);

#endif				/* 
				 */
    }				/*end of while */

    return rc;

}				/*end of remove_data */


/***********************************************/
/***********************************************/
#ifdef LINUX
void *thread_wrap(void *data)
#else				/* 
				 */
DWORD WINAPI thread_wrap(void *data)
#endif				/* 
				 */
{


    unsigned int i = 0;

    int rc;

    thread_config_t *my_config = (thread_config_t *) data;


    char tmp[64];

#ifdef LINUX
    my_config->self = pthread_self();

    sprintf(tmp, "/%lu", (unsigned long) my_config->self);

#else				/* 
				 */
    my_config->self = (HANDLE) GetCurrentThreadId();

    sprintf(tmp, "\\%lu", (unsigned long) my_config->self);

#endif				/* 
				 */

    strcat(my_config->path, tmp);

#ifdef LINUX
    rc = mkdir(my_config->path, 0777);

#else				/* 
				 */
    rc = mkdir(my_config->path);

#endif				/* 
				 */
    if (rc != 0) {

	//continue if directory exists
	if (errno != EEXIST) {

	    printf("ERROR unable to create %s rc %i errno %i\n", my_config->path, rc, errno);

	    my_config->result++;

	    return 0;

	}

	rc = 0;

    }



    for (i = 0; i < my_config->iterations; i++) {

	printf("THREAD %lu  ITERATION %i \n", my_config->self, i);

	rc = write_data(my_config->size_in_MB, my_config->path, my_config->max_files, &my_config->stat);

	fflush(stdout);

	if (rc == 0) {

	    if (!my_config->write_only) {

		rc = verify_data(my_config->size_in_MB, my_config->path, my_config->max_files, &my_config->stat);

		fflush(stdout);

		if (rc != 0) {

		    printf("ERROR in verify data %i\n", rc);

		    my_config->result++;

		}

	    }

	    if (!my_config->keep) {

		rc = remove_data(my_config->size_in_MB, my_config->path, my_config->max_files);

		if (rc != 0) {

		    printf("ERROR in remove data %i\n", rc);

		    my_config->result++;

		}

	    }


	} else {

	    printf("ERROR in write data %i\n", rc);

	    my_config->result++;

	}

	if (my_config->result != 0) {

	    break;

	}

    }				//for iteration

    if (!my_config->keep) {

	rc = rmdir(my_config->path);

	if (rc != 0) {

	    if (errno != ENOTEMPTY) {

		printf("ERROR deleting dir %s rc %i errno %i \n", my_config->path, rc, errno);

		my_config->result++;

	    }

	    rc = 0;

	}

    }



    return 0;

}				/*End of thread_wrap */


/***********************************************/
/***********************************************/
void usage()
{

    printf("VERSION:%s\n", VERSION);
    printf("fio_gen -s <size> -p <path> [options]\n");

    printf("\t Required \n");

    printf("\t -s <size in MB> \tTotal size to write\n");

    printf("\t -p <path> \t\tPath to filesystem for test \n");

    printf("\n");

    printf("\t Optional \n");

    printf("\t -b <block size>\tSize of writes and reads. Must be multiples of 32 Default 4096\n");

    printf("\t -f <max files> \tOverides total size\n");

    printf("\t -i <iterations>\tNumber of test iterations\n");

    printf("\t -t <num>  \t\tNumber of threads. Default 1 \n");

    printf("\t -w \t\tWrite only\n");

    printf("\t -k \t\tKeep directories and files\n");

    printf("\t -h \t\tHelp \n");

    printf("EXAMPLE fio_gen -s 500 -p /tmp -f 1000 -i 1 \n");

    exit(0);

}

void echo_environment(int cnt, char **args)
{

    int i = 0;


    printf("VERSION:%s\n", VERSION);

    printf("TESTCOMMAND: ");

    for (i = 0; i < cnt; i++) {

	printf("%s ", args[i]);

    }

    printf("\n");

    fflush(stdout);


}				// echo_environment()


/***********************************************/
/***********************************************/
int main(int argc, char **argv)
{

    int rc = 0;

    int i = 0;

    int j = 1;

    int iter = 1;

    int num_threads = 1;

    int write_only = 0;

    int keep = 0;

    unsigned long size_in_MB = 0;

    unsigned long max_files = 0;

    char path[1024] = "";

#ifdef LINUX
    pthread_t *thread = NULL;

#else				/* 
				 */
    HANDLE *thread = NULL;

#endif				/* 
				 */

    thread_config_t *thread_config = NULL;

    struct timeval start;

    struct timeval end;

    double elapsed;

    struct stat stat_buf;


    teststat_t total_stat;

    total_stat.write_size = 0;

    total_stat.read_size = 0;

    total_stat.write_avg = 0;

    total_stat.read_avg = 0;

    total_stat.num_files = 0;

    total_stat.num_directorys = 0;

    total_stat.largest_file = 0;

    /*Process Arguements */
    for (i = 1; i < argc; i++) {

	if (strncmp(argv[i], "-s", 2) == 0) {

	    i++;

	    size_in_MB = atoi(argv[i]);

	} else if (strncmp(argv[i], "-f", 2) == 0) {

	    i++;

	    max_files = atoi(argv[i]);

	} else if (strncmp(argv[i], "-t", 2) == 0) {

	    i++;

	    num_threads = atoi(argv[i]);

	} else if (strncmp(argv[i], "-b", 2) == 0) {

	    i++;

	    BLOCK_SIZE = atoi(argv[i]);

	} else if (strncmp(argv[i], "-p", 2) == 0) {

	    i++;

	    strcpy(path, argv[i]);

	} else if (strncmp(argv[i], "-i", 2) == 0) {

	    i++;

	    iter = atoi(argv[i]);

	} else if (strncmp(argv[i], "-w", 2) == 0) {

	    write_only = 1;

	} else if (strncmp(argv[i], "-k", 2) == 0) {

	    keep = 1;

	} else if (strncmp(argv[i], "-h", 2) == 0) {

	    usage();

	} else {

	    printf("Invalid option %s\n", argv[i]);

	    usage();

	}

    }

    /*Basic Arg Checks */
    if (size_in_MB == 0) {

	printf("Invalid size\n");

	usage();

    }

    if (BLOCK_SIZE % 32 != 0) {

	printf("Invalid block size\n");

	usage();

    }

    if (stat(path, &stat_buf) != 0) {

	printf("Invalid path\n");

	usage();

    }



    init_data();

#ifdef LINUX
    thread = (pthread_t *) malloc(sizeof(pthread_t) * num_threads);

#else				/* 
				 */
    thread = (HANDLE *) malloc(sizeof(HANDLE) * num_threads);

#endif				/* 
				 */

    thread_config = (thread_config_t *) malloc(sizeof(thread_config_t) * num_threads);

    if (thread == NULL || thread_config == NULL) {

	return -1;

    }


    echo_environment(argc, argv);

    /*Init the thread configs */
    for (j = 0; j < num_threads; j++) {

	thread_config[j].iterations = iter;

	thread_config[j].size_in_MB = size_in_MB / num_threads;

	thread_config[j].max_files = max_files / num_threads;

	thread_config[j].write_only = write_only;

	thread_config[j].keep = keep;

	strcpy(thread_config[j].path, path);

	thread_config[j].result = 0;


	thread_config[j].stat.write_size = 0;

	thread_config[j].stat.read_size = 0;

	thread_config[j].stat.write_avg = 0;

	thread_config[j].stat.read_avg = 0;

	thread_config[j].stat.num_files = 0;

	thread_config[j].stat.num_directorys = 0;


    }


    gettimeofday(&start, NULL);

    /*Start each thread */
    for (j = 0; j < num_threads; j++) {

#ifdef LINUX
	rc = pthread_create(&thread[j], NULL, thread_wrap, (void *) &thread_config[j]);

#else				/* 
				 */
	thread[j] = CreateThread(NULL, 0, thread_wrap, &thread_config[j], 0, NULL);


	if (thread[j] == NULL)
	    rc = -1;

	else
	    rc = 0;

#endif				/* 
				 */

    }

    /*Wait each thread */
    for (j = 0; j < num_threads; j++) {

#ifdef LINUX
	pthread_join(thread[j], NULL);

#else				/* 
				 */
	WaitForSingleObject(thread[j], INFINITE);

	CloseHandle(thread[j]);

#endif				/* 
				 */

    }


    gettimeofday(&end, NULL);

    elapsed = (end.tv_sec - start.tv_sec);

    elapsed += ((end.tv_usec - start.tv_usec) / 1000.0) / 1000.0;

    printf("TESTCOMPLETE:\n");


    for (j = 0; j < num_threads; j++) {

	total_stat.write_size += thread_config[j].stat.write_size;

	total_stat.read_size += thread_config[j].stat.read_size;

	total_stat.write_avg += thread_config[j].stat.write_avg;

	total_stat.read_avg += thread_config[j].stat.read_avg;

	total_stat.num_files += thread_config[j].stat.num_files;

	total_stat.num_directorys += thread_config[j].stat.num_directorys;


	printf("Thread %lu :NUM FILES:%lu \n", thread_config[j].self, thread_config[j].stat.num_files);

	printf("Thread %lu :NUM DIRECTORYS:%lu \n", thread_config[j].self, thread_config[j].stat.num_directorys);

	printf("Thread %lu :WRITE SIZE:%lu MB\n", thread_config[j].self, thread_config[j].stat.write_size / 1024);

	printf("Thread %lu :READ SIZE:%lu MB\n", thread_config[j].self, thread_config[j].stat.read_size / 1024);

	printf("Thread %lu :WRITE AVG:%lf MB/s\n", thread_config[j].self, thread_config[j].stat.write_avg / iter);

	printf("Thread %lu :READ AVG:%lf MB/s\n", thread_config[j].self, thread_config[j].stat.read_avg / iter);

	if (thread_config[j].result == 0) {

	    printf("Thread %lu :PASS\n", (unsigned long) thread_config[j].self);

	} else {

	    printf("Thread %lu :FAIL\n", (unsigned long) thread_config[j].self);

	}

	rc += thread_config[j].result;

    }

    printf("TESTSTAT:NUM FILES:%lu \n", total_stat.num_files);

    printf("TESTSTAT:NUM DIRECTORYS:%lu \n", total_stat.num_directorys);

    printf("TESTSTAT:WRITE SIZE:%lu MB\n", total_stat.write_size / 1024);

    printf("TESTSTAT:WRITE SIZE:%lu MB\n", total_stat.write_size / 1024);

    printf("TESTSTAT:READ SIZE:%lu MB\n", total_stat.read_size / 1024);

    printf("TESTSTAT:WRITE AVG:%lf MB/s\n", total_stat.write_avg / iter);

    printf("TESTSTAT:READ AVG:%lf MB/s\n", total_stat.read_avg / iter);


    printf("TESTTIME:%.2f s\n", elapsed);

    if (rc == 0) {

	printf("TESTRESULT:PASS \n");

    } else {

	printf("TESTRESULT:FAIL %i\n", rc);

    }

    //free 
    free_data();


    return rc;

}				/*end of main */
