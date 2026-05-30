powinno się mieć zainstalowane simdjson-devel  (obsługuje jsony chyba najszybciej ze wszystkich bibliotek)

założenie jest takie że uruchamia się rts, simScript.py (Dependency: pip install osmium networkx) który wysyła losowe punkty, i okazjonalnie aktualizacje dotyczące wag.

cmake -B build -G Ninja
cmake --build build

python simScript.py maps/Warsaw.osm.pbf --host rts
