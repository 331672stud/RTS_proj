#!/usr/bin/env python3
"""
Fast overview of large road network – plots edges directly with matplotlib.
"""

import argparse
import pickle
import random
import matplotlib.pyplot as plt

def main():
    parser = argparse.ArgumentParser(description="Fast road network visualization")
    parser.add_argument("pbf_file", help="Path to .osm.pbf (will look for _graph.pkl)")
    parser.add_argument("--sample", type=float, default=0.01,
                        help="Fraction of edges to draw (0.0-1.0). Default 0.01 = 1%%")
    parser.add_argument("--save", help="Save plot to file instead of showing")
    args = parser.parse_args()

    cache_path = args.pbf_file.replace('.osm.pbf', '_graph.pkl')
    try:
        with open(cache_path, 'rb') as f:
            bounds, edges_list, G = pickle.load(f)
    except FileNotFoundError:
        print(f"Cache not found at {cache_path}")
        return

    total_nodes = len(G.nodes)
    total_edges = len(G.edges)
    print(f"Loaded: {total_nodes} nodes, {total_edges} edges")

    # Sample edges
    sample_size = max(1, int(total_edges * args.sample))
    all_edges = list(G.edges())
    if sample_size < total_edges:
        sample_edges = random.sample(all_edges, sample_size)
    else:
        sample_edges = all_edges
    print(f"Drawing {len(sample_edges)} edges ({args.sample*100:.1f}%%)")

    # Pre‑fetch coordinates (much faster than repeated lookups)
    x_coords = []
    y_coords = []
    for u, v in sample_edges:
        xu = G.nodes[u]['x']
        yu = G.nodes[u]['y']
        xv = G.nodes[v]['x']
        yv = G.nodes[v]['y']
        # Add segment (NaN separates lines)
        x_coords.extend([xu, xv, None])
        y_coords.extend([yu, yv, None])

    # Plot
    plt.figure(figsize=(12, 10))
    plt.plot(x_coords, y_coords, color='gray', linewidth=1, alpha=1)
    plt.title(f"Road network – {total_nodes} nodes, {total_edges} edges (showing {len(sample_edges)} edges)")
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