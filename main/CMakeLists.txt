idf_component_register(
	SRCS "Main.cpp" "SH1106Display.cpp"
	INCLUDE_DIRS "."
	REQUIRES esp_driver_spi esp_driver_gpio nvs_flash
)
