#include "net_demo.h"

#include <unistd.h>
#include <iostream>

#include "Timestamp.h"

void test_net() {
    int cnt = 6;
    while (cnt--) {
        std::cout << dws::Timestamp::now().toFormattedString(true) << std::endl;
        sleep(1);
    }
}
