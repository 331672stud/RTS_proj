export module app.tasks;

import app.context;

export void taskSamplePosition(TaskContext& ctx);   // polluje gps

export void taskNavigationState(TaskContext& ctx);  // aktualizacja GUI

export void taskLocalReplan(TaskContext& ctx); //aktualizacja pobliskiej ścieżki (typu 100 metrów czy coś)

export void taskGlobalReplan(TaskContext& ctx); //globalna aktualizacja całej trasy

export void taskPeriodicRouteCheck(TaskContext& ctx); // porównanie trasy dotychczasowej z nowo wyliczoną

export void taskWatchdog(TaskContext& ctx);           // watchdog