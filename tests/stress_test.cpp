// stress_simple.cpp - simplified stress test that avoids stack issues
#include "../include/Graph.h"
#include "../include/Algorithms.h"
#include "../include/MapLoader.h"
#include "../include/MapLoader_Large.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <fstream>

#define GREEN  "\033[32m"
#define RED    "\033[31m"
#define YELLOW "\033[33m"
#define CYAN   "\033[36m"
#define BOLD   "\033[1m"
#define RESET  "\033[0m"

static int PASS=0, FAIL=0;

void check(bool c, const std::string& msg){
    if(c){ ++PASS; std::cout<<GREEN<<"  PASS "<<RESET<<msg<<"\n"; }
    else { ++FAIL; std::cout<<RED  <<"  FAIL "<<RESET<<msg<<"\n"; }
}
void section(const std::string& s){
    std::cout<<"\n"<<BOLD<<CYAN<<"  == "<<s<<" =="<<RESET<<"\n";
}
void note(const std::string& s){
    std::cout<<YELLOW<<"  >> "<<RESET<<s<<"\n";
}

static double nowUs(){
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}

int main(){
#ifdef _WIN32
    system("chcp 65001 > nul"); system("");
#endif
    std::cout<<BOLD<<CYAN<<"\n  GMap Stress Test Suite — Phase 10\n"<<RESET<<"\n";

    // ==========================================================
    // SUITE 1: EDGE CASES
    // ==========================================================
    section("1. EDGE CASES");

    // 1a. Same city
    {
        auto g = std::make_unique<Graph>(); loadIndiaMap(*g);
        int d = g->requireCity("Delhi");
        auto r = Dijkstra::shortestPath(*g,d,d);
        check(r.totalDistance==0.0,  "Same city: Dijkstra distance = 0");
        auto ra= AStar::shortestPath(*g,d,d);
        check(ra.totalDistance==0.0, "Same city: A* distance = 0");
        auto rb= BiDijkstra::shortestPath(*g,d,d);
        check(rb.totalDistance==0.0, "Same city: BiDi distance = 0");
    }

    // 1b. Two cities, no road
    {
        auto g = std::make_unique<Graph>();
        g->addCity("Island1",10,10); g->addCity("Island2",20,20);
        auto r = Dijkstra::shortestPath(*g,0,1);
        check(!r.reachable(),  "Disconnected: Dijkstra → unreachable");
        auto ra= AStar::shortestPath(*g,0,1);
        check(!ra.reachable(), "Disconnected: A* → unreachable");
        auto rb= BiDijkstra::shortestPath(*g,0,1);
        check(!rb.reachable(), "Disconnected: BiDi → unreachable");
    }

    // 1c. Direct edge
    {
        auto g = std::make_unique<Graph>();
        g->addCity("A",0,0); g->addCity("B",0,1);
        g->addRoad(0,1,100);
        auto r = Dijkstra::shortestPath(*g,0,1);
        check(r.totalDistance==100.0, "Direct edge: exact distance = 100");
        check((int)r.path.size()==2,  "Direct edge: path has 2 cities");
    }

    // 1d. Known India route
    {
        auto g = std::make_unique<Graph>(); loadIndiaMap(*g);
        int mum=g->requireCity("Mumbai"), nag=g->requireCity("Nagpur");
        auto r = Dijkstra::shortestPath(*g,mum,nag);
        check(r.reachable(),         "Mumbai→Nagpur: reachable");
        check(r.path.front()==mum,   "Mumbai→Nagpur: path starts at Mumbai");
        check(r.path.back()==nag,    "Mumbai→Nagpur: path ends at Nagpur");
    }

    // ==========================================================
    // SUITE 2: GRAPH STRUCTURES
    // ==========================================================
    section("2. GRAPH STRUCTURES");

    // 2a. Linear chain
    {
        auto g = std::make_unique<Graph>();
        for(int i=0;i<100;i++) g->addCity("C"+std::to_string(i),i*0.1,0);
        for(int i=0;i+1<100;i++) g->addRoad(i,i+1,10);
        auto r = Dijkstra::shortestPath(*g,0,99);
        check(r.reachable(),              "Linear chain (100): reachable");
        check(r.totalDistance==990.0,     "Linear chain: distance = 990km");
        auto ra= AStar::shortestPath(*g,0,99);
        auto rb= BiDijkstra::shortestPath(*g,0,99);
        bool agree = std::fabs(r.totalDistance-ra.totalDistance)<1.0 &&
                     std::fabs(r.totalDistance-rb.totalDistance)<1.0;
        check(agree, "Linear chain: Dijkstra = A* = BiDi");
        note("Linear chain = worst case for Dijkstra (must visit every node)");
    }

    // 2b. Star graph
    {
        auto g = std::make_unique<Graph>();
        g->addCity("Hub",20,77);
        for(int i=0;i<50;i++){
            g->addCity("S"+std::to_string(i),20+i*0.01,77+i*0.01);
            g->addRoad(0,i+1,100);
        }
        auto r = Dijkstra::shortestPath(*g,1,50);
        check(r.reachable(),           "Star graph (50 spokes): reachable");
        check(r.totalDistance==200.0,  "Star graph: spoke→hub→spoke = 200km");
        note("Star graph: every path bottlenecks through the hub");
    }

    // 2c. Parallel paths
    {
        auto g = std::make_unique<Graph>();
        g->addCity("A",0,0); g->addCity("B",0,1);
        g->addCity("C",1,0.5); g->addCity("D",0,2);
        g->addRoad(0,1,100); g->addRoad(1,3,100); // short: 200
        g->addRoad(0,2,150); g->addRoad(2,3,200); // long:  350
        auto r = Dijkstra::shortestPath(*g,0,3);
        check(r.totalDistance==200.0, "Parallel paths: shorter route chosen (200 < 350)");
        auto ra= AStar::shortestPath(*g,0,3);
        auto rb= BiDijkstra::shortestPath(*g,0,3);
        check(std::fabs(r.totalDistance-ra.totalDistance)<1.0 &&
              std::fabs(r.totalDistance-rb.totalDistance)<1.0,
              "Parallel paths: all 3 agree on correct route");
    }

    // ==========================================================
    // SUITE 3: HIGH VOLUME (1000 queries)
    // ==========================================================
    section("3. HIGH VOLUME STRESS");
    {
        auto gp = std::make_unique<Graph>();
        loadIndiaMapLarge(*gp);
        Graph& g = *gp;
        int V = g.cityCount();
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> dist(0,V-1);

        int mismatches=0, unreachable=0, queries=0;
        double t0 = nowUs();

        for(int q=0; q<1000; q++){
            int src=dist(rng), dst=dist(rng);
            if(src==dst) continue;
            auto rd = Dijkstra::shortestPath(g,src,dst);
            auto ra = AStar::shortestPath(g,src,dst);
            auto rb = BiDijkstra::shortestPath(g,src,dst);
            if(!rd.reachable()){ unreachable++; continue; }
            queries++;
            if(std::fabs(rd.totalDistance-ra.totalDistance)>1.0 ||
               std::fabs(rd.totalDistance-rb.totalDistance)>1.0)
                mismatches++;
        }

        double ms = (nowUs()-t0)/1000.0;
        check(mismatches==0, "1000 queries (268 cities): 0 correctness mismatches");
        note("Completed in "+std::to_string((int)ms)+" ms · "+
             std::to_string((int)(ms*1000/std::max(queries,1)))+" µs avg");
        note(std::to_string(unreachable)+" unreachable pairs");

        // Repeated same query
        int delhi=g.requireCity("Delhi"), blr=g.requireCity("Bengaluru");
        double t1=nowUs();
        for(int i=0;i<100;i++) Dijkstra::shortestPath(g,delhi,blr);
        double avg=(nowUs()-t1)/100.0;
        check(avg<5000.0, "100x Delhi→Bengaluru: each < 5ms");
        note("Avg: "+std::to_string((int)avg)+" µs per query");
    }

    // ==========================================================
    // SUITE 4: ERROR HANDLING
    // ==========================================================
    section("4. ERROR HANDLING");

    {
        auto g = std::make_unique<Graph>();
        g->addCity("X",10,70);
        bool threw=false;
        try { g->addCity("X",11,71); }
        catch(const std::invalid_argument&){ threw=true; }
        check(threw, "Duplicate city name → throws invalid_argument");
    }
    {
        auto g = std::make_unique<Graph>();
        g->addCity("A",10,70);
        bool threw=false;
        try { g->addRoad(0,0,100); }
        catch(const std::invalid_argument&){ threw=true; }
        check(threw, "Self-loop road → throws invalid_argument");
    }
    {
        auto g = std::make_unique<Graph>();
        g->addCity("A",10,70); g->addCity("B",12,72);
        bool threw=false;
        try { g->addRoad(0,1,-50); }
        catch(const std::invalid_argument&){ threw=true; }
        check(threw, "Negative distance → throws invalid_argument");
    }
    {
        auto g = std::make_unique<Graph>();
        g->addCity("A",10,70); g->addCity("B",12,72);
        bool threw=false;
        try { g->addRoad(0,1,0); }
        catch(const std::invalid_argument&){ threw=true; }
        check(threw, "Zero distance → throws invalid_argument");
    }
    {
        auto g = std::make_unique<Graph>();
        g->addCity("Real",10,70);
        check(g->findCity("Ghost")==-1, "findCity unknown → returns -1");
        bool threw=false;
        try { g->requireCity("Ghost"); }
        catch(const std::invalid_argument&){ threw=true; }
        check(threw, "requireCity unknown → throws invalid_argument");
    }
    {
        auto g = std::make_unique<Graph>();
        g->addCity("A",10,70); g->addCity("B",12,72); g->addRoad(0,1,100);
        bool threw=false;
        try { TripPlanner::planTrip(*g,{}); }
        catch(const std::invalid_argument&){ threw=true; }
        check(threw, "Empty planTrip → throws invalid_argument");
    }
    {
        auto g = std::make_unique<Graph>();
        g->addCity("A",10,70); g->addCity("B",12,72); g->addRoad(0,1,100);
        bool threw=false;
        try { Dijkstra::shortestPath(*g,0,999); }
        catch(const std::invalid_argument&){ threw=true; }
        check(threw, "Out-of-range city ID → throws invalid_argument");
    }

    // ==========================================================
    // SUITE 5: WORST CASE
    // ==========================================================
    section("5. WORST CASE ANALYSIS");
    {
        auto gp = std::make_unique<Graph>();
        loadIndiaMapLarge(*gp);
        Graph& g = *gp;
        int V = g.cityCount();
        std::mt19937 rng(99);
        std::uniform_int_distribution<int> dist(0,V-1);

        int worstSrc=-1, worstDst=-1, maxNodes=0;
        for(int i=0;i<300;i++){
            int s=dist(rng), d=dist(rng);
            if(s==d) continue;
            auto st=Dijkstra::shortestPathStats(g,s,d);
            if(st.nodesExplored>maxNodes){
                maxNodes=st.nodesExplored; worstSrc=s; worstDst=d;
            }
        }

        if(worstSrc>=0){
            auto ds=Dijkstra::shortestPathStats(g,worstSrc,worstDst);
            auto as=AStar::shortestPathStats(g,worstSrc,worstDst);
            auto bs=BiDijkstra::shortestPathStats(g,worstSrc,worstDst);
            note("Worst query: "+g.city(worstSrc).name+" → "+g.city(worstDst).name);
            note("Dijkstra: "+std::to_string(ds.nodesExplored)+" nodes, "+
                 std::to_string((int)ds.timeUs)+" µs");
            note("A*:       "+std::to_string(as.nodesExplored)+" nodes, "+
                 std::to_string((int)as.timeUs)+" µs");
            note("BiDi:     "+std::to_string(bs.nodesExplored)+" nodes, "+
                 std::to_string((int)bs.timeUs)+" µs");
            check(as.nodesExplored<=ds.nodesExplored,
                  "Worst case: A* explores ≤ Dijkstra nodes");
            check(std::fabs(ds.totalDistance-as.totalDistance)<1.0,
                  "Worst case: A* still correct on hardest query");
            int pct=ds.nodesExplored>0?
                (ds.nodesExplored-as.nodesExplored)*100/ds.nodesExplored:0;
            note("A* saves "+std::to_string(pct)+"% even on worst query");
        }

        // TSP worst case
        auto g2=std::make_unique<Graph>(); loadIndiaMap(*g2);
        std::vector<int> ten; for(int i=0;i<10;i++) ten.push_back(i);
        double t0=nowUs();
        auto trip=TripPlanner::planTrip(*g2,ten);
        double elapsed=nowUs()-t0;
        check(trip.totalDistance>0,                 "TSP 10 cities: positive distance");
        check((int)trip.order.size()==11,            "TSP 10 cities: 11-stop round trip");
        check(trip.order.front()==trip.order.back(), "TSP: starts and ends at same city");
        note("TSP 10 cities: "+std::to_string((int)elapsed)+" µs");
    }

    // ==========================================================
    // SUITE 6: EXHAUSTIVE ALL-PAIRS (small graph)
    // ==========================================================
    section("6. EXHAUSTIVE ALL-PAIRS (31 cities)");
    {
        auto gp=std::make_unique<Graph>(); loadIndiaMap(*gp);
        Graph& g=*gp;
        int V=g.cityCount(), mismatches=0, asymmetric=0, reachable=0;

        for(int i=0;i<V;i++) for(int j=0;j<V;j++){
            if(i==j) continue;
            auto rd=Dijkstra::shortestPath(g,i,j);
            auto ra=AStar::shortestPath(g,i,j);
            auto rb=BiDijkstra::shortestPath(g,i,j);
            if(!rd.reachable()) continue;
            reachable++;
            if(std::fabs(rd.totalDistance-ra.totalDistance)>1.0 ||
               std::fabs(rd.totalDistance-rb.totalDistance)>1.0)
                mismatches++;
        }
        for(int i=0;i<V;i++) for(int j=i+1;j<V;j++){
            auto r1=Dijkstra::shortestPath(g,i,j);
            auto r2=Dijkstra::shortestPath(g,j,i);
            if(r1.reachable()&&r2.reachable())
                if(std::fabs(r1.totalDistance-r2.totalDistance)>1.0)
                    asymmetric++;
        }
        check(mismatches==0,
            std::to_string(reachable)+" pairs: Dijkstra=A*=BiDi (exhaustive)");
        check(asymmetric==0,
            "All paths symmetric: dist(A→B) = dist(B→A)");
        note(std::to_string(reachable)+"/"+std::to_string(V*(V-1))+" pairs reachable");
    }

    // ==========================================================
    // SUMMARY
    // ==========================================================
    int total=PASS+FAIL;
    std::cout<<"\n"<<BOLD<<"  ══════════════════════════════════\n"
             <<"  STRESS TEST RESULTS\n"
             <<"  ══════════════════════════════════\n"<<RESET;
    if(FAIL==0)
        std::cout<<GREEN<<BOLD<<"  ALL "<<total<<" TESTS PASSED\n"<<RESET;
    else
        std::cout<<RED<<"  "<<FAIL<<" FAILED / "<<total<<" total\n"<<RESET;

    // Write report
    std::ofstream f("D:/GoogleMap/data/stress_report.txt");
    if(!f) f.open("stress_report.txt");
    if(f){
        f<<"GMap Stress Test Report\n"
         <<"========================\n"
         <<"Passed: "<<PASS<<"/"<<total<<"\n"
         <<"Failed: "<<FAIL<<"/"<<total<<"\n";
        std::cout<<GREEN<<"  Report saved: stress_report.txt\n"<<RESET;
    }
    std::cout<<"\n";
    return FAIL==0?0:1;
}
