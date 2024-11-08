#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <algorithm>

namespace
{
    struct Item
    {
        int c; // cost
        int w; // weight
        bool x; // solution variable

        Item() : c(0), w(0), x(false) {}
    };

    using Items = std::vector<Item>;
    using Diff = std::set<int>;
    using DiffList = std::vector<Diff>;

    
    class State
    {
    private:
        int m_Cap;
        Items m_items;

        // current cost
        int m_C;
        // cutrrent weight
        int m_W;

    public:
        State(const Items& items, int Cap )
            : m_items(items)
            , m_Cap(Cap)
            , m_C(0)
            , m_W(0) 
        {
            // init random state
            for (int i = 0; i < m_items.size(); ++i)
            {
                int j = rand() % m_items.size();
                if (!m_items[j].x && m_W + m_items[j].w <= m_Cap)
                {
                    m_items[j].x = true;
                    m_W += m_items[j].w;
                    m_C += m_items[j].c;
                }
            }
        }

        int getCost() const { return m_C; }
        int getWeight() const { return m_W; }
        const Items& getItems() const { return m_items; }

        void evaluate()
        {
            m_W = 0;
            m_C = 0;
            for (auto& s : m_items)
            {
                if (s.x)
                {
                    m_W += s.w;
                    m_C += s.c;
                }
            }
        }

        // only applies flipped indices from the given set
        // doesn't modify the state
        // return Ci, Wi
        std::pair<int, int> evaluate(const Diff& flipped) const
        {
            int Cy = m_C;
            int Wy = m_W;

            for (auto& i : flipped)
            {
                bool flip = !m_items[i].x;
                if (flip)
                {
                    Cy += m_items[i].c;
                    Wy += m_items[i].w;
                    
                }
                else
                {
                    Cy -= m_items[i].c;
                    Wy -= m_items[i].w;
                }
            }

            if( Wy > m_Cap )
                return { 0, 0 };

            return { Cy, Wy };
        }

        void update(const Diff& flipped)
        {
            for (auto& i : flipped)
            {
                bool flip = !m_items[i].x;
                if (flip)
                {
                    m_C += m_items[i].c;
                    m_W += m_items[i].w;

                }
                else
                {
                    m_C -= m_items[i].c;
                    m_W -= m_items[i].w;
                }
                m_items[i].x = flip;
            }
        }
    };

    using Storage = std::vector<int>;
    class Memory
    {
        // tabu tenure
        int m_tabTenureDuration;
        Storage m_shortMemory;
        Storage m_longMemory;
    public:
        Memory(int size, int tabTenureDuration )
            : m_tabTenureDuration(tabTenureDuration)
        {
            m_shortMemory.resize(size, 0);
            m_longMemory.resize(size, 0);
        }

        void reset()
        {
            std::fill(m_shortMemory.begin(), m_shortMemory.end(), 0);
            std::fill(m_longMemory.begin(), m_longMemory.end(), 0);
        }

        bool hasTabu(const Diff& flipped)
        {
            for (auto& i : flipped)
            {
                if (m_shortMemory[i] > 0)
                    return true;
            }
            return false;
        }

        void updateTabu(const Diff& flipped)
        {
            for (int i = 0; i < m_shortMemory.size(); ++i)
            {
                if (flipped.find(i) != flipped.end())
                {
                    m_shortMemory[i] = m_tabTenureDuration;
                }
                else
                {
                    if (m_shortMemory[i] > 0)
                        --m_shortMemory[i];
                }
            }
        }

        int getLongMemoryFlips(const Diff& flipped)
        {
            int passes = 0;
            for (auto& i : flipped)
            {
                passes += m_longMemory[i];
            }
            return passes;
        }

        void updateLongMemory(const Diff& flipped)
        {
            for (auto& i : flipped)
            {
                ++m_longMemory[i];
            }
        }
    };

    Items loadItems(const std::string& filepath, int& Cap, int& Profit)
    {
        Items items;
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

    void saveItems(const std::string& filepath, const Items& items, int cap, int cost)
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

    static bool spam = true;
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

void tabusearch(Items& itemsList, int Cap, int& profit, FILE* logfile)
{
    FILE* fxfile = fopen("results/fx.txt", "w");

    const int Numresets = 7;

    // This is the most affecting parameter on presicion
    // ythe less the value - the more time is needed to escape local optimas, 
    // but the more precision we have in the end
    const double alpha = 0.7;
    const int MAX_ITERATIONS = 10000000;
    const int N = itemsList.size();

    State x(itemsList, Cap);

    log(logfile, "Initial state profit = %d\n", x.getCost());

    Memory memory(N, 5);
    State best = x;

    int numResets = 0;
    int iteration = 0;

    for( ; iteration < MAX_ITERATIONS; ++iteration )
    {
        int maxFy = 0;
        Diff maxFyDiff;

        // for each neighbour from 2-flip neighbourhood

        for (int flippedIndex = 0; flippedIndex < N; ++flippedIndex)
        {
            Diff flipped = { flippedIndex };
            auto Fy = x.evaluate(flipped);

            if (!memory.hasTabu(flipped))
            {
                int Profit = Fy.first - alpha * memory.getLongMemoryFlips(flipped);
                if (Profit > maxFy)
                {
                    maxFy = Fy.first;
                    maxFyDiff = flipped;
                }
            }
            else
            {
                // aspiration
                if (Fy.first > best.getCost())
                {
                    maxFyDiff = flipped;
                    break;
                }
            }
        }

        if (maxFyDiff.empty())
        {
            if (numResets > Numresets - 1)
            {
//                log(logfile, "Looks like we've found the optimum. Stopping\n");
                break;
            }
            else
            {
//                log(logfile, "Resetting\n");
                memory.reset();
                numResets++;
                continue;
            }
        }

        x.update(maxFyDiff);

        spam = false;
        log(fxfile, "%7d\t%5d\n", iteration, x.getCost());
        spam = true;

        if (x.getCost() > best.getCost())
        {
            log(logfile, "%7d\t%5d\n", iteration, x.getCost());
            best = x;
        }
        
        memory.updateTabu(maxFyDiff);
        memory.updateLongMemory(maxFyDiff);
    }

    profit = best.getCost();
    itemsList = best.getItems();

    log(logfile, "After %d iterations the best profit is = %d (w=%d)\n", iteration + 1, profit, best.getWeight() );

    fclose(fxfile);
}

int main(int argc, char* argv[])
{
    srand(time(NULL));

    int numStarts = 1;
    if( argc >= 2 )
        numStarts = atoi(argv[1]);

    
    int Cap = 0;
    int Profit = 0;

    Items items = loadItems("test.out", Cap, Profit);

    FILE* logfile = fopen("results/log.txt", "w");

    FILE* stat = fopen("results/statistics.txt", "w");

    log(logfile, "Benchmark data: Num items = %llu, Cap = %d, Profit = %d\n", items.size(), Cap, Profit );
    log(logfile, "NumStarts = %d\n", numStarts );

    auto timeStart = std::chrono::system_clock::now();

    int betterProfit = 0;
    int worseProfit = Profit;

    for (int i = 0; i < numStarts; ++i)
    {
        Items x = items;
        int profit = 0;
        log(logfile, "Pass %d started\n", i);

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        tabusearch(x, Cap, profit, logfile);

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        log(logfile, "Pass %d finished in %llu seconds\n", i, std::chrono::duration_cast<std::chrono::seconds>(end - begin).count());
        log(logfile, "The correlation with benchmark = %d\n", Profit - profit);
        log(logfile, "===================================\n");

        spam = false;
        log(stat, "%d\n", profit);
        spam = true;

        std::string filename = "results/testMy" + std::to_string(i) + ".out";

        saveItems(filename, x, Cap, profit);

        if( profit > betterProfit )
            betterProfit = profit;
        else if( profit < worseProfit )
            worseProfit = profit;
    }

    auto timeStop = std::chrono::system_clock::now();

    log(logfile, "%d passes finished in %llu minutes\n", 
        numStarts, 
        std::chrono::duration_cast<std::chrono::minutes>(timeStop - timeStart).count());
    log(logfile, "The best profit = %d, the worst profit = %d\n", betterProfit, worseProfit);


    fclose(logfile);
    fclose(stat);

}