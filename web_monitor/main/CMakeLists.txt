idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES net)
                      
littlefs_create_partition_image(storage ../data FLASH_IN_PROJECT)
