// ============================================================
//  main.cpp  —  GMap: A command-line map & trip planner
// ============================================================
//
//  Features
//  ─────────
//  1. Shortest path between any two cities  (Dijkstra)
//  2. Optimal road trip for a list of cities (NN + 2-opt TSP)
//  3. Add your own city / road
//  4. View the full map
//
//  Architecture
//  ────────────
//  main.cpp  ──► Graph (adjacency list)
//            ──► Algorithms: Dijkstra | TripPlanner
//            ──► MapLoader (preloaded India data)
// ============================================================

#include <iostream>
#include <iomanip>
#include <sstream>
#include "../include/MapLoader_OSM.h"
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

#include "../include/Graph.h"
#include "../include/Algorithms.h"
#include "../include/MapLoader.h"
#include "../include/MapLoader_Large.h"

// ── Terminal colour helpers (ANSI — works on modern Windows too) ──
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define CYAN "\033[36m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RED "\033[31m"
#define BLUE "\033[34m"

// ── Utility: trim whitespace ──────────────────────────────────
static std::string trim(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// ── Utility: ask user for a city name, validate it ───────────
static int askCity(const Graph &g, const std::string &prompt)
{
    while (true)
    {
        std::cout << "  " << prompt << ": ";
        std::string name;
        std::getline(std::cin, name);
        name = trim(name);
        int id = g.findCity(name);
        if (id != -1)
            return id;
        std::cout << RED << "  City \"" << name << "\" not found. Try again.\n"
                  << RESET;
    }
}

// ── Print a pretty horizontal rule ───────────────────────────
static void rule(bool heavy = false, int w = 56)
{
    std::string unit = heavy ? "\xe2\x95\x90" : "\xe2\x94\x80"; // UTF-8: ═  or  ─
    std::cout << "  ";
    for (int i = 0; i < w; ++i)
        std::cout << unit;
    std::cout << "\n";
}

// ════════════════════════════════════════════════════════════
//  Feature 1 — SHORTEST PATH
// ════════════════════════════════════════════════════════════
static void featureShortestPath(const Graph &g)
{
    std::cout << "\n"
              << CYAN << BOLD;
    rule(true);
    std::cout << "   SHORTEST PATH FINDER\n";
    rule(true);
    std::cout << RESET;

    int src = askCity(g, "From city");
    int dst = askCity(g, "To   city");

    if (src == dst)
    {
        std::cout << YELLOW << "\n  You are already there!\n"
                  << RESET;
        return;
    }

    std::cout << "\n  Calculating...\n";

    auto result = Dijkstra::shortestPath(g, src, dst);

    if (!result.reachable())
    {
        std::cout << RED << "\n  No road connection found between "
                  << g.city(src).name << " and "
                  << g.city(dst).name << ".\n"
                  << RESET;
        return;
    }

    // ── Print route ──────────────────────────────────────────
    std::cout << "\n"
              << GREEN << BOLD
              << "  Shortest route found!\n"
              << RESET;
    rule();

    std::cout << "  Route:\n\n";
    for (int i = 0; i < (int)result.path.size(); ++i)
    {
        const std::string &name = g.city(result.path[i]).name;

        if (i == 0)
        {
            // Starting city
            std::cout << "  " << BOLD << "  📍 " << name << RESET << "\n";
        }
        else
        {
            // Find the direct edge distance between consecutive stops
            // (for display — the path may use indirect edges)
            auto prev = result.path[i - 1];
            double segDist = 0.0;
            for (auto [nb, d] : g.neighbours(prev))
                if (nb == result.path[i])
                {
                    segDist = d;
                    break;
                }

            std::cout << "      │\n";
            if (segDist > 0)
                std::cout << "      │  " << YELLOW
                          << std::fixed << std::setprecision(0)
                          << segDist << " km" << RESET << "\n";
            std::cout << "      ▼\n";

            bool isLast = (i == (int)result.path.size() - 1);
            std::cout << "  " << BOLD << "  " << (isLast ? "🏁" : "🔵")
                      << " " << name << RESET << "\n";
        }
    }

    rule();
    std::cout << "  " << BOLD << GREEN
              << "  Total distance : "
              << std::fixed << std::setprecision(1)
              << result.totalDistance << " km\n"
              << RESET;
    std::cout << "  Stops          : " << result.path.size() << " cities\n";
    rule();
}

// ════════════════════════════════════════════════════════════
//  Feature 2 — ROAD TRIP PLANNER
// ════════════════════════════════════════════════════════════
static void featureRoadTrip(const Graph &g)
{
    std::cout << "\n"
              << CYAN << BOLD;
    rule(true);
    std::cout << "   OPTIMAL ROAD TRIP PLANNER\n";
    rule(true);
    std::cout << RESET;

    std::cout << "\n  Enter city names one per line.\n"
              << "  Type  DONE  when finished (minimum 2 cities).\n\n";

    std::vector<int> cityIds;

    while (true)
    {
        std::cout << "  City " << (cityIds.size() + 1) << ": ";
        std::string name;
        std::getline(std::cin, name);
        name = trim(name);

        if (name == "DONE" || name == "done")
        {
            if (cityIds.size() < 2)
                std::cout << YELLOW << "  Please enter at least 2 cities.\n"
                          << RESET;
            else
                break;
        }
        else
        {
            int id = g.findCity(name);
            if (id == -1)
            {
                std::cout << RED << "  \"" << name << "\" not found. Try again.\n"
                          << RESET;
            }
            else
            {
                // Avoid duplicates
                if (std::find(cityIds.begin(), cityIds.end(), id) != cityIds.end())
                    std::cout << YELLOW << "  Already added. Skipping.\n"
                              << RESET;
                else
                {
                    cityIds.push_back(id);
                    std::cout << GREEN << "  ✓ Added " << name << "\n"
                              << RESET;
                }
            }
        }
    }

    std::cout << "\n  Planning optimal route for "
              << cityIds.size() << " cities...\n";

    RoadTripResult trip;
    try
    {
        trip = TripPlanner::planTrip(g, cityIds);
    }
    catch (const std::exception &e)
    {
        std::cout << RED << "\n  Error: " << e.what() << "\n"
                  << RESET;
        return;
    }

    // ── Print trip plan ──────────────────────────────────────
    std::cout << "\n"
              << GREEN << BOLD
              << "  Optimal road trip found!\n"
              << RESET;
    rule();

    std::cout << "  Itinerary (round trip):\n\n";
    for (int i = 0; i < (int)trip.order.size(); ++i)
    {
        const std::string &name = g.city(trip.order[i]).name;
        bool isLast = (i == (int)trip.order.size() - 1);
        bool isFirst = (i == 0);

        if (isFirst && isLast)
        {
            std::cout << "  " << BOLD << "  📍 " << name << RESET << "\n";
        }
        else if (isFirst)
        {
            std::cout << "  " << BOLD << "  🏠 " << name << RESET << "  (Start / End)\n";
        }
        else if (isLast)
        {
            std::cout << "      │\n";
            std::cout << "      ▼\n";
            std::cout << "  " << BOLD << "  🏠 " << name << RESET << "  (Back to start)\n";
        }
        else
        {
            std::cout << "      │\n";
            std::cout << "      ▼\n";
            std::cout << "  " << BOLD << "  📍 " << i << ". " << name << RESET << "\n";
        }
    }

    rule();
    std::cout << "  " << BOLD << GREEN
              << "  Total trip distance : "
              << std::fixed << std::setprecision(1)
              << trip.totalDistance << " km\n"
              << RESET;
    std::cout << "  Cities visited       : "
              << (trip.order.size() - 1) << "\n"; // -1 because start==end
    rule();

    std::cout << "\n  Algorithm used  : Nearest-Neighbour + 2-opt improvement\n"
              << "  Note            : Exact TSP is NP-hard. This gives a\n"
              << "                    near-optimal solution in polynomial time.\n";
}

// ════════════════════════════════════════════════════════════
//  Feature 3 — ADD CITY / ROAD
// ════════════════════════════════════════════════════════════
static void featureAddCity(Graph &g)
{
    std::cout << "\n  Enter new city name: ";
    std::string name;
    std::getline(std::cin, name);
    name = trim(name);
    try
    {
        int id = g.addCity(name);
        std::cout << GREEN << "  ✓ City \"" << name
                  << "\" added with id " << id << ".\n"
                  << RESET;
    }
    catch (const std::exception &e)
    {
        std::cout << RED << "  Error: " << e.what() << "\n"
                  << RESET;
    }
}

static void featureAddRoad(Graph &g)
{
    std::cout << "\n";
    int a = askCity(g, "First city");
    int b = askCity(g, "Second city");
    std::cout << "  Distance (km): ";
    double dist;
    std::string line;
    std::getline(std::cin, line);
    try
    {
        dist = std::stod(trim(line));
        g.addRoad(a, b, dist);
        std::cout << GREEN << "  ✓ Road added: "
                  << g.city(a).name << " ↔ " << g.city(b).name
                  << " (" << dist << " km).\n"
                  << RESET;
    }
    catch (const std::exception &e)
    {
        std::cout << RED << "  Error: " << e.what() << "\n"
                  << RESET;
    }
}

// ── Forward declarations ──────────────────────────────────────
static void featureShortestPath(const Graph &g);
static void featureAStar(const Graph &g);
static void featureBenchmark(const Graph &g);
static void featureRoadTrip(const Graph &g);
static void featureAddCity(Graph &g);
static void featureAddRoad(Graph &g);

// ════════════════════════════════════════════════════════════
//  MAIN MENU
// ════════════════════════════════════════════════════════════
static void printMenu()
{
    std::cout << "\n"
              << BOLD << BLUE;
    rule(true);
    std::cout << "   🗺   G M A P  —  India Road Network\n";
    rule(true);
    std::cout << RESET;
    std::cout << "\n"
              << "   1.  Shortest path  (Dijkstra)\n"
              << "   2.  Shortest path  (A*  — faster, same result)\n"
              << "   3.  Benchmark      (Dijkstra vs A* vs Bidirectional)\n"
              << "   4.  Plan optimal road trip\n"
              << "   5.  View all cities\n"
              << "   6.  View all roads\n"
              << "   7.  Add a new city\n"
              << "   8.  Add a new road\n"
              << "   0.  Exit\n\n"
              << "  Enter choice: ";
}

int main()
{
    // Enable ANSI colours on Windows 10+
#ifdef _WIN32
    system("chcp 65001 > nul"); // UTF-8 code page
    system("");                 // enable VT processing
#endif

    std::cout << BOLD << CYAN
              << "\n  ╔══════════════════════════════════════╗\n"
              << "  ║       Welcome to GMap  v2.0          ║\n"
              << "  ║  Dijkstra | A* | Bidirectional       ║\n"
              << "  ╚══════════════════════════════════════╝\n"
              << RESET;

    Graph g;

    // ── Choose map size ──────────────────────────────────────
    std::cout << "\n  Choose map dataset:\n"
              << "   1.  Small  (  31 cities,    56 roads) — quick demo\n"
              << "   2.  Large  ( 268 cities,   465 roads) — real India scale\n"
              << "   3.  OSM    (5000 cities,  9536 roads) — real OpenStreetMap\n\n"
              << "  Enter choice [1/2/3]: ";

    std::string mapChoice;
    std::getline(std::cin, mapChoice);

    if (trim(mapChoice) == "2")
    {
        loadIndiaMapLarge(g);
        std::cout << GREEN << "  Large map loaded.\n"
                  << RESET;
    }
    else if (trim(mapChoice) == "3")
    {
        loadIndiaMapOSM(g);
        std::cout << GREEN << "  OSM map loaded — 5000 real Indian cities!\n"
                  << RESET;
    }
    else
    {
        loadIndiaMap(g);
    }

    std::string input;
    while (true)
    {
        printMenu();
        std::getline(std::cin, input);
        input = trim(input);

        if (input.empty())
            continue;
        char choice = input[0];

        switch (choice)
        {
        case '1':
            featureShortestPath(g);
            break;
        case '2':
            featureAStar(g);
            break;
        case '3':
            featureBenchmark(g);
            break;
        case '4':
            featureRoadTrip(g);
            break;
        case '5':
            g.printCities();
            break;
        case '6':
            g.printRoads();
            break;
        case '7':
            featureAddCity(g);
            break;
        case '8':
            featureAddRoad(g);
            break;
        case '0':
            std::cout << "\n  Goodbye! Safe travels. 🚗\n\n";
            return 0;
        default:
            std::cout << YELLOW << "  Unknown option. Please enter 0-8.\n"
                      << RESET;
        }
    }
}

// ════════════════════════════════════════════════════════════
//  Feature: A* SHORTEST PATH
// ════════════════════════════════════════════════════════════
static void featureAStar(const Graph &g)
{
    std::cout << "\n"
              << CYAN << BOLD;
    std::cout << "  ══════════════════════════════════════════════════════\n";
    std::cout << "   A* SHORTEST PATH\n";
    std::cout << "  ══════════════════════════════════════════════════════\n";
    std::cout << RESET;

    int src = askCity(g, "From city");
    int dst = askCity(g, "To   city");
    if (src == dst)
    {
        std::cout << YELLOW << "\n  Already there!\n"
                  << RESET;
        return;
    }

    std::cout << "\n  Running A*...\n";
    auto result = AStar::shortestPath(g, src, dst);

    if (!result.reachable())
    {
        std::cout << RED << "\n  No route found.\n"
                  << RESET;
        return;
    }

    std::cout << "\n"
              << GREEN << BOLD << "  A* route found!\n"
              << RESET;
    std::cout << "  ──────────────────────────────────────────────────────\n";
    for (int i = 0; i < (int)result.path.size(); i++)
    {
        bool isLast = (i == (int)result.path.size() - 1);
        std::cout << "  " << BOLD << (i == 0 ? "  📍 " : (isLast ? "  🏁 " : "  🔵 "))
                  << g.city(result.path[i]).name << RESET << "\n";
        if (!isLast)
        {
            auto prev = result.path[i];
            double seg = 0;
            for (auto [nb, d] : g.neighbours(prev))
                if (nb == result.path[i + 1])
                {
                    seg = d;
                    break;
                }
            std::cout << "      │  " << YELLOW << (int)seg << " km" << RESET << "\n";
            std::cout << "      ▼\n";
        }
    }
    std::cout << "  ──────────────────────────────────────────────────────\n";
    std::cout << "  " << BOLD << GREEN
              << "  Total: " << (int)result.totalDistance << " km\n"
              << RESET;
}

// ════════════════════════════════════════════════════════════
//  Feature: BENCHMARK — Dijkstra vs A* vs Bidirectional
// ════════════════════════════════════════════════════════════
static void featureBenchmark(const Graph &g)
{
    std::cout << "\n"
              << CYAN << BOLD;
    std::cout << "  ══════════════════════════════════════════════════════\n";
    std::cout << "   ALGORITHM BENCHMARK\n";
    std::cout << "   Dijkstra  vs  A*  vs  Bidirectional Dijkstra\n";
    std::cout << "  ══════════════════════════════════════════════════════\n";
    std::cout << RESET;

    std::cout << "\n  This runs all 3 algorithms on the same query\n"
              << "  and shows you exactly why A* and Bidirectional\n"
              << "  are faster — fewer nodes explored = less work.\n\n";

    int src = askCity(g, "From city");
    int dst = askCity(g, "To   city");
    if (src == dst)
    {
        std::cout << YELLOW << "\n  Same city!\n"
                  << RESET;
        return;
    }

    Benchmark::compare(g, src, dst);
}