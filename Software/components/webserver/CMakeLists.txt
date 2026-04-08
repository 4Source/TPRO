# Set source files and include directories
set(srcs 
        webserver.cpp
        routes.cpp)
set(include_dirs include)
set(requires 
        esp_http_server
        network)
set(embed)

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

message(STATUS "Configure webserver component: ")

# Create auto generated header for embed files 
message(STATUS "\tCreate embedded files header: ")
set(EMBED_HEADER "${COMPONENT_DIR}/include/embedded_files.hpp")
file(WRITE ${EMBED_HEADER} "#pragma once \n\n // Auto-generated file\n\n")
foreach(FILE IN LISTS embed)
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
            "const size_t ${SYMBOL}_size = (${SYMBOL}_end - ${SYMBOL}_start);\n\n"
        )
    endif()
endforeach()

# Register the component
message(STATUS "\tRegister webserver component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
message(STATUS "\t\tEmbed files: ${embed}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES  ${requires}
                       EMBED_FILES ${embed})

message(STATUS "Configure webserver component done")