module;

#include <cstdint>
#include <variant>

export module core.eventData;

struct GraphUpdateData {
    uint64_t edge_id;
    double new_weight;     
};

struct OffRouteData {
    double deviation_meters;
};

enum class InternalEventType {
    LocalReplanRequest,
    GlobalReplanRequest,
    OffRouteWarning,
    PositionUpdateRequest
};

struct InternalEventData {

};

using EventData = std::variant<
    GraphUpdateData,
    OffRouteData,
    InternalEventData
>;