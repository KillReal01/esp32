idf_component_register(
    SRCS "LittleFs.c"
    INCLUDE_DIRS "."
    REQUIRES  log
)

littlefs_create_partition_image(storage ../../data FLASH_IN_PROJECT)
