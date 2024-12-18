#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

void DearOldDad(int* bankAccount, sem_t* mutex);
void PoorStudent(int* bankAccount, sem_t* mutex);

int main() {
    // Initialize random seed
    srand(time(NULL));
    
    // Create shared memory for bank account
    int ShmID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0) {
        printf("*** shmget error ***\n");
        exit(1);
    }
    
    // Attach shared memory
    int* bankAccount = (int*)shmat(ShmID, NULL, 0);
    if (*bankAccount == -1) {
        printf("*** shmat error ***\n");
        exit(1);
    }
    
    // Initialize bank account
    *bankAccount = 0;
    
    // Create semaphore
    sem_t* mutex = sem_open("/bank_sem", O_CREAT, 0644, 1);
    if (mutex == SEM_FAILED) {
        perror("semaphore initialization");
        exit(1);
    }
    
    // Fork process
    pid_t pid = fork();
    if (pid < 0) {
        printf("*** fork error ***\n");
        exit(1);
    }
    else if (pid == 0) {
        // Child process (Poor Student)
        PoorStudent(bankAccount, mutex);
        exit(0);
    }
    else {
        // Parent process (Dear Old Dad)
        DearOldDad(bankAccount, mutex);
        
        // Wait for child to finish
        wait(NULL);
        
        // Cleanup
        shmdt(bankAccount);
        shmctl(ShmID, IPC_RMID, NULL);
        sem_close(mutex);
        sem_unlink("/bank_sem");
    }
    
    return 0;
}

void DearOldDad(int* bankAccount, sem_t* mutex) {
    while(1) {
        // Sleep random time (0-5 seconds)
        sleep(rand() % 6);
        
        printf("Dear Old Dad: Attempting to Check Balance\n");
        
        // Generate random number
        int random = rand();
        int localBalance;
        
        sem_wait(mutex);
        localBalance = *bankAccount;
        
        if (random % 2 == 0) {  // Even number
            if (localBalance < 100) {
                // Try to deposit money
                int amount = (rand() % 100) + 1;
                if (rand() % 2 == 0) {  // Even number - deposit
                    localBalance += amount;
                    *bankAccount = localBalance;
                    printf("Dear old Dad: Deposits $%d / Balance = $%d\n", amount, localBalance);
                } else {
                    printf("Dear old Dad: Doesn't have any money to give\n");
                }
            } else {
                printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
            }
        } else {  // Odd number
            printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
        }
        
        sem_post(mutex);
    }
}

void PoorStudent(int* bankAccount, sem_t* mutex) {
    while(1) {
        // Sleep random time (0-5 seconds)
        sleep(rand() % 6);
        
        printf("Poor Student: Attempting to Check Balance\n");
        
        // Generate random number
        int random = rand();
        int localBalance;
        
        sem_wait(mutex);
        localBalance = *bankAccount;
        
        if (random % 2 == 0) {  // Even number - try to withdraw
            int need = (rand() % 50) + 1;
            printf("Poor Student needs $%d\n", need);
            
            if (need <= localBalance) {
                localBalance -= need;
                *bankAccount = localBalance;
                printf("Poor Student: Withdraws $%d / Balance = $%d\n", need, localBalance);
            } else {
                printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
            }
        } else {  // Odd number - just check balance
            printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
        }
        
        sem_post(mutex);
    }
}