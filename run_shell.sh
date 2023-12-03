g++ src/dynamics.cpp -lncurses -lm -lrt -lpthread -o bin/dynamics 
echo "Compiling dynamics";

g++ src/server.cpp -lncurses -lm -lrt -lpthread -o bin/server 
echo "Compiling server";

g++ src/master.cpp -lncurses -lm -lrt -lpthread -o bin/master 
echo "Compiling master";

g++ src/watchdog.cpp -lncurses -lm -lrt -lpthread -o bin/watchdog 
echo "Compiling watchdog";  

 
gnome-terminal --command="./bin/master" 

sleep 1
gnome-terminal --command="./bin/watchdog"


