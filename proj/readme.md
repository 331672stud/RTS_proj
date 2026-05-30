powinno się mieć zainstalowane simdjson-devel  (obsługuje jsony chyba najszybciej ze wszystkich bibliotek)

założenie jest takie że uruchamia się rts, simScript.py (Dependency: pip install osmium networkx) który wysyła losowe punkty, i okazjonalnie aktualizacje dotyczące wag.

(w kontenerze albo na hoście)
cmake -B build -G Ninja
cmake --build build
./build/rts_proj

(w kontenerze albo na hoście bez --host rts)
python simScript.py maps/Warsaw.osm.pbf --host rts
