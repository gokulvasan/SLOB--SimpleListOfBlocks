/*
	SLOB: Simple List Of Blocks .

*	This programme is an approach to explain the SLOB in a simple pattern.
*	Inspired from some real-time scenarios which holds the 
	defined pattern of allocation and deallocation.
*	with little addition, works perfectly well for small environments in Real time task based system with flat memory model.
*	this File holds the procedure of slob implementation,
	User needs to  write the app calls to test and understand them.

	SLOB WORKING:
		Large piece of memory is first cut into segments called POOL
		POOL: is a large chunk of memory that holds a array of fixed size memory chunks called BLOCK
		BLOCK: is smallest memory chunk available from the POOL which is provided to the user on dynamic request.
		SLOB works on static fragmentation which happens at the construction phase of the task/environment.
		SLOB works on First-Fit algorithm and has a disadvantage of allocating a bigger block than needed, if
		the requested pool does not hold any free blocks.
		Advantage of SLOB is its simplicity, Faster than buddy and has less fragmentation.


	Schematic representation of POOL and BLOCK: 

		|	M	| ====> a chunk of memory is cut to become a POOL
		---------
		|	E	|
		|	M	|	
		|	O	|
		|	R	|
		|	Y	|	
	POOL: 
			----------------------------------
				|->|		|->|block|	....
			----------------------------------
*/
/*
	TO FIX: 
	1. printf : In lib files we are not supposed to use printf like lib functions, 
					instead open needs to be used,but for the sake of tutorial 
					I am initially using this but later needs correction towards this.					
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define debug
#define __LINUX__

/*
	 Magic number
	 to check the corruption of the block.
*/
#define MAGIC 71001985

// Block size
#define BLOCK_SIZE0	4
#define BLOCK_SIZE1	8

//header size
#define HEADER_SIZE  sizeof(header)


//POOL ID
enum Pool {
	POOL0,
	POOL1,
	TOTAL_POOL
} pool_segment;

//POOL SIZE
#define POOL_SIZE0 (10 * (HEADER_SIZE + BLOCK_SIZE0))
#define POOL_SIZE1 (10 * (HEADER_SIZE + BLOCK_SIZE1))


// header for malloc
/*
	header of the malloc can be further complicated by adding the size, 
	alloc status, task to which it is allocated and other debugging mechanisms to detect memory leaks.
*/
typedef struct __header {
	struct __header *nxt;	// this is used to solve 1 scenarios
							// holds the details of who is before them when @ home
	int pool_id;
	int magic_number; 	// to check the corruption of the block and should always be on the end.
}header;

/* pool itself*/
/* this is a static array used to represent the pool, 
	this could also be complicated by mmaping a anonyomous memory and cutting it into POOL,
	but for the tutorial purporse lets stick to this.
*/
unsigned char pool0 [POOL_SIZE0];
unsigned char pool1 [POOL_SIZE1];

/* FAST POOL */
/*
	a method to access the particular block chunks faster, 
	this could be avoided and the same data could also be injected into the boundry tag,
	but that makes the access to the block little slower and inefficient.
*/
typedef struct __fast_pool {
	unsigned short block_size;
	unsigned int nr_free_blocks;
	header *blocks;
} FAST_POOL;

static FAST_POOL fast_pool[TOTAL_POOL];

/*
	FUNCTION NAME: my_malloc
*/
void * malloc(const int size) {

	header *tmp;

	if(size <= BLOCK_SIZE0 && fast_pool[POOL0].nr_free_blocks) {
		tmp = (header*)fast_pool[POOL0].blocks;
		fast_pool[POOL0].blocks = tmp->nxt;
		tmp++;
		fast_pool[POOL0].nr_free_blocks--;
		return (void *)tmp;
	}	
	
	return NULL;
}

void free(void *block) {
	
	header *tmp;

	tmp = (header*)(block);
	tmp--;
	if(MAGIC == tmp->magic_number){
#ifdef debug
	printf("sucessful return to pool%d\n", tmp->pool_id);
#endif
		tmp->nxt = fast_pool[tmp->pool_id].blocks;
		fast_pool[tmp->pool_id].blocks = tmp;
		fast_pool[tmp->pool_id].nr_free_blocks++;
	}
	else 
		printf("Error: Block is corrupted\n");
}

/*
	FUNCTION NAME : pool_init()
	INPUT PARAMETERS:
		pool_id:   fast pool index
		block_size:size of the block
		pool:		  memory which would be fragmented
		pool_size: size of the memory cake.
	RETURN VALUE:
		void: made void because procedure assumes the person who calls
				this procedure is well aware about its working
	DESCRIPTION:
		initializes the raw memory to become array of blocks of the given size.
		adds them to the fastpool.  
	
*/
#ifdef __LINUX__
__attribute__((constructor))
#endif
static void pool_init(unsigned char pool_id, unsigned char block_size, unsigned char pool[], unsigned int pool_size) {
	
	header *tmp = NULL;
	header *prev = NULL;
	int size = 0;

	if(TOTAL_POOL <= pool_id){
		printf("ERROR: pool_id(%d) is larger than TOTAL_POOLS(%d)\n", pool_id, TOTAL_POOL);
		return;
	}
	if(pool_size % (block_size+HEADER_SIZE)) {
		printf("WARNING: pool_size(%d) is not aligned to the block_size(%d)\n", pool_size, block_size);		
	}

	memset(pool, 0, pool_size);		
	fast_pool[pool_id].block_size = block_size;
	
	while(size < pool_size) {
			
		tmp = (header*)&pool[size];
		tmp->magic_number = MAGIC; 
		tmp->pool_id = pool_id;
		if(NULL == prev) {
			fast_pool[pool_id].blocks = tmp;
		}
		else
			prev->nxt = tmp;
		prev = tmp;
		fast_pool[pool_id].nr_free_blocks++;
		size += HEADER_SIZE + block_size + 1;
		
	}
	tmp->nxt = fast_pool[pool_id].blocks; //close the loop

#ifdef debug
	printf("Nr of blocks created in pool%d : %d\n", pool_id, fast_pool[pool_id].nr_free_blocks);
#endif
	return;
}

#ifdef debug

int main(void) {
	int *i;
	pool_init(0, BLOCK_SIZE0, pool0, sizeof(pool0));
	
	i = (int*)my_malloc(4);
	if(i)
		printf("success\n");
	else
		printf("Error\n");
	my_free(i);
		
	return 0;
}

#endif
