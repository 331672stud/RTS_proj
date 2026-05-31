#!/usr/bin/env python3
"""
Visualize the filtered road network (largest connected component) from .pkl cache.
"""

import argparse
import pickle
import matplotlib.pyplot as plt
import networkx as nx

def main():
    parser = argparse.ArgumentParser(description="Visualize cached road graph")
    parser.add_argument("pbf_file", help="Path to .osm.pbf (will look for _graph.pkl)")
    parser.add_argument("--save", help="Save plot to file instead of showing")
    args = parser.parse_args()

    cache_path = args.pbf_file.replace('.osm.pbf', '_graph.pkl')
    try:
        with open(cache_path, 'rb') as f:
            bounds, edges_list, G = pickle.load(f)
    except FileNotFoundError:
        print(f"Cache not found at {cache_path}")
        print("Run the simulator first to generate the cache.")
        return

    print(f"Loaded graph: {len(G.nodes)} nodes, {len(G.edges)} edges")
    print(f"Bounds: {bounds}")

    # Plot
    plt.figure(figsize=(12, 10))
    # Get node coordinates
    pos = {node: (data['x'], data['y']) for node, data in G.nodes(data=True)}
    # Draw edges
    nx.draw_networkx_edges(G, pos, alpha=0.5, edge_color='gray', width=0.5)
    # Draw nodes (optional, can be slow for large graphs)
    # nx.draw_networkx_nodes(G, pos, node_size=1, node_color='blue', alpha=0.6)
    plt.title(f"Road network (largest component) – {len(G.nodes)} nodes, {len(G.edges)} edges")
    plt.xlabel("Longitude")
    plt.ylabel("Latitude")
    plt.axis('equal')
    plt.tight_layout()

    if args.save:
        plt.savefig(args.save, dpi=150)
        print(f"Saved to {args.save}")
    else:
        plt.show()

if __name__ == "__main__":
    main()