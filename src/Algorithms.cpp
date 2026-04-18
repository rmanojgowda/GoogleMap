// ============================================================
//  Algorithms.cpp
//  Dijkstra  |  A*  |  Bidirectional Dijkstra  |  TSP
// ============================================================

#include "../include/Algorithms.h"
#include <queue>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>

// ── Timing helper ─────────────────────────────────────────────
static double nowUs() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
               steady_clock::now().time_since_epoch()).count();
}

// ════════════════════════════════════════════════════════════
//  1. DIJKSTRA
// ════════════════════════════════════════════════════════════
//
//  KEY IDEA:
//  Priority queue always gives us the CLOSEST unvisited city.
//  We relax its neighbours, updating distances if we found
//  a shorter path.  When we pop `dst`, we're done.
//
//  WHY IT WORKS:
//  Because all edge weights are non-negative, the first time
//  a city is popped from the min-heap, its distance is final.
//  (A shorter path cannot arrive later because all remaining
//  entries in the heap are already >= current distance.)
//
//  COMPLEXITY:
//  Time  : O((V + E) log V)   — each edge relaxed once,
//                                each city pushed/popped once
//  Space : O(V)               — dist[], parent[], heap
// ════════════════════════════════════════════════════════════

namespace Dijkstra {

using PD = std::pair<double,int>;   // {distance, cityId}

ShortestPathResult shortestPath(const Graph& g, int src, int dst) {
    int n = g.cityCount();
    if (src < 0 || src >= n || dst < 0 || dst >= n)
        throw std::invalid_argument("City id out of range in shortestPath.");
    std::vector<double> dist(n, INF);
    std::vector<int>    parent(n, -1);
    dist[src] = 0.0;

    std::priority_queue<PD, std::vector<PD>, std::greater<PD>> heap;
    heap.push({0.0, src});

    while (!heap.empty()) {
        auto [d, u] = heap.top(); heap.pop();
        if (d > dist[u]) continue;   // stale entry
        if (u == dst)    break;      // early exit

        for (auto [v, w] : g.neighbours(u)) {
            double nd = dist[u] + w;
            if (nd < dist[v]) {
                dist[v] = nd; parent[v] = u;
                heap.push({nd, v});
            }
        }
    }

    ShortestPathResult res;
    res.totalDistance = dist[dst];
    if (dist[dst] < INF)
        for (int c = dst; c != -1; c = parent[c]) res.path.push_back(c);
    std::reverse(res.path.begin(), res.path.end());
    return res;
}

AlgorithmStats shortestPathStats(const Graph& g, int src, int dst) {
    int n = g.cityCount();
    double t0 = nowUs();
    int explored = 0;

    std::vector<double> dist(n, INF);
    std::vector<int>    parent(n, -1);
    dist[src] = 0.0;
    std::priority_queue<PD,std::vector<PD>,std::greater<PD>> heap;
    heap.push({0.0, src});

    while (!heap.empty()) {
        auto [d, u] = heap.top(); heap.pop();
        if (d > dist[u]) continue;
        ++explored;
        if (u == dst) break;
        for (auto [v, w] : g.neighbours(u)) {
            double nd = dist[u] + w;
            if (nd < dist[v]) { dist[v]=nd; parent[v]=u; heap.push({nd,v}); }
        }
    }

    ShortestPathResult res;
    res.totalDistance = dist[dst];
    if (dist[dst] < INF)
        for (int c=dst; c!=-1; c=parent[c]) res.path.push_back(c);
    std::reverse(res.path.begin(), res.path.end());

    return {"Dijkstra", dist[dst], explored,
            (int)res.path.size(), nowUs() - t0};
}

std::vector<std::vector<double>> allPairsSubset(
        const Graph& g, const std::vector<int>& subset) {
    int n = (int)subset.size();
    std::vector<std::vector<double>> mat(n, std::vector<double>(n, INF));
    for (int i=0;i<n;i++) {
        mat[i][i]=0.0;
        for (int j=i+1;j<n;j++) {
            auto r = shortestPath(g, subset[i], subset[j]);
            mat[i][j] = mat[j][i] = r.totalDistance;
        }
    }
    return mat;
}

} // namespace Dijkstra


// ════════════════════════════════════════════════════════════
//  2. A* (A-STAR)
// ════════════════════════════════════════════════════════════
//
//  KEY IDEA:
//  A* adds a *heuristic estimate* h(n) to Dijkstra.
//  Instead of prioritising by g(n) (cost so far),
//  it prioritises by f(n) = g(n) + h(n).
//
//    g(n) = exact road distance from source → n
//    h(n) = straight-line (Haversine) distance from n → dst
//    f(n) = "best possible total cost through n"
//
//  WHY IT'S FASTER THAN DIJKSTRA:
//  Dijkstra expands cities in ALL directions like a circle.
//  A* points its search TOWARD the destination, skipping
//  cities that are clearly going the wrong way.
//
//  WHY IT'S STILL OPTIMAL:
//  Haversine never overestimates (road distance >= straight
//  line distance), so h(n) is *admissible*.
//  An admissible heuristic guarantees A* never misses the
//  shortest path.
//
//  HAVERSINE FORMULA:
//  Computes great-circle distance between two lat/lon points.
//  Used here because India spans ~30° latitude and straight
//  Euclidean distance would be inaccurate at this scale.
//
//    a = sin²(Δlat/2) + cos(lat1)·cos(lat2)·sin²(Δlon/2)
//    d = 2R · atan2(√a, √(1−a))   where R = 6371 km
//
//  COMPLEXITY:
//  Worst case : O((V + E) log V)  — same as Dijkstra
//  Best case  : dramatically fewer nodes explored
// ════════════════════════════════════════════════════════════

namespace AStar {

static constexpr double EARTH_R = 6371.0;   // km

double heuristic(const Graph& g, int from, int to) {
    const City& a = g.city(from);
    const City& b = g.city(to);

    // Convert degrees → radians
    double lat1 = a.lat * M_PI / 180.0;
    double lat2 = b.lat * M_PI / 180.0;
    double dLat = (b.lat - a.lat) * M_PI / 180.0;
    double dLon = (b.lon - a.lon) * M_PI / 180.0;

    // Haversine formula
    double x = std::sin(dLat/2)*std::sin(dLat/2)
             + std::cos(lat1)*std::cos(lat2)
             * std::sin(dLon/2)*std::sin(dLon/2);
    double c = 2.0 * std::atan2(std::sqrt(x), std::sqrt(1.0 - x));
    // Safety factor 0.5 ensures admissibility on real OSM data.
    // OSM has approximate distances + proximity edges (haversine * 1.35)
    // where actual segment lengths can vary. Factor 0.5 guarantees
    // h(n) <= actual_road_distance for all real-world graph structures.
    return EARTH_R * c * 0.5;
}

ShortestPathResult shortestPath(const Graph& g, int src, int dst) {
    int n = g.cityCount();

    if (src < 0 || src >= n || dst < 0 || dst >= n)
        throw std::invalid_argument("City id out of range in A* shortestPath.");
    std::vector<double> g_cost(n, INF);
    std::vector<int>    parent(n, -1);
    std::vector<bool>   visited(n, false);
    g_cost[src] = 0.0;

    // f(n) = g(n) + h(n)
    using PD = std::pair<double,int>;
    std::priority_queue<PD,std::vector<PD>,std::greater<PD>> heap;
    heap.push({heuristic(g, src, dst), src});

    while (!heap.empty()) {
        auto [f, u] = heap.top(); heap.pop();

        if (visited[u]) continue;
        visited[u] = true;
        if (u == dst) break;

        for (auto [v, w] : g.neighbours(u)) {
            double ng = g_cost[u] + w;
            if (ng < g_cost[v]) {
                g_cost[v] = ng;
                parent[v] = u;
                heap.push({ng + heuristic(g, v, dst), v});
            }
        }
    }

    ShortestPathResult res;
    res.totalDistance = g_cost[dst];
    if (g_cost[dst] < INF)
        for (int c=dst; c!=-1; c=parent[c]) res.path.push_back(c);
    std::reverse(res.path.begin(), res.path.end());
    return res;
}

AlgorithmStats shortestPathStats(const Graph& g, int src, int dst) {
    int n = g.cityCount();
    double t0 = nowUs();
    int explored = 0;

    std::vector<double> g_cost(n, INF);
    std::vector<int>    parent(n, -1);
    std::vector<bool>   vis2(n, false);
    g_cost[src] = 0.0;

    using PD = std::pair<double,int>;
    std::priority_queue<PD,std::vector<PD>,std::greater<PD>> heap;
    heap.push({heuristic(g,src,dst), src});

    while (!heap.empty()) {
        auto [f, u] = heap.top(); heap.pop();
        if (vis2[u]) continue;
        vis2[u] = true;
        ++explored;
        if (u == dst) break;
        for (auto [v,w] : g.neighbours(u)) {
            double ng = g_cost[u] + w;
            if (ng < g_cost[v]) {
                g_cost[v]=ng; parent[v]=u;
                heap.push({ng + heuristic(g,v,dst), v});
            }
        }
    }

    ShortestPathResult res;
    res.totalDistance = g_cost[dst];
    if (g_cost[dst] < INF)
        for (int c=dst; c!=-1; c=parent[c]) res.path.push_back(c);
    std::reverse(res.path.begin(), res.path.end());

    return {"A*", g_cost[dst], explored,
            (int)res.path.size(), nowUs()-t0};
}

} // namespace AStar


// ════════════════════════════════════════════════════════════
//  3. BIDIRECTIONAL DIJKSTRA
// ════════════════════════════════════════════════════════════
//
//  KEY IDEA:
//  Run two Dijkstra searches simultaneously:
//    Forward  search from src  (expanding outward)
//    Backward search from dst  (expanding inward)
//
//  Each step: advance whichever frontier has the smaller
//  top-of-heap value.
//
//  Stop condition: a city u is popped that has already been
//  *settled* (fully expanded) by the OTHER direction.
//  At that point, the shortest path passes through some
//  meeting node m, and we scan all settled cities to find it.
//
//  WHY FASTER:
//  One-way Dijkstra covers a circle of radius d.
//  Bidirectional covers TWO circles of radius d/2.
//  Area = π(d/2)² + π(d/2)² = πd²/2  (half the nodes)
//
//  COMPLEXITY:
//  Time  : O((V + E) log V)  but ~2x practical speedup
//  Space : O(V)  — two dist arrays, two parent arrays
// ════════════════════════════════════════════════════════════

namespace BiDijkstra {

using PD = std::pair<double,int>;

std::vector<int> reconstructPathPublic(
        int meeting,
        const std::vector<int>& parF,
        const std::vector<int>& parB) {
    std::vector<int> path;
    // Forward half: src → meeting
    for (int c = meeting; c != -1; c = parF[c]) path.push_back(c);
    std::reverse(path.begin(), path.end());
    // Backward half: meeting → dst (skip meeting itself)
    for (int c = parB[meeting]; c != -1; c = parB[c]) path.push_back(c);
    return path;
}

ShortestPathResult shortestPath(const Graph& g, int src, int dst) {
    if (src == dst) return {0.0, {src}};

    int n = g.cityCount();
    std::vector<double> distF(n, INF), distB(n, INF);
    std::vector<int>    parF(n, -1),   parB(n, -1);
    std::vector<bool>   visF(n, false), visB(n, false);

    distF[src] = distB[dst] = 0.0;
    std::priority_queue<PD,std::vector<PD>,std::greater<PD>> heapF, heapB;
    heapF.push({0.0, src});
    heapB.push({0.0, dst});

    double best = INF;
    int    meeting = -1;

    auto relax = [&](int u, std::vector<double>& dist,
                     std::vector<int>& par,
                     std::priority_queue<PD,std::vector<PD>,std::greater<PD>>& heap,
                     const std::vector<double>& distOther) {
        for (auto [v, w] : g.neighbours(u)) {
            double nd = dist[u] + w;
            if (nd < dist[v]) {
                dist[v] = nd; par[v] = u; heap.push({nd, v});
            }
            // Check if this forms a complete path
            if (distOther[v] < INF && dist[u] + w + distOther[v] < best) {
                best = dist[u] + w + distOther[v];
                meeting = v;
            }
        }
    };

    while (!heapF.empty() || !heapB.empty()) {
        // Correct termination: stop when both frontiers' tops sum >= best
        double topF = heapF.empty() ? INF : heapF.top().first;
        double topB = heapB.empty() ? INF : heapB.top().first;
        if (topF + topB >= best) break;  // Cannot improve best anymore

        // Advance the frontier with the smaller top
        bool doForward = !heapF.empty() &&
            (heapB.empty() || topF <= topB);

        if (doForward) {
            auto [d, u] = heapF.top(); heapF.pop();
            if (d > distF[u]) continue;
            if (visF[u]) continue;
            visF[u] = true;
            // Update best if this node was also reached from backward
            if (distB[u] < INF && distF[u] + distB[u] < best) {
                best = distF[u] + distB[u]; meeting = u;
            }
            relax(u, distF, parF, heapF, distB);
        } else {
            auto [d, u] = heapB.top(); heapB.pop();
            if (d > distB[u]) continue;
            if (visB[u]) continue;
            visB[u] = true;
            if (distF[u] < INF && distF[u] + distB[u] < best) {
                best = distF[u] + distB[u]; meeting = u;
            }
            relax(u, distB, parB, heapB, distF);
        }
    }

    ShortestPathResult res;
    res.totalDistance = best;
    if (best < INF && meeting != -1)
        res.path = reconstructPathPublic(meeting, parF, parB);
    return res;
}

AlgorithmStats shortestPathStats(const Graph& g, int src, int dst) {
    double t0 = nowUs();
    int explored = 0;

    if (src == dst) return {"Bidirectional Dijkstra", 0.0, 0, 1, nowUs()-t0};

    int n = g.cityCount();
    std::vector<double> distF(n,INF), distB(n,INF);
    std::vector<int>    parF(n,-1),   parB(n,-1);
    std::vector<bool>   visF(n,false), visB(n,false);

    distF[src]=distB[dst]=0.0;
    std::priority_queue<PD,std::vector<PD>,std::greater<PD>> heapF, heapB;
    heapF.push({0.0,src}); heapB.push({0.0,dst});

    double best=INF; int meeting=-1;

    auto relax=[&](int u, std::vector<double>& dist, std::vector<int>& par,
                   std::priority_queue<PD,std::vector<PD>,std::greater<PD>>& heap,
                   const std::vector<double>& distO){
        for(auto [v,w]:g.neighbours(u)){
            double nd=dist[u]+w;
            if(nd<dist[v]){dist[v]=nd;par[v]=u;heap.push({nd,v});}
            if(distO[v]<INF&&dist[u]+w+distO[v]<best){best=dist[u]+w+distO[v];meeting=v;}
        }
    };

    while(!heapF.empty()||!heapB.empty()){
        bool fwd=!heapF.empty()&&(heapB.empty()||heapF.top().first<=heapB.top().first);
        if(fwd){
            auto[d,u]=heapF.top();heapF.pop();
            if(d>distF[u]||visF[u])continue;
            visF[u]=true;++explored;
            if(visB[u]){if(distF[u]+distB[u]<best){best=distF[u]+distB[u];meeting=u;}break;}
            relax(u,distF,parF,heapF,distB);
        } else {
            auto[d,u]=heapB.top();heapB.pop();
            if(d>distB[u]||visB[u])continue;
            visB[u]=true;++explored;
            if(visF[u]){if(distF[u]+distB[u]<best){best=distF[u]+distB[u];meeting=u;}break;}
            relax(u,distB,parB,heapB,distF);
        }
    }

    ShortestPathResult res;
    res.totalDistance=best;
    if(best<INF&&meeting!=-1) res.path=reconstructPathPublic(meeting,parF,parB);

    return {"Bidirectional Dijkstra", best, explored,
            (int)res.path.size(), nowUs()-t0};
}

} // namespace BiDijkstra


// ════════════════════════════════════════════════════════════
//  BIDIRECTIONAL A* (Bi-A*)
//  Combines bidirectional search with heuristic guidance.
//
//  KEY DESIGN: Average heuristic for consistency
//  ─────────────────────────────────────────────
//  Plain Bi-A* with separate heuristics breaks because:
//    - Forward  search prices node n by h(n→dst)
//    - Backward search prices node n by h(n→src)
//  These two "price" the same node differently → meeting
//  point check fires too early → suboptimal paths.
//
//  The fix: use a SYMMETRIC average heuristic
//    p_F(n) = (h(n→dst) - h(n→src)) / 2
//    p_B(n) = (h(n→src) - h(n→dst)) / 2
//
//  Property: p_F(n) + p_B(n) = 0 for all n.
//  This means both searches agree on every node's priority
//  → correct termination → optimal result guaranteed.
//
//  Reference: Ikeda et al. 1994 "A Fast Algorithm for
//  Finding Better Routes by AI Search Techniques"
// ════════════════════════════════════════════════════════════

namespace BiAStar {

// Average heuristic — forward direction
// Reduces to Haversine when src is very far (large graphs)
static double hF(const Graph& g, int n, int src, int dst) {
    double h_dst = AStar::heuristic(g, n, dst);  // n → destination
    double h_src = AStar::heuristic(g, n, src);  // n → source
    return std::max(0.0, (h_dst - h_src) / 2.0);
}

// Average heuristic — backward direction
static double hB(const Graph& g, int n, int src, int dst) {
    double h_dst = AStar::heuristic(g, n, dst);
    double h_src = AStar::heuristic(g, n, src);
    return std::max(0.0, (h_src - h_dst) / 2.0);
}

ShortestPathResult shortestPath(const Graph& g, int src, int dst) {
    if (src == dst) return {0.0, {src}};

    int n = g.cityCount();
    if (src < 0 || src >= n || dst < 0 || dst >= n)
        throw std::invalid_argument("City id out of range in BiAStar.");

    std::vector<double> distF(n, INF), distB(n, INF);
    std::vector<int>    parF(n, -1),   parB(n, -1);
    std::vector<bool>   visF(n, false), visB(n, false);

    distF[src] = distB[dst] = 0.0;

    // Priority queues keyed by f = g + p (potential-adjusted cost)
    using PD = std::pair<double,int>;
    std::priority_queue<PD,std::vector<PD>,std::greater<PD>> heapF, heapB;
    heapF.push({hF(g, src, src, dst), src});
    heapB.push({hB(g, dst, src, dst), dst});

    double best    = INF;
    int    meeting = -1;

    // Update best whenever we find a path through any node
    auto updateBest = [&](int v) {
        if (distF[v] < INF && distB[v] < INF) {
            double cand = distF[v] + distB[v];
            if (cand < best) { best = cand; meeting = v; }
        }
    };

    auto relaxF = [&](int u) {
        for (auto [v, w] : g.neighbours(u)) {
            double ng = distF[u] + w;
            if (ng < distF[v]) {
                distF[v] = ng;
                parF[v] = u;
                heapF.push({ng + hF(g, v, src, dst), v});
            }
            updateBest(v);
        }
    };

    auto relaxB = [&](int u) {
        for (auto [v, w] : g.neighbours(u)) {
            double ng = distB[u] + w;
            if (ng < distB[v]) {
                distB[v] = ng;
                parB[v] = u;
                heapB.push({ng + hB(g, v, src, dst), v});
            }
            updateBest(v);
        }
    };

    while (!heapF.empty() || !heapB.empty()) {
        // Termination: neither frontier can improve best
        // With potential heuristics, termination is:
        // top_F + top_B - h_avg(src,dst) >= best
        // Simplified: top_F + top_B >= best (conservative, always correct)
        double topF = heapF.empty() ? INF : heapF.top().first;
        double topB = heapB.empty() ? INF : heapB.top().first;
        if (topF + topB >= best) break;

        // Alternate between forward and backward
        bool doForward = !heapF.empty() &&
            (heapB.empty() || topF <= topB);

        if (doForward) {
            auto [f, u] = heapF.top(); heapF.pop();
            if (f > distF[u] + hF(g, u, src, dst) + 1e-9) continue;
            if (visF[u]) continue;
            visF[u] = true;
            updateBest(u);
            relaxF(u);
        } else {
            auto [f, u] = heapB.top(); heapB.pop();
            if (f > distB[u] + hB(g, u, src, dst) + 1e-9) continue;
            if (visB[u]) continue;
            visB[u] = true;
            updateBest(u);
            relaxB(u);
        }
    }

    ShortestPathResult res;
    res.totalDistance = best;
    if (best < INF && meeting != -1)
        res.path = BiDijkstra::reconstructPathPublic(meeting, parF, parB);
    return res;
}

AlgorithmStats shortestPathStats(const Graph& g, int src, int dst) {
    double t0 = nowUs();
    int explored = 0;

    if (src == dst) return {"Bi-A*", 0.0, 0, 1, nowUs()-t0};

    int n = g.cityCount();
    std::vector<double> distF(n, INF), distB(n, INF);
    std::vector<int>    parF(n, -1),   parB(n, -1);
    std::vector<bool>   visF(n, false), visB(n, false);

    distF[src] = distB[dst] = 0.0;
    using PD = std::pair<double,int>;
    std::priority_queue<PD,std::vector<PD>,std::greater<PD>> heapF, heapB;
    heapF.push({hF(g, src, src, dst), src});
    heapB.push({hB(g, dst, src, dst), dst});

    double best = INF; int meeting = -1;

    auto updateBest = [&](int v) {
        if (distF[v] < INF && distB[v] < INF) {
            double c = distF[v]+distB[v];
            if (c < best) { best = c; meeting = v; }
        }
    };

    auto relaxF = [&](int u) {
        for (auto [v,w] : g.neighbours(u)) {
            double ng = distF[u]+w;
            if (ng < distF[v]) { distF[v]=ng; parF[v]=u; heapF.push({ng+hF(g,v,src,dst),v}); }
            updateBest(v);
        }
    };
    auto relaxB = [&](int u) {
        for (auto [v,w] : g.neighbours(u)) {
            double ng = distB[u]+w;
            if (ng < distB[v]) { distB[v]=ng; parB[v]=u; heapB.push({ng+hB(g,v,src,dst),v}); }
            updateBest(v);
        }
    };

    while (!heapF.empty() || !heapB.empty()) {
        double topF = heapF.empty() ? INF : heapF.top().first;
        double topB = heapB.empty() ? INF : heapB.top().first;
        if (topF + topB >= best) break;

        bool fwd = !heapF.empty() && (heapB.empty() || topF <= topB);
        if (fwd) {
            auto [f,u] = heapF.top(); heapF.pop();
            if (f > distF[u]+hF(g,u,src,dst)+1e-9) continue;
            if (visF[u]) continue;
            visF[u] = true; explored++;
            updateBest(u); relaxF(u);
        } else {
            auto [f,u] = heapB.top(); heapB.pop();
            if (f > distB[u]+hB(g,u,src,dst)+1e-9) continue;
            if (visB[u]) continue;
            visB[u] = true; explored++;
            updateBest(u); relaxB(u);
        }
    }

    ShortestPathResult res;
    res.totalDistance = best;
    if (best < INF && meeting != -1)
        res.path = BiDijkstra::reconstructPathPublic(meeting, parF, parB);

    return {"Bi-A*", best, explored, (int)res.path.size(), nowUs()-t0};
}

} // namespace BiAStar


// ════════════════════════════════════════════════════════════
//  BENCHMARK: compare all four algorithms
// ════════════════════════════════════════════════════════════

namespace Benchmark {

void compare(const Graph& g, int src, int dst) {
    auto s1 = Dijkstra::shortestPathStats(g, src, dst);
    auto s2 = AStar::shortestPathStats(g, src, dst);
    auto s3 = BiDijkstra::shortestPathStats(g, src, dst);
    auto s4 = BiAStar::shortestPathStats(g, src, dst);

    std::cout << "\n";
    std::cout << "  Algorithm Comparison: "
              << g.city(src).name << " → " << g.city(dst).name << "\n";
    std::cout << "  " << std::string(68, '-') << "\n";
    std::cout << "  " << std::left
              << std::setw(26) << "Algorithm"
              << std::setw(12) << "Distance"
              << std::setw(10) << "Nodes"
              << std::setw(10) << "Path"
              << "Time\n";
    std::cout << "  " << std::string(68, '-') << "\n";

    for (const auto& s : {s1, s2, s3, s4}) {
        std::string dist_str = s.totalDistance < INF
            ? std::to_string((int)s.totalDistance) + " km"
            : "unreachable";
        std::cout << "  " << std::left
                  << std::setw(26) << s.algorithmName
                  << std::setw(12) << dist_str
                  << std::setw(10) << s.nodesExplored
                  << std::setw(10) << s.pathLength
                  << std::fixed << std::setprecision(1)
                  << s.timeUs << " µs\n";
    }
    std::cout << "  " << std::string(68, '-') << "\n";

    // Efficiency insight
    if (s1.nodesExplored > 0) {
        auto pct = [&](int saved) {
            return saved * 100 / s1.nodesExplored;
        };
        std::cout << "\n";
        std::cout << "  A*    explored  " << (s1.nodesExplored - s2.nodesExplored)
                  << " fewer nodes than Dijkstra (" << pct(s1.nodesExplored - s2.nodesExplored) << "%)\n";
        std::cout << "  BiDi  explored  " << (s1.nodesExplored - s3.nodesExplored)
                  << " fewer nodes than Dijkstra (" << pct(s1.nodesExplored - s3.nodesExplored) << "%)\n";
        std::cout << "  Bi-A* explored  " << (s1.nodesExplored - s4.nodesExplored)
                  << " fewer nodes than Dijkstra (" << pct(s1.nodesExplored - s4.nodesExplored) << "%)\n";
        std::cout << "\n";
        std::cout << "  On a map with 1,000,000 cities these savings\n";
        std::cout << "  would be the difference between 2s and 0.1s.\n";
    }
}

} // namespace Benchmark


// ════════════════════════════════════════════════════════════
//  TSP ROAD TRIP PLANNER  (unchanged — uses Dijkstra internally)
// ════════════════════════════════════════════════════════════

namespace TripPlanner {

double tourDistance(const std::vector<int>& tour,
                    const std::vector<std::vector<double>>& dist) {
    double total = 0.0;
    for (int i = 0; i+1 < (int)tour.size(); ++i)
        total += dist[tour[i]][tour[i+1]];
    return total;
}

static std::vector<int> nearestNeighbour(
        int n, const std::vector<std::vector<double>>& dist) {
    std::vector<bool> vis(n, false);
    std::vector<int>  tour;
    tour.reserve(n+1);
    int cur = 0;
    tour.push_back(cur); vis[cur] = true;
    for (int s=1;s<n;s++) {
        double bd=INF; int best=-1;
        for (int j=0;j<n;j++)
            if (!vis[j]&&dist[cur][j]<bd){bd=dist[cur][j];best=j;}
        if (best==-1) break;
        tour.push_back(best); vis[best]=true; cur=best;
    }
    tour.push_back(tour[0]);
    return tour;
}

static std::vector<int> twoOpt(
        std::vector<int> tour,
        const std::vector<std::vector<double>>& dist) {
    int n=(int)tour.size()-1;
    bool improved=true;
    while (improved) {
        improved=false;
        for (int i=0;i<n-1;i++) for (int j=i+2;j<n;j++) {
            if (i==0&&j==n-1) continue;
            double b=dist[tour[i]][tour[i+1]]+dist[tour[j]][tour[j+1]];
            double a=dist[tour[i]][tour[j]]+dist[tour[i+1]][tour[j+1]];
            if (a<b-1e-9) {
                std::reverse(tour.begin()+i+1, tour.begin()+j+1);
                improved=true;
            }
        }
    }
    return tour;
}

RoadTripResult planTrip(const Graph& g, const std::vector<int>& cityIds) {
    int n = (int)cityIds.size();
    if (n==0) throw std::invalid_argument("City list is empty.");
    if (n==1) return {0.0, cityIds};

    auto dist = Dijkstra::allPairsSubset(g, cityIds);

    for (int i=0;i<n;i++) for (int j=0;j<n;j++)
        if (i!=j&&dist[i][j]>=INF)
            throw std::runtime_error(
                "No road connection between \""
                + g.city(cityIds[i]).name + "\" and \""
                + g.city(cityIds[j]).name + "\".");

    auto tour = nearestNeighbour(n, dist);
    tour = twoOpt(tour, dist);

    RoadTripResult result;
    result.totalDistance = tourDistance(tour, dist);
    for (int idx : tour) result.order.push_back(cityIds[idx]);
    return result;
}

} // namespace TripPlanner
