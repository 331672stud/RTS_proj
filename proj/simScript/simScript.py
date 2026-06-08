"""
Symulator trasy dla SCR - używa pyosmium do bezpośredniego odczytu .osm.pbf.
Dodatkowo cacheuje binarny plik .pkl oraz .nav z węzłami i krawędziami.
Tylko największa spójna składowa (weakly connected) jest zachowana.
"""

import osmium
import networkx as nx
import numpy as np
import struct
import random
import json
import socket
import time
import argparse
from typing import Tuple, Dict, List
import os
import sys
import pickle

class PBFGraphBuilder(osmium.SimpleHandler):
    """Buduje skierowany graf NetworkX z pliku .osm.pbf."""
    def __init__(self):
        super().__init__()
        self.graph = nx.DiGraph()

    def node(self, n):
        self.graph.add_node(n.id, x=n.location.lon, y=n.location.lat)

    def way(self, w):
        if 'highway' not in w.tags:
            return
        tags = dict(w.tags)
        highway_type = tags.get('highway', 'residential')
        speed_kmh = {
            'motorway': 120, 'trunk': 100, 'primary': 70,
            'secondary': 60, 'tertiary': 50, 'residential': 40,
            'living_street': 20, 'service': 30
        }.get(highway_type, 50)
        speed_mps = speed_kmh / 3.6

        node_ids = [n.ref for n in w.nodes]
        for i in range(len(node_ids) - 1):
            u = node_ids[i]
            v = node_ids[i+1]
            if u not in self.graph or v not in self.graph:
                continue
            lon1 = self.graph.nodes[u]['x']
            lat1 = self.graph.nodes[u]['y']
            lon2 = self.graph.nodes[v]['x']
            lat2 = self.graph.nodes[v]['y']

            from math import radians, sin, cos, sqrt, asin
            R = 6371000
            phi1, phi2 = radians(lat1), radians(lat2)
            dphi = radians(lat2 - lat1)
            dlam = radians(lon2 - lon1)
            a = sin(dphi/2)**2 + cos(phi1)*cos(phi2)*sin(dlam/2)**2
            dist = 2 * R * asin(sqrt(a))
            time_sec = dist / speed_mps if speed_mps > 0 else 0.001
            self.graph.add_edge(u, v, length=time_sec)
            if 'oneway' not in tags or tags['oneway'] != 'yes':
                self.graph.add_edge(v, u, length=time_sec)

def keep_largest_component(G):
    """Zwraca podgraf z największą słabo spójną składową."""
    # Dla grafu skierowanego używamy weakly_connected_components
    components = list(nx.weakly_connected_components(G))
    if not components:
        return G
    largest = max(components, key=len)
    return G.subgraph(largest).copy()

def save_nav_file(pbf_path: str, G: nx.DiGraph, default_speed_ms: float = 13.8889):
    """
    Zapisz graf do binarki .nav (tylko największa składowa).
    """
    nav_path = pbf_path.replace('.osm.pbf', '.nav')
    print(f"Writing {nav_path} ...")

    nodes_list = sorted(G.nodes())
    node_count = len(nodes_list)
    node_index = {nid: idx for idx, nid in enumerate(nodes_list)}

    node_osm_ids = np.array(nodes_list, dtype=np.uint64)
    lats = np.zeros(node_count, dtype=np.float64)
    lons = np.zeros(node_count, dtype=np.float64)
    for idx, nid in enumerate(nodes_list):
        lons[idx] = G.nodes[nid]['x']
        lats[idx] = G.nodes[nid]['y']

    edges_u = []
    edges_v = []
    edges_w = []

    for u, v, data in G.edges(data=True):
        u_idx = node_index[u]
        v_idx = node_index[v]
        length_m = data.get('length', 0.0)
        travel_time = length_m / default_speed_ms if default_speed_ms > 0 else 0.0
        edges_u.append(u_idx)
        edges_v.append(v_idx)
        edges_w.append(travel_time)

    edge_count = len(edges_u)

    out_degree = np.zeros(node_count, dtype=np.uint32)
    for u_idx in edges_u:
        out_degree[u_idx] += 1
    edge_offsets = np.zeros(node_count + 1, dtype=np.uint32)
    edge_offsets[1:] = np.cumsum(out_degree)
    edge_targets = np.zeros(edge_count, dtype=np.uint32)
    edge_weights = np.zeros(edge_count, dtype=np.float64)
    current_pos = edge_offsets[:-1].copy()
    for u_idx, v_idx, w in zip(edges_u, edges_v, edges_w):
        pos = current_pos[u_idx]
        edge_targets[pos] = v_idx
        edge_weights[pos] = w
        current_pos[u_idx] += 1

    edge_map_entries = []
    current_pos = edge_offsets[:-1].copy()
    edge_idx_by_uv = {}
    for u_idx, v_idx in zip(edges_u, edges_v):
        pos = current_pos[u_idx]
        edge_idx_by_uv[(u_idx, v_idx)] = pos
        current_pos[u_idx] += 1
    sorted_entries = sorted([(u, v, idx) for (u, v), idx in edge_idx_by_uv.items()])
    edge_map_u = np.array([e[0] for e in sorted_entries], dtype=np.uint32)
    edge_map_v = np.array([e[1] for e in sorted_entries], dtype=np.uint32)
    edge_map_idx = np.array([e[2] for e in sorted_entries], dtype=np.uint32)

    min_lat, max_lat = float(np.min(lats)), float(np.max(lats))
    min_lon, max_lon = float(np.min(lons)), float(np.max(lons))
    cell_size = 0.01
    grid_width = max(1, int(np.ceil((max_lon - min_lon) / cell_size)))
    grid_height = max(1, int(np.ceil((max_lat - min_lat) / cell_size)))
    num_cells = grid_width * grid_height

    cell_offsets = np.zeros(num_cells + 1, dtype=np.uint32)
    cell_counts = np.zeros(num_cells, dtype=np.uint32)
    for idx in range(node_count):
        lon = lons[idx]
        lat = lats[idx]
        cx = int((lon - min_lon) / cell_size)
        cy = int((lat - min_lat) / cell_size)
        cx = max(0, min(cx, grid_width - 1))
        cy = max(0, min(cy, grid_height - 1))
        cell = cy * grid_width + cx
        cell_counts[cell] += 1
    cell_offsets[1:] = np.cumsum(cell_counts)
    cell_nodes = np.zeros(node_count, dtype=np.uint32)
    current_pos = cell_offsets[:-1].copy()
    for idx in range(node_count):
        lon = lons[idx]
        lat = lats[idx]
        cx = int((lon - min_lon) / cell_size)
        cy = int((lat - min_lat) / cell_size)
        cx = max(0, min(cx, grid_width - 1))
        cy = max(0, min(cy, grid_height - 1))
        cell = cy * grid_width + cx
        pos = current_pos[cell]
        cell_nodes[pos] = idx
        current_pos[cell] += 1

    with open(nav_path, 'wb') as f:
        f.write(struct.pack('II', 0x5254534E, 2))
        f.write(struct.pack('QQ', node_count, edge_count))
        f.write(struct.pack('dddd', min_lat, max_lat, min_lon, max_lon))
        f.write(struct.pack('dII', cell_size, grid_width, grid_height))

        f.write(node_osm_ids.tobytes())
        f.write(lats.tobytes())
        f.write(lons.tobytes())
        f.write(edge_offsets.tobytes())
        f.write(edge_targets.tobytes())
        f.write(edge_weights.tobytes())
        edge_map_array = np.empty(edge_count, dtype=[('u', np.uint32), ('v', np.uint32), ('idx', np.uint32)])
        edge_map_array['u'] = edge_map_u
        edge_map_array['v'] = edge_map_v
        edge_map_array['idx'] = edge_map_idx
        f.write(edge_map_array.tobytes())
        f.write(cell_offsets.tobytes())
        f.write(cell_nodes[:np.sum(cell_counts)].tobytes())

    print(f"Saved {node_count} nodes, {edge_count} edges (largest component). Grid: {grid_width}x{grid_height} cells.")

def get_bounds_and_edges_from_pbf(pbf_path: str, cache_path: str = None):
    """
    Wczytuje zcache'owaną mapę (tylko największą składową), albo z .pbf i buduje cached wersję.
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
    G_full = handler.graph
    print(f"Full graph: {len(G_full.nodes)} nodes, {len(G_full.edges)} edges")

    # Keep only the largest weakly connected component
    G = keep_largest_component(G_full)
    print(f"Largest component: {len(G.nodes)} nodes, {len(G.edges)} edges")

    # Compute bounds from filtered graph
    all_lons = [data['x'] for _, data in G.nodes(data=True)]
    all_lats = [data['y'] for _, data in G.nodes(data=True)]
    bounds = {
        'north': max(all_lats),
        'south': min(all_lats),
        'east': max(all_lons),
        'west': min(all_lons)
    }

    edges_list = list(G.edges)

    # Save .nav and cache
    save_nav_file(pbf_path, G)

    print(f"Saving cached graph to {cache_path}...")
    with open(cache_path, 'wb') as f:
        pickle.dump((bounds, edges_list, G), f)

    return bounds, edges_list, G

def random_coordinate(bounds: Dict) -> Tuple[float, float]:
    return (random.uniform(bounds['south'], bounds['north']),
            random.uniform(bounds['west'], bounds['east']))

def send_message(sock: socket.socket, msg: dict):
    sock.sendall((json.dumps(msg) + '\n').encode())

def wait_for_server(host: str, port: int, timeout: float = 120.0, retry_interval: float = 5.0):
    start_time = time.time()
    while True:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((host, port))
            print(f"Connected to {host}:{port}")
            return sock
        except ConnectionRefusedError:
            elapsed = time.time() - start_time
            if elapsed >= timeout:
                print(f"ERROR: Could not connect to {host}:{port} after {timeout} seconds.")
                sys.exit(1)
            print(f"Waiting for RTS server on {host}:{port}... (retry in {retry_interval}s)")
            time.sleep(retry_interval)
        except Exception as e:
            print(f"Unexpected error: {e}")
            time.sleep(retry_interval)

def main():
    parser = argparse.ArgumentParser(description="RTS Event Simulator")
    parser.add_argument("pbf_file", help="Path to .osm.pbf map file")
    parser.add_argument("--points", "-n", type=int, default=5)
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=12345)
    parser.add_argument("--interval", type=float, default=1)
    parser.add_argument("--seed", type=int, default=None)
    parser.add_argument("--server-timeout", type=float, default=30.0,
                        help="Max seconds to wait for RTS server")
    args = parser.parse_args()

    if args.seed:
        random.seed(args.seed)

    print(f"Loading PBF via pyosmium: {args.pbf_file} ...")
    bounds, edges, G = get_bounds_and_edges_from_pbf(args.pbf_file)
    print(f"Bounds (largest component): {bounds}")
    print(f"Loaded {len(G.nodes)} nodes, {len(edges)} directed edges")

    sock = wait_for_server(args.host, args.port, timeout=args.server_timeout)

    # Generate waypoints within bounds of the largest component
    waypoints = [random_coordinate(bounds) for _ in range(args.points)]
    print(f"Waypoints: {waypoints}")

    send_message(sock, {"type": "waypoints", "coordinates": waypoints})
    print("Waypoints sent.")

    # try:
    #     while True:
    #         time.sleep(args.interval)
    #         edge = random.choice(edges)
    #         u, v = edge
    #         new_weight = random.uniform(1.0, 30.0)
    #         send_message(sock, {
    #             "type": "graph_update",
    #             "edge": [u, v],
    #             "new_weight": new_weight
    #         })
    #         print(f"Sent update: edge ({u},{v}) -> {new_weight:.1f}s")
    # except KeyboardInterrupt:
    #     print("Stopped.")
    # finally:
    #     sock.close()

if __name__ == "__main__":
    main()