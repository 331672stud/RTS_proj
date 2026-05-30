export module app.dispatcher;

import core.event;
import app.context;

export void dispatchEvent(TaskContext& ctx, const Event& e) {
    switch (e.type) {
        case EventType::GraphUpdate:
            ctx.onGraphUpdate(e.data);
            break;
        case EventType::OffRouteDetected:
            ctx.onOffRouteDetected(e.data);
            break;
    }
}