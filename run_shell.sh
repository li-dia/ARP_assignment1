g++ src/dynamics.cpp -lncurses -lm -lrt -lpthread -o bin/dynamics 
echo "Compiling dynamics";

# g++ src/keyboardMan.c -lncurses -lm -lrt -lpthread -o bin/keyboardMan 
# echo "Compiling keyboardMan";

g++ src/server.cpp -lncurses -lm -lrt -lpthread -o bin/server 
echo "Compiling server";

g++ src/master.cpp -lncurses -lm -lrt -lpthread -o bin/master 
echo "Compiling master";

g++ src/watchdog.cpp -lncurses -lm -lrt -lpthread -o bin/watchdog 
echo "Compiling watchdog";  

./bin/master 
# ./bin/watchdog 
