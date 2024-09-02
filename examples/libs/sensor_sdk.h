// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef SENSOR_SDK_H
#define SENSOR_SDK_H

#include <map>
#include <string>
#include <utility>
#include <vector>

class SensorSDK {
  public:
    SensorSDK();
    ~SensorSDK();

    bool connect(const std::string& port);
    bool disconnect();
    bool isConnected() const;

    bool setParameter(const std::string& parameter, double value);
    bool getParameter(const std::string& parameter, double &value);

    std::pair<int, double*> readData();
    int readDataInplace(int& counter, double& mesurement);

    int dataSize();
    bool attemptReconnect();

    int increaseFrequency();
    int decreaseFrequency();
    int reset();

  private:
    bool connected = false;
    std::string port;
    int counter = 0;
    double bias = 0.0;
    double amplitude = 1.0;
    double frequency = 1.0;
    std::map<std::string, double> configurations;
    double data[100];

    int data_size;
    void simulateData(double* data);
    double triangularWave(double t, double frequency, double amplitude,
                          double bias);
};

#endif  // SENSOR_SDK_H
