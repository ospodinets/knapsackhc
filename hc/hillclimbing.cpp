#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <algorithm>


struct Item
{
    int c; // cost
    int w; // weight
    bool x; // solution variable

    Item() : c(0), w(0), x(false) {}
};

using State = std::vector<Item>;

void log(FILE* f, const char* format, ...)
{
    va_list args;
    va_start(args, format);


    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("%s", buffer);
    fprintf(f, "%s", buffer);

    fflush(f);


    va_end(args);
}


State loadItems(const std::string& filepath, int& Cap, int& Profit)
{
    std::vector<Item> items;
    // load items from file
    FILE* in = fopen(filepath.c_str(), "r");
    if (in != NULL)
    {
        int len = 0;
        char d1, d2;
        fscanf(in, "%d %c %c\n", &len, &d1, &d2);

        items.resize(len);

        for (int i = 0; i < len; i++)
        {
            int n = 0;
            int x = 0;
            fscanf(in, "%d %d %d %d\n", &n, &items[i].c, &items[i].w, &x);
        }

        fscanf(in, "%d\n", &Cap);
        fscanf(in, "%d\n", &Profit);

        fclose(in);
    }

    return items;
}

void saveItems(const std::string& filepath, const State& items, int cap, int cost)
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

void initRandomState(State& state, int Cap)
{
    int W = 0;
    for (int i = 0; i < state.size(); ++i )
    {
        int j = rand() % state.size();
        if (W + state[j].w <= Cap)
        {
            state[j].x = true;
            W += state[j].w;
        }
    }
}

void findNeighbour(const State& x, int distance, std::set<int>& flipped )
{
    int rdistance = rand() % distance + 1;
    for (int i = 0; i < rdistance; ++i)
    {
        int flip = rand() % x.size();
        // avoid double flips
        if (flipped.find(flip) == flipped.end())
        {
            flipped.insert(flip);
        }
    }
}

int evaluateFull(State& x, int& W0, int& C0, int Cap)
{
    for (auto& s : x)
    {
        if (s.x)
        {
            W0 += s.w;
            C0 += s.c;
        }
    }

    return W0 > Cap ? 0 : C0;
}

int evaluateDelta(const State& x, int W0, int C0, int Cap, const std::set<int>& modified, int& Wy, int& Cy)
{
    Wy = W0;
    Cy = C0;
    for (auto& i : modified)
    {
        bool flip = !x[i].x;
        if (flip)
        {
            Wy += x[i].w;
            Cy += x[i].c;
        }
        else
        {
            Wy -= x[i].w;
            Cy -= x[i].c;
        }
    }

    return Wy > Cap ? 0 : Cy;
}

void updateDelta(State& x, const std::set<int>& modified)
{
    for (auto& i : modified)
    {
        x[i].x = !x[i].x;
    }

}

void hillclimbing(State& x, int Cap, int numAttempts, int& profit, FILE* logfile)
{
    initRandomState(x, Cap);
    int W0 = 0; 
    int C0 = 0;
    int Px = evaluateFull(x, W0, C0, Cap);

    profit = Px;

    log(logfile, "Initial state profit = %d\n", Px);

    bool found = true;
    int ndistance = x.size() / 2;

    int numIterations = 0;

    while (ndistance >= 1)
    {
        found = false;
        for (int i = 0; i < numAttempts; ++i)
        {
            ++numIterations;
            std::set<int> flipped;
            findNeighbour(x, ndistance, flipped);

            int Wy = 0;
            int Cy = 0;

            int Py = evaluateDelta(x, W0, C0, Cap, flipped, Wy, Cy );
            
            if( Py > Px )
            {
                log(logfile, "%7d\t%5d\n", numIterations, Py);
                profit = Py;
                updateDelta(x, flipped);
                W0 = Wy;
                C0 = Cy;
                Px = Py;
                found = true;
                ndistance = ndistance * 4;
                if( ndistance > x.size() )
                    ndistance = x.size();

                break;
            }
        }
        if (!found)
        {
            ndistance = ndistance / 4;
        }
    }

    log(logfile, "After %d iterations the best profit is = %d\n", numIterations, profit );
}



int main(int argc, char* argv[])
{
    srand(time(NULL));

    int numStarts = 1;
    if( argc >= 2 )
        numStarts = atoi(argv[1]);

    int numAttempts = 10000;
    if (argc >= 3)
        numAttempts = atoi(argv[2]);

    
    int Cap = 0;
    int Profit = 0;

    State items = loadItems("test.out", Cap, Profit);

    FILE* logfile = fopen("log.txt", "w");

    log(logfile, "Benchmark data: Num items = %llu, Cap = %d, Profit = %d\n", items.size(), Cap, Profit );
    log(logfile, "NumStarts = %d, NumAttempts = %d\n", numStarts, numAttempts);

    auto time = std::chrono::system_clock::now();

    for (int i = 0; i < numStarts; ++i)
    {
        State x = items;
//        sortItems(x);
        int profit = 0;
        log(logfile, "Pass %d started\n", i);

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        hillclimbing(x, Cap, numAttempts, profit, logfile);

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        log(logfile, "Pass %d finished in %llu seconds\n", i, std::chrono::duration_cast<std::chrono::seconds>(end - begin).count());
        log(logfile, "The correlation with benchmark = %d\n", Profit - profit);
        log(logfile, "===================================\n");

        std::string filename = "testMy" + std::to_string(i) + ".out";

        saveItems(filename, x, Cap, profit);
    }

    fclose(logfile);
}