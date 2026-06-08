module;

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

export module system.nav_graph;

export class NavGraph {
public:
    double min_lat() const noexcept { return min_lat_; }
    double max_lat() const noexcept { return max_lat_; }
    double min_lon() const noexcept { return min_lon_; }
    double max_lon() const noexcept { return max_lon_; }

    // Throws std::runtime_error on failure
    NavGraph(const std::string& nav_path);
    NavGraph();
    ~NavGraph();

    // Move only
    NavGraph(const NavGraph&) = delete;
    NavGraph& operator=(const NavGraph&) = delete;
    NavGraph(NavGraph&& other) noexcept;
    NavGraph& operator=(NavGraph&& other) noexcept;

    // Basic accessors
    uint64_t node_count() const noexcept { return node_count_; }
    uint64_t edge_count() const noexcept { return edge_count_; }

    // Node data
    uint64_t node_osm_id(uint32_t idx) const noexcept { return node_osm_ids_[idx]; }
    double node_lat(uint32_t idx) const noexcept { return lats_[idx]; }
    double node_lon(uint32_t idx) const noexcept { return lons_[idx]; }

    // CSR edge access
    uint32_t edge_offset(uint32_t node_idx) const noexcept { return edge_offsets_[node_idx]; }
    uint32_t edge_target(uint32_t edge_idx) const noexcept { return edge_targets_[edge_idx]; }
    double base_edge_weight(uint32_t edge_idx) const noexcept { return edge_weights_[edge_idx]; }

    // Current weight (base + dynamic override)
    double current_edge_weight(uint32_t edge_idx) const noexcept;

    // Update edge weight (dynamic override)
    void update_edge_weight(uint32_t u_idx, uint32_t v_idx, double new_weight);
    void update_edge_weight_by_index(uint32_t edge_idx, double new_weight) noexcept;

    // Find edge index given (u_idx, v_idx). Returns std::nullopt if not found.
    std::optional<uint32_t> find_edge_index(uint32_t u_idx, uint32_t v_idx) const noexcept;

    // Nearest node lookup using spatial grid
    uint32_t find_nearest_node(double lat, double lon) const noexcept;

    // Direct access to CSR arrays (for pathfinding)
    const std::vector<uint32_t>& edge_offsets() const noexcept { return edge_offsets_; }
    const std::vector<uint32_t>& edge_targets() const noexcept { return edge_targets_; }
    // For edge weights, prefer current_edge_weight(edge_idx) inside loops

private:
    void unmap() noexcept;

    // mmap'd data (read‑only)
    void* mapped_data_ = nullptr;
    size_t mapped_size_ = 0;

    // Pointers into the mapped region (non‑owning)
    const uint64_t* node_osm_ids_ = nullptr;
    const double* lats_ = nullptr;
    const double* lons_ = nullptr;
    const uint32_t* edge_offsets_ptr_ = nullptr;
    const uint32_t* edge_targets_ptr_ = nullptr;
    const double* edge_weights_ptr_ = nullptr;

    // Edge map (sorted by u, then v) – stored as three parallel arrays
    const uint32_t* edge_map_base_ = nullptr;
    uint64_t edge_map_count_ = 0;

    // Spatial grid
    double min_lat_ = 0.0, max_lat_ = 0.0, min_lon_ = 0.0, max_lon_ = 0.0;
    double cell_size_ = 0.0;
    uint32_t grid_width_ = 0, grid_height_ = 0;
    const uint32_t* cell_offsets_ptr_ = nullptr;
    const uint32_t* cell_nodes_ptr_ = nullptr;
    uint64_t cell_nodes_count_ = 0; // = node_count_

    uint64_t node_count_ = 0;
    uint64_t edge_count_ = 0;

    // Dynamic overrides: edge_index -> current weight
    mutable std::unordered_map<uint32_t, double> weight_overrides_;

    // Helper to copy data into local vectors for safe access
    std::vector<uint32_t> edge_offsets_;
    std::vector<uint32_t> edge_targets_;
    std::vector<double> edge_weights_;  
    // We keep the pointers also available; vectors are for range loops etc.
};

// ----------------------------------------------------------------------
// Implementation
// ----------------------------------------------------------------------

NavGraph::NavGraph(const std::string& nav_path) {
    int fd = open(nav_path.c_str(), O_RDONLY);
    if (fd == -1) {
        throw std::runtime_error("Cannot open .nav file: " + nav_path);
    }
    struct stat st;
    if (fstat(fd, &st) != 0) {
        close(fd);
        throw std::runtime_error("fstat failed");
    }
    mapped_size_ = st.st_size;
    mapped_data_ = mmap(nullptr, mapped_size_, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (mapped_data_ == MAP_FAILED) {
        mapped_data_ = nullptr;
        throw std::runtime_error("mmap failed");
    }

    const uint8_t* ptr = static_cast<const uint8_t*>(mapped_data_);
    uint32_t magic, version;
    std::memcpy(&magic,   ptr, 4); ptr += 4;
    std::memcpy(&version, ptr, 4); ptr += 4;
    static constexpr uint32_t MAGIC   = 0x5254534E;
    static constexpr uint32_t VERSION = 2;
    if (magic != MAGIC || version != VERSION) {
        unmap();
        throw std::runtime_error("Invalid .nav magic/version");
    }

    std::memcpy(&node_count_, ptr, 8); ptr += 8;
    std::memcpy(&edge_count_, ptr, 8); ptr += 8;

    std::memcpy(&min_lat_,     ptr, 8); ptr += 8;
    std::memcpy(&max_lat_,     ptr, 8); ptr += 8;
    std::memcpy(&min_lon_,     ptr, 8); ptr += 8;
    std::memcpy(&max_lon_,     ptr, 8); ptr += 8;
    std::memcpy(&cell_size_,   ptr, 8); ptr += 8;
    std::memcpy(&grid_width_,  ptr, 4); ptr += 4;
    std::memcpy(&grid_height_, ptr, 4); ptr += 4;

    node_osm_ids_ = reinterpret_cast<const uint64_t*>(ptr); ptr += node_count_ * 8;
    lats_         = reinterpret_cast<const double*>(ptr);   ptr += node_count_ * 8;
    lons_         = reinterpret_cast<const double*>(ptr);   ptr += node_count_ * 8;

    edge_offsets_ptr_ = reinterpret_cast<const uint32_t*>(ptr); ptr += (node_count_ + 1) * 4;
    edge_targets_ptr_ = reinterpret_cast<const uint32_t*>(ptr); ptr += edge_count_ * 4;
    edge_weights_ptr_ = reinterpret_cast<const double*>(ptr);   ptr += edge_count_ * 8;

    // FIX 1: edge_map is stride-3 interleaved [u, v, idx, ...].
    // Store only one base pointer; stride is handled in find_edge_index.
    edge_map_base_  = reinterpret_cast<const uint32_t*>(ptr);
    edge_map_count_ = edge_count_;
    ptr += edge_count_ * 3 * 4;

    uint64_t num_cells    = static_cast<uint64_t>(grid_width_) * grid_height_;
    cell_offsets_ptr_ = reinterpret_cast<const uint32_t*>(ptr); ptr += (num_cells + 1) * 4;
    cell_nodes_ptr_   = reinterpret_cast<const uint32_t*>(ptr); ptr += node_count_ * 4;
    cell_nodes_count_ = node_count_;

    // FIX 2: copy all three CSR arrays into vectors.
    edge_offsets_.assign(edge_offsets_ptr_, edge_offsets_ptr_ + node_count_ + 1);
    edge_targets_.assign(edge_targets_ptr_, edge_targets_ptr_ + edge_count_);
    // FIX 3: correct type is double, not uint32_t.
    edge_weights_.assign(edge_weights_ptr_, edge_weights_ptr_ + edge_count_);
}

NavGraph::NavGraph() = default;

NavGraph::~NavGraph() {
    unmap();
}

NavGraph::NavGraph(NavGraph&& other) noexcept
    : mapped_data_(other.mapped_data_),
      mapped_size_(other.mapped_size_),
      node_osm_ids_(other.node_osm_ids_),
      lats_(other.lats_),
      lons_(other.lons_),
      edge_offsets_ptr_(other.edge_offsets_ptr_),
      edge_targets_ptr_(other.edge_targets_ptr_),
      edge_weights_ptr_(other.edge_weights_ptr_),
      edge_map_base_(other.edge_map_base_),      // renamed from three pointers
      edge_map_count_(other.edge_map_count_),
      min_lat_(other.min_lat_),
      max_lat_(other.max_lat_),
      min_lon_(other.min_lon_),
      max_lon_(other.max_lon_),
      cell_size_(other.cell_size_),
      grid_width_(other.grid_width_),
      grid_height_(other.grid_height_),
      cell_offsets_ptr_(other.cell_offsets_ptr_),
      cell_nodes_ptr_(other.cell_nodes_ptr_),
      cell_nodes_count_(other.cell_nodes_count_),
      node_count_(other.node_count_),
      edge_count_(other.edge_count_),
      weight_overrides_(std::move(other.weight_overrides_)),
      edge_offsets_(std::move(other.edge_offsets_)),
      edge_targets_(std::move(other.edge_targets_)),
      edge_weights_(std::move(other.edge_weights_))   // was missing entirely
{
    other.mapped_data_ = nullptr;
    other.mapped_size_ = 0;
}

NavGraph& NavGraph::operator=(NavGraph&& other) noexcept {
    if (this != &other) {
        unmap();
        mapped_data_      = other.mapped_data_;
        mapped_size_      = other.mapped_size_;
        node_osm_ids_     = other.node_osm_ids_;
        lats_             = other.lats_;
        lons_             = other.lons_;
        edge_offsets_ptr_ = other.edge_offsets_ptr_;
        edge_targets_ptr_ = other.edge_targets_ptr_;
        edge_weights_ptr_ = other.edge_weights_ptr_;
        edge_map_base_    = other.edge_map_base_;     // renamed
        edge_map_count_   = other.edge_map_count_;
        min_lat_          = other.min_lat_;
        max_lat_          = other.max_lat_;
        min_lon_          = other.min_lon_;
        max_lon_          = other.max_lon_;
        cell_size_        = other.cell_size_;
        grid_width_       = other.grid_width_;
        grid_height_      = other.grid_height_;
        cell_offsets_ptr_ = other.cell_offsets_ptr_;
        cell_nodes_ptr_   = other.cell_nodes_ptr_;
        cell_nodes_count_ = other.cell_nodes_count_;
        node_count_       = other.node_count_;
        edge_count_       = other.edge_count_;
        weight_overrides_ = std::move(other.weight_overrides_);
        edge_offsets_     = std::move(other.edge_offsets_);
        edge_targets_     = std::move(other.edge_targets_);
        edge_weights_     = std::move(other.edge_weights_);  // was missing
        other.mapped_data_ = nullptr;
        other.mapped_size_ = 0;
    }
    return *this;
}

void NavGraph::unmap() noexcept {
    if (mapped_data_ != nullptr) {
        munmap(mapped_data_, mapped_size_);
        mapped_data_ = nullptr;
    }
}

double NavGraph::current_edge_weight(uint32_t edge_idx) const noexcept {
    auto it = weight_overrides_.find(edge_idx);
    if (it != weight_overrides_.end()) {
        return it->second;
    }
    return edge_weights_ptr_[edge_idx];
}

void NavGraph::update_edge_weight(uint32_t u_idx, uint32_t v_idx, double new_weight) {
    auto opt_idx = find_edge_index(u_idx, v_idx);
    if (opt_idx) {
        update_edge_weight_by_index(*opt_idx, new_weight);
    }
}

void NavGraph::update_edge_weight_by_index(uint32_t edge_idx, double new_weight) noexcept {
    weight_overrides_[edge_idx] = new_weight;
}

std::optional<uint32_t> NavGraph::find_edge_index(uint32_t u_idx, uint32_t v_idx) const noexcept {
    int64_t left  = 0;
    int64_t right = static_cast<int64_t>(edge_map_count_) - 1;
    while (left <= right) {
        int64_t mid   = (left + right) / 2;
        uint32_t cur_u   = edge_map_base_[mid * 3 + 0];
        uint32_t cur_v   = edge_map_base_[mid * 3 + 1];
        uint32_t cur_idx = edge_map_base_[mid * 3 + 2];
        if (cur_u < u_idx) {
            left = mid + 1;
        } else if (cur_u > u_idx) {
            right = mid - 1;
        } else {
            if (cur_v < v_idx) {
                left = mid + 1;
            } else if (cur_v > v_idx) {
                right = mid - 1;
            } else {
                return cur_idx;
            }
        }
    }
    return std::nullopt;
}

uint32_t NavGraph::find_nearest_node(double lat, double lon) const noexcept {
    lat = std::clamp(lat, min_lat_, max_lat_);
    lon = std::clamp(lon, min_lon_, max_lon_);
    int cx = static_cast<int>((lon - min_lon_) / cell_size_);
    int cy = static_cast<int>((lat - min_lat_) / cell_size_);
    cx = std::clamp(cx, 0, static_cast<int>(grid_width_ - 1));
    cy = std::clamp(cy, 0, static_cast<int>(grid_height_ - 1));
    uint32_t start_cell = static_cast<uint32_t>(cy * grid_width_ + cx);

    uint32_t best_node = 0;
    double best_dist2 = std::numeric_limits<double>::max();
    const uint64_t num_cells = static_cast<uint64_t>(grid_width_) * grid_height_;
    std::vector<bool> visited(num_cells, false);
    std::vector<uint32_t> cells_to_check;
    cells_to_check.reserve(num_cells);
    cells_to_check.push_back(start_cell);
    visited[start_cell] = true;

    size_t idx = 0;
    while (idx < cells_to_check.size()) {
        uint32_t cur_cell = cells_to_check[idx];
        uint32_t start = cell_offsets_ptr_[cur_cell];
        uint32_t end = cell_offsets_ptr_[cur_cell + 1];
        for (uint32_t i = start; i < end; ++i) {
            uint32_t node_idx = cell_nodes_ptr_[i];
            double dx = lon - lons_[node_idx];
            double dy = lat - lats_[node_idx];
            double dist2 = dx*dx + dy*dy;
            if (dist2 < best_dist2) {
                best_dist2 = dist2;
                best_node = node_idx;
            }
        }

        // Expand neighbours
        int cur_cx = static_cast<int>(cur_cell % grid_width_);
        int cur_cy = static_cast<int>(cur_cell / grid_width_);
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                int nx = cur_cx + dx;
                int ny = cur_cy + dy;
                if (nx >= 0 && nx < static_cast<int>(grid_width_) &&
                    ny >= 0 && ny < static_cast<int>(grid_height_)) {
                    uint32_t neigh = static_cast<uint32_t>(ny * grid_width_ + nx);
                    if (!visited[neigh]) {
                        visited[neigh] = true;
                        cells_to_check.push_back(neigh);
                    }
                }
            }
        }

        ++idx;

        // Optional early stop: if we've checked a cell and the best distance is less than
        // the distance to the farthest corner of the current cell, we could stop.
        // For simplicity we continue until no cells left.
    }
    return best_node;
}
