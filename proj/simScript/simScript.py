#!/usr/bin/env python3
"""
Symulator trasy dla SCR
- N losowych koordynatów
- Periodyczne Eventy wysyłające aktualizację wagi krawędzi

Dependency: pip install osmnx networkx
"""

import osmnx as ox
import random
import json
import socket
import time
import argparse
from typing import List, Tuple

def get_map_bounds_and_edges(pbf_path: str):
    """Wczytaj pbf, zwróć krawędzie, wymiar"""
    G = ox.graph_from_xml(pbf_path, simplify=True, retain_all=False)
    nodes = G.nodes(data=True)
    lats = [data['y'] for _, data in nodes]
    lons = [data['x'] for _, data in nodes]
    bounds = {
        'north': max(lats),
        'south': min(lats),
        'east': max(lons),
        'west': min(lons)
    }
    edges = list(G.edges(keys=False)) 
    return bounds, edges

def random_coordinate(bounds: dict) -> Tuple[float, float]:
    return (random.uniform(bounds['south'], bounds['north']),
            random.uniform(bounds['west'], bounds['east']))

def send_message(sock: socket.socket, msg: dict):
    sock.sendall((json.dumps(msg) + '\n').encode())

def main():
    parser = argparse.ArgumentParser(description="RTS Event Simulator")
    parser.add_argument("pbf_file", help="Path to .pbf map file")
    parser.add_argument("--points", "-n", type=int, default=5)
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=12345)
    parser.add_argument("--interval", type=float, default=10.0)
    parser.add_argument("--seed", type=int, default=None)
    args = parser.parse_args()

    if args.seed:
        random.seed(args.seed)

    print(f"Loading map from {args.pbf_file} ...")
    bounds, edges = get_map_bounds_and_edges(args.pbf_file)
    print(f"Bounds: {bounds}")
    print(f"Found {len(edges)} directed edges (no routing done).")

    waypoints = [random_coordinate(bounds) for _ in range(args.points)]
    print(f"Waypoints (lat,lon): {waypoints}")

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((args.host, args.port))
    print(f"Connected to {args.host}:{args.port}")

    # Send waypoints
    send_message(sock, {"type": "waypoints", "coordinates": waypoints})
    print("Waypoints sent.")

    # Periodic edge updates
    try:
        while True:
            time.sleep(args.interval)
            edge = random.choice(edges)
            u, v = edge
            new_weight = random.uniform(1.0, 30.0)  # seconds
            send_message(sock, {
                "type": "graph_update",
                "edge": [u, v],
                "new_weight": new_weight
            })
            print(f"Sent update: edge ({u},{v}) -> {new_weight:.1f}s")
    except KeyboardInterrupt:
        print("Stopped.")
    finally:
        sock.close()

if __name__ == "__main__":
    main()