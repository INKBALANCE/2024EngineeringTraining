#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct User {
    int id;
    char username[50];
    char password[50];
    char contact[50];
    struct User* next;
} User;

User* head = NULL;

// Function prototypes
User* createUser(int id, char* username, char* password, char* contact);
void loadUsersFromFile(const char* filename);
void addUserInOrder(int id, char* username, char* password, char* contact);
void deleteUser(int id);
void updateUser(int id, char* username, char* password, char* contact);
void printUsers();
void writeUsersToFile(const char* filename);
void sortUsersByUsername();
void sortUsersById();
void menu();

User* createUser(int id, char* username, char* password, char* contact) {
    User* newUser = (User*)malloc(sizeof(User));
    newUser->id = id;
    strcpy(newUser->username, username);
    strcpy(newUser->password, password);
    strcpy(newUser->contact, contact);
    newUser->next = NULL;
    return newUser;
}

void loadUsersFromFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        printf("File could not be opened.\n");
        return;
    }

    char buffer[256];
    fgets(buffer, sizeof(buffer), file); // Skip header line

    while (fgets(buffer, sizeof(buffer), file)) {
        int id;
        char username[50], password[50], contact[50];
        sscanf(buffer, "%d,%[^,],%[^,],%s", &id, username, password, contact);
        addUserInOrder(id, username, password, contact);
    }
    fclose(file);
}

void addUserInOrder(int id, char* username, char* password, char* contact) {
    User* newUser = createUser(id, username, password, contact);
    if (head == NULL || head->id >= newUser->id) {
        newUser->next = head;
        head = newUser;
    } else {
        User *current = head;
        while (current->next != NULL && current->next->id < newUser->id) {
            current = current->next;
        }
        newUser->next = current->next;
        current->next = newUser;
    }
}

void deleteUser(int id) {
    User *temp = head, *prev = NULL;
    while (temp != NULL && temp->id < id) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL || temp->id != id) {
        printf("User not found.\n");
        return;
    }
    if (prev == NULL) head = temp->next;
    else prev->next = temp->next;
    free(temp);
}

void updateUser(int id, char* username, char* password, char* contact) {
    User* temp = head;
    while (temp != NULL && temp->id < id) {
        temp = temp->next;
    }
    if (temp != NULL && temp->id == id) {
        strcpy(temp->username, username);
        strcpy(temp->password, password);
        strcpy(temp->contact, contact);
    } else {
        printf("User not found.\n");
    }
}

void printUsers() {
    User* temp = head;
    printf("Current Users:\n");
    while (temp != NULL) {
        printf("%d, %s, %s, %s\n", temp->id, temp->username, temp->password, temp->contact);
        temp = temp->next;
    }
}

void writeUsersToFile(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file");
        return;
    }
    fprintf(file, "id,username,password,contact\n");
    User* temp = head;
    while (temp != NULL) {
        fprintf(file, "%d,%s,%s,%s\n", temp->id, temp->username, temp->password, temp->contact);
        temp = temp->next;
    }
    fclose(file);
}

void sortUsersByUsername() {
    if (head == NULL || head->next == NULL) return;
    User *sorted = NULL, *current = head, *next = NULL;
    while (current != NULL) {
        next = current->next;
        if (sorted == NULL || strcmp(current->username, sorted->username) < 0) {
            current->next = sorted;
            sorted = current;
        } else {
            User *temp = sorted;
            while (temp->next != NULL && strcmp(current->username, temp->next->username) > 0) {
                temp = temp->next;
            }
            current->next = temp->next;
            temp->next = current;
        }
        current = next;
    }
    head = sorted;
}

void sortUsersById() {
    if (head == NULL || head->next == NULL) return;
    User *sorted = NULL, *current = head, *next = NULL;
    while (current != NULL) {
        next = current->next;
        if (sorted == NULL || current->id < sorted->id) {
            current->next = sorted;
            sorted = current;
        } else {
            User *temp = sorted;
            while (temp->next != NULL && current->id > temp->next->id) {
                temp = temp->next;
            }
            current->next = temp->next;
            temp->next = current;
        }
        current = next;
    }
    head = sorted;
}

void menu() {
    int choice;
    const char* filename = "/home/mo/ET/day1/users2.csv"; // Adjust the file path as needed
    loadUsersFromFile(filename);
    do {
        printf("\n1. Add User\n2. Delete User\n3. Update User\n4. List Users\n5. Sort Users by Username\n6. Sort Users by ID\n7. Exit\nEnter choice: ");
        scanf("%d", &choice);

        int id;
        char username[50], password[50], contact[50];
        switch (choice) {
            case 1:
                printf("Enter ID: ");
                scanf("%d", &id);
                printf("Enter username: ");
                scanf("%s", username);
                printf("Enter password: ");
                scanf("%s", password);
                printf("Enter contact: ");
                scanf("%s", contact);
                addUserInOrder(id, username, password, contact);
                writeUsersToFile(filename);
                break;
            case 2:
                printf("Enter ID of user to delete: ");
                scanf("%d", &id);
                deleteUser(id);
                writeUsersToFile(filename);
                break;
            case 3:
                printf("Enter ID of user to update: ");
                scanf("%d", &id);
                printf("Enter new username: ");
                scanf("%s", username);
                printf("Enter new password: ");
                scanf("%s", password);
                printf("Enter new contact: ");
                scanf("%s", contact);
                updateUser(id, username, password, contact);
                writeUsersToFile(filename);
                break;
            case 4:
                printUsers();
                break;
            case 5:
                sortUsersByUsername();
                printUsers();
                break;
            case 6:
                sortUsersById();
                printUsers();
                break;
            case 7:
                break;
            default:
                printf("Invalid choice.\n");
        }
    } while (choice != 7);
}

int main() {
    menu();
    return 0;
}

