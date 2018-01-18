/*
  vmms.cpp - This file contains the code for each of the memory functions as well as initialization of the "shared" memory.
*/

#include <Windows.h>
#include <stdio.h> //debugging
#include <Psapi.h>
#include <process.h>
#include "vmms_error.h"
#include <string.h>

#define MAX_PHY_SIZE 8196    // 8K Bytes     ** Hardcode for testing !!
#define MAX_TABLE_SIZE 1024  // 1K entries
#define DEFAULT_BOUNDRY 8    // minimum 8 byte allocation block


/* Memory Block Structure */
typedef struct block {
	unsigned int    alloc_size;		// allocated block size
	unsigned int    req_size;		// requested size
	struct block *next;				// block->next
	struct block *prev;				// block->prev
	int    free;					// memory flag
	int    pid;						// process ID
	char*  ptr;						// pointer to data
} mem_block;


// ************************************************************************
// Global Shared variables (shared by multiple instances of the DLL)
// ************************************************************************

/* Global shared memory section */
#pragma data_seg (".SHARED")  // Simulated Physical Mem // Hardcoded for now !!
int byte_boundry = DEFAULT_BOUNDRY;
int tab_size = MAX_TABLE_SIZE;				// size of memory table
int mem_size = MAX_PHY_SIZE;				// size of simulated phy mem (in bytes)
char mem_start[MAX_PHY_SIZE] = { 0 };  		// simulated Phy Memory
void* mem_table[MAX_TABLE_SIZE*sizeof(mem_block)] = { 0 };	// memory table
void* base = NULL;							// ptr to start of mem_table
#pragma data_seg ()

/* Here are the 5 exported functions for the application programs to use */
__declspec(dllexport) char* vmms_malloc(int size, int* error_code);
__declspec(dllexport) int vmms_memset(char* dest_ptr, char c, int size);
__declspec(dllexport) int vmms_memcpy(char* dest_ptr, char* src_ptr, int size);
__declspec(dllexport) int vmms_print(char* src_ptr, int size);
__declspec(dllexport) int vmms_free(char* mem_ptr);

/* Here are several exported functions specifically for mmc.cpp */
__declspec(dllexport) int mmc_initialize(int boundry_size);
__declspec(dllexport) int mmc_display_memtable(char* filename);
__declspec(dllexport) int mmc_display_memory(char* filename);

/* Local Prototypes */
static void log_malloc(int pid, char* operation, char* out, int p1, int p2);
static void log_memset(int pid, char* operation, int out, char* p1, char p2, int p3);
static void log_memcpy(int pid, char* operation, int out, char* p1, char*p2, int p3);
static void log_print(int pid, char* operation, int out, char* p1, int p2);
static void log_free(int pid, char* operation, int out, char* p1);
static void log_fail(int pid, char* operation, int out, char* error_message);
static mem_block* extend_heap(mem_block* last, int size);
static mem_block* get_heap(mem_block* last, int s);
static mem_block* join(mem_block* b);
static mem_block* find_block(mem_block** last, int size);
static mem_block* get_block(char* p);
static void split_block(mem_block* b, int s);
static int valid_addr(char* p);
static int resize(int size);
/* End of Local Prototypes*/

__declspec(dllexport) int mmc_initialize(int boundry_size)
{
	int rc = VMMS_SUCCESS;
	byte_boundry = boundry_size;
	return rc;
}

__declspec(dllexport) int mmc_display_memtable(char* filename)
{
	int rc = VMMS_SUCCESS;
	mem_block* tmp = (mem_block*)base;

	printf("| ADDRESS  | PID | FREE | ALLOC_SIZE | REQ_SIZE |\n");
	printf("------------------------------------------------\n");
	while (tmp) {
		printf("| %08p | %03d | %3d | %10d | %8d |\n", tmp, tmp->pid, tmp->free, tmp->alloc_size, tmp->req_size);
		tmp = tmp->next;
	}
	printf("------------------------------------------------\n");

	/* Creating VMMS.MEM*/
	tmp = (mem_block *)base;
	FILE* fp;
	fp = fopen("VMMS.LOG", "a");
	if (fp) {
		fprintf(fp, "| ADDRESS  | PID | FREE | ALLOC_SIZE | REQ_SIZE |\n");
		fprintf(fp, "------------------------------------------------\n");
		while (tmp) {
			fprintf(fp, "| %08p | %03d | %3d | %10d | %8d |\n", tmp, tmp->pid, tmp->free, tmp->alloc_size, tmp->req_size);
			tmp = tmp->next;
		}
		fprintf(fp, "------------------------------------------------\n");
		fclose(fp);
	}

	return rc;
}

__declspec(dllexport) int mmc_display_memory(char* filename)
{
	int rc = VMMS_SUCCESS;
	mem_block* mem_tab = (mem_block *)base;
	char* ptr = mem_tab->ptr;


	printf("Address  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 0 1 2 3 4 5 6 7 8 9 a b c d e f\n");
	printf("----------------------------------------------------------------------------------------\n");
	for (int i = 0; i < MAX_PHY_SIZE/10; i++) {
		if (i % 16 == 0) { // print address
			printf("%08p ", (ptr + i));
		}

		printf("%02x ", (unsigned int)*(ptr + i)); // print hex value

		if ((i + 1) % 16 == 0) {
			for (int j = 0; j < 16; j++) {
				printf("%c ", *(ptr + j + (i - 15))); // print character representation 
			}
			printf("\n");
		}
	}
	printf("\n");

	/* Creating VMMS.MEM*/
	mem_tab = (mem_block *)base;
	ptr = mem_tab->ptr;
	FILE* fp;
	fp = fopen("VMMS.MEM", "w");
	if (fp) {
		for (int i = 0; i < MAX_PHY_SIZE; i++) {
			fprintf(fp, "%08p ", (ptr + i)); // print address
			if ((i + 1) % 16 == 0) { fprintf(fp, "\n"); }
		}
		fprintf(fp, "\n");
		fclose(fp);
	}
	return rc;
}

__declspec(dllexport) char* vmms_malloc(int size, int* error_code)
{
	printf("stepping into vmms_malloc()\n");
	mem_block* b;     // new block
	mem_block* last;  // last block visited

	int fsize = resize(size); // new size, multiple of byte_boundry

	if (base) {
		// First time finding a block
		last = (mem_block *)base;
		b = find_block(&last, fsize);
		if (b) {
			// Can we split?
			if (fsize < b->alloc_size) {
				split_block(b, fsize);
			}
			b->req_size = size;
			b->pid = _getpid();
			b->free = 0;
		}
		else {
			/* No fitting block, extend the heap */
			b = extend_heap(last, fsize);
			if (!b) { // extend_heap() Failed
				error_code = (int *)OUT_OF_MEM;
				log_fail(0, "vmms_malloc", (int)error_code, "invalid out of memory");
				return (NULL);
			}
			b->req_size = size;
			b->pid = _getpid();
		}
	}
	else {
		/* First time initializing memory block */
		b = extend_heap(NULL, fsize);
		if (!b) { // extend_heap() failed;
			log_fail(0, "vmms_malloc", (int)error_code, "invalid out of memory");
			return (NULL);
		}
		b->req_size = size;
		b->pid = _getpid();

		base = b; // void* base = (mem_block*)b
		vmms_memset((char*)mem_start, '.', MAX_PHY_SIZE); // initialize all memory with '.' at first
	}

	mem_size = mem_size - b->alloc_size; // update external variable mem_size
	tab_size--;							 // update external variable tab_size
	error_code = (int *)VMMS_SUCCESS;

	log_malloc(b->pid, "vmms_malloc", b->ptr, size, (int)error_code);
	vmms_memset(b->ptr, ' ', b->alloc_size);

	printf("vmms_malloc() SUCCESS\n");
	return (b->ptr);
}

__declspec(dllexport) int vmms_memset(char* dest_ptr, char c, int size)
{
	printf("stepping into vmms_memset\n");
	int rc = VMMS_SUCCESS;

	if (valid_addr(dest_ptr)) {
		for (int i = 0; i < size; i++) {
			*dest_ptr = (char)c;
			dest_ptr++;
		}
		mem_block* tmp = get_block(dest_ptr);
		if (tmp) {
			log_memset(tmp->pid, "vmms_memset", rc, dest_ptr, c, size);
		}
		printf("vmms_memset SUCCESS\n");
		return rc;
	}
	else {
		rc = MEM_TOO_SMALL;
	}

	rc = INVALID_DEST_ADDR;
	if (rc = INVALID_DEST_ADDR) {
		log_fail(0, "vmms_memset", rc, "invalid destination address");
	}
	else {
		log_fail(0, "vmms_memset", rc, "invalid memory too small");
	}
	return rc;
}


__declspec(dllexport) int vmms_memcpy(char* dest_ptr, char* src_ptr, int size)
{
	printf("stepping into vmms_memcpy\n");
	int rc = VMMS_SUCCESS;

	if (valid_addr(dest_ptr) && valid_addr(src_ptr)) {
		for (int i = 0; i < size; i++) {
			*dest_ptr = *src_ptr;
			dest_ptr++; src_ptr++;
		}
		log_memcpy((get_block(src_ptr))->pid, "vmms_memcpy", rc, dest_ptr, src_ptr, size);
		printf("vmms_memcpy SUCESS");
		return rc;
	}
	rc = INVALID_CPY_ADDR;
	log_fail(0, "vmms_memcpy", rc, "invalid copy address");
	return rc;
}


__declspec(dllexport) int vmms_print(char* src_ptr, int size)
{
	int rc = VMMS_SUCCESS;
	int c = 0;

	if (valid_addr(src_ptr)) {
		if (size == 0) {
			while (src_ptr != 0) {
				printf("%02x ", (unsigned int)(char)src_ptr);
				src_ptr++;
			}
		}
		else {
			while (c < size) {
				printf("%c ", *src_ptr);
				src_ptr++; c++;
			}
		}
		log_print((get_block(src_ptr))->pid, "vmms_print", rc, src_ptr, size);
		return rc;
	}
	rc = INVALID_CPY_ADDR;
	log_fail(0, "vmms_print", rc, "invalid copy address");
	return rc;
}

__declspec(dllexport) int vmms_free(char* mem_ptr)
{
	printf("stepping into vmms_free()\n");
	mem_block *b;

	if (valid_addr(mem_ptr)) {
		b = get_block(mem_ptr);
		b->free = 1;
		vmms_memset(b->ptr, 'F', b->alloc_size); // set freed locations with 'F'
		// join with previous if possible
		if (b->prev && b->prev->free) {
			b = join(b->prev);
		}
		// join with next if possible
		if (b->next) {
			join(b);
		}
		else {
			// free the end of heap
			if (b->prev) { 
				vmms_memset(b->ptr, '.', b->alloc_size);
				b->prev->next = NULL;
			}
			else {
				/* Table is empty */
				/* Update External Variables */
				base = NULL;
				mem_size = MAX_PHY_SIZE;
				tab_size = MAX_TABLE_SIZE;
				vmms_memset((char*)mem_start, '.', MAX_PHY_SIZE); // reinitialize all memory with '.'
			}
		}
		if (base) { // Check since the heap can be completely empty and reset already to proper size
			mem_size = mem_size + b->alloc_size; // update external variable mem_size
			tab_size--;							 // update external variable tab_size
		}
		log_free(b->pid, "vmms_free", VMMS_SUCCESS, mem_ptr);
		printf("vmms_free() SUCCESS\n");
		return VMMS_SUCCESS;
	}

	log_fail(0, "vmms_free", INVALID_MEM_ADDR, "invalid memory address");
	printf("vmms_free() FAILED\n");
	return INVALID_MEM_ADDR;
}


/*********************** Local Functions ***********************/

static void log_malloc(int pid, char* operation, char* out, int p1, int p2){
	FILE* fp;
	SYSTEMTIME time;
	HANDLE processHandle = NULL;
	char fpath[512];
	char* filename = fpath;

	GetSystemTime(&time);
	GetModuleFileName(NULL, filename, 512);

	fp = fopen("vmms.log", "a");

	fprintf(fp, "%d%d%d%d%d%d %s %d %s %04x %d %d\n",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, filename, pid, operation, (unsigned int)*out, p1, p2);

	fclose(fp);
	CloseHandle(processHandle);
}
static void log_memset(int pid, char* operation, int out, char* p1, char p2, int p3) {
	FILE* fp;
	SYSTEMTIME time;
	HANDLE processHandle = NULL;
	char fpath[512];
	char* filename = fpath;

	GetSystemTime(&time);
	GetModuleFileName(NULL, filename, 512);

	fp = fopen("vmms.log", "a");

	fprintf(fp, "%d%d%d%d%d%d %s %d %s %d %04x %04x %d\n",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, filename, pid, operation, out, (unsigned int)*p1, (unsigned int)p2, p3);

	fclose(fp);
	CloseHandle(processHandle);
}
static void log_memcpy(int pid, char* operation, int out, char* p1, char*p2, int p3) {
	FILE* fp;
	SYSTEMTIME time;
	HANDLE processHandle = NULL;
	char fpath[512];
	char* filename = fpath;

	GetSystemTime(&time);
	GetModuleFileName(NULL, filename, 512);

	fp = fopen("vmms.log", "a");

	fprintf(fp, "%d%d%d%d%d%d %s %d %s %d %04x %04x %d\n",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, filename, pid, operation, out, (unsigned int)*p1, (unsigned int)*p2, p3);

	fclose(fp);
	CloseHandle(processHandle);
}
static void log_print(int pid, char* operation, int out, char* p1, int p2) {
	FILE* fp;
	SYSTEMTIME time;
	HANDLE processHandle = NULL;
	char fpath[512];
	char* filename = fpath;

	GetSystemTime(&time);
	GetModuleFileName(NULL, filename, 512);

	fp = fopen("vmms.log", "a");

	fprintf(fp, "%d%d%d%d%d%d %s %d %s %d %04x %d\n",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, filename, pid, operation, out, (unsigned int)*p1, p2);

	fclose(fp);
	CloseHandle(processHandle);
}
static void log_free(int pid, char* operation, int out, char* p1) {
	FILE* fp;
	SYSTEMTIME time;
	HANDLE processHandle = NULL;
	char fpath[512];
	char* filename = fpath;

	GetSystemTime(&time);
	GetModuleFileName(NULL, filename, 512);

	fp = fopen("vmms.log", "a");

	fprintf(fp, "%d%d%d%d%d%d %s %d %s %d %04x\n",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, filename, pid, operation, out, (unsigned int)*p1);

	fclose(fp);
	CloseHandle(processHandle);
}
static void log_fail(int pid, char* operation, int out, char* error_message) {
	FILE* fp;
	SYSTEMTIME time;
	HANDLE processHandle = NULL;
	char fpath[512];
	char* filename = fpath;

	GetSystemTime(&time);
	GetModuleFileName(NULL, filename, 512);

	fp = fopen("vmms.log", "a");

	fprintf(fp, "%d%d%d%d%d%d %s %d %s %d %s\n",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, filename, pid, operation, out, error_message);

	fclose(fp);
	CloseHandle(processHandle);
}

static mem_block* extend_heap(mem_block* last, int size) {
	printf("stepping into extend_heap()\n");
	mem_block *b;

	b = get_heap(last, size);

	if (b < 0) {
		return (NULL);
	}
	b->alloc_size = size;
	b->next = NULL;
	b->prev = last;
	if (last) {
		last->next = b;
	}
	b->free = 0;
	return (b);
}

static mem_block* get_heap(mem_block* last, int s) {
	printf("stepping into get_heap()\n");
	mem_block* b;

	if (s <= mem_size && tab_size > 0) {
		/* First call to get_heap(); */
		if (!(last)) {
			b = (mem_block *)mem_table;
			b->ptr = mem_start;
			return (b);
		}
		/* get_heap() has been called before */
		b = last + sizeof(mem_block);
		b->ptr = last->ptr + last->alloc_size;
		return (b);
	}
	return (NULL);
}


static mem_block* join(mem_block* b) {
	printf("stepping into join()\n");
	if (b->next && b->next->free) {
		b->alloc_size = b->alloc_size + b->next->alloc_size;
		b->next = b->next->next;
		if (b->next) {
			b->next->prev = b;
		}
		printf("joined block b and b->next\n");
	}
	return (b);
}

static void split_block(mem_block *b, int s) {
	printf("stepping into split_block()\n");
	mem_block* new_block;
	new_block = b + sizeof(mem_block);
	new_block->ptr = b->ptr + s;
	new_block->alloc_size = (b->alloc_size) - s;
	new_block->next = b->next;
	new_block->prev = b;
	new_block->free = 1;
	b->alloc_size = s;
	b->next = new_block;
	if (new_block->next) {
		new_block->next->prev = new_block;
	}
}

static mem_block* find_block(mem_block** last, int size) {
	printf("stepping into find_block()\n");
	mem_block* b = (mem_block*)base;

	while (b && !(b->free && b->alloc_size >= size)) {
		*last = b;
		b = b->next;
	}
	return (b);
} // return NULL if no memory block found

static mem_block *get_block(char *p) {
	printf("stepping into get_block()\n");
	mem_block* tmp = (mem_block *)base;
	while (tmp) {
		if (p >= tmp->ptr && p < (tmp->ptr + tmp->alloc_size)) {
			return (tmp);
		}
		tmp = tmp->next;
	}
	return tmp;
}

static int valid_addr(char * p) {
	printf("stepping into valid_addr()\n");
	mem_block* tmp = (mem_block *)base;
	if (tmp) { // first test if base has been initialized
		while (tmp) { // iterate through list
			if (p >= tmp->ptr && p < (tmp->ptr + tmp->alloc_size)) {
				return (true); // char* p found
			}
			tmp = tmp->next;
		}
	}
	return (false); // base is NULL, or char* p not found
}

static int resize(int size) {
	printf("stepping into resize()\n");
	int new_size;
	if (size >= 0) {
		if (size % byte_boundry == 0) {
			return size;
		}
		new_size = (size + byte_boundry) / 8;
		if (new_size == 0) {
			new_size = new_size + byte_boundry;
		}
		else {
			new_size = new_size * byte_boundry;
		}
	}
	else {
		return (-1);
	}

	return new_size;
}