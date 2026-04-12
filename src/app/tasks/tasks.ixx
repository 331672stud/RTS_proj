export module app.tasks;

import core.task;
import app.context;

// High priority: validation / safety
export void taskValidate(TaskContext&){}

// Medium: local replanning trigger
export void taskLocalReplan(TaskContext&){}

// Low: global optimization trigger
export void taskGlobalReplan(TaskContext&){}

// Periodic state update
export void taskUpdateState(TaskContext&){}