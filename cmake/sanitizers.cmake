# TODO: wrap each of these sanitizer names around a check to see if a corresponding option is set, ie ENABLE_SANITIZER_ADDRESS for 'address' sanitizer. this lets the user select which sanitizers to add at configure time. (note: i wrote it in this verbose way to make it easier to add those checks later, instead of just writing the santizer flags with this short list of known flags)
# NOTE: we can either enable "-fsanitize=thread" xor these sanitizers
set(SANITIZERS "")
list(APPEND SANITIZERS "address")
list(APPEND SANITIZERS "undefined")
list(APPEND SANITIZERS "pointer-compare")
list(APPEND SANITIZERS "pointer-subtract")
list(JOIN SANITIZERS "," SANITIZERS_LIST)
if(SANITIZERS_LIST)
    set(SANITIZER_FLAGS -fsanitize=${SANITIZERS_LIST})
endif()

# NOTE: idk if msvc supports these sanitizers...

add_library(sanitizers INTERFACE)

if(ENABLE_SANITIZERS)
    target_compile_options(sanitizers
        INTERFACE
            "$<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:${SANITIZER_FLAGS}>"
    )
    target_link_options(sanitizers
        INTERFACE
            "$<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:${SANITIZER_FLAGS}>"
    )
endif()
