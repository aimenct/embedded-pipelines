// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <GL/glut.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "sensor_sdk.h"

SensorSDK sensor;
double data[100];
int counter = 0;

void generateData()
{
  auto [newCounter, newData] = sensor.readData();
  if (newData != nullptr) {
    //    std::cout << "Read " << newCounter << " data points:" << std::endl;
    for (int j = 0; j < sensor.dataSize(); ++j) {
      //  std::cout << newData[j] << " ";
      data[j] = newData[j];
    }
    counter = newCounter;
    // std::cout << std::endl;
  }
  else {
    std::cerr << "Error: Sensor is disconnected or failed to read data."
              << std::endl;
    // Attempt to reconnect if the read fails
    if (sensor.attemptReconnect()) {
      std::cout << "Sensor reconnected successfully." << std::endl;
    }
    else {
      std::cerr << "Error: Sensor reconnection failed." << std::endl;
    }
  }
}

void display()
{
  glClear(GL_COLOR_BUFFER_BIT);

  // Draw the counter as text
  std::string counter_str = "Counter: " + std::to_string(counter);
  glRasterPos2f(-0.9f, 0.9f);
  for (char c : counter_str) {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
  }

  // Draw the triangular wave
  glBegin(GL_LINE_STRIP);
  for (int i = 0; i < sensor.dataSize(); ++i) {
    float x =
        (static_cast<float>(i) / static_cast<float>(sensor.dataSize())) * 2.0f -
        1.0f;
    float y = static_cast<float>(data[i]);
    glVertex2f(x, y);
  }
  glEnd();

  glutSwapBuffers();
}

void idle()
{
  generateData();
  glutPostRedisplay();
  std::this_thread::sleep_for(
      std::chrono::milliseconds(100));  // Slow down the update rate
}

void setupOpenGL()
{
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glColor3f(0.0f, 1.0f, 0.0f);
  glPointSize(2.0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
}

void keyboard(unsigned char key, int /*x*/, int /*y*/)
{
  static float bias = 0.0f;
  static float amplitude = 1.0f;
  static float frequency = 1.0f;
  switch (key) {
    case 'b':
      bias += 0.1f;
      break;
    case 'B':
      bias -= 0.1f;
      break;
    case 'a':
      amplitude += 0.1f;
      break;
    case 'A':
      amplitude -= 0.1f;
      break;
    case 'f':
      frequency += 0.1f;
      break;
    case 'F':
      frequency -= 0.1f;
      break;
  }
  sensor.setParameter("bias", bias);
  sensor.setParameter("amplitude", amplitude);
  sensor.setParameter("frequency", frequency);
  generateData();
}

int main(int argc, char** argv)
{
  if (!sensor.connect("COM3")) {
    std::cerr << "Error: Failed to connect to the sensor." << std::endl;
    return -1;
  }

  // Initialize GLUT
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(800, 600);
  glutCreateWindow("Sensor Data Visualization");

  setupOpenGL();
  generateData();

  // Register GLUT callbacks
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  glutKeyboardFunc(keyboard);

  // Enter the main loop
  glutMainLoop();

  sensor.disconnect();

  return 0;
}
