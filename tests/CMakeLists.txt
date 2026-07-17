# 解析器单元测试：只链接纯解析函数，不碰网络/钥匙串

add_executable(test_parsers
    test_parsers.cpp
    ${CMAKE_SOURCE_DIR}/src/providers/GlmParser.cpp
    ${CMAKE_SOURCE_DIR}/src/providers/DeepSeekParser.cpp
    ${CMAKE_SOURCE_DIR}/src/providers/MoonshotParser.cpp
    ${CMAKE_SOURCE_DIR}/src/providers/OpenAiCostsParser.cpp
    ${CMAKE_SOURCE_DIR}/src/providers/CustomJsonParser.cpp
)

target_include_directories(test_parsers PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test_parsers PRIVATE
    Qt6::Core
    Qt6::Test
)

# 夹具复制到测试二进制旁，运行期按 applicationDirPath 定位
file(COPY fixtures DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME parsers COMMAND test_parsers)
