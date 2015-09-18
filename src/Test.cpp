#include"castle.h"
#include<string.h>
#define UNIFORM_WORKLOAD
#define SORTED_WORKLOAD
int NUM_OF_THREADS;
int findPercent;
int insertPercent;
int deletePercent;
unsigned long keyRange;
double MOPS;
volatile bool start=false;
volatile bool stop=false;
volatile bool steadyState=false;
struct timespec runTime,transientTime;
int SORT_PERCENT;
int LOG_FREQUENCY;
int CONSTANT_FACTOR;
int LOG_ARRAY_SIZE;
unsigned long* hotKeys;
char KEY_SPREAD;
char KEY_DISTRIBUTION;
double alpha;
double c = 0;
double* powerTable;
unsigned long* zipfDistribution;
double* sum_prob; // Sum of probabilities

static inline int findIndex(double target) 
{
  int s=0;
  int e=keyRange-1;
  int mid;
  while(s<=e)
  {
      mid = (s+e)/2;
      if(target < sum_prob[mid])
      {
          e = mid-1;
      }
      else if(target > sum_prob[mid])
      {
          s= mid+1;
      }
      else
      {
          return mid;
      }
  }
  return s;
}
		
static inline  unsigned long preComputeZipf(gsl_rng* r)
{
  double z;                     // Uniform random number (0 < z < 1)
  // Pull a uniform random number (0 < z < 1)
  do
  {
    z = gsl_rng_uniform(r);
  }while ((z == 0) || (z == 1));
  // Map z to the value
	return findIndex(z);
}

static inline void initZipf(gsl_rng* r)
{
	powerTable = (double*) malloc((1+keyRange)*sizeof(double));
	sum_prob = (double*) malloc(keyRange*sizeof(double));	
	zipfDistribution = (unsigned long*) malloc(keyRange*sizeof(unsigned long));
	for(int i=1;i<=keyRange;i++)
	{
		powerTable[i] = pow((double) i, alpha);
	}
	for (int i=1;i<=keyRange;i++)
	{
    c = c + (1.0 / powerTable[i]);
	}
  c = 1.0 / c;
	sum_prob[0]=0;
	for (int i=1;i<=keyRange;i++)
	{
		sum_prob[i] = sum_prob[i-1] + c / powerTable[i];
	}
	for(int i=0;i<keyRange;i++)
	{
		zipfDistribution[i] = preComputeZipf(r);
	}
}

static inline void initZipfFromFile(char* part1, char* part2)
{
	zipfDistribution = (unsigned long*) malloc(keyRange*sizeof(unsigned long));
	char fileName[100];
	strcpy(fileName,"/work/02348/arunmoez/research/generators/data/zipf-");
	strcat(fileName,part1);
	strcat(fileName,"-");
	strcat(fileName,part2);
	FILE *fp;
	fp = fopen(fileName,"r");
	unsigned long i;
	for(int j=0;j<keyRange;j++)
	{
		fscanf(fp,"%lu",&i);
		zipfDistribution[j] = i;
	}
}

static inline unsigned long getZipf(gsl_rng* r)
{
	return zipfDistribution[gsl_rng_uniform_int(r,keyRange)];
}

static inline unsigned long getSorted(gsl_rng* r, unsigned long *prev_key)
{
	*prev_key = (*prev_key + 1)%keyRange;
	return *prev_key;
}

static inline unsigned long getRandom(gsl_rng* r)
{
	return(gsl_rng_uniform_int(r,keyRange) + 1);
}

static inline unsigned long getKey(gsl_rng* r, unsigned long* prev_key)
{
	int logOrGeneral = gsl_rng_uniform(r)*100;
	unsigned long key;
	if(logOrGeneral < LOG_FREQUENCY)
  {
		if(KEY_DISTRIBUTION == 'z')
		{
			key = getZipf(r);
		}
		else
		{
			key = hotKeys[gsl_rng_uniform_int(r,LOG_ARRAY_SIZE)];
		}
  }
  else
  {
		int sortOrGeneral = gsl_rng_uniform(r)*100;
	  if(sortOrGeneral < SORT_PERCENT)
		{
			key = getSorted(r,prev_key)+1;
		}
		else
		{
			key = getRandom(r);
		}
  }
	//printf("%lu\n",key);
	return key;
}
	
void *operateOnTree(void* tArgs)
{
	unsigned long prev_key;
	struct timespec s,e;
  int chooseOperation;
	int sortOrGeneral;
	int logOrGeneral;
  unsigned long lseed;
	unsigned long key;
  int threadId;
  struct tArgs* tData = (struct tArgs*) tArgs;
  threadId = tData->tId;
  const gsl_rng_type* T;
  gsl_rng* r;
  gsl_rng_env_setup();
  T = gsl_rng_default;
  r = gsl_rng_alloc(T);
	lseed = tData->lseed;
  gsl_rng_set(r,lseed);
	prev_key = getRandom(r);
	tData->newNode=NULL;
  tData->isNewNodeAvailable=false;
	tData->readCount=0;
  tData->successfulReads=0;
  tData->unsuccessfulReads=0;
  tData->readRetries=0;
  tData->insertCount=0;
  tData->successfulInserts=0;
  tData->unsuccessfulInserts=0;
  tData->insertRetries=0;
  tData->deleteCount=0;
  tData->successfulDeletes=0;
  tData->unsuccessfulDeletes=0;
  tData->deleteRetries=0;
	tData->seekLength=0;

  while(!start)
  {
  }
	
	while(!steadyState)
	{
	  chooseOperation = gsl_rng_uniform(r)*100;
		key = getKey(r, &prev_key);
    if(chooseOperation < findPercent)
    {
      search(tData, key);
    }
    else if (chooseOperation < insertPercent)
    {
      insert(tData, key);
    }
    else
    {
      remove(tData, key);
    }
	}
	
  tData->readCount=0;
  tData->successfulReads=0;
  tData->unsuccessfulReads=0;
  tData->readRetries=0;
  tData->insertCount=0;
  tData->successfulInserts=0;
  tData->unsuccessfulInserts=0;
  tData->insertRetries=0;
  tData->deleteCount=0;
  tData->successfulDeletes=0;
  tData->unsuccessfulDeletes=0;
  tData->deleteRetries=0;
	tData->seekLength=0;
	tData->searchTime.tv_sec=0;
	tData->searchTime.tv_nsec=0;
	tData->insertTime.tv_sec=0;
	tData->insertTime.tv_nsec=0;
	tData->deleteTime.tv_sec=0;
	tData->deleteTime.tv_nsec=0;
	tData->seekTime.tv_sec=0;
	tData->seekTime.tv_nsec=0;
	
	while(!stop)
  {
    chooseOperation = gsl_rng_uniform(r)*100;
		key = getKey(r, &prev_key);
    if(chooseOperation < findPercent)
    {
			tData->readCount++;
      clock_gettime(CLOCK_REALTIME,&s);
      search(tData, key);
			clock_gettime(CLOCK_REALTIME,&e);
			tData->searchTime.tv_sec += diff(s,e).tv_sec;tData->searchTime.tv_nsec += diff(s,e).tv_nsec;
    }
    else if (chooseOperation < insertPercent)
    {
			tData->insertCount++;
      clock_gettime(CLOCK_REALTIME,&s);
      insert(tData, key);
			clock_gettime(CLOCK_REALTIME,&e);
			tData->insertTime.tv_sec += diff(s,e).tv_sec;tData->insertTime.tv_nsec += diff(s,e).tv_nsec;
    }
    else
    {
			tData->deleteCount++;
      clock_gettime(CLOCK_REALTIME,&s);
      remove(tData, key);
			clock_gettime(CLOCK_REALTIME,&e);
			tData->deleteTime.tv_sec += diff(s,e).tv_sec;tData->deleteTime.tv_nsec += diff(s,e).tv_nsec;
    }
  }
  return NULL;
}

int main(int argc, char *argv[])
{
  struct tArgs** tArgs;
  double totalTime=0;
  double avgTime=0;
	unsigned long lseed;
	//get run configuration from command line
  NUM_OF_THREADS = atoi(argv[1]);
  findPercent = atoi(argv[2]);
  insertPercent= findPercent + atoi(argv[3]);
  deletePercent = insertPercent + atoi(argv[4]);
  runTime.tv_sec = atoi(argv[5]);
	runTime.tv_nsec =0;
	transientTime.tv_sec=1;
	//transientTime.tv_nsec=2000000;
  keyRange = (unsigned long) atol(argv[6]);
	lseed = (unsigned long) atol(argv[7]);
	LOG_FREQUENCY = atoi(argv[8]);
	CONSTANT_FACTOR = atoi(argv[9]);
	SORT_PERCENT = atoi(argv[10]);
	KEY_SPREAD = *argv[11];
	KEY_DISTRIBUTION = *argv[12];
	alpha = atof(argv[13]);
	
  tArgs = (struct tArgs**) malloc(NUM_OF_THREADS * sizeof(struct tArgs*)); 

  const gsl_rng_type* T;
  gsl_rng* r;
  gsl_rng_env_setup();
  T = gsl_rng_default;
  r = gsl_rng_alloc(T);
  gsl_rng_set(r,lseed);
	
  createHeadNodes(); //Initialize the tree. Must be called before doing any operations on the tree
  if(KEY_DISTRIBUTION == 'z')
	{
		//initZipfFromFile(argv[6],argv[13]);
		initZipf(r);
	}
  struct tArgs* initialInsertArgs = (struct tArgs*) malloc(sizeof(struct tArgs));
  initialInsertArgs->successfulInserts=0;
	initialInsertArgs->newNode=NULL;
  initialInsertArgs->isNewNodeAvailable=false;
	unsigned long prev_key;
	prev_key = getRandom(r);
	LOG_ARRAY_SIZE = CONSTANT_FACTOR*log2(keyRange);
	hotKeys = (unsigned long*) malloc(LOG_ARRAY_SIZE * sizeof(unsigned long));
	unsigned long hotKeyStartKey= getRandom(r);
	if(KEY_SPREAD == 's')
	{
		for(int i=0;i<LOG_ARRAY_SIZE;i++)
		{
			hotKeys[i] = (hotKeyStartKey+i)%keyRange;
		}
	}
	else if (KEY_SPREAD == 'r')
	{
		for(int i=0;i<LOG_ARRAY_SIZE;i++)
		{
			hotKeys[i] = getRandom(r);
		}
	}
	else if (KEY_SPREAD == 'c')
	{
		for(int i=0;i<LOG_ARRAY_SIZE-3;i+=4)
		{
			hotKeys[i] = getRandom(r);
			hotKeys[i+1] = hotKeys[i]+1;
			hotKeys[i+2] = hotKeys[i+1]+1;
			hotKeys[i+3] = hotKeys[i+2]+1;
		}
		hotKeys[LOG_ARRAY_SIZE-4] = getRandom(r);
		hotKeys[LOG_ARRAY_SIZE-3] = hotKeys[LOG_ARRAY_SIZE-4]+1;
		hotKeys[LOG_ARRAY_SIZE-2] = hotKeys[LOG_ARRAY_SIZE-3]+1;
		hotKeys[LOG_ARRAY_SIZE-1] = hotKeys[LOG_ARRAY_SIZE-2]+1;
	}
	
	
	while(initialInsertArgs->successfulInserts < keyRange/2) //populate the tree with 50% of keys
  {
		//unsigned long key = getKey(r, &prev_key);
		unsigned long key = getRandom(r);
    insert(initialInsertArgs, key);
  }
	

	unsigned long initialHeight = height();
  pthread_t threadArray[NUM_OF_THREADS];
  for(int i=0;i<NUM_OF_THREADS;i++)
  {
    tArgs[i] = (struct tArgs*) malloc(sizeof(struct tArgs));
    tArgs[i]->tId = i;
    tArgs[i]->lseed = gsl_rng_get(r);
  }

	for(int i=0;i<NUM_OF_THREADS;i++)
	{
		pthread_create(&threadArray[i], NULL, operateOnTree, (void*) tArgs[i] );
	}
	
	start=true; //start operations
	nanosleep(&transientTime,NULL); //warmup
	steadyState=true;
	nanosleep(&runTime,NULL);
	stop=true;	//stop operations
	
	for(int i=0;i<NUM_OF_THREADS;i++)
	{
		pthread_join(threadArray[i], NULL);
	}	
	
	unsigned long finalHeight = height();
	
  unsigned long totalReadCount=0;
  unsigned long totalSuccessfulReads=0;
  unsigned long totalUnsuccessfulReads=0;
  unsigned long totalReadRetries=0;
  unsigned long totalInsertCount=0;
  unsigned long totalSuccessfulInserts=0;
  unsigned long totalUnsuccessfulInserts=0;
  unsigned long totalInsertRetries=0;
  unsigned long totalDeleteCount=0;
  unsigned long totalSuccessfulDeletes=0;
  unsigned long totalUnsuccessfulDeletes=0;
  unsigned long totalDeleteRetries=0;
	unsigned long totalSeekLength=0;
	unsigned long totalSearchTime=0;
	unsigned long totalInsertTime=0;
	unsigned long totalDeleteTime=0;
	unsigned long totalSeekTime=0;
 
  for(int i=0;i<NUM_OF_THREADS;i++)
  {
    totalReadCount += tArgs[i]->readCount;
    totalSuccessfulReads += tArgs[i]->successfulReads;
    totalUnsuccessfulReads += tArgs[i]->unsuccessfulReads;
    totalReadRetries += tArgs[i]->readRetries;

    totalInsertCount += tArgs[i]->insertCount;
    totalSuccessfulInserts += tArgs[i]->successfulInserts;
    totalUnsuccessfulInserts += tArgs[i]->unsuccessfulInserts;
    totalInsertRetries += tArgs[i]->insertRetries;
    totalDeleteCount += tArgs[i]->deleteCount;
    totalSuccessfulDeletes += tArgs[i]->successfulDeletes;
    totalUnsuccessfulDeletes += tArgs[i]->unsuccessfulDeletes;
    totalDeleteRetries += tArgs[i]->deleteRetries;
		totalSeekLength += tArgs[i]->seekLength;
		totalSearchTime +=tArgs[i]->searchTime.tv_sec*1000000000;totalSearchTime +=tArgs[i]->searchTime.tv_nsec;
		totalInsertTime +=tArgs[i]->insertTime.tv_sec*1000000000;totalInsertTime +=tArgs[i]->insertTime.tv_nsec;
		totalDeleteTime +=tArgs[i]->deleteTime.tv_sec*1000000000;totalDeleteTime +=tArgs[i]->deleteTime.tv_nsec;
		totalSeekTime   +=tArgs[i]->seekTime.tv_sec*1000000000;totalSeekTime     +=tArgs[i]->seekTime.tv_nsec;
  }
	unsigned long totalOperations = totalReadCount + totalInsertCount + totalDeleteCount;
	MOPS = totalOperations/(runTime.tv_sec*1000000.0);
	printf("k%d;%d-%d-%d;%d;%ld;",atoi(argv[6]),findPercent,(insertPercent-findPercent),(deletePercent-insertPercent),NUM_OF_THREADS,size());
	printf("(%.2f %.2f %.2f);",(totalReadRetries * 100.0/totalOperations),(totalInsertRetries * 100.0/totalInsertCount),(totalDeleteRetries * 100.0/totalDeleteCount));
	printf("(%.2f %.2f %.2f);",(double) totalSearchTime/(totalReadCount*1000),(double) totalInsertTime/(totalInsertCount*1000),(double) totalDeleteTime/(totalDeleteCount*1000));
	printf("%.2f;%lu;%lu;%.2f;%.2f\n",(totalSeekLength*1.0/totalOperations),initialHeight,finalHeight, (double) totalSeekTime/(totalOperations*1000),MOPS);
	assert(isValidTree());
	pthread_exit(NULL);
}
