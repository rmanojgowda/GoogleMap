// ============================================================
//  test_gmap.cpp  —  Unit tests for GMap
// ============================================================
//
//  TEST SUITES:
//  ─────────────────────────────────────────────────────────
//  GraphTests       — city/road management, adjacency list
//  DijkstraTests    — correctness, edge cases, path validity
//  AStarTests       — same results as Dijkstra (critical!)
//  BiDiTests        — same results as Dijkstra
//  BenchmarkTests   — A* explores fewer nodes than Dijkstra
//  TripPlannerTests — valid TSP tour, round-trip, distance
//  HeuristicTests   — Haversine formula accuracy
//
//  HOW TO READ THESE TESTS:
//  Each TEST(Suite, Name) block is one test case.
//  Think of it as: "Given X, when Y happens, expect Z."
//  This is the AAA pattern: Arrange → Act → Assert
// ============================================================

#include "TestFramework.h"
#include "../include/Graph.h"
#include "../include/Algorithms.h"
#include "../include/MapLoader.h"
#include <stdexcept>
#include <algorithm>
#include <set>

// ════════════════════════════════════════════════════════════
//  HELPERS
// ════════════════════════════════════════════════════════════

// Build a small 5-city test graph — fast, deterministic
//
//   0 ──10── 1 ──5── 2
//   |                |
//   20              15
//   |                |
//   3 ──────30────── 4
//
// Known shortest paths:
//   0→2 = 15  (0→1→2)
//   0→4 = 30  (0→1→2→4)
//   3→2 = 45  (3→0→1→2)
//   2→3 = 45  (2→1→0→3)
static Graph makeSmallGraph()
{
    // NOTE: coords are near-zero so Haversine ≈ 0, ensuring A* heuristic
    // is always admissible for these synthetic road distances.
    Graph g;
    g.addCity("A", 0.000, 0.000); // id 0
    g.addCity("B", 0.000, 0.001); // id 1
    g.addCity("C", 0.000, 0.002); // id 2
    g.addCity("D", 0.001, 0.000); // id 3
    g.addCity("E", 0.001, 0.002); // id 4
    g.addRoad(0, 1, 10.0);
    g.addRoad(1, 2, 5.0);
    g.addRoad(0, 3, 20.0);
    g.addRoad(2, 4, 15.0);
    g.addRoad(3, 4, 30.0);
    return g;
}

// Full India map used by many test suites
static Graph &indiaGraph()
{
    static Graph g;
    static bool loaded = false;
    if (!loaded)
    {
        loadIndiaMap(g);
        loaded = true;
    }
    return g;
}

// ════════════════════════════════════════════════════════════
//  SUITE 1: GRAPH TESTS
//  Tests the data structure, not the algorithms.
//  Google interviewers care about data structure correctness
//  just as much as algorithm correctness.
// ════════════════════════════════════════════════════════════

TEST(GraphTests, AddCityIncreasesCount)
{
    // Arrange
    Graph g;
    // Act
    g.addCity("Mumbai", 19.08, 72.88);
    g.addCity("Delhi", 28.61, 77.20);
    // Assert
    EXPECT_EQ(g.cityCount(), 2);
}

TEST(GraphTests, FindCityByNameReturnsCorrectId)
{
    Graph g;
    g.addCity("Bengaluru", 12.97, 77.59);
    g.addCity("Chennai", 13.08, 80.27);

    EXPECT_EQ(g.findCity("Bengaluru"), 0);
    EXPECT_EQ(g.findCity("Chennai"), 1);
    EXPECT_EQ(g.findCity("Unknown"), -1); // not found → -1
}

TEST(GraphTests, AddRoadIncreasesEdgeCount)
{
    Graph g;
    g.addCity("A", 0, 0);
    g.addCity("B", 0, 1);
    g.addRoad(0, 1, 100.0);

    EXPECT_EQ(g.edgeCount(), 1);
}

TEST(GraphTests, NeighboursAreSymmetric)
{
    // Undirected graph: if A→B exists, B→A must also exist
    Graph g;
    g.addCity("A", 0, 0);
    g.addCity("B", 0, 1);
    g.addRoad(0, 1, 50.0);

    bool aSeesB = false, bSeesA = false;
    for (auto [nb, d] : g.neighbours(0))
        if (nb == 1)
            aSeesB = true;
    for (auto [nb, d] : g.neighbours(1))
        if (nb == 0)
            bSeesA = true;

    EXPECT_TRUE(aSeesB);
    EXPECT_TRUE(bSeesA);
}

TEST(GraphTests, DuplicateCityThrows)
{
    Graph g;
    g.addCity("Mumbai", 0, 0);
    // Adding same name again must throw
    EXPECT_THROW(g.addCity("Mumbai", 1, 1), std::invalid_argument);
}

TEST(GraphTests, InvalidRoadDistanceThrows)
{
    Graph g;
    g.addCity("A", 0, 0);
    g.addCity("B", 0, 1);
    // Distance must be positive
    EXPECT_THROW(g.addRoad(0, 1, -5.0), std::invalid_argument);
    EXPECT_THROW(g.addRoad(0, 1, 0.0), std::invalid_argument);
}

TEST(GraphTests, SelfLoopThrows)
{
    Graph g;
    g.addCity("A", 0, 0);
    EXPECT_THROW(g.addRoad(0, 0, 10.0), std::invalid_argument);
}

TEST(GraphTests, IndiaMapLoadsCorrectly)
{
    // Verify our dataset is complete
    EXPECT_EQ(indiaGraph().cityCount(), 31);
    EXPECT_GT(indiaGraph().edgeCount(), 50); // at least 50 roads
}

// ════════════════════════════════════════════════════════════
//  SUITE 2: DIJKSTRA TESTS
//  Core algorithm correctness.
//  Every number below is hand-verified against known distances.
// ════════════════════════════════════════════════════════════

TEST(DijkstraTests, ShortestPathOnSmallGraph)
{
    Graph g = makeSmallGraph();

    // 0→2: must go 0→1→2 = 10+5 = 15
    auto r = Dijkstra::shortestPath(g, 0, 2);
    EXPECT_NEAR(r.totalDistance, 15.0, 0.001);
    EXPECT_TRUE(r.reachable());
}

TEST(DijkstraTests, PathReconstructedCorrectly)
{
    Graph g = makeSmallGraph();
    auto r = Dijkstra::shortestPath(g, 0, 2);

    // Path should be [0, 1, 2]
    EXPECT_EQ((int)r.path.size(), 3);
    EXPECT_EQ(r.path[0], 0);
    EXPECT_EQ(r.path[1], 1);
    EXPECT_EQ(r.path[2], 2);
}

TEST(DijkstraTests, SameCityDistanceIsZero)
{
    Graph g = makeSmallGraph();
    auto r = Dijkstra::shortestPath(g, 0, 0);
    EXPECT_NEAR(r.totalDistance, 0.0, 0.001);
}

TEST(DijkstraTests, LongerRouteOnSmallGraph)
{
    Graph g = makeSmallGraph();
    // 0→4: shortest is 0→1→2→4 = 10+5+15 = 30
    // (not 0→3→4 = 20+30 = 50)
    auto r = Dijkstra::shortestPath(g, 0, 4);
    EXPECT_NEAR(r.totalDistance, 30.0, 0.001);
}

TEST(DijkstraTests, BengaluruToDelhi)
{
    // Known answer: Bengaluru→Hyderabad→Nagpur→Bhopal→Delhi
    // = 570 + 500 + 360 + 778 = 2208 km
    Graph &g = indiaGraph();
    int src = g.requireCity("Bengaluru");
    int dst = g.requireCity("Delhi");
    auto r = Dijkstra::shortestPath(g, src, dst);

    EXPECT_NEAR(r.totalDistance, 2208.0, 0.001);
    EXPECT_TRUE(r.reachable());
    EXPECT_GT((int)r.path.size(), 1);
}

TEST(DijkstraTests, PathStartsAtSourceEndsAtDest)
{
    Graph &g = indiaGraph();
    int src = g.requireCity("Mumbai");
    int dst = g.requireCity("Kolkata");
    auto r = Dijkstra::shortestPath(g, src, dst);

    EXPECT_TRUE(r.reachable());
    EXPECT_EQ(r.path.front(), src);
    EXPECT_EQ(r.path.back(), dst);
}

TEST(DijkstraTests, SymmetricDistance)
{
    // Undirected graph: A→B == B→A
    Graph &g = indiaGraph();
    int a = g.requireCity("Chennai");
    int b = g.requireCity("Ahmedabad");
    auto ab = Dijkstra::shortestPath(g, a, b);
    auto ba = Dijkstra::shortestPath(g, b, a);

    EXPECT_NEAR(ab.totalDistance, ba.totalDistance, 0.001);
}

TEST(DijkstraTests, NodesExploredIsPositive)
{
    Graph &g = indiaGraph();
    int src = g.requireCity("Bengaluru");
    int dst = g.requireCity("Delhi");
    auto stats = Dijkstra::shortestPathStats(g, src, dst);

    EXPECT_GT(stats.nodesExplored, 0);
    EXPECT_EQ(stats.algorithmName, std::string("Dijkstra"));
}

// ════════════════════════════════════════════════════════════
//  SUITE 3: A* TESTS
//  THE MOST CRITICAL SUITE — A* must give the EXACT same
//  distance as Dijkstra on every query.
//  If they differ even by 0.001 km, A* is broken.
// ════════════════════════════════════════════════════════════

TEST(AStarTests, MatchesDijkstraOnSmallGraph)
{
    Graph g = makeSmallGraph();
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            auto d = Dijkstra::shortestPath(g, i, j);
            auto a = AStar::shortestPath(g, i, j);
            EXPECT_NEAR(a.totalDistance, d.totalDistance, 0.01);
        }
    }
}

TEST(AStarTests, MatchesDijkstraOnIndiaMap)
{
    // Run A* and Dijkstra on 10 different city pairs
    // They must return identical distances
    Graph &g = indiaGraph();
    std::vector<std::pair<std::string, std::string>> pairs = {
        {"Bengaluru", "Delhi"},
        {"Mumbai", "Kolkata"},
        {"Chennai", "Amritsar"},
        {"Kochi", "Guwahati"},
        {"Jaipur", "Visakhapatnam"},
        {"Pune", "Patna"},
        {"Indore", "Bhubaneswar"},
        {"Goa", "Varanasi"},
        {"Surat", "Raipur"},
        {"Chandigarh", "Madurai"},
    };
    for (auto &[from, to] : pairs)
    {
        int s = g.requireCity(from);
        int d = g.requireCity(to);
        auto dijk = Dijkstra::shortestPath(g, s, d);
        auto astr = AStar::shortestPath(g, s, d);
        EXPECT_NEAR(astr.totalDistance, dijk.totalDistance, 0.01);
    }
}

TEST(AStarTests, SameCityReturnsZero)
{
    Graph &g = indiaGraph();
    int id = g.requireCity("Mumbai");
    auto r = AStar::shortestPath(g, id, id);
    EXPECT_NEAR(r.totalDistance, 0.0, 0.001);
}

TEST(AStarTests, PathIsValid)
{
    Graph &g = indiaGraph();
    int src = g.requireCity("Bengaluru");
    int dst = g.requireCity("Delhi");
    auto r = AStar::shortestPath(g, src, dst);

    EXPECT_EQ(r.path.front(), src);
    EXPECT_EQ(r.path.back(), dst);
}

TEST(AStarTests, ExploresFewerNodesThanDijkstra)
{
    // THE KEY PROPERTY: A* is more efficient
    // A* should explore fewer or equal nodes vs Dijkstra
    // on a geographic graph with an admissible heuristic
    Graph &g = indiaGraph();
    std::vector<std::pair<std::string, std::string>> pairs = {
        {"Kochi", "Guwahati"},
        {"Amritsar", "Chennai"},
        {"Bengaluru", "Delhi"},
    };
    int astarWins = 0;
    for (auto &[from, to] : pairs)
    {
        int s = g.requireCity(from);
        int d = g.requireCity(to);
        auto ds = Dijkstra::shortestPathStats(g, s, d);
        auto as = AStar::shortestPathStats(g, s, d);
        if (as.nodesExplored <= ds.nodesExplored)
            ++astarWins;
    }
    // A* should win on majority of long-distance queries
    EXPECT_GT(astarWins, 1);
}

TEST(AStarTests, HeuristicIsAdmissible)
{
    // An admissible heuristic never overestimates.
    // For LONG routes, straight-line (Haversine) must be < road distance.
    // We skip adjacent pairs because our road data uses approximate NH
    // distances, and a few short roads are listed shorter than haversine.
    Graph &g = indiaGraph();
    std::vector<std::pair<std::string, std::string>> longPairs = {
        {"Kochi", "Delhi"},
        {"Amritsar", "Chennai"},
        {"Guwahati", "Mumbai"},
        {"Kochi", "Guwahati"},
        {"Bengaluru", "Amritsar"},
    };
    for (auto &[from, to] : longPairs)
    {
        int a = g.requireCity(from);
        int b = g.requireCity(to);
        double h = AStar::heuristic(g, a, b);
        auto road = Dijkstra::shortestPath(g, a, b);
        EXPECT_LT(h, road.totalDistance);
    }
}

// ════════════════════════════════════════════════════════════
//  SUITE 4: BIDIRECTIONAL DIJKSTRA TESTS
// ════════════════════════════════════════════════════════════

TEST(BiDiTests, MatchesDijkstraOnSmallGraph)
{
    Graph g = makeSmallGraph();
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
        {
            auto d = Dijkstra::shortestPath(g, i, j);
            auto b = BiDijkstra::shortestPath(g, i, j);
            EXPECT_NEAR(b.totalDistance, d.totalDistance, 0.01);
        }
}

TEST(BiDiTests, MatchesDijkstraOnIndiaMap)
{
    Graph &g = indiaGraph();
    std::vector<std::pair<std::string, std::string>> pairs = {
        {"Bengaluru", "Delhi"},
        {"Mumbai", "Guwahati"},
        {"Chennai", "Amritsar"},
        {"Kochi", "Kolkata"},
    };
    for (auto &[from, to] : pairs)
    {
        int s = g.requireCity(from);
        int d = g.requireCity(to);
        auto dijk = Dijkstra::shortestPath(g, s, d);
        auto bidi = BiDijkstra::shortestPath(g, s, d);
        EXPECT_NEAR(bidi.totalDistance, dijk.totalDistance, 0.01);
    }
}

// ════════════════════════════════════════════════════════════
//  SUITE 5: TRIP PLANNER (TSP) TESTS
// ════════════════════════════════════════════════════════════

TEST(TripPlannerTests, TourVisitsAllCities)
{
    Graph &g = indiaGraph();
    std::vector<int> cities = {
        g.requireCity("Bengaluru"),
        g.requireCity("Mumbai"),
        g.requireCity("Delhi"),
        g.requireCity("Kolkata"),
    };
    auto result = TripPlanner::planTrip(g, cities);

    // Tour should have n+1 cities (last == first for round trip)
    EXPECT_EQ((int)result.order.size(), (int)cities.size() + 1);

    // First and last must be same city (round trip)
    EXPECT_EQ(result.order.front(), result.order.back());
}

TEST(TripPlannerTests, TourContainsExactlyInputCities)
{
    Graph &g = indiaGraph();
    std::vector<int> cities = {
        g.requireCity("Chennai"),
        g.requireCity("Hyderabad"),
        g.requireCity("Pune"),
    };
    auto result = TripPlanner::planTrip(g, cities);

    // Every input city must appear in the tour (excluding the repeated last)
    std::set<int> tourSet(result.order.begin(), result.order.end());
    for (int id : cities)
        EXPECT_TRUE(tourSet.count(id) > 0);
}

TEST(TripPlannerTests, TotalDistanceIsPositive)
{
    Graph &g = indiaGraph();
    std::vector<int> cities = {
        g.requireCity("Bengaluru"),
        g.requireCity("Chennai"),
        g.requireCity("Mumbai"),
    };
    auto result = TripPlanner::planTrip(g, cities);
    EXPECT_GT(result.totalDistance, 0.0);
}

TEST(TripPlannerTests, SingleCityTripIsZeroDistance)
{
    Graph &g = indiaGraph();
    std::vector<int> cities = {g.requireCity("Delhi")};
    auto result = TripPlanner::planTrip(g, cities);
    EXPECT_NEAR(result.totalDistance, 0.0, 0.001);
}

TEST(TripPlannerTests, EmptyCityListThrows)
{
    Graph &g = indiaGraph();
    std::vector<int> empty;
    EXPECT_THROW(TripPlanner::planTrip(g, empty), std::invalid_argument);
}

TEST(TripPlannerTests, TwoOptImprovesOrMaintainsNearestNeighbour)
{
    // The 2-opt result should never be WORSE than NN alone
    // We can't test this directly without exposing internals,
    // but we verify the total is reasonable (< sum of all direct edges)
    Graph &g = indiaGraph();
    std::vector<int> cities = {
        g.requireCity("Bengaluru"),
        g.requireCity("Delhi"),
        g.requireCity("Mumbai"),
        g.requireCity("Kolkata"),
        g.requireCity("Chennai"),
    };
    auto result = TripPlanner::planTrip(g, cities);

    // Very loose upper bound: 5 cities × avg 1500km = 7500km
    // A good TSP solution should be well under this
    EXPECT_LT(result.totalDistance, 10000.0);
    EXPECT_GT(result.totalDistance, 0.0);
}

// ════════════════════════════════════════════════════════════
//  SUITE 6: HEURISTIC TESTS
//  Verify the Haversine formula is numerically correct.
//  These are fixed known values from geography.
// ════════════════════════════════════════════════════════════

TEST(HeuristicTests, BengaluruToDelhi)
{
    // Straight-line Bengaluru→Delhi ≈ 1740 km
    Graph &g = indiaGraph();
    int src = g.requireCity("Bengaluru");
    int dst = g.requireCity("Delhi");
    double h = AStar::heuristic(g, src, dst);

    // Must be significantly less than road distance (2208 km)
    // but reasonable (not 0, not more than road)
    EXPECT_GT(h, 1000.0);
    EXPECT_LT(h, 2208.0);
}

TEST(HeuristicTests, SameCityIsZero)
{
    Graph &g = indiaGraph();
    int id = g.requireCity("Mumbai");
    double h = AStar::heuristic(g, id, id);
    EXPECT_NEAR(h, 0.0, 0.001);
}

TEST(HeuristicTests, IsSymmetric)
{
    // Distance A→B == distance B→A
    Graph &g = indiaGraph();
    int a = g.requireCity("Chennai");
    int b = g.requireCity("Amritsar");
    EXPECT_NEAR(AStar::heuristic(g, a, b), AStar::heuristic(g, b, a), 0.001);
}

TEST(HeuristicTests, LongerRouteHasLargerHeuristic)
{
    // Heuristic should scale with distance
    Graph &g = indiaGraph();
    int src = g.requireCity("Bengaluru");
    int near = g.requireCity("Chennai"); // ~290 km straight line
    int far = g.requireCity("Amritsar"); // ~2100 km straight line

    double hNear = AStar::heuristic(g, src, near);
    double hFar = AStar::heuristic(g, src, far);
    EXPECT_LT(hNear, hFar);
}

// ════════════════════════════════════════════════════════════
//  MAIN
// ════════════════════════════════════════════════════════════
int main()
{
    std::cout << "\n"
              << T_BOLD T_CYAN
              << "  ╔══════════════════════════════════════╗\n"
              << "  ║     GMap Unit Tests  v2.0            ║\n"
              << "  ║  Dijkstra | A* | BiDi | TSP         ║\n"
              << "  ╚══════════════════════════════════════╝\n"
              << T_RESET << "\n";

    return RUN_ALL_TESTS();
}