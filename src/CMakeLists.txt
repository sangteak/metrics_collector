# create .exe
add_executable(my_exe
	main.cpp
	mmf.cpp
)

# create static (.a, .lib)
#add_library(my_lib	STATIC
#	main.cpp
#	mmf.cpp
#)	

# create shared (.so, .dll)
#add_library(my_lib	SHARED
#	main.cpp
#	mmf.cpp
#)	

target_include_directories(my_exe
PUBLIC
	${CMAKE_SOURCE_DIR}/include
)

if(MSVC)	# Microsoft Visual C++ Compiler
	target_compile_options(my_exe
	PUBLIC
		/std:c++latest	/W4	# MSVC 가 식별 가능한 옵션을 지정
	)	
endif()

if(MSVC)	# Microsoft Visual C++ Compiler
	target_compile_definitions(my_exe
	PRIVATE
		WIN32_LEAN_AND_MEAN
		NOMINMAX
		_CRT_SECURE_NO_WARNINGS
	)	
endif()