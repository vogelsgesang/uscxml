############################################################
# Actual files of core library
############################################################

file(GLOB_RECURSE USCXML_SERVER
	server/*.cpp
	server/*.h
)
source_group("Interpreter" FILES ${USCXML_SERVER})
list (APPEND USCXML_FILES ${USCXML_SERVER})

file(GLOB_RECURSE USCXML_UTIL
  util/*.cpp
  util/*.hpp
  util/*.c
	util/*.h
)
source_group("Interpreter" FILES ${USCXML_UTIL})
list (APPEND USCXML_FILES ${USCXML_UTIL})

file(GLOB_RECURSE USCXML_CONCURRENCY
	concurrency/*.cpp
	concurrency/*.h
)
source_group("Interpreter" FILES ${USCXML_CONCURRENCY})
list (APPEND USCXML_FILES ${USCXML_CONCURRENCY})

file(GLOB_RECURSE USCXML_MESSAGES
	messages/*.cpp
	messages/*.h
)
source_group("Interpreter" FILES ${USCXML_MESSAGES})
list (APPEND USCXML_FILES ${USCXML_MESSAGES})

if (NOT BUILD_MINIMAL)
	file(GLOB_RECURSE USCXML_DEBUG
		debug/*.cpp
		debug/*.h
	)
	source_group("Interpreter" FILES ${USCXML_DEBUG})
	list (APPEND USCXML_FILES ${USCXML_DEBUG})

	file(GLOB_RECURSE USCXML_TRANSFORM
		transform/*.cpp
		transform/*.h
	)
	source_group("Interpreter" FILES ${USCXML_TRANSFORM})
	list (APPEND USCXML_TRANSFORM_FILES ${USCXML_TRANSFORM})
endif()

file(GLOB_RECURSE USCXML_INTERPRETERS
	interpreter/*.cpp
	interpreter/*.h
)
source_group("Interpreter" FILES ${USCXML_INTERPRETERS})
list (APPEND USCXML_FILES ${USCXML_INTERPRETERS})

if(APPLE OR IOS)
	file(GLOB USCXML_APPLE
		*.mm
	)
	source_group("Interpreter" FILES ${USCXML_APPLE})
	list (APPEND USCXML_FILES ${USCXML_APPLE})
endif()

file(GLOB USCXML_CORE
  ${CMAKE_SOURCE_DIR}/contrib/src/jsmn/jsmn.c
  ${CMAKE_SOURCE_DIR}/contrib/src/evws/*.c
	*.cpp
	*.h
)
source_group("Interpreter" FILES ${USCXML_CORE})
list (APPEND USCXML_FILES ${USCXML_CORE})

file(GLOB USCXML_PLUGINS
	plugins/DataModel.h
	plugins/EventHandler.h
	plugins/ExecutableContent.h
	plugins/Invoker.h
	plugins/IOProcessor.h
)
source_group("Interpreter" FILES ${USCXML_PLUGINS})
list (APPEND USCXML_FILES ${USCXML_PLUGINS})

if (BUILD_AS_PLUGINS)
	list (APPEND USCXML_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/plugins)
	file(GLOB PLUMA
		plugins/Pluma/*.cpp
		plugins/Pluma/*.h
		plugins/*.cpp 
		plugins/*.h
	)
	source_group("Interpreter" FILES ${PLUMA})
	list (APPEND USCXML_FILES ${PLUMA})
	add_definitions("-DBUILD_AS_PLUGINS")
endif()

add_subdirectory(plugins/datamodel)
add_subdirectory(plugins/element)
add_subdirectory(plugins/ioprocessor)
add_subdirectory(plugins/invoker)

SET(USCXML_INVOKERS ${USCXML_INVOKERS} PARENT_SCOPE)
SET(USCXML_IOPROCESSORS ${USCXML_IOPROCESSORS} PARENT_SCOPE)
SET(USCXML_ELEMENTS ${USCXML_ELEMENTS} PARENT_SCOPE)
SET(USCXML_DATAMODELS ${USCXML_DATAMODELS} PARENT_SCOPE)
SET(USCXML_LANGUAGE_BINDINGS ${USCXML_LANGUAGE_BINDINGS} PARENT_SCOPE)

set(USCXML_INCLUDE_DIRS ${USCXML_INCLUDE_DIRS} PARENT_SCOPE)
set(USCXML_OPT_LIBS ${USCXML_OPT_LIBS} PARENT_SCOPE)
set(USCXML_FILES ${USCXML_FILES} PARENT_SCOPE)
set(USCXML_TRANSFORM_FILES ${USCXML_TRANSFORM_FILES} PARENT_SCOPE)
SET(PLUMA ${PLUMA} PARENT_SCOPE)