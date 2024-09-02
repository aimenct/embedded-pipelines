#-- -- -- -- -- - test_queue -- -- -- -- -- #
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#Add the output executable for the test
add_executable(test_queue)

#Add sources specific for the test
target_sources(
  test_queue
  PRIVATE
  test_queue.cpp
  )

#Link against the library itself.
target_link_libraries(
  test_queue
  PRIVATE Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
  )

#-- -- -- -- -- - test_message -- -- -- -- -- #
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(test_message)

#Add sources specific for the test
target_sources(
  test_message
  PRIVATE
  test_message.cpp
  )

#Link against the library itself.
target_link_libraries(
  test_message
  PRIVATE Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
)

#-- -- -- -- -- - test_settings -- -- -- -- -- #
#Add the output executable for the test
add_executable(test_settings)

#Add sources specific for the test
target_sources(
  test_settings
  PRIVATE
  test_settings.cpp
  )

#Link against the library itself.
target_link_libraries(
  test_settings
  PRIVATE Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
  )

#-- -- -- -- -- - test_filter -- -- -- -- -- #
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(test_filter)

#Add sources specific for the test
target_sources(
  test_filter
  PRIVATE
  test_filter.cpp
  )

#Link against the library itself.
target_link_libraries(
  test_filter
  PRIVATE Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
  )

#-- -- -- -- -- - test_pipeline -- -- -- -- -- #
#Add the output executable for the test
add_executable(test_pipeline)

#Add sources specific for the test
target_sources(
  test_pipeline
  PRIVATE
  test_pipeline.cpp
  )

#Link against the library itself.
target_link_libraries(
  test_pipeline
  PRIVATE Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
)

#-- -- -- -- -- - test_signals -- -- -- -- -- #
# Add the output executable for the test
add_executable(test_signals)

# Add sources specific for the test
target_sources(
  test_signals
  PRIVATE
  test_signals.cpp
  )

#Link against the library itself.
target_link_libraries(
  test_signals
  PRIVATE Threads::Threads
  ${TIFF_LIBRARIES}
  PUBLIC
  ${PROJECT_NAME}
  )


#-- -- -- -- -- - test_sink -- -- -- -- -- #
#Add the output executable for the test
add_executable(test_sink)

#Add sources specific for the test
target_sources(
  test_sink
  PRIVATE
  test_sink.cpp
  )

#Link against the library itself.
target_link_libraries(
  test_sink
  PRIVATE Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
  )

#-- -- -- -- -- - test_src -- -- -- -- -- #
#Add the output executable for the test
add_executable(test_src)

#Add sources specific for the test
target_sources(
  test_src
  PRIVATE
  test_src.cpp
  )

#Link against the library itself.
target_link_libraries(
  test_src
  PRIVATE Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
)

#-- -- -- -- -- - test_display -- -- -- -- -- #
#Add your source files
add_executable(test_display)

#Add sources specific for the test
target_sources(
test_display
PRIVATE
test_display.cpp
)

#Link against the library itself.
target_link_libraries(
  test_display
  PRIVATE
  Threads::Threads
  PUBLIC
  ${PROJECT_NAME}
)


#Add targets to the global "tests" target
add_dependencies(tests test_message test_queue test_settings test_filter test_pipeline test_signals test_sink test_src test_display)

#-- -- -- -- -- - test_display -- -- -- -- -- #
#Add your source files
#add_executable(test_display)

#Find LibTIFF package
#find_package(TIFF REQUIRED)
#Include LibTIFF headers
#include_directories(${TIFF_INCLUDE_DIRS})

#Find OPENCV package
#find_package(OpenCV REQUIRED)

#Add sources specific for the test
#target_sources(
#test_display
#PRIVATE
#test_display.cpp
#../../ examples / filters / image_src.cpp
#../../ examples / filters / filter_template.cpp
#)

#Link against the library itself.
#target_link_libraries(
#test_display
#PRIVATE
#Threads::Threads
#${TIFF_LIBRARIES }
#${OpenCV_LIBS }
#PUBLIC
#${PROJECT_NAME }
#)

