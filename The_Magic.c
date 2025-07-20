#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

#define SHM_NAME "/memory"
#define SEM_NAME "/memory_semaphore"

typedef struct {
    char first_name[30];
    char last_name[30];
    int bank_account;
    int code;
} Customer;

typedef struct {
    int tickets;
    int id;
    Customer ticket_Customers[100];
    int is_valid[100]; // 1 = occupied, 0 = cancelled
} shared_data_t;

int main() {
    int shm_fd;
    shared_data_t *shared_data;
    sem_t *sem;

    // Open or create shared memory
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("can\'t open shared memory...");
        exit(EXIT_FAILURE);
    }

    // Set size of shared memory
    if (ftruncate(shm_fd, sizeof(shared_data_t)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory
    shared_data = mmap(0, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("Mapping issue...");
        exit(EXIT_FAILURE);
    }

    // Open or create the semaphore
    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("Can\'t create semaphore...");
        exit(EXIT_FAILURE);
    }

    // Initialize tickets once
    sem_wait(sem);
    if (shared_data->id == 0 && shared_data->tickets == 0) {
        shared_data->tickets = 10;
        shared_data->id = 0;
        for (int i = 0; i < 100; i++) {
            shared_data->is_valid[i] = 0;
        }
    }
    sem_post(sem);

    // Main loop
    while (1) {
        system("clear");
        system("figlet -w 100 -c flying_process.com");
        printf("\n——————————————————————————————————————————————————\n");
        printf("| Tickets available: %d                           |\n", shared_data->tickets);
        printf("——————————————————————————————————————————————————\n");

        printf("Options:\n");
        printf("1 - Book ticket\n");
        printf("2 - Cancel reservation\n");
        printf("3 - Show booking list\n");
        printf("4 - Refresh page\n");
        printf("q - Quit\n");
        printf("Choice: ");

        char choice = getchar();
        getchar(); // consume newline

        switch (choice) {
        case '1': {
            sem_wait(sem);
            if (shared_data->tickets > 0) {
                Customer *b = &shared_data->ticket_Customers[shared_data->id];
                printf("Enter client information:\n");

                printf("First Name: ");
                scanf("%s", b->first_name);

                printf("Last Name: ");
                scanf("%s", b->last_name);

                printf("Bank Account: ");
                scanf("%d", &b->bank_account);

                printf("Code: ");
                scanf("%d", &b->code);
                getchar(); // consume newline

                shared_data->is_valid[shared_data->id] = 1;
                printf("Reservation successful! the client's ID is %d\n", shared_data->id);
                shared_data->id++;
                shared_data->tickets--;
            } else {
                printf("No tickets available!\n");
            }
            sem_post(sem);
            sleep(2);
            break;
        }

        case '2': {
            int cancel_id;
            printf("Enter your reservation ID to cancel: ");
            scanf("%d", &cancel_id);
            getchar(); // consume newline

            sem_wait(sem);
            if (cancel_id >= 0 && cancel_id < 100 && shared_data->is_valid[cancel_id]) {
                shared_data->is_valid[cancel_id] = 0;
                shared_data->tickets++;
                printf("Reservation canceled successfully.\n");
            } else {
                printf("Invalid ID or no reservation found.\n");
            }
            sem_post(sem);
            sleep(2);
            break;
        }

        case '3': {
            sem_wait(sem);
            printf("————————————————————————————————————————————————\n");
            printf("| ID |     First Name     |     Last Name      |\n");
            printf("————————————————————————————————————————————————\n");
            for (int i = 0; i < shared_data->id; i++) {
                if (shared_data->is_valid[i]) {
                    printf("| %-2d | %-18s | %-18s |\n",
                           i,
                           shared_data->ticket_Customers[i].first_name,
                           shared_data->ticket_Customers[i].last_name);
                }
            }
            printf("————————————————————————————————————————————————\n");
            sem_post(sem);
            sleep(4);
            break;
        }
        case '4':
            system("clear");
            break;
        case 'q':
            goto end_loop;

        default:
            system("clear");
            break;
        }
    }
    
end_loop:
    // Cleanup
    munmap(shared_data, sizeof(shared_data_t));
    close(shm_fd);
    sem_close(sem);

    // Ask if user wants to unlink
    printf("Unlink shared memory and semaphore? (y/n): ");
    char ans = getchar();
    if (ans == 'y' || ans == 'Y') {
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
    }

    return 0;
}

