file(GLOB FreqGenSrc "*.cpp")

add_executable(freq_gen ${FreqGenSrc})

install(FILES freq_gen DESTINATION bin PERMISSIONS WORLD_EXECUTE)
