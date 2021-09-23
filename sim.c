#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct cacheline {
	unsigned long long accessedAt, arrivedAt, tag;
	short valid, dirty;
};

int main(int argc, char*argv[]) {
	if (argc != 6) {
		printf("Error: Exactly 5 arguments are required.\n");
		return 1;
	}
	unsigned long long totalaccesses = 0, memoryreads = 0, memorywrites = 0, cachehits = 0, cachemisses = 0, cachesize, replacementpolicy, blocksize = 64, associativity, writepolicy;
	cachesize = atoi(argv[1]);
	if (!((cachesize != 0) && ((cachesize & (cachesize - 1)) == 0))) {
		printf("Error: cache size needs to be in power of 2.\n");
		return 1;
	}
	associativity = atoi(argv[2]);
	if (!((associativity != 0) && ((associativity & (associativity - 1)) == 0))) {
		printf("Error: associativity needs to be in power of 2.\n");
		return 1;
	}
	replacementpolicy = atoi(argv[3]);
	if (replacementpolicy!=0  && replacementpolicy != 1) {
		printf("Error: replacement policy must either be 0 (LRU) or 1 (FIF0).\n");
		return 1;
	}
	writepolicy = atoi(argv[4]);
	if (writepolicy != 0 && writepolicy != 1) {
		printf("Error: write policy must either be 0 (write-through) or 1 (write-back).\n");
		return 1;
	}
	FILE* filedescriptor = fopen(argv[5], "r");
	if (filedescriptor == NULL) {
		printf("Error: Input trace file not found.\n");
		return 1;
	}
	unsigned long long i, j, k, address, tag, setno, timer = 0, offsetbits = (unsigned)log2(blocksize), tagbits, totallines = cachesize / blocksize;
	char accesstype[5];
	struct cacheline * cachelines;
	cachelines = malloc((totallines)*(sizeof(struct cacheline)));
	for (i = 0; i < (totallines); i++)
		cachelines[i].arrivedAt  = cachelines[i].accessedAt =  cachelines[i].valid = cachelines[i].dirty  = 0;
	while (fscanf(filedescriptor, "%s", accesstype) == 1) {
		if (strcmp(accesstype, "#eof") == 0)
			break;
		fscanf(filedescriptor, "%llx", &address);
		if (accesstype[0] == 'W' && writepolicy==0)
			memorywrites++;
		totalaccesses++;
		unsigned long long setnobits = (unsigned)log2(cachesize / (blocksize*associativity));
		tagbits = 64 - offsetbits - setnobits;
		setno = ((address << tagbits) >> (tagbits + offsetbits));
		tag = (address >> (setnobits + offsetbits));
		for (i = setno*associativity; i < (setno + 1)*associativity; i++) {
			if (cachelines[i].valid == 1 && cachelines[i].tag == tag) {
				cachehits++;
				if (accesstype[0] == 'W')
					cachelines[i].dirty = 1;
				j = i;
				break;
			}
		}
		if (i == (setno + 1)*associativity) {
			for (i = setno*associativity; i < (setno + 1)*associativity; i++) {
				if (cachelines[i].valid == 0) {
					cachelines[i].valid = 1;
					cachelines[i].tag = tag;
					cachelines[i].arrivedAt = timer++;
					memoryreads++;
					cachemisses++;
					if (accesstype[0] == 'W')
						cachelines[i].dirty = 1;
					j = i;
					break;
				}
			}
		}
		if (i == (setno + 1)*associativity) {
			j = setno*associativity;
			for (i = setno*associativity; i < (setno + 1)*associativity; i++) {
				if (replacementpolicy==1){
					if (cachelines[i].arrivedAt < cachelines[j].arrivedAt)
						j = i;
				}
				if (replacementpolicy==0){
					if (cachelines[i].accessedAt > cachelines[j].accessedAt)
						j = i;
				}
			}
			cachelines[j].tag = tag;
			cachelines[j].arrivedAt = timer++;
			if (writepolicy == 1 && cachelines[j].dirty == 1)
				memorywrites++;
			if(accesstype[0]=='R')
				cachelines[j].dirty = 0;
			memoryreads++;
			cachemisses++;
		}
		for (k = 0; k < (cachesize / blocksize); k++) {
			if (cachelines[k].valid == 1)
				cachelines[k].accessedAt++;
		}
		cachelines[j].accessedAt = 0;
	}
	free(cachelines);
	printf("%f\n", (double)cachemisses/(double)totalaccesses);
	printf("%lld\n", memorywrites);
	printf("%lld\n", memoryreads);
	return 0;
}