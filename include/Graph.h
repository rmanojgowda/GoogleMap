#pragma once
// ============================================================
//  Graph.h  —  Adjacency-list weighted undirected graph
// ============================================================
//
//  The Graph is the backbone of GMap.  It holds:
//    • A list of City nodes
//    • A list of Edge records
//    • An adjacency list so neighbour look-up is O(degree)
//
//  All mutating operations validate their arguments and throw
//  std::invalid_argument on bad input so the UI layer can
//  catch and display a friendly message.
// ============================================================

#include "Types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

class Graph
{
public:
    // ── Construction ─────────────────────────────────────────

    Graph() = default;

    // ── City management ──────────────────────────────────────

    /// Add a new city.  Returns the assigned id.
    /// Throws if a city with the same name already exists.
    int addCity(const std::string &name, double lat = 0.0, double lon = 0.0);

    /// Look up a city by name (case-sensitive).
    /// Returns -1 if not found.
    int findCity(const std::string &name) const;

    /// Same as findCity but throws std::invalid_argument if not found.
    int requireCity(const std::string &name) const;

    // ── Road management ──────────────────────────────────────

    /// Add an undirected road between two cities (by id).
    /// Throws if either city id is out of range or distance ≤ 0.
    void addRoad(int fromId, int toId, double distance);

    /// Convenience overload: look cities up by name first.
    void addRoad(const std::string &from, const std::string &to, double distance);

    // ── Accessors ────────────────────────────────────────────

    int cityCount() const { return (int)cities_.size(); }
    int edgeCount() const { return (int)edges_.size(); } // undirected pairs

    const City &city(int id) const { return cities_.at(id); }
    const std::vector<City> &cities() const { return cities_; }
    const std::vector<Edge> &edges() const { return edges_; }

    /// Neighbours of city id: vector of (neighbourId, distance)
    const std::vector<std::pair<int, double>> &neighbours(int id) const;

    /// Complete N×N distance matrix (INF = no direct road).
    /// Useful for TSP algorithms that need random access.
    std::vector<std::vector<double>> distanceMatrix() const;

    // ── Display helpers ──────────────────────────────────────
    void printCities() const;
    void printRoads() const;

private:
    std::vector<City> cities_;
    std::vector<Edge> edges_;                              // unique undirected pairs
    std::vector<std::vector<std::pair<int, double>>> adj_; // adj_[id] = {(neighbour, dist), ...}
    std::unordered_map<std::string, int> nameIndex_;       // name → id

    void validateId(int id) const;
};