#define _GNU_SOURCE

#include <cjson/cJSON.h>
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

char timestamp_buf[256] = {0};

char *get_timestamp() {
  time_t rawtime;
  struct tm timeinfo;
  time(&rawtime);
  localtime_r(&rawtime, &timeinfo);
  strftime(timestamp_buf, sizeof(timestamp_buf), "%Y-%m-%dT%H:%M:%S",
           &timeinfo);
  return timestamp_buf;
}

#define LOG_INFO(fmt, ...) printf("INFO: " fmt "\n", ##__VA_ARGS__)
#ifdef DEBUG
#define LOG_DEBUG(fmt, ...) printf("DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif

void on_connect(struct mosquitto *mosq, void *obj, int rc) {
  LOG_INFO("Connected to MQTT broker");
  mosquitto_subscribe(mosq, NULL, "rotom-heat/climate", 0);
}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc) {
  LOG_INFO("Disconnected from MQTT broker");
}

void on_message(struct mosquitto *mosq, void *obj,
                const struct mosquitto_message *msg) {
  LOG_DEBUG("Received message on \"%s\": %s", msg->topic, (char *)msg->payload);
  cJSON *json = cJSON_Parse(msg->payload);
  cJSON *temperature = cJSON_GetObjectItemCaseSensitive(json, "temperature");
  cJSON *humidity = cJSON_GetObjectItemCaseSensitive(json, "humidity");
  if (cJSON_IsNumber(temperature) && cJSON_IsNumber(humidity)) {
    fprintf(obj, "%s,%.2f,%.2f\n", get_timestamp(), temperature->valuedouble,
            humidity->valuedouble);
    fflush(obj);
  }
  cJSON_Delete(json);
}

int main(int argc, char *argv[]) {
  int c;
  char *username = getenv("ROTOM_MONITOR_USERNAME");
  char *password = getenv("ROTOM_MONITOR_PASSWORD");
  char *address = getenv("ROTOM_MONITOR_HOST");

  if (username == NULL || password == NULL || address == NULL) {
    printf("Missing required environment variables\n");
    return 1;
  }

  // TODO: Make output file configurable, maybe use XDG
  struct stat st = {0};
  if (stat("./data", &st) == -1) {
    mkdir("./data", 0700);
  }
  FILE *fp = fopen("./data/rotom-monitor.csv", "a");
  stat("./data/rotom-monitor.csv", &st);
  if (st.st_size == 0) {
    fprintf(fp, "timestamp,temperature,humidity\n");
    fflush(fp);
  }

  mosquitto_lib_init();

  struct mosquitto *client = mosquitto_new(NULL, true, fp);

  // set up auth and client options
  mosquitto_username_pw_set(client, username, password);

  // set up callbacks
  mosquitto_connect_callback_set(client, on_connect);
  mosquitto_disconnect_callback_set(client, on_disconnect);
  mosquitto_message_callback_set(client, on_message);

  // connect to broker
  LOG_INFO("Trying to connected to MQTT broker at %s", address);
  mosquitto_connect(client, address, 1883, 60);

  // already handles reconnection internally
  mosquitto_loop_forever(client, -1, 1);

  mosquitto_lib_cleanup();
  fclose(fp);
  return 0;
}
