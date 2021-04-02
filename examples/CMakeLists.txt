function(add_example NAME)
  add_executable(${NAME}-example
    ${NAME}-example.cpp)
  target_compile_options(${NAME}-example
    PUBLIC
      -Wall
      -Wextra)
  target_link_libraries(${NAME}-example
    PUBLIC
      bongo)
endfunction()

add_example(channel)
add_example(select)
add_example(cancel_context)
add_example(timeout_context)
add_example(deadline_context)
add_example(value_context)
add_example(wait_group)
add_example(timer)
add_example(timer_reset)
