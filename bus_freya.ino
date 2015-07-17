// Copyright (c) 2014-2015 by Wayne C. Gramlich.  All rights reserved.
//
// This code drives the Freya robot.

#include <Bus_Slave.h>
#include <Frame_Buffer.h>
#include <bus_server.h>
#include <RAB_Sonar.h>
#include <Sonar.h>

#define TEST TEST_RAB_FREYA

#define BUS_STANDBY       3

class Freya_Motor_Encoder : Bus_Motor_Encoder {
 public:
  Freya_Motor_Encoder(Bus_Slave *bus_slave, UShort address, 
   UByte encoder_get_command, UByte encoder_set_command,
   UByte pwm_set_command, Logical pwm_invert, Logical encoder_invert);
  Integer encoder_get();
  void encoder_set(Integer encoder);
  void pwm_set(Byte pwm);
 private:
  UShort address_;
  Bus_Slave *bus_slave_;
  UByte encoder_get_command_;
  Logical encoder_invert_;
  UByte encoder_set_command_;
  Logical pwm_invert_;
  UByte pwm_set_command_;
};

Freya_Motor_Encoder::Freya_Motor_Encoder(Bus_Slave *bus_slave, UShort address,
 UByte encoder_get_command, UByte encoder_set_command, UByte pwm_set_command,
 Logical pwm_invert, Logical encoder_invert) {
  address_ = address;
  bus_slave_ = bus_slave;
  encoder_get_command_ = encoder_get_command;
  encoder_invert_ = encoder_invert;
  encoder_set_command_ = encoder_set_command;
  pwm_invert_ = pwm_invert;
  pwm_set_command_ = pwm_set_command;
}

void Freya_Motor_Encoder::pwm_set(Byte pwm) {
  if (pwm_invert_) {
    pwm = -pwm;
  }
  bus_slave_->command_byte_put(address_, pwm_set_command_, pwm);
  bus_slave_->flush();
}

Integer Freya_Motor_Encoder::encoder_get() {
  Integer encoder_value = 
   bus_slave_->command_integer_get(address_, encoder_get_command_);
  if (encoder_invert_) {
    encoder_value = -encoder_value;
  }
  return encoder_value;
}

void Freya_Motor_Encoder::encoder_set(Integer encoder_value) {
  if (encoder_invert_) {
    encoder_value = -encoder_value;
  }
  bus_slave_->command_integer_put(
   address_, encoder_set_command_, encoder_value);
  bus_slave_->flush();
}

class Freya_RAB_Sonar : RAB_Sonar {
 public:
  Freya_RAB_Sonar(UART *debug_uart, Bus_Slave *bus_slave);
  virtual UShort ping_get(UByte sonar_index);
  virtual UShort debug_flags_get();
  virtual void debug_flags_set(UShort debug_flags);
  virtual UByte sonars_count_get();
 private:
  Bus_Slave *bus_slave_;
  UByte debug_flags_;
};

Freya_RAB_Sonar::Freya_RAB_Sonar(UART *debug_uart, Bus_Slave *bus_slave) :
 RAB_Sonar(debug_uart) {
  bus_slave_ = bus_slave;
  debug_flags_ = 0;
}

UShort Freya_RAB_Sonar::ping_get(UByte sonar) {
  static const UByte front_address = 41;
  static const UByte rear_address = 40;

  // Figure out whether to use the front or read sonar modules:
  UByte half_sonars_count = sonars_count_get() >> 1;
  UByte address = front_address;
  if (sonar >= half_sonars_count) {
    sonar -= half_sonars_count;
    address = rear_address;
  }

  // Select the appropriate *sonar* for the appropriate module:
  bus_slave_->command_ubyte_put(address, 9, sonar);

  // Read the distance back:
  //UByte select = bus_slave_->command_ubyte_get(address, 8);
  //debug_uart_->string_print((Text)" [");
  //debug_uart_->integer_print((Integer)select);
  //debug_uart_->string_print((Text)"]:");
  UShort distance = bus_slave_->command_ushort_get(address, 10);
  //debug_uart_->integer_print((Integer)distance);
  return (Short)distance;
}

UShort Freya_RAB_Sonar::debug_flags_get() {
  return debug_flags_;
}

void Freya_RAB_Sonar::debug_flags_set(UShort debug_flags) {
  debug_flags_ = debug_flags;
}

UByte Freya_RAB_Sonar::sonars_count_get() {
  return 20;
}

// The *setup* routine runs on power up and when you press reset:

// Define the UART's:
NULL_UART null_uart;
AVR_UART *bus_uart = &avr_uart1;
AVR_UART *debug_uart = &avr_uart0;
AVR_UART *host_uart = &avr_uart0;

// These are debugging routines that eventually need to be removed:
int system_debug_flags_get() {
  return 0;
}

int usonar_getLastDistInMm(int a) {
  return a;
}

int usonar_inlineReadMeters(int a) {
  return a;
}

void system_debug_flags_set(int flags) {
}

Bus_Slave bus_slave((UART *)bus_uart, (UART *)host_uart);
static const UShort address = 33;
Freya_Motor_Encoder left_motor_encoder(
 &bus_slave, address, 2, 3, 9, (Logical)0, (Logical)0);
Freya_Motor_Encoder right_motor_encoder(
 &bus_slave, address, 4, 5, 11, (Logical)0, (Logical)0);
Freya_RAB_Sonar freya_rab_sonar(debug_uart, &bus_slave);

Bridge bridge(&avr_uart0, &avr_uart1, &avr_uart0, &bus_slave,
 (Bus_Motor_Encoder *)&left_motor_encoder,
 (Bus_Motor_Encoder *)&right_motor_encoder,
 (RAB_Sonar *)&freya_rab_sonar);

void setup() {
  pinMode(BUS_STANDBY, OUTPUT);
  digitalWrite(BUS_STANDBY, LOW);
  bridge.setup(TEST);
}

void loop () {
  bridge.loop(TEST);
}
