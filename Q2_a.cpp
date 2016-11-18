#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define array_size 10000
#define M 10

// function prototypes
void generate_random_array(int array[], int size);// this function full the array with random number
							 // from 1 to 10000	
void recursive_sort(int array[], int size, int low, int high);
void recursive_merging(int array[], int size, int low, int mid, int high);
void do_sort(int array[], int size, int low, int high);

// global variables


// ----------------------------------- function main
int main() { 
	// file management
	FILE *fp;
	fp = fopen("result.txt", "w+");
	
	// shared memory
	int shmid;
	int *shmaddress;
	shmid = shmget(IPC_PRIVATE, array_size * sizeof(int), IPC_CREAT | 0666);
	if (shmid < 0) {
		printf("failed to create shm segment\n");
		perror("shmget");
		return -1;
	}
	shmaddress = (int *) shmat(shmid, NULL, 0);
	if (*shmaddress == -1) {
		return -1;
	}

	generate_random_array(shmaddress, array_size);
	/*
	fprintf(fp,"List before sorting\n\n----------------------------\n");
	for(int i = 0; i < array_size; i++)
		fprintf(fp, "%d :  %d\n", i,  shmaddress[i]);

	fprintf(fp, "\n--------------------------------------\n\n");
*/
//	recursive_sort(shmaddress, array_size, 0, array_size - 1);
//	fprintf(fp, "here\n");
	
	do_sort(shmaddress, array_size, 0, array_size - 1);


	fprintf(fp,"\n----------------------------\nList after sorting\n----------------------------\n");

	for(int i = 0; i < array_size; i++)
		fprintf(fp, "%d :  %d\n", i, shmaddress[i]);
	
	return 0;
}

void do_sort(int array[], int size, int low, int high){
	if(high+M >= low){
		recursive_sort(array, size, low, high);
		return;
	}

	int mid;
	int pid1, pid2;	
	mid = (low + high) / 2;
	pid1 = fork();

	if(pid1 == 0){ // child
		do_sort(array, size, low, mid); // left child
		exit(0);
	} else { // parent
		pid2 = fork();
		if(pid2 == 0) {// child
			do_sort(array, size, mid+1, high); // right child
			exit(0);
		} else { // parent
			waitpid(pid1, NULL, 0);
			waitpid(pid2, NULL, 0);
			recursive_merging(array, size, low, mid, high); // merging right and left
		}
	}
}

void recursive_sort(int array[], int size, int low, int high){
	int mid;

	if(low < high) {
		mid = (low + high) / 2;
		recursive_sort(array, size, low, mid);
		recursive_sort(array, size, mid+1, high);
		recursive_merging(array, size, low, mid, high);
	} else {
		return;
	}
}

void recursive_merging(int array[], int size, int low, int mid, int high){
	int l1, l2, i;
	int tempMergArr[size]; // this temporary array is used for merging recursive.

	for(l1 = low, l2 = mid + 1, i = low; l1 <= mid && l2 <= high; i++) {
		if(array[l1] <= array[l2])
			tempMergArr[i] = array[l1++];
		else
			tempMergArr[i] = array[l2++];
	}

	while(l1 <= mid)    
		tempMergArr[i++] = array[l1++];

	while(l2 <= high)   
		tempMergArr[i++] = array[l2++];

	for(i = low; i <= high; i++)
		array[i] = tempMergArr[i];
}

void generate_random_array(int array[], int size){
	srand(time(NULL));
	for(int i = 0; i < size; i++){
		array[i] = rand() % 10000 + 1;
	}
}
