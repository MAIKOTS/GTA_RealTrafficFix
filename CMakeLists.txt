cmake_minimum_required(VERSION 3.10.2)
project(RealTrafficFix)

# Define o padrão C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define o nome do módulo dependendo da arquitetura (equivalente ao ifeq do .mk)
if(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(LOCAL_MODULE_NAME RealTrafficFix)
else()
    set(LOCAL_MODULE_NAME RealTrafficFix64)
endif()

# Cria a biblioteca compartilhada (.so)
add_library(${LOCAL_MODULE_NAME} SHARED
    main.cpp
    mod/logger.cpp
    mod/config.cpp
)

# Diretórios de include (equivalente ao LOCAL_C_INCLUDES)
target_include_directories(${LOCAL_MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Definições de compilação (-DNDEBUG)
target_compile_definitions(${LOCAL_MODULE_NAME} PRIVATE
    NDEBUG
)

# Flags de otimização e compilação
target_compile_options(${LOCAL_MODULE_NAME} PRIVATE
    -O2
    $<$<STREQUAL:${ANDROID_ABI},armeabi-v7a>:-mfloat-abi=softfp>
)

# Bibliotecas do sistema vinculadas (equivalente ao -llog)
target_link_libraries(${LOCAL_MODULE_NAME} PRIVATE
    log
)
