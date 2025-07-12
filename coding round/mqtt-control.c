#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <time.h>
#include <string.h>
#include <json-c/json.h>
#include <unistd.h>
#include <pthread.h>

#define audiodirectory "/home/sammisam8888/Desktop/GDG-BBSR-2025-Submission/audiofiles"

struct audioplayback{
    time_t timestamp;
    char musicname[256];
    int priority;
    struct audioplayback *next;
};
//DECLARED PRIORITY QUEUE FOR THE AUDIO PLAYBACK

struct audioplayback *head = NULL;
//INITIALISED HEADER OF PQ TO NULL

int checkduplicate(const char *filename) {
    //CHECKS DUPLICATE ENTRY FOR THE SAME FILE NAME
    struct audioplayback *temp = head;
    while (temp) {
        if (strcmp(temp->musicname, filename) == 0) {
            return 1;
        }
        temp = temp->next;
    }
    return 0;

}

void enqueue(const char *filename,int priority){
    //ADDS A NEW MESSAGE TO THE PQ

    if (checkduplicate(filename)) {
        printf("Duplicate entry found for %s. Ignored\n", filename);
        return;
    }

    struct audioplayback *newnode = malloc(sizeof(struct audioplayback));
    newnode->timestamp = time(NULL);
    strcpy(newnode->musicname, filename);
    newnode->musicname[sizeof(newnode->musicname) - 1] = '\0';
    newnode->priority = priority;
    newnode->next = NULL;

    if (head == NULL) {
        head = newnode;
        return;
    } 
    if (!head || (priority > head->priority) || (priority == head->priority && newnode->timestamp < head->timestamp)) {
        //IF THE PRIORITY IS MAXIMUM IN THE RECIEVED QUERIES QUEUE, THEN IT IS PLAYED FIRST
        newnode->next = head;
        head = newnode;
        return;
    }

    struct audioplayback *temp = head;
    //ITERATING THROUGH THE PRIORITY QUEUE TO IMPLEMENT THE AUDIO PLAYBACK USING PQ LOGIC
    while (temp->next &&
          ((temp->next->priority > priority) || 
          (temp->next->priority == priority && temp->next->timestamp <= newnode->timestamp))) {
        temp = temp->next;
    }

    temp->next = newnode;

}

void receivemessage(struct mosquitto *mqtt, void *user, const struct mosquitto_message *msg) {

    //RECEIVED THE JSON QUERY VIA THE mosquitto_pub COMMAND FROM THE SERVER
    //RECEIVED MESSAGE IS STORED AS msg->payload

    printf("Message received on topic %s: %s\n", msg->topic, (char *)msg->payload);

    struct json_object *json = NULL;
    struct json_object *filename = NULL;
    struct json_object *priority = NULL;

    json = json_tokener_parse((char *)msg->payload);
    if (json == NULL) {
        printf("Warning: Malformed JSON. Ignoring this message.\n");
        return;
        //CHECKS MALFORMED JSON IF NULL
    }

    if (!json_object_object_get_ex(json, "file", &filename) ||
        !json_object_object_get_ex(json, "priority", &priority)) {
        //CHECKS IF THE FILENAME AND PRIORITY IS IN CORRECT FORMAT WITHIN THE JSON 
        
        printf("Warning: Missing 'file' or 'priority' field. Ignored message.\n");
        json_object_put(json); 
        return;
    }

    const char *file = json_object_get_string(filename);
    int prio = json_object_get_int(priority);

    printf("Queued audio file: %s with priority: %d\n", file, prio);
    enqueue(file, prio);

    json_object_put(json); 
}

void *playaudio(void *arg){
    //PLAYS THE AUDIO FILE FROM THE LOCAL DIRECTORY AS PER THE SEQUENCE OBTAINED FROM PRIORITY QUEUE

    while (1) {
        if (head) {
            struct audioplayback *prev = NULL;
            struct audioplayback *maxprevious = NULL;
            struct audioplayback *highestpriority = head;
            struct audioplayback *curr = head;

            while (curr->next) {
                if ((curr->next->priority < highestpriority->priority) ||
                    (curr->next->priority == highestpriority->priority &&
                     curr->next->timestamp < highestpriority->timestamp)) {
                    maxprevious = curr;
                    highestpriority = curr->next;
                }
                curr = curr->next;
            }

            if (highestpriority == head) {
                head = head->next;
            } 
            else {
                maxprevious->next = highestpriority->next;
            }

            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", audiodirectory, highestpriority->musicname);
            printf("Playing audio file: %s with priority : %d\n", highestpriority->musicname, highestpriority->priority);
            //ADDS THE FILE NAME TO THE AUDIO DIRECTORY AND FETCHES THE AUDIO FILETO BE PLAYED

            if (access(fullpath, F_OK) != 0) {
                printf("File not found: %s. Skipping to Next.\n", fullpath);
                free(highestpriority);
                continue;
            }

            char cmd[600];
            snprintf(cmd, sizeof(cmd), "aplay '%s' > /dev/null 2>&1", fullpath);
            system(cmd);
            //USING THE aplay METHOD IN COMMAND PROMPT THE MUSIC FILE IS PLAYED IN THE BACKGROUND

            free(highestpriority);
        }
        sleep(1);
        //REPEATS FOR INFINITE LOOP
    }
    return NULL;
}


int main(){
    mosquitto_lib_init();  
    //INITIALISES THE MOSQUITTO SERVER CONNECTION

    struct mosquitto *mqtt = mosquitto_new(NULL, true, NULL);
    mosquitto_username_pw_set(mqtt, "nosslstage_client2", "client@5645##");
    mosquitto_tls_set(mqtt, NULL, NULL, NULL, NULL, NULL); 
    int test = mosquitto_connect(mqtt, "stagingmqttnossl.txninfra.com", 1883, 60);
    if (test != MOSQ_ERR_SUCCESS){
        printf("Connection failed with error : %s\n", mosquitto_strerror(test));
    }
    //CHECKS IF THE MOSQUITTO MQTT CONNECTION WAS SUCCESSFUL OR NOT

    else {
        printf("Connected to MQTT broker successfully.\n");
    }

    char mobno[15], topic[200];
    strcpy(mobno, "9337903728");
    strcpy(topic, "/comp/");
    strcat(topic, mobno);
    mosquitto_subscribe(mqtt, NULL, topic, 0);
    //CREATES A SUBSCRIBER ON THE GIVEN MOBILE NUMBER

    printf("Subscribed to topic: %s \n", topic);
    mosquitto_message_callback_set(mqtt, receivemessage);

    mosquitto_loop_start(mqtt);
    pthread_t tid;
    pthread_create(&tid, NULL, playaudio, NULL);

    while(1){
    //RUNS AN INFINITE LOOP TO KEEP THE CODE RUNNING IN THE BACKGROUND
        sleep(1);
    }
    mosquitto_destroy(mqtt);

    //DESTROYING THE MEMORY SPACE OCCUPIED BY THE MQTT OBJECT
    
    mosquitto_lib_cleanup();
    return 0;

}
