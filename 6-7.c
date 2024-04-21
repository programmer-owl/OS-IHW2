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

typedef struct {
  sem_t bees;
sem_t beehive_access;
  int honey_portions;
} HiveData;

HiveData* hive_data;

// имя области разделяемой памяти
const char *shar_object = "/posix-shar-object";

// Функция, осуществляющая при запуске манипуляции с памятью и семафорами
void my_init(int n) {  
  int shmid = shm_open(shar_object, O_CREAT | O_RDWR, 0666);
  if (shmid == -1) {
    perror("shm_open");
    exit(-1);
  }
  // Задание размера объекта памяти
  if (ftruncate(shmid, sizeof(HiveData)) == -1) {
    perror("ftruncate: memory sizing error");
    exit(-1);
  }
  hive_data = mmap(0, sizeof(HiveData), PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
  if (hive_data == MAP_FAILED) {
      perror("mmap");
      exit(-1);
  }
  hive_data->honey_portions = 0;
  
  // Изначально все n пчел находятся в улье, улететь могут (n-1) из них
  if (sem_init(&hive_data->bees, 0, n - 1) == -1) {
    perror("sem_init: Can not create bees semaphore");
    exit(-1);
  };
  // Изначально с ульем никто не взаимодействует, доступ открыт
  if (sem_init(&hive_data->beehive_access, 0, 1) == -1) {
    perror("sem_init: Can not create beehive access semaphore");
    exit(-1);
  };
}

// Функция, закрывающая семафоры и разделяемую память
void my_close(void) {
  if (sem_destroy(&hive_data->bees) == -1) {
    perror("sem_destroy: Incorrect destruction of bees semaphore");
    exit(-1);
  };
  if (sem_destroy(&hive_data->beehive_access) == -1) {
    perror("sem_destroy: Incorrect destruction of beehive access semaphore");
    exit(-1);
  };
}

// Функция, удаляющая азделяемую память
void my_unlink(void) {
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
  sem_getvalue(&hive_data->bees, &check);
  printf("How many bees can leave the hive: %d\n", check);
  sem_getvalue(&hive_data->beehive_access, &check);
  printf("Hive access: %d\n", check);
  printf("Honey portions in the hive: %d\n", hive_data->honey_portions);

  // Создание подпроцессов пчел
  for (int i = 0; i < n; i++) {
    if ((pid = fork()) == 0)
      continue; // процесс-родитель

    // внутри подпроцесса пчелы
    while (true) {
      int bees_in_hive = 0;
      sem_wait(&hive_data->bees);
      sem_getvalue(&hive_data->bees, &bees_in_hive);
      ++bees_in_hive;
      printf("Bee %d is leaving the hive. There are %d bees left there.\n", i + 1, bees_in_hive);
      sleep(my_rand(1, 4)); // Искать мед - сложная работа!

      // пчела кладет мед в улей
      sem_wait(&hive_data->beehive_access);
      if (hive_data->honey_portions < BEEHIVE_SIZE) {
        ++(hive_data->honey_portions);
        printf("Bee %d is putting honey in the hive.\n", i + 1);
      } else {
        printf("The hive is full!\n");
      }
      // улей снова доступен
      sem_post(&hive_data->beehive_access);

      sem_post(&hive_data->bees);
      sem_getvalue(&hive_data->bees, &bees_in_hive);
      ++bees_in_hive;
      printf("Bee %d stays in the hive for a while. There are %d bees in hive now.\n", i + 1, bees_in_hive);
      sleep(my_rand(1, 3));
    }
  }

  // процесс Винни Пуха
  if ((pid = fork()) != 0) {
    // внутри процесса Винни Пуха
    while (true) {
      if (hive_data->honey_portions >= HALF_BEEHIVE) {
        printf(
            "There are %d portions of honey in the hive. Winnie is trying to "
            "steal them!\n",
          hive_data->honey_portions);
        int bees_in_hive;
        sem_getvalue(&hive_data->bees, &bees_in_hive);
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
          sem_wait(&hive_data->beehive_access);
        hive_data->honey_portions = 0;
          sem_post(&hive_data->beehive_access);
        }
        sleep(my_rand(1, 3));
      }
      else {
        printf("There are %d portions of honey in the hive. Winnie is not interested!\n", hive_data->honey_portions);
        sleep(my_rand(1, 3));
      }
    }
  }

  while (true) {} // процесс-родитель отдыхает
}
