idf_component_register(SRCS "main.c" "softap_config.c" "softap_webserver.c"
                    PRIV_REQUIRES esp_wifi nvs_flash esp_http_server espressif__cJSON
                    INCLUDE_DIRS ".")

# Generate a header from .env (if present) to expose DEFAULT_SSID_PREFIX_TEMPLATE
set(ENV_FILE ${CMAKE_SOURCE_DIR}/.env)
set(GENERATED_HEADER ${CMAKE_CURRENT_BINARY_DIR}/softap_env.h)
if(EXISTS ${ENV_FILE})
    file(READ ${ENV_FILE} ENV_CONTENTS)
    string(REGEX MATCH "DEFAULT_SSID_PREFIX_TEMPLATE[ \t]*=[ \t]*\"([^\"]+)\"" _match ${ENV_CONTENTS})
    if(_match)
        string(REGEX REPLACE ".*DEFAULT_SSID_PREFIX_TEMPLATE[ \t]*=[ \t]*\"([^\"]+)\".*" "\\1" DEFAULT_PREFIX ${ENV_CONTENTS})
    else()
        set(DEFAULT_PREFIX "ESP32-XXXX")
    endif()
    
    # Also read DEFAULT_AUTH_MODE
    string(REGEX MATCH "DEFAULT_AUTH_MODE[ \t]*=[ \t]*\"([^\"]+)\"" _auth_match ${ENV_CONTENTS})
    if(_auth_match)
        string(REGEX REPLACE ".*DEFAULT_AUTH_MODE[ \t]*=[ \t]*\"([^\"]+)\".*" "\\1" DEFAULT_AUTH_MODE ${ENV_CONTENTS})
    else()
        set(DEFAULT_AUTH_MODE "WPA2_PSK")
    endif()
    
    # Also read DEFAULT_COUNTRY_CODE
    string(REGEX MATCH "DEFAULT_COUNTRY_CODE[ \t]*=[ \t]*\"([^\"]+)\"" _country_match ${ENV_CONTENTS})
    if(_country_match)
        string(REGEX REPLACE ".*DEFAULT_COUNTRY_CODE[ \t]*=[ \t]*\"([^\"]+)\".*" "\\1" DEFAULT_COUNTRY_CODE ${ENV_CONTENTS})
    else()
        set(DEFAULT_COUNTRY_CODE "US")
    endif()
else()
    set(DEFAULT_PREFIX "ESP32-XXXX")
    set(DEFAULT_AUTH_MODE "WPA2_PSK")
    set(DEFAULT_COUNTRY_CODE "US")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/softap_env.h.in ${GENERATED_HEADER} @ONLY)
target_include_directories(${COMPONENT_LIB} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
