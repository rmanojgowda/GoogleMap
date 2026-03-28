#pragma once
// ============================================================
//  Types.h  —  Core data types for GMap
// ============================================================

#include <string>
#include <vector>
#include <limits>

// ── Sentinel: "infinity" distance ──────────────────────────
constexpr double INF = std::numeric_limits<double>::infinity();

// ── A city node ────────────────────────────────────────────
struct City
{
    int id;
    std::string name;
    double lat; // Geographic latitude  (used by A* heuristic)
    double lon; // Geographic longitude
};

// ── A directed weighted edge ───────────────────────────────
struct Edge
{
    int from;
    int to;
    double distance; // Road distance in kilometres
};

// ── Result returned by any shortest-path algorithm ─────────
struct ShortestPathResult
{
    double totalDistance;  // INF if unreachable
    std::vector<int> path; // City ids source → destination
    bool reachable() const { return totalDistance < INF; }
};

// ── Result returned by the road-trip planner ───────────────
struct RoadTripResult
{
    double totalDistance;
    std::vector<int> order;
};

// ── Per-run stats: lets us benchmark Dijkstra vs A* vs BiDi ─
//
//  nodesExplored = cities popped from the priority queue.
//  This is the KEY efficiency metric:
//    Dijkstra explores every city up to distance d from source.
//    A*       skips cities that are in the "wrong direction".
//    BiDi     meets in the middle, cutting the search radius in half.
//  Fewer nodes = faster on large real-world graphs.
struct AlgorithmStats
{
    std::string algorithmName; // "Dijkstra" | "A*" | "Bidirectional Dijkstra"
    double totalDistance;      // km
    int nodesExplored;         // priority-queue pops
    int pathLength;            // cities in path
    double timeUs;             // microseconds wall-clock
};