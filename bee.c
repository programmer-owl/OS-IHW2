#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define BEEHIVE_SIZE 30

key_t key_bees, key_access, key_portions;
int bees_id, access_id, portions_id;
struct sembuf sb;

int pid = 0;
const int HALF_BEEHIVE = BEEHIVE_SIZE / 2;


// Функция, осуществляющая при запуске манипуляции с памятью и семафорами
void my_init() {
  key_bees = ftok(".", 'R');
  key_access = ftok(".", 'W');
  key_portions = ftok(".", 'S');
  printf(
  "key_bees: %d, key_access: %d, key_portions: %d\n", key_bees, key_access, key_portions);
  if (key_bees == -1 || key_access == -1 || key_portions == -1) {
      perror("ftok");
      exit(-1);
  }
  bees_id = semget(key_bees, 1, 0666 | IPC_CREAT);
  if (bees_id == -1) {
      perror("semget: bees");
      exit(-1);
  }
  access_id = semget(key_access, 1, 0666 | IPC_CREAT);
  if (access_id == -1) {
      perror("semget: access");
      exit(-1);
  }
  portions_id = semget(key_portions, 1, 0666 | IPC_CREAT);
  if (portions_id == -1) {
      perror("semget: portions");
      exit(-1);
  }
  // Устанавливать значения должна программа Винни Пуха :)
}

// Удаляет семафоры тоже Винни Пух :))

int my_rand(int min, int max) {
  srand(time(NULL));
  return rand() % (max - min + 1) + min;
}

void sigint_handler(int signum) {
  if (pid == 0) {
    printf("You've stopped the program. The bees are grateful for saving them from the bear!\n");
  }
  exit(signum);
}

int main() {
  signal(SIGINT, sigint_handler);
  my_init();
  int check = semctl(bees_id, 0, GETVAL);
  printf("How many bees can leave the hive: %d\n", check);
  check = semctl(access_id, 0, GETVAL);
  printf("Hive access: %d\n", check);
  check = semctl(portions_id, 0, GETVAL);
  printf("Honey portions in the hive: %d\n", check);
  while (true) {   
    sb.sem_num = 0;
      sb.sem_op = -1;
      sb.sem_flg = 0;
      semop(bees_id, &sb, 1);

      int bees_in_hive = semctl(bees_id, 0, GETVAL);
      ++bees_in_hive;
      printf("Bee is leaving the hive. There are %d bees left there.\n", bees_in_hive);
      sleep(my_rand(1, 6)); // Искать мед - сложная работа!

      // пчела кладет мед в улей
      sb.sem_num = 0;
      sb.sem_op = -1;
      sb.sem_flg = 0;
      semop(access_id, &sb, 1);
      if (semctl(portions_id, 0, GETVAL) < BEEHIVE_SIZE) {
        sb.sem_num = 0;
        sb.sem_op = 1;
        sb.sem_flg = 0;
        semop(portions_id, &sb, 1);
        printf("Bee is putting honey in the hive.\n");
      } else {
        printf("The hive is full!\n");
      }
      // улей снова доступен
      sb.sem_num = 0;
      sb.sem_op = 1;
      sb.sem_flg = 0;
      semop(access_id, &sb, 1);

      sb.sem_num = 0;
      sb.sem_op = 1;
      sb.sem_flg = 0;
      semop(bees_id, &sb, 1);
      bees_in_hive = semctl(bees_id, 0, GETVAL);
      ++bees_in_hive;
      printf("Bee stays in the hive for a while. There are %d bees in hive now.\n", bees_in_hive);
      sleep(my_rand(1, 3));
    }
}
