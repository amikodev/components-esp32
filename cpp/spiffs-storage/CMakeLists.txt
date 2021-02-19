set(COMPONENT_SRCS "spiffs-storage.cpp")
set(COMPONENT_ADD_INCLUDEDIRS "./include")
set(COMPONENT_PRIV_REQUIRES spiffs nvs_flash)
register_component()