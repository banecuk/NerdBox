#include <cstring>

#include "network/UrlUtils.h"

#include <gtest/gtest.h>

TEST(UrlUtilsTest, ParsesHostWithSchemeAndPort) {
    char host[64];
    uint16_t port;
    UrlUtils::parseHostPort("http://192.168.1.11:8086/api/v1/system", host, sizeof(host), port);
    EXPECT_STREQ(host, "192.168.1.11");
    EXPECT_EQ(port, 8086);
}

TEST(UrlUtilsTest, DefaultsToPort80WithoutExplicitPort) {
    char host[64];
    uint16_t port;
    UrlUtils::parseHostPort("http://nerdwinsense.local/api/v1/system", host, sizeof(host), port);
    EXPECT_STREQ(host, "nerdwinsense.local");
    EXPECT_EQ(port, 80);
}

TEST(UrlUtilsTest, HandlesMissingScheme) {
    char host[64];
    uint16_t port;
    UrlUtils::parseHostPort("192.168.1.11:8086/api/v1/system", host, sizeof(host), port);
    EXPECT_STREQ(host, "192.168.1.11");
    EXPECT_EQ(port, 8086);
}

TEST(UrlUtilsTest, HandlesPortWithNoTrailingPath) {
    char host[64];
    uint16_t port;
    UrlUtils::parseHostPort("http://192.168.1.11:8086", host, sizeof(host), port);
    EXPECT_STREQ(host, "192.168.1.11");
    EXPECT_EQ(port, 8086);
}

TEST(UrlUtilsTest, HandlesHostWithNoPortAndNoPath) {
    char host[64];
    uint16_t port;
    UrlUtils::parseHostPort("http://192.168.1.11", host, sizeof(host), port);
    EXPECT_STREQ(host, "192.168.1.11");
    EXPECT_EQ(port, 80);
}

TEST(UrlUtilsTest, TruncatesHostLongerThanBuffer) {
    char host[8];
    uint16_t port;
    UrlUtils::parseHostPort("http://averylonghostname.example.com:8086/x", host, sizeof(host),
                            port);
    EXPECT_EQ(strlen(host), sizeof(host) - 1);
    EXPECT_EQ(port, 8086);
}

TEST(UrlUtilsTest, MalformedInputStillNullTerminatesAndDefaultsPort) {
    char host[64];
    uint16_t port;
    UrlUtils::parseHostPort("", host, sizeof(host), port);
    EXPECT_STREQ(host, "");
    EXPECT_EQ(port, 80);
}
