#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BEEHIVE_SIZE 30

key_t key_bees, key_access, key_portions;
int bees_id, access_id, portions_id;
struct sembuf sb;

int pid = 0;
const int HALF_BEEHIVE = BEEHIVE_SIZE / 2;

// Функция, осуществляющая при запуске манипуляции с памятью и семафорами
void my_init(int n) {
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
  // Изначально все n пчел находятся в улье, улететь могут (n-1) из них
  semctl(bees_id, 0, SETVAL, n - 1);
  // Изначально с ульем никто не взаимодействует, доступ открыт
  semctl(access_id, 0, SETVAL, 1);
  // Изначально в улье нет меда
  semctl(portions_id, 0, SETVAL, 0);
}

// Функция, удаляющая все семафоры
void my_unlink(void) {
  if (semctl(bees_id, 0, IPC_RMID) == -1) {
    perror("semctl: bees");
  }
  if (semctl(access_id, 0, IPC_RMID) == -1) {
    perror("semctl: access");
  }
  if (semctl(portions_id, 0, IPC_RMID) == -1) {
    perror("semctl: portions");
  }
}

int my_rand(int min, int max) {
  srand(time(NULL));
  return rand() % (max - min + 1) + min;
}

void sigint_handler(int signum) {
  if (pid == 0) {
    my_unlink();
    printf("You've stopped the program. Winnie is offended! He wanted more "
           "honey!\n");
  }
  exit(signum);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Not enough arguments!\nExpected one number greater than 3.\n");
    exit(-1);
  }
  if (argc > 2) {
    printf("Too many arguments!\nExpected only one number greater than 3.\n");
    exit(-1);
  }
  int n = atoi(argv[1]);
  if (n <= 3) {
    printf("Expected one number greater than 3.\n");
    exit(-1);
  }
  signal(SIGINT, sigint_handler);
  my_init(n);

  int check = semctl(bees_id, 0, GETVAL);
  printf("How many bees can leave the hive: %d\n", check);
  check = semctl(access_id, 0, GETVAL);
  printf("Hive access: %d\n", check);
  check = semctl(portions_id, 0, GETVAL);
  printf("Honey portions in the hive: %d\n", check);
  while (true) {
    int honey_portions = semctl(portions_id, 0, GETVAL);
    if (honey_portions >= HALF_BEEHIVE) {
      printf("There are %d portions of honey in the hive. Winnie is trying to "
             "steal them!\n",
             honey_portions);
      int bees_in_hive = semctl(bees_id, 0, GETVAL);
      ++bees_in_hive;
      if (bees_in_hive >= 3) {
        printf(
            "There are %d bees in the hive. Winnie got stung and run away!\n",
            bees_in_hive);
        sleep(my_rand(1, 3));
        printf("Winnie is healed!\n");
      } else {
        printf("There are %d bees in the hive. Winnie is able to steal the "
               "honey!\n",
               bees_in_hive);
        // Винни опустошает улей
        sb.sem_num = 0;
        sb.sem_op = -1;
        sb.sem_flg = 0;
        semop(access_id, &sb, 1);

        sb.sem_num = 0;
        sb.sem_op = -honey_portions;
        sb.sem_flg = 0;
        semop(portions_id, &sb, 1);

        sb.sem_num = 0;
        sb.sem_op = 1;
        sb.sem_flg = 0;
        semop(access_id, &sb, 1);
      }
      sleep(my_rand(1, 3));
    } else {
      printf("There are %d portions of honey in the hive. Winnie is not "
             "interested!\n",
             honey_portions);
      sleep(my_rand(1, 3));
    }
  }

  while (true) {} // процесс-родитель отдыхает
}