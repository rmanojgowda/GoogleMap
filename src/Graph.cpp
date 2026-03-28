// ============================================================
//  Graph.cpp  —  Implementation of the adjacency-list graph
// ============================================================

#include "../include/Graph.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

// ── Private helper ───────────────────────────────────────────

void Graph::validateId(int id) const
{
    if (id < 0 || id >= (int)cities_.size())
        throw std::invalid_argument("Invalid city id: " + std::to_string(id));
}

// ── City management ──────────────────────────────────────────

int Graph::addCity(const std::string &name, double lat, double lon)
{
    if (nameIndex_.count(name))
        throw std::invalid_argument("City already exists: " + name);

    int id = (int)cities_.size();
    cities_.push_back({id, name, lat, lon});
    adj_.emplace_back(); // empty neighbour list for new city
    nameIndex_[name] = id;
    return id;
}

int Graph::findCity(const std::string &name) const
{
    auto it = nameIndex_.find(name);
    return (it != nameIndex_.end()) ? it->second : -1;
}

int Graph::requireCity(const std::string &name) const
{
    int id = findCity(name);
    if (id == -1)
        throw std::invalid_argument("Unknown city: \"" + name + "\"");
    return id;
}

// ── Road management ──────────────────────────────────────────

void Graph::addRoad(int fromId, int toId, double distance)
{
    validateId(fromId);
    validateId(toId);
    if (fromId == toId)
        throw std::invalid_argument("Self-loop roads are not allowed.");
    if (distance <= 0.0)
        throw std::invalid_argument("Distance must be positive.");

    // Undirected: add both directions to the adjacency list
    adj_[fromId].emplace_back(toId, distance);
    adj_[toId].emplace_back(fromId, distance);

    // Store one canonical edge record (from < to for deduplication)
    int u = std::min(fromId, toId);
    int v = std::max(fromId, toId);
    edges_.push_back({u, v, distance});
}

void Graph::addRoad(const std::string &from, const std::string &to, double distance)
{
    addRoad(requireCity(from), requireCity(to), distance);
}

// ── Accessors ────────────────────────────────────────────────

const std::vector<std::pair<int, double>> &Graph::neighbours(int id) const
{
    validateId(id);
    return adj_[id];
}

std::vector<std::vector<double>> Graph::distanceMatrix() const
{
    int n = cityCount();
    // Initialise with INF; diagonal = 0
    std::vector<std::vector<double>> mat(n, std::vector<double>(n, INF));
    for (int i = 0; i < n; ++i)
        mat[i][i] = 0.0;

    for (const auto &[u, v, d] : edges_)
    {
        // Keep the shorter road if duplicates exist
        mat[u][v] = std::min(mat[u][v], d);
        mat[v][u] = std::min(mat[v][u], d);
    }
    return mat;
}

// ── Display helpers ──────────────────────────────────────────

void Graph::printCities() const
{
    std::cout << "\n  Cities in the map (" << cityCount() << "):\n";
    std::cout << "  " << std::string(40, '-') << "\n";
    for (const auto &c : cities_)
        std::cout << "  [" << std::setw(2) << c.id << "]  " << c.name << "\n";
    std::cout << "\n";
}

void Graph::printRoads() const
{
    std::cout << "\n  Roads in the map (" << edgeCount() << "):\n";
    std::cout << "  " << std::string(50, '-') << "\n";
    for (const auto &e : edges_)
    {
        std::cout << "  " << std::setw(15) << std::left << cities_[e.from].name
                  << "  <->  "
                  << std::setw(15) << std::left << cities_[e.to].name
                  << "  " << std::fixed << std::setprecision(0)
                  << e.distance << " km\n";
    }
    std::cout << "\n";
}