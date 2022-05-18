#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "slab.h"
#include "stdbool.h"

struct {
	struct spinlock lock;
	struct slab slab[NSLAB];
} stable;

unsigned int nextPowerOf2(unsigned int n){
	unsigned count=0;

	if(n && !(n&(n-1))){
		n=n-1;
	}
	while(n!=0){
		n>>=1;
		count+=1;
	}
	return count;
}

bool get_bit(char *b, int i){
	int index = (i / 8);
	int offset = (i % 8);
	b += index;
	return (*b & (1 << offset)) != 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
}

void set_bit(char *b, int i){
	int index = (i / 8);
	int offset = (i % 8);
	b += index;
	*b = *b | (1 << offset);
}

void clear_bit(char *b, int i){
	int index = (i / 8);
	int offset = (i % 8);
	b += index;
	*b = *b & ~(1 << offset);
}

void slabinit(){
	/* fill in the blank */

	acquire(&stable.lock);
	int object_per_size=8;
	for(int i=0; i<NSLAB; i++){
		object_per_size=8<<i;
		stable.slab[i].size=object_per_size;
		stable.slab[i].num_pages=0;
		stable.slab[i].num_free_objects=PGSIZE/object_per_size;
		stable.slab[i].num_used_objects=0;
		stable.slab[i].num_objects_per_page= PGSIZE/object_per_size;
		stable.slab[i].bitmap=kalloc();
		memset(stable.slab[i].bitmap,0,PGSIZE);
		stable.slab[i].page[stable.slab[i].num_pages++]=kalloc();
	}
	release(&stable.lock);

}

char *kmalloc(int size){
	/* fill in the blank */
	if(size<0 || size>2048)
		return 0;

	acquire(&stable.lock);

	int slabindex = nextPowerOf2(size)-3;

	char *addr = 0;
	if(stable.slab[slabindex].num_free_objects ==0){
		if(stable.slab[slabindex].num_pages >= MAX_PAGES_PER_SLAB){
			release(&stable.lock);
			return 0;
		}

		//8bytes ofjects nums >= 32768
		if(slabindex == 0 && stable.slab[slabindex].num_pages >= 64) {
			release(&stable.lock);
			return 0;
		}

		stable.slab[slabindex].page[stable.slab[slabindex].num_pages] = kalloc();
		if(stable.slab[slabindex].page[stable.slab[slabindex].num_pages] ==0){
			release(&stable.lock);
			return 0;
		}

		stable.slab[slabindex].num_pages++;
		stable.slab[slabindex].num_free_objects += stable.slab[slabindex].num_objects_per_page;
	}
	
	int range = stable.slab[slabindex].num_pages * stable.slab[slabindex].num_objects_per_page;
				
	for(int i=0 ; i<range; i++) {
		if(!get_bit(stable.slab[slabindex].bitmap, i)){

			int page_index = i / stable.slab[slabindex].num_objects_per_page;
			int page_offset = i % stable.slab[slabindex].num_objects_per_page;
			addr = stable.slab[slabindex].page[page_index] + (page_offset * stable.slab[slabindex].size);
			set_bit(stable.slab[slabindex].bitmap, i);
			stable.slab[slabindex].num_free_objects--;
			stable.slab[slabindex].num_used_objects++;
			break;
		}
	}
	
	release(&stable.lock);
	return addr;
}

void kmfree(char *addr, int size){
	/* fill in the blank */
	if(size<0 || size>2048)
		return ;

	int slabindex = nextPowerOf2(size)-3;
	

	acquire(&stable.lock);
	int range = stable.slab[slabindex].num_pages * stable.slab[slabindex].num_objects_per_page;
	for(int i=0; i<range; i++){
		int page_index = i / stable.slab[slabindex].num_objects_per_page;
		int page_offset = i % stable.slab[slabindex].num_objects_per_page;

		if(addr == (stable.slab[slabindex].page[page_index] + (page_offset * stable.slab[slabindex].size))){
			memset(addr, 1, stable.slab[slabindex].size);
			stable.slab[slabindex].num_free_objects++;
			stable.slab[slabindex].num_used_objects--;
			clear_bit(stable.slab[slabindex].bitmap, i);
		}
	}

	release(&stable.lock);
	
}

void slabdump(){
	cprintf("__slabdump__\n");

	struct slab *s;

	cprintf("size\tnum_pages\tused_objects\tfree_objects\n");

	for(s = stable.slab; s < &stable.slab[NSLAB]; s++){
		cprintf("%d\t%d\t\t%d\t\t%d\n", 
			s->size, s->num_pages, s->num_used_objects, s->num_free_objects);
	}
}

int numobj_slab(int slabid)
{
	return stable.slab[slabid].num_used_objects;
}
