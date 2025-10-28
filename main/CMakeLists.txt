idf_component_register(SRCS "main.c" "softap_config.c" "softap_webserver.c"
                    PRIV_REQUIRES esp_wifi nvs_flash esp_http_server espressif__cJSON wpa_supplicant
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
    
    # Also read DEFAULT_WPS_DEVICE_NAME
    string(REGEX MATCH "DEFAULT_WPS_DEVICE_NAME[ \t]*=[ \t]*\"([^\"]+)\"" _wps_match ${ENV_CONTENTS})
    if(_wps_match)
        string(REGEX REPLACE ".*DEFAULT_WPS_DEVICE_NAME[ \t]*=[ \t]*\"([^\"]+)\".*" "\\1" DEFAULT_WPS_DEVICE_NAME ${ENV_CONTENTS})
    else()
        set(DEFAULT_WPS_DEVICE_NAME "ESP32-XXXX")
    endif()
    
    # Also read DEFAULT_WPS_ENABLED
    string(REGEX MATCH "DEFAULT_WPS_ENABLED[ \t]*=[ \t]*\"([^\"]+)\"" _wps_enabled_match ${ENV_CONTENTS})
    if(_wps_enabled_match)
        string(REGEX REPLACE ".*DEFAULT_WPS_ENABLED[ \t]*=[ \t]*\"([^\"]+)\".*" "\\1" DEFAULT_WPS_ENABLED ${ENV_CONTENTS})
    else()
        set(DEFAULT_WPS_ENABLED "true")
    endif()
    
    # Also read DEFAULT_APNAME_IE_ENABLED
    string(REGEX MATCH "DEFAULT_APNAME_IE_ENABLED[ \t]*=[ \t]*\"([^\"]+)\"" _apname_ie_match ${ENV_CONTENTS})
    if(_apname_ie_match)
        string(REGEX REPLACE ".*DEFAULT_APNAME_IE_ENABLED[ \t]*=[ \t]*\"([^\"]+)\".*" "\\1" DEFAULT_APNAME_IE_ENABLED ${ENV_CONTENTS})
    else()
        set(DEFAULT_APNAME_IE_ENABLED "true")
    endif()
else()
    set(DEFAULT_PREFIX "ESP32-XXXX")
    set(DEFAULT_AUTH_MODE "WPA2_PSK")
    set(DEFAULT_COUNTRY_CODE "US")
    set(DEFAULT_WPS_DEVICE_NAME "ESP32-XXXX")
    set(DEFAULT_WPS_ENABLED "true")
    set(DEFAULT_APNAME_IE_ENABLED "true")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/softap_env.h.in ${GENERATED_HEADER} @ONLY)
target_include_directories(${COMPONENT_LIB} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
