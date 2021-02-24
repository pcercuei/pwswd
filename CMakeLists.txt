cmake_minimum_required(VERSION 3.4)
project(pwswd LANGUAGES C VERSION 1.0)

set(BACKENDS "volume;brightness;poweroff;reboot;screenshot;tvout;suspend;kill;ratiomode" CACHE STRING "Semicolon-separated list of enabled backends")
option(REVERSED_X_Y "Inverted X/Y buttons" OFF)

set(SRCS event_listener.c main.c shortcut_handler.c)

foreach (BACKEND ${BACKENDS})
	list(APPEND SRCS backend/${BACKEND}/${BACKEND}.c)
endforeach()

add_executable(${PROJECT_NAME} ${SRCS})

if (volume IN_LIST BACKENDS)
	find_package(ALSA REQUIRED)
	target_link_libraries(${PROJECT_NAME} LINK_PRIVATE ${ALSA_LIBRARY})
	target_include_directories(${PROJECT_NAME} PRIVATE EXTERNAL ${ALSA_INCLUDE_DIR})

	set(MIXER_CONTROL "PCM" CACHE STRING "Default mixer control" FORCE)
endif()

find_library(PTHREAD_LIBRARIES pthread REQUIRED)
find_library(LIBINI_LIBRARIES ini REQUIRED)
find_path(LIBINI_INCLUDE_DIR ini.h REQUIRED)

target_link_libraries(${PROJECT_NAME} LINK_PRIVATE ${LIBINI_LIBRARIES} ${PTHREAD_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE EXTERNAL ${LIBINI_INCLUDE_DIR})

foreach (BACKEND ${BACKENDS})
	target_compile_definitions(${PROJECT_NAME} PRIVATE BACKEND_$<UPPER_CASE:${BACKEND}>)
endforeach()

target_compile_definitions(${PROJECT_NAME} PRIVATE
	$<IF:$<IN_LIST:volume,${BACKENDS}>,DEFAULT_MIXER="${MIXER_CONTROL}",>
	$<IF:$<BOOL:${REVERSED_X_Y}>,REVERSED_X_Y,>
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})
