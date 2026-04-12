export module app.dispatcher;

import core.event;
import app.context;

export void dispatchEvent(TaskContext& ctx, const Event& e) {
    switch (e.type) {
        case EventType::PositionUpdate:
            ctx.onPositionUpdate(e.data);
            break;
        case EventType::TrafficUpdate:
            ctx.onTrafficUpdate(e.data);
            break;
        case EventType::RoadBlocked:
            ctx.onRoadBlocked(e.data);
            break;
        case EventType::ReplanRequest:
            ctx.requestLocalReplan();
            break;
        case EventType::GlobalReplanRequest:
            ctx.requestGlobalReplan();
            break;
    }
}