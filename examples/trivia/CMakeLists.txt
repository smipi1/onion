include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(trivia trivia.c)
target_link_libraries(trivia onion)
