#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>
#include <LedStatus.h>
#include <Settings.h>
#include <string>
#include <ArduinoJson.h>

using std::string;

TEST_SUITE("LED level tests") {
    TEST_CASE("empty") {
        char* ledData = buildLedData(0, 0);
        CHECK(string(ledData) == "OOOOOOOOOO");
    }
    TEST_CASE("50\% full shows 5 bars") {
        char* ledData = buildLedData(50, 0);
        CHECK(string(ledData) == "BBBBBOOOOO");
    }
    TEST_CASE("90\% full shows 9 bars") {
        char* ledData = buildLedData(90, 0);
        CHECK(string(ledData) == "BBBBBBBBBO");
    }
    TEST_CASE("95\% full shows 9 bars") {
        char* ledData = buildLedData(95, 0);
        CHECK(string(ledData) == "BBBBBBBBBO");
    }
    TEST_CASE("96\% full shows full bars") {
        char* ledData = buildLedData(96, 0);
        CHECK(string(ledData) == "CCCCCCCCCC");
    }
    TEST_CASE("full shows fully charged") {
        char* ledData = buildLedData(100, 0);
        CHECK(string(ledData) == "CCCCCCCCCC");
    }
}


TEST_SUITE("LED charging tests") {
    TEST_CASE("50\% +1c shows 5+1 bars") {
        char* ledData = buildLedData(50, 1);
        CHECK(string(ledData) == "BBBBBCOOOO");
    }
    TEST_CASE("50\% +2c shows 5+2 bars") {
        char* ledData = buildLedData(50, 2);
        CHECK(string(ledData) == "BBBBBCCOOO");
    }
    TEST_CASE("50\% +3c shows 5+3 bars") {
        char* ledData = buildLedData(50, 3);
        CHECK(string(ledData) == "BBBBBCCCOO");
    }
    TEST_CASE("80\% +3c shows 8+2 bars") {
        char* ledData = buildLedData(80, 3);
        CHECK(string(ledData) == "BBBBBBBBCC");
    }
    TEST_CASE("90\% +3c shows 8+2 bars") {
        char* ledData = buildLedData(90, 3);
        CHECK(string(ledData) == "BBBBBBBBBC");
    }
}

TEST_SUITE("LED discharging tests") {
    TEST_CASE("50\% -1c shows 5-1 bars") {
        char* ledData = buildLedData(50, -1);
        CHECK(string(ledData) == "BBBBDOOOOO");
    }
    TEST_CASE("50\% -2c shows 5-2 bars") {
        char* ledData = buildLedData(50, -2);
        CHECK(string(ledData) == "BBBDDOOOOO");
    }
    TEST_CASE("50\% -3c shows 5-3 bars") {
        char* ledData = buildLedData(50, -3);
        CHECK(string(ledData) == "BBDDDOOOOO");
    }
    TEST_CASE("30\% -3c shows 3 bars") {
        char* ledData = buildLedData(30, -3);
        CHECK(string(ledData) == "DDDOOOOOOO");
    }
    TEST_CASE("10\% -3c shows 1 bars") {
        char* ledData = buildLedData(10, -3);
        CHECK(string(ledData) == "DOOOOOOOOO");
    }
    
}

TEST_SUITE("Settings tests") {
    TEST_CASE("print settings") {
        auto str = printSettings();
        INFO(str);
        CHECK(str.find("seg_brightness") != -1);
        CHECK(str.find("seg_brightness\":15") != -1);
    }

    TEST_CASE("print settings2") {
        auto str = printSettings();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, str);
        CHECK(settings.seg_brightness == 15);
        CHECK((int)doc["seg_brightness"] == 15);
    }

    TEST_CASE("update settings adds timestamp") {
        int old = settings.time;
        loadSettings("{}");
        CHECK(settings.time !=  old);
        old = settings.time;
        loadSettings("{}");
        CHECK(settings.time !=  old);
    }

    TEST_CASE("update time is ignored") {
        loadSettings("{\"time\":1}");
        CHECK(settings.time != 1);
    }

    TEST_CASE("update seg_brightness") {
        settings.seg_brightness=1;
        loadSettings("{\"seg_brightness\":6}");
        CHECK(settings.seg_brightness == 6);
        
    }

    TEST_CASE("no update if time matches") {
        settings.time = 123;
        settings.seg_brightness = 2;
        loadSettings("{\"time\":123,\"seg_brightness\":6}");
        CHECK(settings.time == 123);
        CHECK(settings.seg_brightness == 2);
    }

    TEST_CASE("update no or bad value uses default") {
        settings.seg_brightness == 10;
        loadSettings("{}");
        CHECK(settings.seg_brightness == 15);
        
        settings.seg_brightness == 10;
        loadSettings("{\"seg_brightness\":\"7\"}");
        CHECK(settings.seg_brightness == 15);
        
        settings.seg_brightness == 10;
        loadSettings("{\"seg_brightness\":\"a\"}");
        CHECK(settings.seg_brightness == 15);
    }

    TEST_CASE("update battery_color rgb") {
        settings.battery_color = RGB{0,0,0};
        loadSettings("{\"battery_color\": [1,2,3]}");
        CHECK(settings.battery_color.r == 1);
        CHECK(settings.battery_color.g == 2);
        CHECK(settings.battery_color.b == 3);
    }

    TEST_CASE("update battery_charge_color rgb") {
        settings.battery_charge_color = RGB{0,0,0};
        loadSettings("{\"battery_charge_color\": [1,2,3]}");
        CHECK(settings.battery_charge_color.r == 1);
        CHECK(settings.battery_charge_color.g == 2);
        CHECK(settings.battery_charge_color.b == 3);
    }

    TEST_CASE("update battery_discharge_color rgb") {
        settings.battery_discharge_color = RGB{0,0,0};
        loadSettings("{\"battery_discharge_color\": [1,2,3]}");
        CHECK(settings.battery_discharge_color.r == 1);
        CHECK(settings.battery_discharge_color.g == 2);
        CHECK(settings.battery_discharge_color.b == 3);
    }
}


int main(int argc, char **argv)
{
  doctest::Context context;
  //context.setOption("success", true);     // Report successful tests
  context.setOption("no-exitcode", true); // Do not return non-zero code on failed test case
  context.applyCommandLine(argc, argv);
  return context.run();
}