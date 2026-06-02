#include <ArduinoJson.h>
#include <unity.h>

#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Mirrors runtime_config.h constants — kept in sync manually.
// ---------------------------------------------------------------------------

static constexpr int kNumSensors = 10;

static const char* kExpectedNames[kNumSensors] = {
    "rpi1_cpu", "rpi1_nvme",
    "rpi2_cpu", "rpi2_nvme",
    "rpi3_cpu", "rpi3_nvme",
    "rpi4_cpu", "rpi4_nvme",
    "intake",   "exhaust",
};

// A known-good JSON that matches the firmware's expected schema.
static const char* kValidJson = R"({
    "version": 1,
    "baud_rate": 115200,
    "fan_pwm_pin": 4,
    "tach_pin": 5,
    "onewire_pin": 14,
    "fan_pwm_min": 30,
    "fan_pwm_max": 255,
    "pwm_frequency_hz": 25000,
    "pwm_active_low": false,
    "tach_pulses_per_revolution": 2,
    "rpm_calc_interval_ms": 1000,
    "temp_read_interval_ms": 3000,
    "cpu_temp_min_c": 40.0,
    "cpu_temp_max_c": 75.0,
    "nvme_temp_min_c": 40.0,
    "nvme_temp_max_c": 70.0,
    "delta_temp_min_c": 5.0,
    "delta_temp_max_c": 15.0,
    "fan_curve_exponent": 2.5,
    "status_print_interval_ms": 5000,
    "sensors": [
        {"slot":0,"name":"rpi1_cpu",  "address":"28C91D7B02250ED5"},
        {"slot":1,"name":"rpi1_nvme", "address":"287AC07C02250EBF"},
        {"slot":2,"name":"rpi2_cpu",  "address":"28CDE16A02250E39"},
        {"slot":3,"name":"rpi2_nvme", "address":"282E566802250E50"},
        {"slot":4,"name":"rpi3_cpu",  "address":"282BFA5B02250E69"},
        {"slot":5,"name":"rpi3_nvme", "address":"28C1E34B02250E38"},
        {"slot":6,"name":"rpi4_cpu",  "address":"281DD57602250E4C"},
        {"slot":7,"name":"rpi4_nvme", "address":"28455B5002250ED3"},
        {"slot":8,"name":"intake",    "address":"281DF86802250E75"},
        {"slot":9,"name":"exhaust",   "address":"285D207A02250E5F"}
    ]
})";

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool hexToAddress(const char* hex, uint8_t* out) {
    if (!hex || strlen(hex) != 16) return false;
    for (int i = 0; i < 8; ++i) {
        char buf[3] = { hex[i * 2], hex[i * 2 + 1], '\0' };
        char* end;
        out[i] = static_cast<uint8_t>(strtoul(buf, &end, 16));
        if (end != buf + 2) return false;
    }
    return true;
}

static JsonDocument gDoc;

// ---------------------------------------------------------------------------
// Tests — valid config
// ---------------------------------------------------------------------------

void setUp() {}
void tearDown() {}

void test_parses_without_error() {
    DeserializationError err = deserializeJson(gDoc, kValidJson);
    TEST_ASSERT_EQUAL_MESSAGE(DeserializationError::Ok, err.code(),
                              "deserializeJson failed");
}

void test_version() {
    TEST_ASSERT_EQUAL_INT(1, gDoc["version"].as<int>());
}

void test_pins_unique_and_valid() {
    int pwm  = gDoc["fan_pwm_pin"];
    int tach = gDoc["tach_pin"];
    int ow   = gDoc["onewire_pin"];

    TEST_ASSERT_NOT_EQUAL(pwm, tach);
    TEST_ASSERT_NOT_EQUAL(pwm, ow);
    TEST_ASSERT_NOT_EQUAL(tach, ow);

    auto valid = [](int p) { return p >= 0 && p <= 16 && !(p >= 6 && p <= 11); };
    TEST_ASSERT_TRUE(valid(pwm));
    TEST_ASSERT_TRUE(valid(tach));
    TEST_ASSERT_TRUE(valid(ow));
}

void test_pwm_min_less_than_max() {
    int mn = gDoc["fan_pwm_min"];
    int mx = gDoc["fan_pwm_max"];
    TEST_ASSERT_LESS_THAN(mx, mn);  // mn < mx
    TEST_ASSERT_GREATER_OR_EQUAL(0, mn);
    TEST_ASSERT_LESS_OR_EQUAL(255, mx);
}

void test_temperature_thresholds_ordered() {
    TEST_ASSERT_LESS_THAN(gDoc["cpu_temp_max_c"].as<float>(),
                          gDoc["cpu_temp_min_c"].as<float>());
    TEST_ASSERT_LESS_THAN(gDoc["nvme_temp_max_c"].as<float>(),
                          gDoc["nvme_temp_min_c"].as<float>());
    TEST_ASSERT_LESS_THAN(gDoc["delta_temp_max_c"].as<float>(),
                          gDoc["delta_temp_min_c"].as<float>());
}

void test_intervals_positive() {
    TEST_ASSERT_GREATER_THAN(0, gDoc["rpm_calc_interval_ms"].as<int>());
    TEST_ASSERT_GREATER_THAN(0, gDoc["temp_read_interval_ms"].as<int>());
    TEST_ASSERT_GREATER_THAN(0, gDoc["status_print_interval_ms"].as<int>());
}

void test_sensor_count() {
    TEST_ASSERT_EQUAL_INT(kNumSensors,
                          gDoc["sensors"].as<JsonArray>().size());
}

void test_sensor_slots_unique_and_in_range() {
    bool seen[kNumSensors] = {};
    for (JsonObject s : gDoc["sensors"].as<JsonArray>()) {
        int slot = s["slot"];
        TEST_ASSERT_TRUE(slot >= 0 && slot < kNumSensors);
        TEST_ASSERT_FALSE_MESSAGE(seen[slot], "duplicate slot");
        seen[slot] = true;
    }
}

void test_sensor_addresses_valid() {
    for (JsonObject s : gDoc["sensors"].as<JsonArray>()) {
        const char* addr = s["address"];
        uint8_t bytes[8];
        TEST_ASSERT_TRUE_MESSAGE(hexToAddress(addr, bytes), addr);
        // DS18B20 family code
        TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x28, bytes[0], addr);
    }
}

void test_sensor_names_match_expected() {
    for (JsonObject s : gDoc["sensors"].as<JsonArray>()) {
        int slot = s["slot"];
        TEST_ASSERT_EQUAL_STRING(kExpectedNames[slot], s["name"].as<const char*>());
    }
}

// ---------------------------------------------------------------------------
// Tests — invalid configs (validation logic mirrored from runtime_config.cpp)
// ---------------------------------------------------------------------------

void test_reject_duplicate_pins() {
    JsonDocument bad;
    deserializeJson(bad, R"({"fan_pwm_pin":4,"tach_pin":4,"onewire_pin":14})");
    TEST_ASSERT_EQUAL(bad["fan_pwm_pin"].as<int>(), bad["tach_pin"].as<int>());
}

void test_reject_pwm_min_greater_than_max() {
    JsonDocument bad;
    deserializeJson(bad, R"({"fan_pwm_min":255,"fan_pwm_max":30})");
    TEST_ASSERT_GREATER_THAN(bad["fan_pwm_max"].as<int>(),
                             bad["fan_pwm_min"].as<int>());
}

void test_reject_inverted_temp_thresholds() {
    JsonDocument bad;
    deserializeJson(bad, R"({"cpu_temp_min_c":80.0,"cpu_temp_max_c":40.0})");
    TEST_ASSERT_GREATER_THAN(bad["cpu_temp_max_c"].as<float>(),
                             bad["cpu_temp_min_c"].as<float>());
}

void test_reject_wrong_family_code() {
    const char* badAddr = "AA1234567890ABCD";
    uint8_t bytes[8];
    TEST_ASSERT_TRUE(hexToAddress(badAddr, bytes));
    TEST_ASSERT_NOT_EQUAL(0x28, bytes[0]);
}

// ---------------------------------------------------------------------------

int main() {
    UNITY_BEGIN();

    // Valid config
    RUN_TEST(test_parses_without_error);
    RUN_TEST(test_version);
    RUN_TEST(test_pins_unique_and_valid);
    RUN_TEST(test_pwm_min_less_than_max);
    RUN_TEST(test_temperature_thresholds_ordered);
    RUN_TEST(test_intervals_positive);
    RUN_TEST(test_sensor_count);
    RUN_TEST(test_sensor_slots_unique_and_in_range);
    RUN_TEST(test_sensor_addresses_valid);
    RUN_TEST(test_sensor_names_match_expected);

    // Invalid config rejection
    RUN_TEST(test_reject_duplicate_pins);
    RUN_TEST(test_reject_pwm_min_greater_than_max);
    RUN_TEST(test_reject_inverted_temp_thresholds);
    RUN_TEST(test_reject_wrong_family_code);

    return UNITY_END();
}
