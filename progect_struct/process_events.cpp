#include "events.cpp"

namespace {
enum class Event {
    Add, 
    DeL
    // ...
}
}; 


int main() {
    

    DecRec current_session;
    Event x = Event::Add;
    
    
    swich(x) {
        case Event::Add {
            DecRep::add();
        } break;
        case Event::Add {
            DecRep::add();
        } break;
        case Event::Add {
            DecRep::add();
        } break;
        // ...
    }
}