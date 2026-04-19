# Set source files and include directories
set(srcs 
        webserver.cpp
        websocket_server.cpp
        routes.cpp
        restserver.cpp)
set(include_dirs include)
set(requires 
        esp_https_server 
        task_handles
        network
        led
        apis
        config_manager)
set(embed)
set(embed_txt)

# Add webpage files to embedded files
message(STATUS "Collect webserver files: ")
set(web_files 
        "www/assets/app.js"
        "www/assets/index.css"
        "www/assets/worldmap.svg"
        "www/index.html"
        "www/vite.svg")

foreach(FILE IN LISTS web_files)
    set(FILE_PATH ${COMPONENT_DIR}/${FILE})
    if(NOT EXISTS ${FILE_PATH})
        message(FATAL_ERROR "Error: Website files (${FILE_PATH}) are missing! Try running the 'Preact: build' first.")
    else()
        message(STATUS "\t- ${FILE_PATH}")
        list(APPEND embed ${FILE})
    endif()
endforeach()

# Add certificates embedded files
message(STATUS "Collect certificates files: ")
set(cert_files 
        "certs/server_key.pem"
        "certs/server_cert.pem")

foreach(FILE IN LISTS cert_files)
    set(FILE_PATH ${COMPONENT_DIR}/${FILE})
    if(NOT EXISTS ${FILE_PATH})
        message(FATAL_ERROR "Error: Certificate files (${FILE_PATH}) are missing! Try running the 'Cert: generate' first.")
    else()
        message(STATUS "\t- ${FILE_PATH}")
        list(APPEND embed_txt ${FILE})
    endif()
endforeach()

message(STATUS "Configure webserver component: ")

# Create auto generated header for embed files 
message(STATUS "\tCreate embedded files header: ")
set(EMBED_HEADER "${COMPONENT_DIR}/include/embedded_files.hpp")
set(CLANG_IGNORE "readability-identifier-naming, cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays, cppcoreguidelines-interfaces-global-init")
file(WRITE ${EMBED_HEADER} "#pragma once \n#include <cstddef>\n\n// Auto-generated file\n\n// NOLINTBEGIN(${CLANG_IGNORE})\n\n")
foreach(FILE IN LISTS embed embed_txt)
    set(FILE_PATH ${COMPONENT_DIR}/${FILE})
    if(NOT EXISTS ${FILE_PATH})
        message(FATAL_ERROR "Error: Embedded file (${FILE_PATH}) is missing!")
    else()
        # Convert path → symbol name
        get_filename_component(SYMBOL ${FILE} NAME)
        string(REPLACE "/" "_" SYMBOL ${SYMBOL})
        string(REPLACE "." "_" SYMBOL ${SYMBOL})

        message(STATUS "\t\t- ${FILE}: '${SYMBOL}_start' '${SYMBOL}_end' '${SYMBOL}_size'")

        file(APPEND ${EMBED_HEADER}
            "extern const unsigned char ${SYMBOL}_start[] asm(\"_binary_${SYMBOL}_start\");\n"
        )
        file(APPEND ${EMBED_HEADER}
            "extern const unsigned char ${SYMBOL}_end[] asm(\"_binary_${SYMBOL}_end\");\n"
        )
        file(APPEND ${EMBED_HEADER}
            "const size_t ${SYMBOL}_size = static_cast<size_t>(&${SYMBOL}_end[0] - &${SYMBOL}_start[0]);\n\n"
        )
    endif()
endforeach()
file(APPEND ${EMBED_HEADER} "// NOLINTEND(${CLANG_IGNORE})\n")

# Register the component
message(STATUS "\tRegister webserver component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
message(STATUS "\t\tEmbed files: ${embed}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES  ${requires}
                       EMBED_FILES ${embed}
                       EMBED_TXTFILES ${embed_txt})

message(STATUS "Configure webserver component done")