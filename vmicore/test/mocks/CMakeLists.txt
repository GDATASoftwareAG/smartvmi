add_executable(mock-aggregator MockAggregator.cpp)
target_link_libraries(mock-aggregator GTest::gmock GTest::gmock_main vmicore-public-test-headers)
