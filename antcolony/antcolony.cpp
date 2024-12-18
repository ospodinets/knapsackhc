#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <random>
#include <tuple>

#include "ant_colony_optimization.h"

namespace
{
    using ACO = AntColonyKnapsackOptimization;
    

    ACO::Items loadItems(const std::string& filepath, int& Cap, int& Profit)
    {
        ACO::Items items;
        // load items from file
        FILE* in = fopen(filepath.c_str(), "r");
        if (in != NULL)
        {
            int len = 0;
            char d1, d2;
            fscanf(in, "%d %c %c\n", &len, &d1, &d2);

            for (int i = 0; i < len; i++)
            {
                int n = 0;
                int x = 0;
                int c = 0;
                int w = 0;
                fscanf(in, "%d %d %d %d\n", &n, &c, &w, &x);

                items.push_back(ACO::Item(c, w));
            }

            fscanf(in, "%d\n", &Cap);
            fscanf(in, "%d\n", &Profit);

            fclose(in);
        }

        return items;
    }

    void saveItems(const std::string& filepath, const ACO::Items& items, int cap, int cost)
    {
        FILE* out = fopen(filepath.c_str(), "w");
        if (out != NULL)
        {
            fprintf(out, "%5llu     C     W\n", items.size());
            for (int i = 0; i < items.size(); ++i)
            {
                fprintf(out, "%5d %5d %5d %5d\n", i, items[i].c, items[i].w, items[i].x);
            }
            fprintf(out, "%d\n", cap);
            fprintf(out, "%d\n", cost);
            fclose(out);
        }
    }

    bool spam = true;
    void log(FILE* f, const char* format, ...)
    {
        va_list args;
        va_start(args, format);


        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);

        if( spam )
            printf("%s", buffer);
        fprintf(f, "%s", buffer);

        fflush(f);
        va_end(args);
    }
}

int main(int argc, char* argv[])
{
    int numStarts = 10;
    if (argc >= 2)
        numStarts = atoi(argv[1]);

    int Cap = 0;
    int Profit = 0;
    auto items = loadItems("test.out", Cap, Profit);

    FILE* logfile = fopen("results/log.txt", "w");
    FILE* deviation = fopen("results/deviation.txt", "w");
    FILE* fx = fopen("results/fx.txt", "w");

    log(logfile, "Benchmark data: Num items = %llu, Cap = %d, Profit = %d\n", items.size(), Cap, Profit);
    log(logfile, "NumStarts = %d\n", numStarts);

    auto timeStart = std::chrono::system_clock::now();

    int bestProfit = 0;
    int worstProfit = -1;
    int globalIteration = 0;

    ACO alg(0.5, 2.5, 0.9);

    alg.setFxLogger([&](const std::string& msg)
        {
            spam = false;
            log(fx, "%d\t%s\n", globalIteration++, msg.c_str());
            spam = true;
        });

    for (int i = 0; i < numStarts; ++i)
    {
        log(logfile, "Pass %d started\n", i);

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        alg.setItems(items, Cap);
        alg.run(700);

        auto profit = alg.getProfit();

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        log(logfile, "Pass %d finished in %llu seconds\n", i, std::chrono::duration_cast<std::chrono::seconds>(end - begin).count());
        log(logfile, "The deviation from ideal value = %d\n", Profit - profit);
        log(logfile, "===================================\n");

        fprintf(deviation, "%d\t%f\n", profit, double(Profit - profit) / double( Profit ) );


        std::string filename = "results/testMy" + std::to_string(i) + ".out";

        saveItems(filename, alg.getItems(), Cap, profit);

        if( worstProfit < 0 )
            worstProfit = profit;

        if (profit > bestProfit)
            bestProfit = profit;
        else if (profit < worstProfit)
            worstProfit = profit;
    }

    auto timeStop = std::chrono::system_clock::now();

    log(logfile, "%d passes finished in %llu minutes\n",
        numStarts,
        std::chrono::duration_cast<std::chrono::minutes>(timeStop - timeStart).count());
    log(logfile, "The best profit = %d, the worst profit = %d\n", bestProfit, worstProfit);


    fclose(logfile);
    fclose(deviation);
    fclose(fx);

}