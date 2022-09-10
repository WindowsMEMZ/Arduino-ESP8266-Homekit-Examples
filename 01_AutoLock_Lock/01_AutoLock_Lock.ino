/*
 * 本项目为Homekit自锁开关
 * 使用前：
 *   修改本文件中的PIN_LOCK为你的继电器（或其他用于控制锁的硬件）的GPIO
 *   修改wifi_info.h中的ssid和password
 */

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

#define PIN_LOCK 0

void setup() {
  Serial.begin(115200);
  wifi_connect();
  //homekit_storage_reset();
  my_homekit_setup();
}

void loop() {
  my_homekit_loop();
  delay(10);
}

//==============================
// HomeKit setup and loop
//==============================

// access lock-mechanism HomeKit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;

extern "C" homekit_characteristic_t cha_lock_current_state;
extern "C" homekit_characteristic_t cha_lock_target_state;

static uint32_t next_heap_millis = 0;


// called when the lock-mechanism target-set is changed by iOS Home APP
void set_lock(const homekit_value_t value) {
  
  uint8_t state = value.int_value;
  cha_lock_current_state.value.int_value = state;
  
  if(state == 0){
    // lock-mechanism was unsecured by iOS Home APP
    open_lock();
  }
  if(state == 1){
    // lock-mechanism was secured by iOS Home APP
    close_lock();
  }
  
  //report the lock-mechanism current-sate to HomeKit
  homekit_characteristic_notify(&cha_lock_current_state, cha_lock_current_state.value);
  
}

void my_homekit_setup() {
  pinMode(PIN_LOCK, OUTPUT);
  digitalWrite(PIN_LOCK, LOW);
  cha_lock_target_state.setter = set_lock;
  arduino_homekit_setup(&config);

  
}


void my_homekit_loop() {
  arduino_homekit_loop();
  const uint32_t t = millis();
  if (t > next_heap_millis) {
    // show heap info every 30 seconds
    next_heap_millis = t + 30 * 1000;
    LOG_D("Free heap: %d, HomeKit clients: %d",
        ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
  }
}

/* use this functions to let your lock mechanism do whatever yoi want */
void open_lock(){
  Serial.println("unsecure"); 
  digitalWrite(PIN_LOCK, HIGH);
  delay(1000);
  close_lock();
}

void close_lock(){
  Serial.println("secure");
  digitalWrite(PIN_LOCK, LOW);
  cha_lock_target_state.value.int_value = 1;
  homekit_characteristic_notify(&cha_lock_target_state, cha_lock_target_state.value);
  delay(1000);
  cha_lock_current_state.value.int_value = 1;
  homekit_characteristic_notify(&cha_lock_current_state, cha_lock_current_state.value);
}
