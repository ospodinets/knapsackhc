#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <string>

class AntColonyKnapsackOptimization
{
public:
    struct Item
    {
        int c; // cost
        int w; // weight
        bool x; // solution variable

        Item( int c, int w ) 
            : c(c)
            , w(w)
            , x(false)
            , attractiveness(0.0)
        {
            attractiveness = double(c) / double(w);
        }

        // the more attractiveness the better
        double getAttractiveness() const { return attractiveness; }

    private:
        double attractiveness;
    };
    using Items = std::vector<Item>;
    using Logger = std::function<void(const std::string&)>;

public:
    AntColonyKnapsackOptimization(double alpha, double beta, double evaporation);
    ~AntColonyKnapsackOptimization();
    void setFxLogger(const Logger& logger);
    void setItems(const Items& items, int Cap);
    
    
    Items getItems() const;
    int getProfit() const;

    void run(int colonySize);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};