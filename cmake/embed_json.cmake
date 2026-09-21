if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "embed_json.cmake requires INPUT and OUTPUT")
endif()

file(READ "${INPUT}" content)
string(HEX "${content}" content_hex)
string(
    REGEX REPLACE
    "([0-9A-Fa-f][0-9A-Fa-f])"
    "\\\\x\\1"
    content_escaped
    "${content_hex}"
)

get_filename_component(output_directory "${OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${output_directory}")
file(WRITE "${OUTPUT}"
    "// Generated from the canonical Archimedean model JSON. Do not edit.\n"
    "#include \"archview/embedded_model.hpp\"\n\n"
    "namespace archview {\n\n"
    "const char kEmbeddedModelJson[] = \"${content_escaped}\";\n\n"
    "}  // namespace archview\n"
)
