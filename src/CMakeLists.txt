# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

##PRIV_REQUIRES mqtt esp_partition nvs_flash esp_netif esp_tls esp_wifi esp_event esp_system lwip

idf_component_register(SRCS ${app_sources} 
    INCLUDE_DIRS "."
    EMBED_TXTFILES "certs/AmazonRootCA1.pem" "certs/Device_certificate.crt" "certs/Private_key.key"
)


