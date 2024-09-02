// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "sensor_sdk.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

SensorSDK::SensorSDK()
    : connected(false),
      counter(0),
      bias(0.0),
      amplitude(1.0),
      frequency(1.0),
      data_size(100)
{
}

SensorSDK::~SensorSDK()
{
  if (connected) {
    disconnect();
  }
}

bool SensorSDK::connect(const std::string& port)
{
  if (!connected) {
    this->port = port;
    connected = true;
    return true;
  }
  return false;
}

bool SensorSDK::disconnect()
{
  if (connected) {
    connected = false;
    return true;
  }
  return false;
}

bool SensorSDK::isConnected() const
{
  return connected;
}

bool SensorSDK::setParameter(const std::string& parameter, double value)
{
  configurations[parameter] = value;
  if (parameter == "Bias") {
    bias = value;
  }
  else if (parameter == "Amplitude") {
    amplitude = value;
  }
  else if (parameter == "Frequency") {
    frequency = value;
  }
  return true;
}

bool SensorSDK::getParameter(const std::string& parameter, double &value)
{
  configurations[parameter] = value;

  printf("parameter %s\n",parameter.c_str());
  
  if (parameter == "Bias") {
    value = bias;
  }
  else if (parameter == "Amplitude") {
    value = amplitude;
  }
  else if (parameter == "Frequency") {
    value = frequency;
  }
  return true;
}

std::pair<int, double*> SensorSDK::readData()
{
  if (!connected) {
    std::cerr << "Error: Sensor is disconnected." << std::endl;
    return {-1, nullptr};
  }
  simulateData(data);
  return {counter, data};
}

int SensorSDK::dataSize()
{
  return data_size;
}

void SensorSDK::simulateData(double* data)
{
  double t = counter / 100.0;
  for (int i = 0; i < data_size; ++i) {
    data[i] = triangularWave(t + i / 100.0, frequency, amplitude, bias);
  }
  ++counter;
}

double SensorSDK::triangularWave(double t, double frequency, double amplitude,
                                 double bias)
{
  double period = 1.0 / frequency;
  double phase = fmod(t, period) / period;
  double value = 0.0;

  if (phase < 0.25) {
    value = 4 * phase;
  }
  else if (phase < 0.75) {
    value = 2.0 - 4 * phase;
  }
  else {
    value = -4 + 4 * phase;
  }

  return amplitude * value + bias;
}

bool SensorSDK::attemptReconnect()
{
  if (!connected) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return connect(port);
  }
  return false;
}

int SensorSDK::increaseFrequency()
{
  frequency += 10;
  return 0;
}
int SensorSDK::decreaseFrequency()
{
  frequency -= 10;
  return 0;
}
int SensorSDK::reset()
{
  counter = 0;
  bias = 0.0;
  amplitude = 1.0;
  frequency = 1.0;
  return 0;
}
