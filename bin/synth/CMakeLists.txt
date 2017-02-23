file(GLOB SynthSrc "*.cpp")

add_executable(synth ${SynthSrc})
target_link_libraries(synth LINK_PUBLIC synthetizer)

install(FILES synth DESTINATION bin PERMISSIONS WORLD_EXECUTE)

add_custom_target (
	test_synth
	DEPENDS synth
	COMMAND ./synth 1000 sinus 440
	)
