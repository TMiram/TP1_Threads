#include <iostream>
#include <thread>




bool func(int arg) {

    long int fact=1;
    int currentFact=0;
    int maxFact=arg;

    while(currentFact<maxFact){
        std::cout<<"running "<<std::this_thread::get_id()<<std::endl;
        currentFact+=1;
        fact=fact*currentFact;

    }
    
    std::cout<<"factorial value is "<<fact<<std::endl;
    return true;
}

int main(){

    std::thread th1;
    std::thread th2;

    int a=3;
    int b=5;

    th1 = std::thread(func, a);
    th2 = std::thread(func, b);

    th1.join();
    th2.join();

    return 0;
}