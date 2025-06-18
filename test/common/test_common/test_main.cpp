#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>


int factorial(int number) { return number <= 1 ? number : factorial(number - 1) * number; }

TEST_CASE("testing the factorial function") {
    //CHECK(factorial(0) == 1);
    CHECK(factorial(1) == 1);
    CHECK(factorial(2) == 2);
    CHECK(factorial(3) == 6);
    CHECK(factorial(10) == 3628800);
}


TEST_SUITE("some TS" * doctest::description("all tests will have this")) {
    TEST_CASE("has a description from the surrounding test suite") {
        CHECK(1==1);
    }
}
TEST_SUITE("some TS") {
    TEST_CASE("no description even though in the same test suite as the one above") {
        // asserts
    }
}
// TEST_CASE ...
// TEST_SUITE ...


int main(int argc, char **argv)
{
  doctest::Context context;
  //context.setOption("success", true);     // Report successful tests
  context.setOption("no-exitcode", true); // Do not return non-zero code on failed test case
  context.applyCommandLine(argc, argv);
  return context.run();
}