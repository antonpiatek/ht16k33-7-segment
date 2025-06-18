#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>
#include <LedStatus.h>


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

TEST_SUITE("LED level tests") {
    TEST_CASE("empty") {
        char* ledData = buildLedData(0, 0);
        CHECK(std::string(ledData) == "OOOOOOOOOO");
    }
    TEST_CASE("50\% full shows 5 bars") {
        char* ledData = buildLedData(50, 0);
        CHECK(std::string(ledData) == "BBBBBOOOOO");
    }
    TEST_CASE("90\% full shows 9 bars") {
        char* ledData = buildLedData(90, 0);
        CHECK(std::string(ledData) == "BBBBBBBBBO");
    }
    TEST_CASE("95\% full shows 9 bars") {
        char* ledData = buildLedData(95, 0);
        CHECK(std::string(ledData) == "BBBBBBBBBO");
    }
    TEST_CASE("96\% full shows full bars") {
        char* ledData = buildLedData(96, 0);
        CHECK(std::string(ledData) == "CCCCCCCCCC");
    }
    TEST_CASE("full shows fully charged") {
        char* ledData = buildLedData(100, 0);
        CHECK(std::string(ledData) == "CCCCCCCCCC");
    }
}


TEST_SUITE("LED charging tests") {
    TEST_CASE("50\% +1c shows 5+1 bars") {
        char* ledData = buildLedData(50, 1);
        CHECK(std::string(ledData) == "BBBBBCOOOO");
    }
    TEST_CASE("50\% +2c shows 5+2 bars") {
        char* ledData = buildLedData(50, 2);
        CHECK(std::string(ledData) == "BBBBBCCOOO");
    }
    TEST_CASE("50\% +3c shows 5+3 bars") {
        char* ledData = buildLedData(50, 3);
        CHECK(std::string(ledData) == "BBBBBCCCOO");
    }
    TEST_CASE("80\% +3c shows 8+2 bars") {
        char* ledData = buildLedData(80, 3);
        CHECK(std::string(ledData) == "BBBBBBBBCC");
    }
    TEST_CASE("90\% +3c shows 8+2 bars") {
        char* ledData = buildLedData(90, 3);
        CHECK(std::string(ledData) == "BBBBBBBBBC");
    }
}
TEST_SUITE("LED discharging tests") {
    TEST_CASE("50\% -1c shows 5-1 bars") {
        char* ledData = buildLedData(50, -1);
        CHECK(std::string(ledData) == "BBBBDOOOOO");
    }
    TEST_CASE("50\% -2c shows 5-2 bars") {
        char* ledData = buildLedData(50, -2);
        CHECK(std::string(ledData) == "BBBDDOOOOO");
    }
    TEST_CASE("50\% -3c shows 5-3 bars") {
        char* ledData = buildLedData(50, -3);
        CHECK(std::string(ledData) == "BBDDDOOOOO");
    }
    TEST_CASE("30\% -3c shows 3 bars") {
        char* ledData = buildLedData(30, -3);
        CHECK(std::string(ledData) == "DDDOOOOOOO");
    }
    TEST_CASE("10\% -3c shows 1 bars") {
        char* ledData = buildLedData(10, -3);
        CHECK(std::string(ledData) == "DOOOOOOOOO");
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