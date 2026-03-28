#pragma once
// ============================================================
//  Algorithms.h  —  All shortest-path algorithms + TSP
// ============================================================
//
//  THREE shortest-path algorithms are provided so they can be
//  compared side-by-side:
//
//  ┌─────────────────────────────────────────────────────────┐
//  │ 1. DIJKSTRA                                             │
//  │    Classic single-source shortest path.                 │
//  │    Explores cities in order of distance from source.    │
//  │    Guaranteed optimal.                                  │
//  │    Time : O((V + E) log V)                              │
//  │    Nodes: explores ALL cities within radius d           │
//  ├─────────────────────────────────────────────────────────┤
//  │ 2. A* (A-STAR)                                          │
//  │    Dijkstra + a heuristic function h(n).                │
//  │    h(n) = straight-line (Haversine) distance from n     │
//  │           to the destination.                           │
//  │    Instead of expanding the city with lowest g(n),      │
//  │    it expands the city with lowest f(n) = g(n) + h(n).  │
//  │      g(n) = actual road distance from source to n       │
//  │      h(n) = estimated straight-line distance n → dst    │
//  │    Because h(n) is admissible (never overestimates),    │
//  │    A* is also guaranteed optimal.                       │
//  │    Time : O((V + E) log V)  worst case = Dijkstra       │
//  │    Nodes: explores FAR FEWER cities in practice         │
//  ├─────────────────────────────────────────────────────────┤
//  │ 3. BIDIRECTIONAL DIJKSTRA                               │
//  │    Runs two simultaneous Dijkstra searches:             │
//  │      Forward  : from source toward destination          │
//  │      Backward : from destination toward source          │
//  │    Stops when the two frontiers meet.                   │
//  │    Because each search covers radius ≈ d/2, the total   │
//  │    nodes explored ≈ 2 * π*(d/2)² = half of one-way.    │
//  │    Time : O((V + E) log V)  but with ~2x speedup        │
//  │    Nodes: roughly half of plain Dijkstra                │
//  └─────────────────────────────────────────────────────────┘
//
//  All three return the SAME distance — we verify this in the
//  benchmark output.  The difference is purely efficiency.
// ============================================================

#include "Types.h"
#include "Graph.h"
#include <vector>

// ── 1. Dijkstra ───────────────────────────────────────────────
namespace Dijkstra
{
    ShortestPathResult shortestPath(const Graph &g, int src, int dst);
    AlgorithmStats shortestPathStats(const Graph &g, int src, int dst);

    std::vector<std::vector<double>> allPairsSubset(
        const Graph &g, const std::vector<int> &subset);
}

// ── 2. A* ─────────────────────────────────────────────────────
namespace AStar
{
    // Haversine straight-line distance between two cities (km).
    // This is the admissible heuristic h(n):
    //   • Never overestimates  → A* remains optimal
    //   • Consistent           → each city processed at most once
    double heuristic(const Graph &g, int from, int to);

    ShortestPathResult shortestPath(const Graph &g, int src, int dst);
    AlgorithmStats shortestPathStats(const Graph &g, int src, int dst);
}

// ── 3. Bidirectional Dijkstra ─────────────────────────────────
namespace BiDijkstra
{
    ShortestPathResult shortestPath(const Graph &g, int src, int dst);
    AlgorithmStats shortestPathStats(const Graph &g, int src, int dst);
}

// ── Benchmark: run all three and print a comparison table ─────
namespace Benchmark
{
    // Run Dijkstra, A*, and BiDi on the same query and print
    // a formatted table showing distance, nodes explored, time.
    void compare(const Graph &g, int src, int dst);
}

// ── Road Trip Planner (TSP) ───────────────────────────────────
namespace TripPlanner
{
    RoadTripResult planTrip(const Graph &g, const std::vector<int> &cityIds);
    double tourDistance(const std::vector<int> &tour,
                        const std::vector<std::vector<double>> &dist);
}