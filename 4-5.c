#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define BEEHIVE_SIZE 30

int pid = 0;
const int HALF_BEEHIVE = BEEHIVE_SIZE / 2;

// имя области разделяемой памяти
const char *shar_object = "/posix-shar-object";

int *honey_portions = 0; // указатель на разделямую память, хранящую количество порций меда в улье

// имя семафора для количества пчел, которое может улететь
const char *bees_sem_name = "/bees-semaphore";
sem_t *bees; // указатель на семафор количества пчел, которое может улететь

// имя семафора для доступа к улью
const char *beehive_access_sem_name = "/honey-semaphore";
sem_t *beehive_access; // указатель на семафор доступа к улью

// Функция, осуществляющая при запуске манипуляции с памятью и семафорами
void my_init(int n) {
  // Изначально все n пчел находятся в улье, улететь могут (n-1) из них
  if ((bees = sem_open(bees_sem_name, O_CREAT, 0666, n - 1)) == 0) {
    perror("sem_open: Can not create bees semaphore");
    exit(-1);
  };
  // Изначально с ульем никто не взаимодействует, доступ открыт
  if ((beehive_access = sem_open(beehive_access_sem_name, O_CREAT, 0666, 1)) == 0) {
    perror("sem_open: Can not create beehive access semaphore");
    exit(-1);
  };

  int shmid = shm_open(shar_object, O_CREAT | O_RDWR, 0666);
  if (shmid == -1) {
    perror("shm_open");
    exit(-1);
  }
  if (ftruncate(shmid, sizeof(int)) == -1) {
    perror("ftruncate");
    exit(-1);
  }
  honey_portions =
      mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
  if (honey_portions == MAP_FAILED) {
    perror("mmap");
    exit(-1);
  }
}

// Функция, закрывающая семафоры и разделяемую память
void my_close(void) {
  if (sem_close(bees) == -1) {
    perror("sem_close: Incorrect close of bees semaphore");
    exit(-1);
  };
  if (sem_close(beehive_access) == -1) {
    perror("sem_close: Incorrect close of beehive access semaphore");
    exit(-1);
  };
}

// Функция, удаляющая все семафоры и разделяемую память
void my_unlink(void) {
  if (sem_unlink(bees_sem_name) == -1) {
    perror("sem_unlink: Incorrect unlink of bees semaphore");
    exit(-1);
  };
  if (sem_unlink(beehive_access_sem_name) == -1) {
    perror("sem_unlink: Incorrect unlink of beehive access semaphore");
    exit(-1);
  }

  // удаление разделяемой памяти
  if (shm_unlink(shar_object) == -1) {
    perror("shm_unlink: shared memory");
    exit(-1);
  }
}

int my_rand(int min, int max) {
  srand(time(NULL));
  return rand() % (max - min + 1) + min;
}

void sigint_handler(int signum) {
  my_close();
  if (pid == 0) {
    my_unlink();
    printf("You've stopped the program. The bees are grateful for saving them from the bear!\n");
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

  int check = 0;
  sem_getvalue(bees, &check);
  printf("How many bees can leave the hive: %d\n", check);
  sem_getvalue(beehive_access, &check);
  printf("Hive access: %d\n", check);
  printf("Honey portions in the hive: %d\n", *honey_portions);

  // Создание подпроцессов пчел
  for (int i = 0; i < n; i++) {
    if ((pid = fork()) == 0)
      continue; // процесс-родитель

    // внутри подпроцесса пчелы
    while (true) {
      int bees_in_hive = 0;
      sem_wait(bees);
      sem_getvalue(bees, &bees_in_hive);
      ++bees_in_hive;
      printf("Bee %d is leaving the hive. There are %d bees left there.\n", i + 1, bees_in_hive);
      sleep(my_rand(1, 6)); // Искать мед - сложная работа!

      // пчела кладет мед в улей
      sem_wait(beehive_access);
      if (*honey_portions < BEEHIVE_SIZE) {
        ++(*honey_portions);
        printf("Bee %d is putting honey in the hive.\n", i + 1);
      } else {
        printf("The hive is full!\n");
      }
      // улей снова доступен
      sem_post(beehive_access);

      sem_post(bees);
      sem_getvalue(bees, &bees_in_hive);
      ++bees_in_hive;
      printf("Bee %d stays in the hive for a while. There are %d bees in hive now.\n", i + 1, bees_in_hive);
      sleep(my_rand(1, 3));
    }
  }

  // процесс Винни Пуха
  if ((pid = fork()) != 0) {
    // внутри процесса Винни Пуха
    while (true) {
      if ((*honey_portions) >= HALF_BEEHIVE) {
        printf(
            "There are %d portions of honey in the hive. Winnie is trying to "
            "steal them!\n",
            *honey_portions);
        int bees_in_hive;
        sem_getvalue(bees, &bees_in_hive);
        ++bees_in_hive;
        if (bees_in_hive >= 3) {
          printf(
              "There are %d bees in the hive. Winnie got stung and run away!\n",
              bees_in_hive);
          sleep(my_rand(1, 3));
          printf("Winnie is healed!\n");
        } 
      else {
          printf("There are %d bees in the hive. Winnie is able to steal the "
                 "honey!\n",
                 bees_in_hive);
          // Винни опустошает улей
          sem_wait(beehive_access);
          *honey_portions = 0;
          sem_post(beehive_access);
        }
        sleep(my_rand(1, 3));
      }
      else {
        printf("There are %d portions of honey in the hive. Winnie is not interested!\n", *honey_portions);
        sleep(my_rand(1, 3));
      }
    }
  }

  while (true) {} // процесс-родитель отдыхает
}
