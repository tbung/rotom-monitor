#include <cjson/cJSON.h>
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
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

#define LOG_INFO(fmt, ...)                                                     \
  printf("%s INFO: " fmt "\n", get_timestamp(), ##__VA_ARGS__)

void on_connect(struct mosquitto *mosq, void *obj, int rc) {
  LOG_INFO("Connected to MQTT broker");
}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc) {
  LOG_INFO("Disconnected from MQTT broker");
}

void on_message(struct mosquitto *mosq, void *obj,
                const struct mosquitto_message *msg) {
  LOG_INFO("Received message: %s", (char *)msg->payload);
  cJSON *json = cJSON_Parse(msg->payload);
  cJSON *temperature = cJSON_GetObjectItemCaseSensitive(json, "temperature");
  if (cJSON_IsNumber(temperature)) {
    printf("Temperature = %.2f°C\n", temperature->valuedouble);
  }
  cJSON *humidity = cJSON_GetObjectItemCaseSensitive(json, "humidity");
  if (cJSON_IsNumber(humidity)) {
    printf("Humidity = %.2f%%\n", humidity->valuedouble);
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

  mosquitto_lib_init();

  struct mosquitto *client = mosquitto_new(NULL, true, NULL);

  // set up auth and client options
  mosquitto_username_pw_set(client, username, password);

  // set up callbacks
  mosquitto_connect_callback_set(client, on_connect);
  mosquitto_disconnect_callback_set(client, on_disconnect);
  mosquitto_message_callback_set(client, on_message);

  // connect to broker
  LOG_INFO("Trying to connected to MQTT broker at %s", address);
  mosquitto_connect(client, address, 1883, 60);
  mosquitto_subscribe(client, NULL, "rotom-heat/climate", 0);

  mosquitto_loop_forever(client, -1, 1);

  mosquitto_lib_cleanup();
  return 0;
}
