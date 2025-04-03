idf_component_register(
	SRCS
		"Main.cpp"
		"Peripherals/SH1106Display.cpp"
		"Peripherals/INA226.cpp"
		"Peripherals/RotaryEncoder.cpp"
		"Selector.cpp"
		"Render.cpp"
		"Font.cpp"
		
	INCLUDE_DIRS
		"."
		
	REQUIRES
		esp_driver_spi
		esp_driver_i2c
		esp_driver_gpio
		esp_timer
		nvs_flash
		esp_adc
		
	EMBED_FILES
		"font.bin"
)
