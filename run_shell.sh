gcc src/dynamics.cpp -lncurses -lm -lrt -lpthread -o bin/dynamics 
echo "Compiling dynamics";

# gcc src/keyboardMan.c -lncurses -lm -lrt -lpthread -o bin/keyboardMan 
# echo "Compiling keyboardMan";

gcc src/server.c -lncurses -lm -lrt -lpthread -o bin/server 
echo "Compiling server";

gcc src/master.c -lncurses -lm -lrt -lpthread -o bin/master 
echo "Compiling master";

gcc src/watchdog.c -lncurses -lm -lrt -lpthread -o bin/watchdog 
echo "Compiling watchdog";  

./bin/master
# ./bin/watchdog
