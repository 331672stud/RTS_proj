#!/usr/bin/env python3
"""
Symulator trasy dla SCR - używa pyosmium do bezpośredniego odczytu .osm.pbf.
"""

import osmium
import networkx as nx
import random
import json
import socket
import time
import argparse
from typing import Tuple, Dict, List

import os
import pickle

class PBFGraphBuilder(osmium.SimpleHandler):
    """Buduje skierowany graf NetworkX z pliku .osm.pbf."""
    def __init__(self):
        super().__init__()
        self.graph = nx.DiGraph()
        self.nodes = {}          # id -> (lon, lat)
        self.ways = []           # lista krawędzi (u, v, length_seconds)

    def node(self, n):
        self.nodes[n.id] = (n.location.lon, n.location.lat)

    def way(self, w):
        if 'highway' not in w.tags:
            return
        tags = dict(w.tags)
        # Domyślna prędkość (km/h) w zależności od typu drogi
        highway_type = tags.get('highway', 'residential')
        speed_kmh = {
            'motorway': 120, 'trunk': 100, 'primary': 70,
            'secondary': 60, 'tertiary': 50, 'residential': 40,
            'living_street': 20, 'service': 30
        }.get(highway_type, 50)
        speed_mps = speed_kmh / 3.6

        # Iteruj po kolejnych parach węzłów (tworzy krawędzie skierowane)
        node_ids = [n.ref for n in w.nodes]
        for i in range(len(node_ids) - 1):
            u = node_ids[i]
            v = node_ids[i+1]
            if u in self.nodes and v in self.nodes:
                lon1, lat1 = self.nodes[u]
                lon2, lat2 = self.nodes[v]
                # Odcinek w metrach (aproksymacja sferyczna)
                from math import radians, sin, cos, sqrt, asin
                R = 6371000
                phi1, phi2 = radians(lat1), radians(lat2)
                dphi = radians(lat2 - lat1)
                dlam = radians(lon2 - lon1)
                a = sin(dphi/2)**2 + cos(phi1)*cos(phi2)*sin(dlam/2)**2
                dist = 2 * R * asin(sqrt(a))
                time_sec = dist / speed_mps if speed_mps > 0 else 0.001
                self.graph.add_edge(u, v, length=time_sec, geometry=None)
                # Dodaj również krawędź w przeciwnym kierunku (dla ruchu dwukierunkowego)
                if 'oneway' not in tags or tags['oneway'] != 'yes':
                    self.graph.add_edge(v, u, length=time_sec, geometry=None)

def get_bounds_and_edges_from_pbf(pbf_path: str, cache_path: str = None):
    """
    Load from cache if exists, otherwise process PBF and save cache.
    """
    if cache_path is None:
        cache_path = pbf_path.replace('.osm.pbf', '_graph.pkl')
    
    if os.path.exists(cache_path):
        print(f"Loading cached graph from {cache_path}...")
        with open(cache_path, 'rb') as f:
            bounds, edges_list, G = pickle.load(f)
        print(f"Cached graph loaded: {len(G.nodes)} nodes, {len(edges_list)} edges")
        return bounds, edges_list, G
    
    print(f"Processing PBF (first time, this will take a while): {pbf_path}...")
    handler = PBFGraphBuilder() 
    handler.apply_file(pbf_path)
    G = handler.graph
    
    lons = [lon for lon, _ in handler.nodes.values()]
    lats = [lat for _, lat in handler.nodes.values()]
    bounds = {
        'north': max(lats),
        'south': min(lats),
        'east': max(lons),
        'west': min(lons)
    }
    edges_list = list(G.edges)   
    
    # Save to cache
    print(f"Saving cached graph to {cache_path}...")
    with open(cache_path, 'wb') as f:
        pickle.dump((bounds, edges_list, G), f)
    
    return bounds, edges_list, G

def random_coordinate(bounds: Dict) -> Tuple[float, float]:
    return (random.uniform(bounds['south'], bounds['north']),
            random.uniform(bounds['west'], bounds['east']))

def send_message(sock: socket.socket, msg: dict):
    sock.sendall((json.dumps(msg) + '\n').encode())

def main():
    parser = argparse.ArgumentParser(description="RTS Event Simulator")
    parser.add_argument("pbf_file", help="Path to .osm.pbf map file")
    parser.add_argument("--points", "-n", type=int, default=5)
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=12345)
    parser.add_argument("--interval", type=float, default=2.0)
    parser.add_argument("--seed", type=int, default=None)
    args = parser.parse_args()

    if args.seed:
        random.seed(args.seed)

    print(f"Loading PBF via pyosmium: {args.pbf_file} ...")
    bounds, edges, G = get_bounds_and_edges_from_pbf(args.pbf_file)
    print(f"Bounds: {bounds}")
    print(f"Loaded {len(G.nodes)} nodes, {len(edges)} directed edges")

    waypoints = [random_coordinate(bounds) for _ in range(args.points)]
    print(f"Waypoints: {waypoints}")

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((args.host, args.port))
    print(f"Connected to {args.host}:{args.port}")

    send_message(sock, {"type": "waypoints", "coordinates": waypoints})
    print("Waypoints sent.")

    try:
        while True:
            time.sleep(args.interval)
            edge = random.choice(edges)
            u, v = edge
            new_weight = random.uniform(1.0, 30.0)
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