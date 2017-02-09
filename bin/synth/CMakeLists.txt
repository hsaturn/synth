file(GLOB SynthSrc "*.cpp")

add_executable(synth ${SynthSrc})
target_link_libraries(synth LINK_PUBLIC synthetizer)

add_custom_target (
	test_synth
	DEPENDS synt
	COMMAND mkdir -p CartPoleCfg_d
	COMMAND ./synth 1000 sinus 440
	)
