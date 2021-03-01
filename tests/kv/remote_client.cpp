// Copyright 2019 Mikhail Kazhamiaka
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "include/kv_store/remote_server.h"
#include "include/client/remote_client.h"
#include "common/common.h"

#include <random>

const int num_keys = KV_SIZE;

std::string timestamps() {
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm gmt{};
    gmtime_r(&tt, &gmt);
    std::chrono::duration<double> fractional_seconds =
            (tp - std::chrono::system_clock::from_time_t(tt)) + std::chrono::seconds(gmt.tm_sec);
    std::string buffer("hr:mn:sc.xxxxxx");
    sprintf(&buffer.front(), "%02d:%02d:%08.6f", gmt.tm_hour, gmt.tm_min, fractional_seconds.count());
    return buffer;
}

int getOp() {
    static thread_local std::default_random_engine generator;
    std::uniform_int_distribution<int> intDistribution(0,99);
    return intDistribution(generator);
}

int main(int argc, char **argv) {
    if (argc < 5) {
        printf("Usage: %s server_addr server_port numOps readProb\n", argv[0]);
        return -1;
    }

    printConfig();

    std::string server_addr(argv[1]);
    int server_port = std::stoi(argv[2]);
    int num_ops = std::stoi(argv[3]);
    int read_prob = std::stoi(argv[4]);

    LogInfo("Starting test");
    RemoteClient client(server_addr, server_port);

    LogInfo("Populating store with " << num_keys << " values...");
    // Populate the store with values
    // Population is single-threaded, might take a long time if KV_SIZE is large
    for (int i = 0; i < num_keys; i++) {
        std::string key("keykeykey" + std::to_string(i));
        std::string value("this is a test value " + std::to_string(i));
        client.put(key, value);
    }
    LogInfo("Done populating kv store");

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(1,num_keys-1);

    LogInfo("Running workload...");
    std::string str1 = timestamps();
    uint64_t completed_gets = 0;
    uint64_t completed_puts = 0;

    for (int i = 0; i < num_ops; i++) {
        int op = getOp();
        std::string key("keykeykey" + std::to_string(dist(rng)));

        if (op < read_prob) {
            client.get(key);
            completed_gets++;
        } else {
            std::string value("this is a test string " + std::to_string(i));
            client.put(key, value);
            completed_puts++;
        }
    }

    std::string str2 = timestamps();
    std::string str3 = "";
    int flag = 0;
    for(int i=0; i<12; i++){
        if(str1[i] != ':' || str1[i] != '.'){
            char temp = (char *)str2[i] - str1[i];
            if(temp !=0 || flag == 1){
                str3 += temp;
                flag = 1;
            }
        }
    }
    LogInfo("Results: " << completed_gets << " gets, " << completed_puts << " puts, Total consume " << str3 << "ms");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}
