#pragma once
// ============================================================
//  MapLoader.h  —  Preloaded India road network
// ============================================================
//
//  This module seeds the Graph with real Indian cities and
//  approximate road distances (in km via NH highways).
//  The network covers major cities across all regions so
//  every demo feature works out of the box.
// ============================================================

#include "../include/Graph.h"
#include <iostream>

// Loads a curated set of Indian cities and highway connections.
inline void loadIndiaMap(Graph &g)
{

    // ── Cities (name, lat, lon) ──────────────────────────────
    // South India
    g.addCity("Bengaluru", 12.97, 77.59);
    g.addCity("Chennai", 13.08, 80.27);
    g.addCity("Hyderabad", 17.38, 78.48);
    g.addCity("Kochi", 9.93, 76.27);
    g.addCity("Mysuru", 12.29, 76.64);
    g.addCity("Mangaluru", 12.86, 74.84);
    g.addCity("Coimbatore", 11.00, 76.96);
    g.addCity("Visakhapatnam", 17.69, 83.21);
    g.addCity("Madurai", 9.92, 78.12);
    g.addCity("Vijayawada", 16.50, 80.62);

    // West India
    g.addCity("Mumbai", 19.08, 72.88);
    g.addCity("Pune", 18.52, 73.86);
    g.addCity("Ahmedabad", 23.03, 72.57);
    g.addCity("Surat", 21.17, 72.83);
    g.addCity("Nagpur", 21.14, 79.09);
    g.addCity("Goa", 15.49, 73.82);

    // North India
    g.addCity("Delhi", 28.61, 77.20);
    g.addCity("Jaipur", 26.91, 75.79);
    g.addCity("Agra", 27.18, 78.01);
    g.addCity("Lucknow", 26.85, 80.95);
    g.addCity("Varanasi", 25.32, 82.97);
    g.addCity("Chandigarh", 30.74, 76.79);
    g.addCity("Amritsar", 31.63, 74.87);
    g.addCity("Kanpur", 26.45, 80.35);

    // East India
    g.addCity("Kolkata", 22.57, 88.36);
    g.addCity("Bhubaneswar", 20.30, 85.82);
    g.addCity("Patna", 25.60, 85.14);

    // Central India
    g.addCity("Bhopal", 23.26, 77.40);
    g.addCity("Indore", 22.72, 75.86);
    g.addCity("Raipur", 21.25, 81.63);

    // North-East
    g.addCity("Guwahati", 26.14, 91.74);

    // ── Roads (NH approximate distances in km) ───────────────
    // South India connections
    g.addRoad("Bengaluru", "Chennai", 346);
    g.addRoad("Bengaluru", "Hyderabad", 570);
    g.addRoad("Bengaluru", "Mysuru", 143);
    g.addRoad("Bengaluru", "Mangaluru", 352);
    g.addRoad("Bengaluru", "Coimbatore", 366);
    g.addRoad("Bengaluru", "Kochi", 566);
    g.addRoad("Chennai", "Coimbatore", 498);
    g.addRoad("Chennai", "Madurai", 462);
    g.addRoad("Chennai", "Vijayawada", 435);
    g.addRoad("Chennai", "Visakhapatnam", 791);
    g.addRoad("Hyderabad", "Vijayawada", 275);
    g.addRoad("Hyderabad", "Visakhapatnam", 625);
    g.addRoad("Hyderabad", "Nagpur", 500);
    g.addRoad("Kochi", "Coimbatore", 183);
    g.addRoad("Kochi", "Madurai", 246);
    g.addRoad("Mysuru", "Coimbatore", 215);
    g.addRoad("Mangaluru", "Goa", 381);
    g.addRoad("Mangaluru", "Kochi", 325);
    g.addRoad("Coimbatore", "Madurai", 213);
    g.addRoad("Vijayawada", "Visakhapatnam", 347);

    // South to West
    g.addRoad("Hyderabad", "Pune", 564);
    g.addRoad("Goa", "Pune", 456);
    g.addRoad("Goa", "Mumbai", 590);
    g.addRoad("Nagpur", "Pune", 738);

    // West India connections
    g.addRoad("Mumbai", "Pune", 155);
    g.addRoad("Mumbai", "Surat", 290);
    g.addRoad("Mumbai", "Nagpur", 900);
    g.addRoad("Pune", "Nagpur", 738);
    g.addRoad("Surat", "Ahmedabad", 272);
    g.addRoad("Ahmedabad", "Jaipur", 673);
    g.addRoad("Ahmedabad", "Indore", 397);
    g.addRoad("Nagpur", "Raipur", 296);
    g.addRoad("Nagpur", "Bhopal", 360);

    // West to North
    g.addRoad("Jaipur", "Delhi", 281);
    g.addRoad("Jaipur", "Agra", 240);
    g.addRoad("Bhopal", "Indore", 195);
    g.addRoad("Bhopal", "Delhi", 778);
    g.addRoad("Indore", "Mumbai", 585);

    // North India connections
    g.addRoad("Delhi", "Agra", 233);
    g.addRoad("Delhi", "Chandigarh", 245);
    g.addRoad("Delhi", "Lucknow", 555);
    g.addRoad("Delhi", "Kanpur", 490);
    g.addRoad("Chandigarh", "Amritsar", 237);
    g.addRoad("Agra", "Lucknow", 363);
    g.addRoad("Agra", "Kanpur", 296);
    g.addRoad("Lucknow", "Varanasi", 330);
    g.addRoad("Lucknow", "Kanpur", 82);
    g.addRoad("Kanpur", "Varanasi", 335);
    g.addRoad("Varanasi", "Patna", 292);

    // East India connections
    g.addRoad("Kolkata", "Bhubaneswar", 473);
    g.addRoad("Kolkata", "Patna", 596);
    g.addRoad("Kolkata", "Guwahati", 1031);
    g.addRoad("Bhubaneswar", "Visakhapatnam", 450);
    g.addRoad("Bhubaneswar", "Raipur", 499);
    g.addRoad("Patna", "Varanasi", 292);
    g.addRoad("Raipur", "Kolkata", 700);

    std::cout << "\n  ✅  India road map loaded: "
              << g.cityCount() << " cities, "
              << g.edgeCount() << " roads.\n";
}