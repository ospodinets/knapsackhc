#include "ant_colony_optimization.h"

#include <set>
#include <random>


using ACO = AntColonyKnapsackOptimization;

namespace
{
    class Knapsack
    {
    private:
        int m_Cap;
        ACO::Items m_items;

        // current cost
        int m_C;
        // cutrrent weight
        int m_W;

        std::set<int> m_takenItems;

    public:
        Knapsack(const ACO::Items& items, int Cap)
            : m_items(items)
            , m_Cap(Cap)
            , m_C(0)
            , m_W(0)
        {
        }

        int getCost() const { return m_C; }
        int getWeight() const { return m_W; }
        const ACO::Items& getItems() const { return m_items; }
        int getCap() const { return m_Cap; }

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

        bool canAddItem(int index) const
        {
            return m_takenItems.find(index) == m_takenItems.end() && 
                m_W + m_items[index].w <= m_Cap;
        }

        bool addItem(int index)
        {
            if (canAddItem(index))
            {
                m_items[index].x = true;
                m_W += m_items[index].w;
                m_C += m_items[index].c;
                m_takenItems.insert(index);
                return true;
            }
            return false;
        }
    };

    struct Edge
    {
        int i;
        int j;
        double factor;

        double pv;
        // cumulative probability
        double p0;
        double p1;

        Edge(int i = -1, 
            int j = -1,
            double factor = 0.0, 
            double pv = 0.0, 
            double p0 = 0.0, 
            double p1 = 0.0,
            double pheromone = 0.0)
            : i(i)
            , j(j)
            , factor(factor)
            , pv(pv)
            , p0(p0)
            , p1(p1)
        {
        }

        bool isValid() const { return j >= 0; }
    };
    using Edges = std::vector<Edge>;

    using Matrix = std::vector<std::vector<double>>;

    int getRandom()
    {
        // select the item with the highest probability
        static std::random_device rand_dev;
        static std::mt19937 generator(rand_dev());
        static std::uniform_int_distribution<int> distr(0, 99);

        return distr(generator);
    }
}

struct ACO::Impl
{
    const double basePheromone = 0.001;
    Logger logger;

    ACO::Items items;
    int Cap;
    int Profit;

    double alpha;
    double beta;
    double evaporation;

    Matrix pheromoneRemnant;

    Edge selectNextItem(int current, const Knapsack& knapsack)
    {
        // for each neighbour except ones in the memory and the current
        // calculate the probability of selection
        Edges edges;

        double sum = 0.0;
        for (int i = 0; i < items.size(); ++i)
        {
            if (i == current )
                continue;

            if (!knapsack.canAddItem(i))
                continue;

            Edge edge;
            edge.i = current;
            edge.j = i;
            double pheromone = current == -1 ? basePheromone : pheromoneRemnant[current][i];
            edge.factor = pow(pheromone, alpha) * pow(items[i].getAttractiveness(), beta);
            sum += edge.factor;

            edges.push_back(edge);
        }

        if (!edges.empty())
        {
            // calculate the probability of selection
            double cp = 0;
            for (auto& e : edges)
            {
                e.pv = e.factor / sum;
                e.p0 = cp;
                e.p1 = e.p0 + e.pv * 100;
                cp = e.p1;
            }

            auto r = getRandom();
            for (auto& e : edges)
            {
                if (r >= e.p0 && r < e.p1)
                    return e;
            }
        }

        // invalid edge
        return Edge();
    }

    std::pair<Knapsack, Edges> antActivity()
    {
        Knapsack knapsack(items, Cap);

        Edges path;

        int current = -1;

        while (knapsack.getWeight() < Cap)
        {
            auto next = selectNextItem( current, knapsack );
            if (!next.isValid())
                break;

            knapsack.addItem(next.j);
            path.push_back(next);
            current = next.j;
        }

        return { knapsack, path };
    }

    void run(int colonySize)
    {
        pheromoneRemnant.clear();
        pheromoneRemnant.resize(items.size(), std::vector<double>(items.size(), basePheromone));

        Knapsack best(items, Cap);

        for (int i = 0; i < colonySize; ++i)
        {
            auto [knapsack, path] = antActivity();

            logger(std::to_string(knapsack.getCost()));

            if (knapsack.getCost() > best.getCost())
            {
                best = knapsack;
            }

            // pheromone amount which is left by the ant
            double pheromone = 1.0 / (1.0 + (best.getCost() - knapsack.getCost()) / best.getCost());

            for (auto& edge : path)
            {
                if (edge.i >= 0)
                {
                    pheromoneRemnant[edge.i][edge.j] += pheromone;
                }
            }

            // evaporate pheromone
            for (int i = 0; i < pheromoneRemnant.size(); ++i)
                for (int j = 0; j < pheromoneRemnant[i].size(); ++j)
                    pheromoneRemnant[i][j] *= evaporation;
            
        }

        Profit = best.getCost();
        items = best.getItems();
    }

    Impl(double alpha, double beta, double evaporation)
        : alpha(alpha)
        , beta(beta)
        , evaporation(evaporation)
        , Cap(0)
        , Profit(0)

    {
        logger = [](const std::string&) {};
    };
};

AntColonyKnapsackOptimization::AntColonyKnapsackOptimization(double alpha, double beta, double evaporation)
    : m_impl(std::make_unique<Impl>(alpha, beta, evaporation))
{
}

AntColonyKnapsackOptimization::~AntColonyKnapsackOptimization()
{
}

void AntColonyKnapsackOptimization::setFxLogger(const Logger& logger)
{
    m_impl->logger = logger;
}

void AntColonyKnapsackOptimization::setItems(const Items& items, int Cap)
{
    m_impl->items = items;
    m_impl->Cap = Cap;
    m_impl->Profit = 0;
}

ACO::Items AntColonyKnapsackOptimization::getItems() const
{
    return m_impl->items;
}

int AntColonyKnapsackOptimization::getProfit() const
{
    return m_impl->Profit;
}

void AntColonyKnapsackOptimization::run(int colonySize)
{
    m_impl->run(colonySize);
}
