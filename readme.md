założenie jest takie że uruchamia się rts, simScript.py który wysyła losowe punkty, i okazjonalnie aktualizacje dotyczące wag.

(w kontenerze albo na hoście)
cmake -B build -G Ninja
cmake --build build
./build/rts_proj

(w kontenerze albo na hoście bez --host rts)
python simScript.py maps/Warsaw.osm.pbf --host rts

NA RAZIE DALEJ TRZEBA SAMEMU ODPALAĆ W KONTENERACH

RTS CZEKA NA SIMSCRIPT BO ON CACHE'UJE MU MAPĘ W .PBF

NA RAZIE TRZEBA W MAIN ZMIENIĆ GDZIE OTWIERA .NAV (MOŻEMY PÓŹNIEJ DAĆ JAKO ARGUMENT)
