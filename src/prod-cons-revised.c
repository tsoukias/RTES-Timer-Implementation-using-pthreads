//Author: Stefanos Tsoukias
//Timer Implementation using pthreads 
//Final Academic Project for Real Time Embedded Systems
//Supervisor: Nikolaos Pitsianis

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#define QUEUESIZE 10
#define P 1
#define Q 8


FILE  *stats_prod;
FILE  *stats_cons;
typedef struct {
	void * (*work)(void *);
	void * arg;
} workFunction;
int po=0;

typedef struct {
  void *argd;
  struct timeval tval;
} passData;


typedef struct {
  workFunction buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

typedef struct {
	int TasksToExecute;
	int Period;
	void *queue;
  void * (*StartFcn)(void *);
  void * (*StopFcn)();
  void * (*ErrorFcn)(void *);
  void * (*TimerFcn)(void *);
  void * UserData;
  pthread_t pid;
}timer;


queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in);
void queueDel (queue *q, workFunction *out);

void *producer(void *t);
void *consumer(void *args);


void *StopTimerFcn();
void *ErrorTimerFcn(void *arg);
void *StartTimerFcn(void *arg);
void start(timer *t);
void startat(timer*t,int y,int m,int d,int h,int min,int sec);
void *function1(void *t);
void timerInit(timer *t, int TasksToExecute, int Period, queue *q, void * UserData, void * (*StartFcn)(void *),void * (*TimerFcn)(void *), void * (*ErrorFcn)(void *), void * (*StopFcn)());
int main()
{
  
  char str[33];
  sprintf(str,"stats/queuetime_all.csv");
  stats_cons = fopen(str,"w");
  sprintf(str,"stats/timedrifting_all.csv");
  stats_prod = fopen(str,"w");
  
  int i;
  pthread_t con[Q];

  double *number,argument;
  argument = 3.14;
  number = &argument;
  

  //Initializing FIFO
  queue *fifo;
  fifo = queueInit ();
  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }
  

  //Initializing Timers
  timer *t,*n,*l;
  t = (timer *)malloc(sizeof(timer));
  n = (timer *)malloc(sizeof(timer));
  l = (timer *)malloc(sizeof(timer));
  timerInit(t,360000,10,fifo, number, StartTimerFcn, function1, ErrorTimerFcn, StopTimerFcn);
  timerInit(n,36000,100,fifo, number, StartTimerFcn, function1, ErrorTimerFcn, StopTimerFcn);
  timerInit(l,3600,1000,fifo, number, StartTimerFcn, function1, ErrorTimerFcn, StopTimerFcn);

  //Creating Consumers
  for (i=0;i<Q;i++){
    pthread_create(&con[i], NULL, consumer, fifo);
  }
  
  //startat(t,2020,11,29,22,07,05);        //Timer, Year, Month, Day, Hour, Minute, Second
  start(t);
  start(n);
  start(l);

  pthread_join(t->pid,NULL);
  pthread_join(n->pid,NULL);
  pthread_join(l->pid,NULL);

  free(t);
  free(n);
  free(l);

  /*Ending Process */
  workFunction flag;
  flag.work=NULL;
  while (!fifo->empty);
  pthread_mutex_lock (fifo->mut);
  queueAdd (fifo, flag);
  pthread_cond_signal (fifo->notEmpty);
  pthread_mutex_unlock (fifo->mut);
  pthread_cond_signal (fifo->notEmpty); 
  for (i=0; i<Q; i++)
  {
    pthread_join(con[i], NULL);
  }
  queueDelete(fifo);
  fclose(stats_cons);
  fclose(stats_prod);
  printf("Exiting..\n");
  t->StopFcn();
}
void *StartTimerFcn(void *arg)
{
  printf("Preproccesing the data\n");
}

void timerInit(timer *t,int TasksToExecute, int Period, queue *q, void * UserData, void * (*StartFcn)(void *),void * (*TimerFcn)(void *), void *(*ErrorFcn)(void *),void * (*StopFcn)())
{
  t->TasksToExecute = TasksToExecute;
  t->Period  = Period;
  t->queue = q; 
  t->UserData = UserData;
  t->StartFcn = StartFcn;
  t->TimerFcn = TimerFcn;
  t->ErrorFcn = ErrorFcn;
  t->StopFcn = StopFcn;
  printf("Tasks: %d\nPeriod: %dms\n",t->TasksToExecute,t->Period);
}


void *ErrorTimerFcn(void *arg)
{
  printf("Queue full\n");
  return NULL;
}

void *function1(void *t)
{
  double *x;
  x = (double*)t;
  int k;
  long long address = (long long)&k;
  srand(address);
  k = rand()%100 +1;
  double y = sin(*x/k);
  //printf("sin(π/%d): %f\n",k, y);
  return NULL;
}

void start(timer *t)
{
  pthread_create(&t->pid, NULL, producer,(timer *)t);
}
void startat(timer*t,int y,int m,int d,int h,int min,int sec)
{
  int delay;
  double delay_secs;
  time_t now;
  struct tm info;
  now = time(NULL);
  info = *localtime(&now);
  info.tm_year=y-1900; info.tm_mon = m-1; info.tm_mday = d; 
  info.tm_hour = h;    info.tm_min = min; info.tm_sec = sec;
  delay_secs = difftime(mktime(&info),now);
  delay = (int)delay_secs;
  if (delay >= 0){
    printf("Wait for %0.f secs\n",delay_secs);
    sleep(delay);
    start(t);
  }
  else{
    printf("ERROR:The date you entered belongs to the past\n");
    return;
  }
  return;
}
void *StopTimerFcn()
{
  printf("Bye bye...\n");
}

queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);
  
  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);  
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

void queueAdd (queue *q,  workFunction in)
{
  q->buf[q->tail] = in;
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel (queue *q,  workFunction *out)
{
  *out = q->buf[q->head];

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}


void *consumer (void *q)
{
  struct timeval p_timestamp,timestamp;
  long int timeInQueue,prod_time,time_now;
  queue *fifo;
  workFunction item_r,flag;         
  fifo = (queue *)q;
  passData *arrivaldata;                                      
         

  while(1) {

    pthread_mutex_lock (fifo->mut);
    
    
    while (fifo->empty) {
      //printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
      
    }
    queueDel (fifo, &item_r);

    if (item_r.work == NULL) 
    {
      queueAdd (fifo, flag); 
      pthread_mutex_unlock (fifo->mut);
      pthread_cond_signal (fifo->notEmpty);
      return (NULL);
    }

    //Execute Function
    gettimeofday(&timestamp,NULL);
    arrivaldata = item_r.arg; 
    p_timestamp = arrivaldata->tval;
    prod_time = p_timestamp.tv_sec*1000000+p_timestamp.tv_usec;
    time_now = timestamp.tv_sec*1000000+timestamp.tv_usec;
    timeInQueue = time_now-prod_time;
    fprintf(stats_cons,"%ld\n",timeInQueue);                                                      
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
    item_r.work(arrivaldata->argd);
    
  }
 
  free(arrivaldata);
  return (NULL);
}

void *producer (void *t)
{
  int i;
  timer *tim;
  tim = (timer *)t;
  struct timeval timestamp,timetosend;
  long int timedrift,p_time,timePassed,sleep;
  
  sleep = tim->Period*1000;
  //Preparing Items to be sent
  passData *pdata;
  pdata = malloc(sizeof(passData));
  pdata->argd = tim->UserData;
  workFunction item_s;
  item_s.work = tim->TimerFcn;
  item_s.arg = pdata; 
  queue *fifo;
  fifo = (queue *)tim->queue;

  
  gettimeofday(&timestamp,NULL);
  p_time =timestamp.tv_sec*1000000 + timestamp.tv_usec-tim->Period*1000;
  for(i=0;i<tim->TasksToExecute;i++)
  {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      printf("FIFO full\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    gettimeofday(&timetosend,NULL);
    pdata->tval = timetosend;
   
    queueAdd (fifo, item_s);
    gettimeofday(&timestamp,NULL); 
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
    timePassed = timestamp.tv_sec*1000000 + timestamp.tv_usec - p_time;
    timedrift = timePassed -tim->Period*1000;
    sleep = sleep - timedrift;
    fprintf(stats_prod,"%ld\n",timedrift);
    //printf("TimePassed = %ld\ntimedrift = %ld\nsleep = %ld\n\n",timePassed,timedrift,sleep);
    p_time = timestamp.tv_sec*1000000 + timestamp.tv_usec;
    usleep(sleep);
  }


}
  
 

  
