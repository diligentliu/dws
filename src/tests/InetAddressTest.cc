#include <gtest/gtest.h>

#include "InetAddress.h"
#include "Logging.h"

using dws::net::InetAddress;
using std::string;

TEST(InetAddressTest, InetAddressTest) {
    {
        InetAddress addr0(1234);
        EXPECT_EQ(addr0.toIp(), "0.0.0.0");
        EXPECT_EQ(addr0.toIpPort(), "0.0.0.0:1234");
        EXPECT_EQ(addr0.port(), 1234);
    }
    {
        InetAddress addr1(4321, true);
        EXPECT_EQ(addr1.toIp(), "127.0.0.1");
        EXPECT_EQ(addr1.toIpPort(), "127.0.0.1:4321");
        EXPECT_EQ(addr1.port(), 4321);
    }
    {
        InetAddress addr2("1.2.3.4", 8888);
        EXPECT_EQ(addr2.toIp(), "1.2.3.4");
        EXPECT_EQ(addr2.toIpPort(), "1.2.3.4:8888");
        EXPECT_EQ(addr2.port(), 8888);
    }
    {
        InetAddress addr3("255.254.253.252", 65535);
        EXPECT_EQ(addr3.toIp(), "255.254.253.252");
        EXPECT_EQ(addr3.toIpPort(), "255.254.253.252:65535");
        EXPECT_EQ(addr3.port(), 65535);
    }
}

TEST(InetAddressTest, InetAddressTest_ipv6) {
    InetAddress addr0(1234, false, true);
    EXPECT_EQ(addr0.toIp(), string("::"));
    EXPECT_EQ(addr0.toIpPort(), string("[::]:1234"));
    EXPECT_EQ(addr0.port(), 1234);

    InetAddress addr1(1234, true, true);
    EXPECT_EQ(addr1.toIp(), string("::1"));
    EXPECT_EQ(addr1.toIpPort(), string("[::1]:1234"));
    EXPECT_EQ(addr1.port(), 1234);

    InetAddress addr2("2001:db8::1", 8888, true);
    EXPECT_EQ(addr2.toIp(), string("2001:db8::1"));
    EXPECT_EQ(addr2.toIpPort(), string("[2001:db8::1]:8888"));
    EXPECT_EQ(addr2.port(), 8888);

    InetAddress addr3("fe80::1234:abcd:1", 8888, true);
    EXPECT_EQ(addr3.toIp(), string("fe80::1234:abcd:1"));
    EXPECT_EQ(addr3.toIpPort(), string("[fe80::1234:abcd:1]:8888"));
    EXPECT_EQ(addr3.port(), 8888);
}

TEST(InetAddressTest, InetAddressTest_Resolve) {
    InetAddress addr(80);
    if (InetAddress::resolve("google.com", &addr)) {
        std::cerr << "google.com resolved to " << addr.toIpPort() << std::endl;
    } else {
        std::cerr << "Unable to resolve google.com" << std::endl;
    }
}
