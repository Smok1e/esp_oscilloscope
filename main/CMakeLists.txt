idf_component_register(
	SRCS
		"Main.cpp"
		"Peripherals/SH1106Display.cpp"
		"Peripherals/INA226.cpp"
		"Peripherals/RotaryEncoder.cpp"
		"Selector.cpp"
		"Render.cpp"
		
	INCLUDE_DIRS
		"."
		
	REQUIRES
		driver
		nvs_flash
		
	EMBED_FILES
		"font.bin"
)
