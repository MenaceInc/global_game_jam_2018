for dep in $(ldd global_game_jam_2018 | awk '/\/mingw64\//' | awk 'BEGIN{ORS=" "}$1~/^\//{print $1}$3~ /^\//{print $3}'); do cp $dep .; done
