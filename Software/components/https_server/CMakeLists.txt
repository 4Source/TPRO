message(STATUS "-------------------------------------------------------")
message(STATUS "CHECKING VARIABLE NAMES FOR: Httpsserver")
message(STATUS "Variable 'CONFIG_ENABLE_HTTPS_SERVER' is: '${CONFIG_ENABLE_HTTPS_SERVER}'")
message(STATUS "-------------------------------------------------------")

# KConfig Httpsserver aktiv sonst nichts registrieren
set(srcs "")
set(requires "esp_http_server" "esp_https_server" "mbedtls" "led" "config_manager" "task_handles" "network" "apis" "file_manager")
set(embed "")
set(embed_txt "")
set(include_dirs "include")

if(CONFIG_ENABLE_HTTPS_SERVER)
    message(STATUS "Configure https_server component: ENABLED")
    
    set(srcs 
        "fileserver.cpp"
        "https_server.cpp"
        "webserver.cpp"
        "websocket_server.cpp"
        "routes.cpp"
        "restserver.cpp")

    list(APPEND requires 
        )

    # Web Dateien sammeln (Nur wenn aktiv, um Flash zu sparen)
    set(web_files 
        "www/assets/app.js"
        "www/assets/index.css"
        "www/assets/worldmap.svg"
        "www/index.html"
        "www/vite.svg")

    foreach(FILE IN LISTS web_files)
        set(FILE_PATH "${COMPONENT_DIR}/${FILE}")
        if(EXISTS "${FILE_PATH}")
            list(APPEND embed "${FILE}")
        else()
            message(FATAL_ERROR "Error: Website file ${FILE_PATH} missing! Run Preact build.")
        endif()
    endforeach()

    # Zertifikate sammeln
    set(cert_files 
        "certs/server_key.pem"
        "certs/server_cert.pem")

    foreach(FILE IN LISTS cert_files)
        set(FILE_PATH "${COMPONENT_DIR}/${FILE}")
        if(EXISTS "${FILE_PATH}")
            list(APPEND embed_txt "${FILE}")
        else()
            message(FATAL_ERROR "Error: Cert file ${FILE_PATH} missing!")
        endif()
    endforeach()

    # Header für Embedded Files generieren (In den IF-Block verschoben)
    set(EMBED_HEADER "${COMPONENT_DIR}/embedded_files.hpp")
    set(CLANG_IGNORE "readability-identifier-naming, cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays, cppcoreguidelines-interfaces-global-init")
    file(WRITE ${EMBED_HEADER} "#pragma once \n#include <cstddef>\n\n// Auto-generated\n\n// NOLINTBEGIN(${CLANG_IGNORE})\n\n")
    
    foreach(FILE IN LISTS embed embed_txt)
        get_filename_component(SYMBOL ${FILE} NAME)
        string(REPLACE "/" "_" SYMBOL ${SYMBOL})
        string(REPLACE "." "_" SYMBOL ${SYMBOL})
        file(APPEND ${EMBED_HEADER} "extern const unsigned char ${SYMBOL}_start[] asm(\"_binary_${SYMBOL}_start\");\n")
        file(APPEND ${EMBED_HEADER} "extern const unsigned char ${SYMBOL}_end[] asm(\"_binary_${SYMBOL}_end\");\n")
        file(APPEND ${EMBED_HEADER} "const size_t ${SYMBOL}_size = static_cast<size_t>(&${SYMBOL}_end[0] - &${SYMBOL}_start[0]);\n\n")
    endforeach()
    file(APPEND ${EMBED_HEADER} "// NOLINTEND(${CLANG_IGNORE})\n")

else()
    message(STATUS "Configure https_server component: DISABLED (providing headers only)")
endif()

# Register the component
message(STATUS "\tRegister https_server component with:")
message(STATUS "\t\tSource: ${srcs}")
message(STATUS "\t\tInclude: ${include_dirs}")
message(STATUS "\t\tRequires: ${requires}")
message(STATUS "\t\tEmbed files: ${embed}")
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requires}
                       EMBED_FILES ${embed}
                       EMBED_TXTFILES ${embed_txt})

message(STATUS "Configure https_server component done")