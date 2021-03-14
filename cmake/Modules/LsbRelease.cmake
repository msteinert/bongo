#.rst:
# LsbRelease
# ----------
#
# .. command:: lsb_release
#
# Run the lsb_release program.
function(lsb_release OPT OUT)
  find_program(LSB_RELEASE lsb_release
    CACHE INTERNAL "Path to lsb_release")
  if(LSB_RELEASE)
    execute_process(
      COMMAND ${LSB_RELEASE} ${OPT}
      OUTPUT_VARIABLE __out
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REPLACE "\"" "" __out ${__out})
    string(STRIP ${__out} __out)
  else()
    set(__out "unknown-NOTFOUND")
  endif()
  set(${OUT} ${__out} PARENT_SCOPE)
endfunction()
