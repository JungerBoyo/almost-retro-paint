#include <catch2/catch.hpp>
#include <hello_world/hello_world.hpp>

TEST_CASE("test of test") {
  CHECK_NOTHROW(template_project::helloWorld());
}