#include <stdio.h>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

// ------------------------- function prototypes
void *baby(void *baby_id);
void *mother(void *mother_id);
void ready_to_eat(int baby_id);
void finish_eating(int baby_id);
void test(int baby_id);
void awake_mother();
void feeding();

// ------------------------- global variables
int baby_number; // numbers of crow babies
int dish_number; // numbers of dishes
int *baby_state; // this array keeps the state of any baby;
				 //	0:palying, 1:hungry, 2:eating, 3:awaking
int *dish_state; // this array keeps the state of any dish;
				 // 0:empty, 1:busy, 2:full
int *dish_baby; // this array shows that which baby is on which dish.
				// the array parameter is the dish_id and value of it is baby_id.
				// value 100 is for the dish that no one is on it.
int t; // how many feeds there are.
int counter = 0; // a counter for the number of eating babies.
pthread_mutex_t check_dish; // synchronize checking the dishes by every baby.
pthread_mutex_t finishing; // synchronize finishing eating by every baby.
sem_t go; // this semaphore lets babies to go eating or not.
sem_t sleep_m; // mother waits on this semaphore(sleeping). 
pthread_t *babies; // babies thread.
pthread_t mother_t; // mother thread.

// ------------------------- function : main 
int main(){
	// get variables to initialize. 
	printf("Please Enter the number of babies :   ");
	scanf("%d", &baby_number);
	baby_state = new int[baby_number];
	
	printf("\nPlease Enter the number of dishes :   ");
	scanf("%d", &dish_number);
	dish_state = new int[dish_number];
	dish_baby = new int[dish_number];
	
	printf("\nPlease Enter the number of feedings :   ");
	scanf("%d", &t);
	
	printf("\nThere are %d babies, %d food dishes and %d feedings\n"
	, baby_number, dish_number, t);
	printf("STARTS !!!!\n--------------------------------------------\n\n");
	
	// initialize arrays
	for(int i = 0; i < baby_number; i++){
		baby_state[i] = 0; // at first all babies are playing.
	}
	for(int i = 0; i < dish_number; i++){
		dish_state[i] = 2; // at first all dishes are full of food.
		dish_baby[i] = 100; // at first no one is on dishes so the value is 100.
	}
	
	// local variables
	babies = new pthread_t[baby_number];
	int mother_id = 1;
	int rc;
	
	// initialize mutexes and semaphores
	pthread_mutexattr_t check_dish_Attr;
	pthread_mutexattr_init(&check_dish_Attr);
	pthread_mutexattr_setpshared(&check_dish_Attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&check_dish, &check_dish_Attr);
	pthread_mutexattr_destroy(&check_dish_Attr);
	
	pthread_mutexattr_t finishing_Attr;
	pthread_mutexattr_init(&finishing_Attr);
	pthread_mutexattr_setpshared(&finishing_Attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&finishing, &finishing_Attr);
	pthread_mutexattr_destroy(&finishing_Attr);
	
	sem_init(&sleep_m, 0, 0);
	sem_init(&go, 0, 0);
	
	// creating mother thread
	rc = pthread_create(&mother_t, NULL, &mother, (void *) mother_id);
	if(rc){
		printf("Unable to create thread :(");
		return 0;
	}
	
	// creating baby threads
	for(int i = 0; i < baby_number; i++ ){
		rc = pthread_create(&babies[i], NULL, &baby, (void *)i);
		if (rc){
			printf("Unable to create thread :(");
			return 0;
		}
   }
	
	pthread_exit(NULL);
}

// ------------------------- baby starting thread function
void *baby(void *baby_id){
	long b_id;
	b_id = (long)baby_id;
	int play_duration;
	int eat_duration;
	printf("baby crow %ld started...\n", b_id);
	while(true){
		play_duration = rand() % 5 + 1;
		eat_duration = rand() % 5 + 1;
		// every baby plays for a while. duration of playing is random.
		printf("...baby crow %ld is palying for %d seconds...\n\n", b_id, play_duration);
		sleep(play_duration);
		ready_to_eat((int)b_id);
		// every baby eats for a while. duration of eating is random.
		printf("\n...baby crow %ld is eating for %d seconds...\n\n", b_id, eat_duration);
		finish_eating((int)b_id);
	}
	
	pthread_exit(NULL);
}

// ------------------------- mother starting thread function
void *mother(void *mother_id){
	long m_id;
	m_id = (long)mother_id;	
	int feed = 0; // counts the number of feeds by mother.
	printf("mother %ld started ...\n\n", m_id);

	while(feed <= t){
		// here mother sleeps. she waits until a baby awake her.
		printf("...mother takes a nap\n\n");
		sem_wait(&sleep_m);
		// she makes food ready for her babies.
		feeding();
		feed++;
	}
	
	for(int j = 0; j < baby_number; j++){
		pthread_cancel(babies[j]);
	}
	printf("\n-----------------------------------\nmother is retired. Game finished!!!\n");
	pthread_exit(NULL);
}

// ------------------------- ready_to_eat function
void ready_to_eat(int baby_id){
	// babies check dishes one atfer another.
	pthread_mutex_lock(&check_dish);
		printf("baby crow %d is hungry and is checking dishes to eat.\n", baby_id);
		baby_state[baby_id] = 1; // he says that he's hungry.
		test(baby_id); // he test dishes
		sem_wait(&go);
		if(baby_state[baby_id] == 1){ // if he still is hungry
			printf("baby %d is awaking his mother to feed dishes.\n", baby_id);
			awake_mother();
			printf("baby %d is waiting for mother to feed dishes.\n", baby_id);
			sem_wait(&go);
			test(baby_id);
			sem_wait(&go);
		}
	pthread_mutex_unlock(&check_dish);
}

// ------------------------- finish_eating function
void finish_eating(int baby_id){
	pthread_mutex_lock(&finishing);
		printf("...baby crow %d is finishing eating.\n", baby_id);
		counter--; // decrease the number of eating babies.
		baby_state[baby_id] = 0; // makes himself playing.
		int temp_dish_id;
		for(int j = 0; j < dish_number; j++){
			if(dish_baby[j] == baby_id){
				temp_dish_id = j;
				break;
			}
		}
		dish_baby[temp_dish_id] = 100; // say that no one is on dish.
		dish_state[temp_dish_id] = 0; // say that the dish is empty.
		
		bool flag = false; // all dishes are empty
		for (int k = 0; k < dish_number; k++)
			if(dish_state[k] != 0)
				flag = true; // one dish is not empty
				
		if(counter == 0 and flag == false){
			printf("...baby crow %d tells the hungry baby to continue and awake mother.\n", baby_id);
			sem_post(&go); // tell the hungry baby to continue and awake his mother
		}
		printf("...baby crow %d finished eating.\n", baby_id);
	pthread_mutex_unlock(&finishing);
}

// ------------------------- test function
void test(int baby_id){
	for(int i = 0; i < dish_number; i++){
		printf("baby %d is checking dish %d\n", baby_id, i);
		if(dish_state[i] == 2){ // if one dish is full of food.
			dish_state[i] = 1; // make the dish busy so others can not use it.
			dish_baby[i] = baby_id;
			baby_state[baby_id] = 2; // change his state to eating.
			sem_post(&go);
			counter++;
			printf("baby %d chooses dish %d for himself.\n", baby_id, i);
			break;
		}
	}
}

// ------------------------- awake_mother function
void awake_mother(){
	sem_post(&sleep_m);
}

// ------------------------- feed function
void feeding(){
	printf("\n...mother is making dishes full...\n");
	for(int i = 0; i < dish_number; i++){
		dish_state[i] = 2; // full : 2
	}
	sem_post(&go);
}
